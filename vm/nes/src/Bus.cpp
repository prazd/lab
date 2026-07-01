#include "Bus.hpp"
#include "Constants.hpp"
void Bus::write(unsigned short addr, unsigned char value) {
    adjust_cpu_mirroring(addr);
    cpu_open_bus_value = value;
    cpu_internal_bus_value = value;
#if DEBUG_MODE
    cpu_write_set_addresses[addr] = true;
    cpu_write_addresses.push_back(std::pair<unsigned short, unsigned char>{addr, unsafe_read_prg(addr)});
    cpu_addr_triggered_on_write = cpu_trigger_write_addresses[addr];
#endif
#if GAME_GENIE_MODE
    if (addr >= 0x8000 && addr <= 0x800b){
        gg.handle(addr, value);
        return;
    }
#endif
    if (addr >= 0x8000 && addr <= 0xffff){
        mapper.handle(addr, value);
        return;
    }

    if (addr >= 0x2000 && addr <= 0x2007){
        open_bus_value_cycles = 0;
        ppu_open_bus_value = value;
    }

    switch (addr) {
        case constants::PPUCTRL:   {
            ppuctrl = value;
            ppu_t &= 0xf3ff;
            ppu_t |= ((ppuctrl & 0x3) << 10);
            sprite_mode = static_cast<SpriteMode>((ppuctrl >> 5) & 1);
            return;}
        case constants::PPUMASK:   { ppumask = value; return;}
        case constants::OAMADDR:   { oamaddr = value; return;}
        case constants::OAMDATA:   { oamdata = value; oam[oamaddr++] = value;return;}
        case constants::PPUSCROLL: {
            ppuscroll = value;
            if (!ppu_w){
                // set x coarse + x
                ppu_t &= 0x7fe0;
                ppu_t |= ((ppuscroll >> 3) & 0x1f);
                ppu_x = ppuscroll & 0x7;
            } else {
                // set y coarse + y fine
                ppu_t &= 0xc1f;
                ppu_t |= ((ppuscroll >> 3) & 0x1f) << 5;
                ppu_t |= (ppuscroll & 0x7) << 12;
            }
            ppu_w = !ppu_w;
            return;}
        case constants::PPUADDR: {
            ppuaddr = value;
            if (ppu_w){
                ppu_t &= 0x7f00;
                ppu_t |= ppuaddr;
            } else {
                ppu_t &= 0xff;
                ppu_t |= (((ppuaddr & 0x7f) << 8));
            }
            if (ppu_w){
                ppu_t &= 0x3fff;
                mapper.update_state(ppu_t, ppu_v);
                ppu_v = ppu_t;
                last_ppu_address = ppu_v;
            }
            ppu_w = !ppu_w;
            return;
        }
        case constants::PPUDATA:   {
            write_chr(ppu_v, value);
            inc_by_ppudata();
            return;}
        case constants::OAMDMA : {
            dma_address = value << 8;
            dma_active = true;
            dma_start = true;
            dma_index = 0;
            return;
        }
        case constants::CONTROLLER1: { latch_req = true; latch_val = value; return; }
        case constants::CONTROLLER2_AND_APU_FC: {apufc_requested = true; apufc = value; return; }
        case constants::APU_CONTROL: {
            apu_enable_req = true;
            apu_enable = value; return;}
    }
    if (addr == constants::PULSE1_DUTY_ENVELOPE){
        apu_pulse1_envelope_update = true;
    } else if (addr == constants::PULSE1_TIMER_HIGH){
        apu_pulse1_start_flag = true;
        apu_pulse1_timer_update = true;
    }
    cpu_map[addr] = value;
}

void Bus::unsafe_write(unsigned short addr, unsigned char value) {
    adjust_cpu_mirroring(addr);
    if (addr >= 0x8000 && addr <= 0xffff){
        mapper.handle(addr, value);
        return;
    }
    switch (addr) {
        case constants::PPUCTRL:   {ppuctrl   =   value; return;}
        case constants::PPUMASK:   {ppumask   =   value; return;}
        case constants::OAMADDR:   {oamaddr   =   value; return;}
        case constants::OAMDATA:   {oamdata   =   value; oam[oamaddr++] = value; return;}
        case constants::PPUSCROLL: {ppuscroll =   value; return;}
        case constants::PPUADDR:   {ppuaddr   =   value; return;}
        case constants::PPUDATA:   {ppudata   =   value; return;}
        case constants::OAMDMA : {
            dma_address = value << 8;
            dma_active = true;
            dma_start = true;
            dma_index = 0;
            return;
        }
        case constants::CONTROLLER1: { if (value) c.latch1 = true; else c.update_status1();
            return; }
        case constants::CONTROLLER2_AND_APU_FC: { apufc = value;
            return; }
    }
    cpu_map[addr] = value;
}

unsigned char Bus::read_prg(unsigned short addr) {
    auto res = read_prg_internal(addr);
    if (addr != 0x4015) cpu_open_bus_value = res;
    cpu_internal_bus_value = res;
    return res;
}

unsigned char Bus::dmc_dma_read_prg(unsigned short addr) {
    auto res = read_prg_internal(addr);
    if (addr != 0x4015) cpu_open_bus_value = res;
    return res;
}

void Bus::inc_ppu_v_x() {
    unsigned char prev = (ppu_v & 0x1f);
    unsigned char add_val = (((ppu_v & 0x1f) + 1) % 32);
    if (prev == 31)
        ppu_v ^= 0x400;
    ppu_v &= 0xffe0;
    ppu_v |= add_val;
}

void Bus::inc_ppu_v_y() {
    unsigned char y_fine_add_prev = ((ppu_v >> 12) & 0x7);
    unsigned char y_fine_add_val = ((((ppu_v >> 12) & 0x7) + 1) & 0x7);
    ppu_v &= 0x8fff;
    ppu_v |= y_fine_add_val << 12;
    if (y_fine_add_prev == 7){
        auto y_coarse_val = ((ppu_v >> 5) & 0x1f); // todo: check
        if (y_coarse_val == 29) {
            ppu_v = 0;
            ppu_v ^= 0x800;
        } else if (y_coarse_val == 31) {
            ppu_v = 0;
        } else {
            auto add_val = ((y_coarse_val + 1) & 0x1f);
            ppu_v &= 0xfc1f;
            ppu_v |= (add_val << 5);
        }
    }
}

// All regs for debug
unsigned char Bus::read_prg_internal(unsigned short addr) {
    adjust_cpu_mirroring(addr);
    last_read_address = addr;
#if DEBUG_MODE
    cpu_read_set_addresses[addr] = true;
    cpu_addr_triggered_on_read = cpu_trigger_read_addresses[addr];
#endif
#if GAME_GENIE_MODE
    if (auto i = gg.find_index(addr); i != -1)
        return gg.get(i, unsafe_read_prg(addr)); // todo: check
#endif
    if (addr >= 0x8000 && addr <= 0xffff){
        return mapper.read_prg_rom(addr);
    }
    switch (addr) {
        case constants::PPUCTRL:
        case constants::PPUMASK:
        case constants::PPUSCROLL:
        case constants::PPUADDR:  return ppu_open_bus_value;

        case constants::PPUSTATUS: {
            ppu_w = false;
            status_read_cycle = current_ppu_cycle;
            if (ppustatus & 0x80 && current_ppu_cycle - current_nmi_cycle == 0) { // todo: debug
                status_read_cycle = current_ppu_cycle;
                nmi_interrupt_flip_flop = false;
                auto tmp = ppustatus & 0x7f;
                ppustatus = 0;
                ppu_open_bus_value &= 0x1f;
                ppu_open_bus_value |= (tmp & 0xe0);
                return tmp;
            }
            unsigned char current = (ppustatus & 0xe0) | (ppu_open_bus_value & 0x1f);
            ppustatus &= 0x60;
            ppu_open_bus_value &= 0x1f;
            ppu_open_bus_value |= (current & 0xe0);
            return current;
        }
        case constants::OAMADDR:     return ppu_open_bus_value;
        case constants::OAMDATA: {
            if (oamaddr == 0x2) {
                ppu_open_bus_value = 0;
                open_bus_value_cycles = 0;
            }
            // 3 X
            // 2 Attrib
            // 1 Tile index
            // 0 Y
            return oam[oamaddr] & ((oamaddr & 0x3) == 2 ? 0xe3 : 0xff);
        }

        // https://www.nesdev.org/wiki/PPU_registers#PPUDATA_-_VRAM_data_($2007_read/write)
        case constants::PPUDATA:     {
            if (ppu_v >= constants::IMAGE_PALETTE_BEGIN){
                auto val = read_chr(ppu_v);
                if (ppumask & 1) val &= 0xf0;
                auto res = (val & 0x3f) | (ppu_open_bus_value & 0xc0);
                ppu_open_bus_value = res;

                //  $3000-$3EFF	$0F00	Table Mirror $2000-$2EFF
                ppudata = read_chr(ppu_v-0x1000);
                inc_by_ppudata();
                return res;
            }
            auto res = ppudata;
            ppu_open_bus_value = res;
            ppudata = read_chr(ppu_v);
            inc_by_ppudata();
            return res;
        }
        case constants::CONTROLLER1:{
            unsigned char res = false;
            if (instructions_count - last_instruction_num_read){
                c.shift_req = true;
                res = c.read_status1();
            } else {
                res = c.prev_res;
            }
            res &= 0x1f;
            res |= cpu_open_bus_value & 0xe0;
            last_instruction_num_read = instructions_count;
            return res;
        }
        case constants::CONTROLLER2_AND_APU_FC:
            return cpu_open_bus_value & 0xfe;
        case constants::APU_CONTROL: {
            apu_clear_frame_irq_req = true;
            return (apu_control & 0xdf) | (cpu_internal_bus_value & 0x20);
        }
        case constants::PULSE1_DUTY_ENVELOPE:{
            return cpu_open_bus_value;
        }
    }

    if ((addr >= 0 && addr <= 0x7FF) ||
        (mapper.is_prg_ram() && addr >= 0x6000 && addr <= 0x7fff))
        return cpu_map[addr];

    return cpu_open_bus_value;
}

void Bus::inc_by_ppudata() {
    if (ppumask & 0x18 && ppu_current_y < 241){
        auto prev = ppu_v;
        inc_ppu_v_x();
        inc_ppu_v_y();
        mapper.update_state(ppu_v, prev);
    } else {
        auto prev = std::exchange(ppu_v, (ppu_v + ((ppuctrl & 0x4) ? 32 : 1)) & 0x3fff);
        mapper.update_state(ppu_v, prev);
    }
}

unsigned char Bus::unsafe_read_prg(unsigned short addr) {
    adjust_cpu_mirroring(addr);
    if (addr >= 0x8000 && addr <= 0xffff){
        return mapper.read_prg_rom(addr);
    }
    switch (addr) {
        case constants::PPUCTRL:     return ppuctrl;
        case constants::PPUMASK:     return ppumask;
        case constants::PPUSTATUS:   return ppustatus;
        case constants::OAMADDR:     return oamaddr;
        case constants::OAMDATA:     return oam[oamaddr];
        case constants::PPUSCROLL:   return ppuscroll;
        case constants::PPUADDR:     return ppuaddr;
        case constants::PPUDATA:     return ppudata;
        case constants::CONTROLLER1: return c.read_status1();
        case constants::CONTROLLER2_AND_APU_FC:
            return cpu_open_bus_value & 0xfe;
        case constants::APU_CONTROL: {
            apu_clear_frame_irq_req = true;
            return (apu_control & 0xdf) | (cpu_open_bus_value & 0x20);
        }
        case constants::PULSE1_DUTY_ENVELOPE:{
            return cpu_open_bus_value;
        }
    }

    // TODO: check
    if (addr >= 0x4000 && addr <= 0x4019) // APU regs
        return cpu_map[addr];

    if ((addr >= 0 && addr <= 0x7FF) ||
        (mapper.is_prg_ram() && addr >= 0x6000 && addr <= 0x7fff))
        return cpu_map[addr];

    return cpu_open_bus_value;
}

unsigned char Bus::read_chr(unsigned short addr) {
    adjust_ppu_mirroring(addr);
#if DEBUG_MODE
    if (addr >= 0x0000 && addr <= 0x3fff) {
        ppu_read_set_addresses[addr] = true;
        ppu_addr_triggered_on_read = ppu_trigger_read_addresses[addr];
    }
#endif
    if (addr >= 0x0000 && addr <= 0x1FFF){
        return mapper.read_chr_rom(addr);
    } else if (addr >= 0x2000 && addr <= 0x3fff) {
        return ppu_map[addr];
    }
    return 0xff; // dummy
}

unsigned char Bus::unsafe_read_chr(unsigned short addr) {
    adjust_ppu_mirroring(addr);
    if (addr >= 0x0000 && addr <= 0x1FFF){
        return mapper.unsafe_read_chr_rom(addr);
    } else if (addr >= 0x2000 && addr <= 0x3fff) {
        return ppu_map[addr];
    }
    return 0xff;
}

void Bus::write_chr(unsigned short addr, unsigned char value){
    adjust_ppu_mirroring(addr);
#if DEBUG_MODE
    if (addr >= 0x0000 && addr <= 0x3fff){
        ppu_write_set_addresses[addr] = true;
        ppu_write_addresses.push_back(std::pair<unsigned short, unsigned char>{addr, read_chr(addr)});
        ppu_addr_triggered_on_write = ppu_trigger_write_addresses[addr];
    }
#endif
    if (addr >= 0x0000 && addr <= 0x1FFF && ((ppustatus >> 7) & 1 || !(ppumask & 0x8) || !(ppumask & 0x10))) {
        mapper.write_chr_ram(addr, value);
    } else if (addr >= 0x2000 && addr <= 0x3fff){
        ppu_map[addr] = value;
    }
}

void Bus::unsafe_write_chr(unsigned short addr, unsigned char value){
    adjust_ppu_mirroring(addr);
    if (addr >= 0x0000 && addr <= 0x1FFF && ((ppustatus >> 7) & 1 || !(ppumask & 0x8) || !(ppumask & 0x10))) {
        mapper.write_chr_ram(addr, value);
    } else if (addr >= 0x2000 && addr <= 0x3fff){
        ppu_map[addr] = value;
    }
}

void Bus::controller_write_tick() {
    if (latch_req){
        if (latch_val & 1) {c.latch1 = true;} else {c.update_status1(); c.latch1 = false;}
        latch_req = false;
    }

}
void Bus::controller_read_tick() {
    if (c.shift_req){
        ++c.current1;
        c.status1 >>= 1;
        c.shift_req = false;
    }
}

void Bus::tick() {
    ++master_clock;
    open_bus_decay();
}

void Bus::open_bus_decay() {
    if (ppu_open_bus_value && open_bus_value_cycles++ == 5'000){ // todo: check
        open_bus_value_cycles = 0;
        ppu_open_bus_value = 0;
    }
}

void Bus::adjust_cpu_mirroring(unsigned short& addr) {
    if (addr >= constants::RAM_MIRROR1_BEGIN && addr <= constants::RAM_MIRROR1_END)
        addr-=constants::RAM_MIRROR1_BEGIN;
    else if (addr >= constants::RAM_MIRROR2_BEGIN && addr <= constants::RAM_MIRROR2_END)
        addr-=constants::RAM_MIRROR2_BEGIN;
    else if (addr >= constants::RAM_MIRROR3_BEGIN && addr <= constants::RAM_MIRROR3_END)
        addr-=constants::RAM_MIRROR3_BEGIN;
    else if (addr >= constants::PPU_REGS_MIRROR_BEGIN && addr <= constants::PPU_REGS_MIRROR_END){
        addr = constants::PPU_REGS_BEGIN + (addr % 8);
    }
}

void Bus::adjust_ppu_mirroring(unsigned short& addr) {
    if (addr >= constants::PALETTES_MIRROR_BEGIN && addr <= constants::PALETTES_MIRROR_END)
        addr = constants::IMAGE_PALETTE_BEGIN + (addr % 32);
    if (addr == 0x3f10 || addr == 0x3f14 || addr == 0x3f18 || addr == 0x3f1c)
        addr -= 0x10;
    if (addr >= constants::TABLES_MIRROR_BEGIN && addr <= constants::TABLES_MIRROR_END)
        addr = constants::NAME_TABLE0_BEGIN + (addr - constants::TABLES_MIRROR_BEGIN);
    adjust_ppu_nt_mirroring(addr); // TODO: check
}

unsigned short Bus::get_nt_by_mirror_mode(Mapper::ScreenMirroring sm, unsigned char n) {
    using SM = Mapper::ScreenMirroring;
    switch (sm) {
        case SM::Vertical:   { return (n == 0 || n == 2) ? constants::NAME_TABLE0_BEGIN : constants::NAME_TABLE3_BEGIN;}
        case SM::Horizontal: { return (n == 0 || n == 1) ? constants::NAME_TABLE0_BEGIN : constants::NAME_TABLE3_BEGIN;}
        case SM::SingleScreenLower: {return constants::NAME_TABLE0_BEGIN;}
        case SM::SingleScreenUpper: {return constants::NAME_TABLE1_BEGIN;}
        default:
            return constants::NAME_TABLE0_BEGIN;
    }
}

void Bus::adjust_ppu_nt_mirroring(unsigned short& address) {
    for (int i = 0; i < 4; ++i){
        auto current_nt = constants::NAME_TABLE0_BEGIN + (i * 0x400);
        if (address >= current_nt && address <= current_nt + 0x3ff){
            auto r = get_nt_by_mirror_mode(mapper.current_screen_mirroring, i);
            address = r + (address - current_nt);
            return;
        }
    }
}

void Bus::reset(){
    memset(ppu_map, 0, sizeof(ppu_map));
    memset(cpu_map, 0, sizeof(cpu_map));
    ppu_t = 0;
    ppu_v = 0;
    ppu_w = false;
    ppu_x = 0;
    ppuctrl = 0;
    ppumask = 0;
    ppustatus = 0;
    oamaddr = 0;
    oamdata = 0;
    ppuscroll = 0;
    ppuaddr = 0;
    ppudata = 0;
    ppu_open_bus_value = 0;
    cpu_open_bus_value = 0;
    dma_active = false;
    dma_index = 0;
    dma_address = 0;
    nmi_interrupt_flip_flop = false;
    apu_irq_interrupt_flip_flop = false;
    ppu_irq_interrupt_flip_flop = false;
    open_bus_value_cycles = 0;
    last_ppu_address = 0;
    mapper.reset();
}
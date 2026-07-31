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
    if (!gg.started && addr >= 0x8000 && addr <= 0x800b){
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
#if DEBUG_MODE
                start_v_value = ppu_v;
#endif
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
            oam_dma_address = value << 8;
            dma_active = true;
            dma_start = true;
            dma_index = 0;
            return;
        }
        case constants::CONTROLLER1: { latch_req = true; latch_val = value; return; }
        case constants::CONTROLLER2_AND_APU_FC: {
            apufc = value;
            frame_counter_mode = static_cast<FrameCounterMode>((apufc >> 7) & 1);
            apu_irq_inhibit = (apufc & 0x40);
            if (apu_irq_inhibit && frame_counter_mode == Step4) {
                apu_control &= 0xbf;
                apu_irq_interrupt_flip_flop = false;
            }
            frame_counter_reset = true;
            if (!(apu_current_tick & 1)){
                frame_counter_reset_ticks = 2;
            } else
                frame_counter_reset_ticks = 3;
            return; }
        case constants::APU_CONTROL: {
            apu_enable_req = true;
            apu_enable = value;
            apu_enable_handle();
            return;}
    }
    if (addr == constants::PULSE1_DUTY_ENVELOPE){
        apu_pulse1_envelope_update = true;
    } else if (addr == constants::PULSE1_TIMER_HIGH){
        apu_pulse1_start_flag = true;
        apu_pulse1_timer_update = true;
    } else if (addr == constants::PULSE1_SWEEP){
        apu_pulse1_sweep_update = true;
    } else if (addr == constants::PULSE2_DUTY_ENVELOPE){
        apu_pulse2_envelope_update = true;
    } else if (addr == constants::PULSE2_TIMER_HIGH){
        apu_pulse2_start_flag = true;
        apu_pulse2_timer_update = true;
    } else if (addr == constants::PULSE2_SWEEP){
        apu_pulse2_sweep_update = true;
    } else if (addr == constants::TRIANGLE_CHAN_TIMER_HIGH){
        apu_triangle_timer_update = true;
    } else if (addr == constants::TRIANGLE_CHAN_LINEAR_COUNTER){
        apu_triangle_linear_counter_update = true;
    } else if (addr == constants::NOISE_CHAN_LOOP){
        apu_noise_chan_loop_update = true;
    } else if (addr == constants::NOISE_CHAN_ENVELOPE){
        apu_noise_envelope_update = true;
    } else if (addr == constants::NOISE_CHAN_LENGTH_COUNTER){
        apu_noise_start_flag = true;
        apu_noise_timer_update = true;
    } else if (addr == constants::DMC_CHAN_SAMPLE_ADDRESS) {
        apu_dmc_dma_address_update = true;
    }
    cpu_map[addr] = value;
}

// TODO: debug
void Bus::apu_enable_handle() {
    apu_control &= 0x7f;
    if (!apu_is_enable(DMC))  {dmc_bytes_remaining = 0; return;}
    if (dmc_bytes_remaining) return;
    auto s_len = unsafe_read_prg(constants::DMC_CHAN_SAMPLE_LENGTH);
    dmc_bytes_remaining = (s_len << 4) | 1;
    auto chan_direct_load_val = unsafe_read_prg(constants::DMC_CHAN_DIRECT_LOAD);
    dmc_output_level = (char)(chan_direct_load_val & 0x7f);
    dmc_rate_counter = ntsc_rates[dmc_rate_index];
    auto s_addr = unsafe_read_prg(constants::DMC_CHAN_SAMPLE_ADDRESS);
    dmc_dma_address = 0xc000 | s_addr << 6;
    if (dmc_sample_buffer.empty()){
        dmc_delay = 3;
        dmc_need_sample_update = true;
    } else if (!dmc_shift_count){
        dmc_shift_reg = dmc_sample_buffer.back(); dmc_sample_buffer.pop_back();
        dmc_shift_count = 8;
    }
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
            oam_dma_address = value << 8;
            dma_active = true;
            dma_start = true;
            dma_index = 0;
            return;
        }
        case constants::CONTROLLER1: { if (value) c.latch1 = true; else {c.update_status1(); c.update_status2();}
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
            ppu_v &= 0x0800;
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
        case constants::PPUADDR:  return ppu_open_bus_value; // TODO: debug GameGenie(ZZZLTT)

        case constants::PPUSTATUS: {
            ppu_w = false;
            status_read_cycle = current_ppu_cycle;
// TODO: debug
//            if (ppustatus & 0x80 && ppu_current_y == 241 && ppu_current_scanline == 0) {
//                nmi_reject = true;
//                nmi_interrupt_flip_flop = false;
//                auto tmp = ppustatus & 0x7f;
//                ppustatus = 0;
//                ppu_open_bus_value &= 0x1f;
//                ppu_open_bus_value |= (tmp & 0xe0);
//                return tmp;
//            }
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
                c.shift_req1 = true;
                res = c.read_status1();
            } else {
                res = c.prev_res1;
            }
            res &= 0x1f;
            res |= cpu_open_bus_value & 0xe0;
            last_instruction_num_read = instructions_count;
            return res;
        }
        case constants::CONTROLLER2_AND_APU_FC:{
            unsigned char res = false;
            if (instructions_count - last_instruction_num_read){
                c.shift_req2 = true;
                res = c.read_status2();
            } else {
                res = c.prev_res2;
            }
            res &= 0x1f;
            res |= cpu_open_bus_value & 0xe0;
            last_instruction_num_read = instructions_count;
            return res;
        }
        case constants::APU_CONTROL: {
            if (master_clock - apu_fc_irq_set_cycle > 3) // TODO: check
                apu_clear_frame_irq_req = true;
            return (apu_control & 0xdf) | (cpu_internal_bus_value & 0x20);
        }

        // TODO: check
        case constants::DMC_CHAN_SAMPLE_LENGTH:
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
            // TODO: check
            apu_clear_frame_irq_req = true;
            return (apu_control & 0xdf) | (cpu_open_bus_value & 0x20);
        }
// TODO: check
//        case constants::PULSE1_DUTY_ENVELOPE:{
//            return cpu_open_bus_value;
//        }
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
        if (latch_val & 1) {c.latch1 = true;} else {c.update_status1(); c.update_status2(); c.latch1 = false;}
        latch_req = false;
    }

}
void Bus::controller_read_tick() {
    if (c.shift_req1){
        ++c.current1;
        c.status1 >>= 1;
        c.shift_req1 = false;
    }
    if (c.shift_req2){
        ++c.current2;
        c.status2 >>= 1;
        c.shift_req2 = false;
    }
}

void Bus::tick() {
    ++master_clock;
    ppu_irq_interrupt_flip_flop = mapper.is_irq_requested();
// TODO: debug
//    if (!nmi_detect)
//        nmi_detect = ppustatus & 0x80 && ppuctrl & 0x80 && nmi_interrupt_flip_flop && master_clock - current_nmi_cycle >= 0;
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
#if DEBUG_MODE
    start_v_value = 0;
#endif
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
    oam_dma_address = 0;
    nmi_interrupt_flip_flop = false;
    apu_irq_interrupt_flip_flop = false;
    ppu_irq_interrupt_flip_flop = false;
    open_bus_value_cycles = 0;
    last_ppu_address = 0;
    mapper.reset();
}
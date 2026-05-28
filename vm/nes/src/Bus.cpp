#include "Bus.hpp"
#include "Constants.hpp"
void Bus::write(unsigned short addr, unsigned char value) {
    adjust_cpu_mirroring(addr);
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
        open_bus_value = value;
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
            unsigned short w_addr = ppu_v;
            write_chr(w_addr, value);
//            ppudata = value; // todo: check
            ppu_v = (ppu_v + ((ppuctrl >> 2) & 1 ? 32 : 1)) & 0x3fff;
            return;}
        case constants::OAMDMA : {
            dma_address = value << 8;
            dma_active = true;
            dma_start = true;
            dma_index = 0;
            return;
        }
        case constants::CONTROLLER1: { if (value) c.latch1 = true; else c.update_status1(); return; }
        case constants::CONTROLLER2_AND_APU_FC: { apufc = value; return; }
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

// All regs for debug
unsigned char Bus::read_prg(unsigned short addr) {
    adjust_cpu_mirroring(addr);
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
        case constants::PPUADDR:  return open_bus_value;

        case constants::PPUSTATUS: {
            ppu_w = false;
            status_read_cycle = current_ppu_cycle;
            if (ppustatus & 0x80 && current_ppu_cycle - current_nmi_cycle == 0) { // todo: debug
                status_read_cycle = current_ppu_cycle;
                nmi_interrupt_flip_flop = false;
                auto tmp = ppustatus & 0x7f;
                ppustatus = 0;
                return  tmp;
            }
            unsigned char current = (ppustatus & 0xe0) | (open_bus_value & 0x1f);
            ppustatus &= 0x60;
            return current;
        }
        case constants::OAMADDR:     return open_bus_value;
        case constants::OAMDATA: {
            if (oamaddr == 0x2) {
                open_bus_value = 0;
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
                auto res = read_chr(ppu_v) | (open_bus_value & 0xc0);
                open_bus_value = res;

                //  $3000-$3EFF	$0F00	Table Mirror $2000-$2EFF
                ppudata = read_chr(ppu_v-0x1000);
                ppu_v = (ppu_v + ((ppuctrl >> 2) & 1 ? 32 : 1)) & 0x3fff;
                return res;
            }
            auto res = ppudata;
            open_bus_value = res;
            ppudata = read_chr(ppu_v);
            ppu_v = (ppu_v + ((ppuctrl >> 2) & 1 ? 32 : 1)) & 0x3fff;
            return res;
        }
        case constants::CONTROLLER1: return c.read_status1();
        case constants::CONTROLLER2_AND_APU_FC: return 0;
    }

    return cpu_map[addr];
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
    }
    return cpu_map[addr];
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
        return mapper.read_chr_rom(addr);
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
    if (addr >= 0x0000 && addr <= 0x1FFF && ((ppustatus >> 7) & 1 || !((ppumask >> 3) & 1) || !((ppumask >> 4) & 1))) {
        mapper.write_chr_ram(addr, value);
    } else if (addr >= 0x2000 && addr <= 0x3fff){
        ppu_map[addr] = value;
    }
}

void Bus::unsafe_write_chr(unsigned short addr, unsigned char value){
    adjust_ppu_mirroring(addr);
    if (addr >= 0x0000 && addr <= 0x1FFF && ((ppustatus >> 7) & 1 || !((ppumask >> 3) & 1) || !((ppumask >> 4) & 1))) {
        mapper.write_chr_ram(addr, value);
    } else if (addr >= 0x2000 && addr <= 0x3fff){
        ppu_map[addr] = value;
    }
}

void Bus::write_reg(Regs r, unsigned short val) {
    mmio_updates.push_front(std::pair{r, val});
}

void Bus::tick() {
    ++master_clock;
    open_bus_decay();
    while(!mmio_updates.empty()){
        auto u = mmio_updates.back(); mmio_updates.pop_back();
        switch (u.first) {
            case PPUCTRL:   ppuctrl =   (unsigned char)u.second; break;
            case PPUMASK:   ppumask =   (unsigned char)u.second; break;
            case PPUSTATUS: ppustatus = (unsigned char)u.second; break;
            case PPUSCROLL: ppuscroll = (unsigned char)u.second; break;
            case PPUADDR:   ppuaddr =   (unsigned char)u.second; break;
            case PPUDATA:   ppudata =   (unsigned char)u.second; break;
            case PPU_V:     ppu_v = u.second; break;
            case PPU_T:     ppu_t = u.second; break;
            case PPU_X:     ppu_x = (unsigned char)u.second; break;
        }
    }
}

void Bus::open_bus_decay() {
    if (open_bus_value && open_bus_value_cycles++ == 5'000){ // todo: check
        open_bus_value_cycles = 0;
        open_bus_value = 0;
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
        case SM::Vertical: { return (n == 0 || n == 2) ? constants::NAME_TABLE0_BEGIN : constants::NAME_TABLE1_BEGIN;}
        case SM::Horizontal: { return (n == 0 || n == 1) ? constants::NAME_TABLE0_BEGIN : constants::NAME_TABLE1_BEGIN;}
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
    ppuctrl = 0;
    ppumask = 0;
    ppustatus = 0;
    oamaddr = 0;
    oamdata = 0;
    ppuscroll = 0;
    ppuaddr = 0;
    ppudata = 0;
    open_bus_value = 0;
    dma_active = false;
    dma_index = 0;
    dma_address = 0;
    nmi_interrupt_flip_flop = false;
    irq_interrupt_flip_flop = false;
    open_bus_value_cycles = 0;
    last_ppu_address = 0;
    mapper.reset();
}
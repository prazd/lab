#include "Bus.hpp"
#include "Constants.hpp"
void Bus::write(unsigned short addr, unsigned char value) {
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
            ppuctrl   =   {value, true};
            break;}
        case constants::PPUMASK:   {
            ppumask   =   {value, true};
            break;}
        case constants::OAMADDR:   { oamaddr   =   {value, true}; break;}
        case constants::OAMDATA:   { oamdata   =   {value, true}; oam[oamaddr.reg++] = value; break;}
        case constants::PPUSCROLL: { ppuscroll =   {value, true}; break;}
        case constants::PPUADDR:   { ppuaddr   =   {value, true}; break;}
        case constants::PPUDATA:   { ppudata   =   {value, true}; break;}
        case constants::OAMDMA : {
            dma_address = value << 8;
            dma_active = true;
            dma_index = 0;
            break;
        }
        case constants::CONTROLLER1: { if (value) c.latch1 = true; else c.update_status1();  break; }
        case constants::CONTROLLER2_AND_APU_FC: { apufc = {value, true};  break; }
    }
}

void Bus::unsafe_write(unsigned short addr, unsigned char value) {
    if (addr >= 0x8000 && addr <= 0xffff){
        mapper.handle(addr, value);
        return;
    }
    switch (addr) {
        case constants::PPUCTRL:   {
            ppuctrl   =   {value, true};
            break;}
        case constants::PPUMASK:   {
            ppumask   =   {value, true};
            break;}
        case constants::OAMADDR:   { oamaddr   =   {value, true}; break;}
        case constants::OAMDATA:   { oamdata   =   {value, true}; oam[oamaddr.reg++] = value; break;}
        case constants::PPUSCROLL: { ppuscroll =   {value, true}; break;}
        case constants::PPUADDR:   { ppuaddr   =   {value, true}; break;}
        case constants::PPUDATA:   { ppudata   =   {value, true}; break;}
        case constants::OAMDMA : {
            dma_address = value << 8;
            dma_active = true;
            dma_start = true;
            dma_index = 0;
            break;
        }
        case constants::CONTROLLER1: { if (value) c.latch1 = true; else c.update_status1();  break; }
        case constants::CONTROLLER2_AND_APU_FC: { apufc = {value, true}; break; }
    }
}

// All regs for debug
unsigned char Bus::read_prg(unsigned short addr) {
    if (addr >= 0x8000 && addr <= 0xffff){
        return mapper.read_prg_rom(addr);
    }
    switch (addr) {
        case constants::PPUCTRL:
        case constants::PPUMASK:
        case constants::PPUSCROLL:
        case constants::PPUADDR:  return open_bus_value;

        case constants::PPUSTATUS: {
//            if (ppustatus.reg & 0x80 && current_ppu_cycle - current_nmi_cycle == 1) {
//                status_read_cycle = current_ppu_cycle;
//                nmi_interrupt_flip_flop = false;
//                auto tmp = ppustatus.reg & 0x7f;
//                ppustatus.reg = 0;
//                return tmp;
//            }
            status_read_cycle = current_ppu_cycle;
            unsigned char current = (ppustatus.reg & 0xe0) | (open_bus_value & 0x1f);
            ppustatus.reg &= 0x60;
            ppustatus.read_requested = true;
            return current;
        }
        case constants::OAMADDR:     return oamaddr.reg;
        case constants::OAMDATA: {
            if (oamaddr.reg == 0x2) {
                open_bus_value = 0;
                open_bus_value_cycles = 0;
            }
            // 3 X
            // 2 Attrib
            // 1 Tile index
            // 0 Y
            if (oamaddr.reg % 4 == 2) { // for attrib
                return oam[oamaddr.reg] & 0xe3;
            }
            return oam[oamaddr.reg];
        }

        case constants::PPUDATA:     {
            if (last_ppu_address >= constants::IMAGE_PALETTE_BEGIN){
                return (ppudata.reg & 0x3f) | ((open_bus_value & 0xc0) & 0xff);
            }
            ppudata.read_requested = true;
            open_bus_value_cycles = 0;
            open_bus_value = ppudata.reg;
            return ppudata.reg;
        }
        case constants::CONTROLLER1: return c.read_status1();
        case constants::CONTROLLER2_AND_APU_FC: return 0;
        default: return 0xff; // dummy
    }
}

unsigned char Bus::unsafe_read_prg(unsigned short addr) {
    if (addr >= 0x8000 && addr <= 0xffff){
        return mapper.read_prg_rom(addr);
    }
    switch (addr) {
        case constants::PPUCTRL:
            return ppuctrl.reg;
        case constants::PPUMASK:
            return ppumask.reg;
        case constants::PPUSTATUS: {
            return ppustatus.reg;
        }
        case constants::OAMADDR:     return oamaddr.reg;
        case constants::OAMDATA: {
            return oam[oamaddr.reg];
        }
        case constants::PPUSCROLL:   return ppuscroll.reg;
        case constants::PPUADDR:     return ppuaddr.reg;
        case constants::PPUDATA:     {return ppudata.reg;}
        case constants::CONTROLLER1: return c.read_status1();
        default: return 0xff; // dummy
    }
}

unsigned char Bus::read_chr(unsigned short addr) {
    return mapper.read_chr_rom(addr);
}

void Bus::tick() {
    open_bus_decay();
}

void Bus::open_bus_decay() {
    if (open_bus_value && open_bus_value_cycles++ == 5'000){ // todo: check
        open_bus_value_cycles = 0;
        open_bus_value = 0;
    }
}

void Bus::reset(){
    ppuctrl.reg = 0;
    ppuctrl.reg = 0;
    ppumask.reg = 0;
    ppustatus.reg = 0;
    oamaddr.reg = 0;
    oamdata.reg = 0;
    ppuscroll.reg = 0;
    ppuaddr.reg = 0;
    ppudata.reg = 0;
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
#include "Bus.hpp"
#include "Constants.hpp"
void Bus::write(unsigned short addr, unsigned char value) {
    if (addr >= 0x8000 && addr <= 0xffff){
        if (mapper.mtype == Mapper::MapperType::MMC1){
            mapper.handle(addr, value);
        }
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
    }
}

// All regs for debug
unsigned char Bus::read_prg(unsigned short addr) {
    if (addr >= 0x8000 && addr <= 0xffff){
        return mapper.read_prg_rom(addr);
    }
    switch (addr) {
        case constants::PPUCTRL:
            return open_bus_value;
        case constants::PPUMASK:
            return open_bus_value;
        case constants::PPUSTATUS: {
            auto current = (ppustatus.reg & 0xe0) | open_bus_value;
            ppustatus.reg = 0;
            ppustatus.was_written = true;
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
        case constants::PPUSCROLL:   return open_bus_value;
        case constants::PPUADDR:     return open_bus_value;
        case constants::PPUDATA:     {
            if (last_ppu_address >= constants::IMAGE_PALETTE_BEGIN){
                return (ppudata.reg & 0x3f) | ((open_bus_value & 0xc0) & 0xff);
            }
            open_bus_value_cycles = 0;
            open_bus_value = ppudata.reg;
            return ppudata.reg;
        }
        case constants::CONTROLLER1: return c.read_status1();
        default: return 0xff; // dummy
    }
}

unsigned char Bus::read_chr(unsigned short addr) {
    return mapper.read_chr_rom(addr);
}

void Bus::dma_tick(unsigned char val) {
    if (dma_active){
        oam[dma_index++] = val;
        dma_active = (dma_index != 256);
    }
}

void Bus::open_bus_decay() {
    if (open_bus_value && open_bus_value_cycles++ == 10'000){ // todo: check
        open_bus_value_cycles = 0;
        open_bus_value = 0;
    }
}
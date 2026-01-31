#include "Bus.hpp"
#include "Constants.hpp"

void Bus::write(unsigned short addr, unsigned char value) {
    if (addr >= 0x8000 && addr <= 0xffff){
        if (mapper.mtype == Mapper::MapperType::MMC1){
            mapper.mmc1.handle(addr, value);
        }
    }
    switch (addr) {
        case constants::PPUCTRL:   { ppuctrl   =   {value, true}; break;}
        case constants::PPUMASK:   { ppumask   =   {value, true}; break;}
        case constants::OAMADDR:   { oamaddr   =   {value, true}; break;}
        case constants::OAMDATA:   { oamdata   =   {value, true}; break;}
        case constants::PPUSCROLL: { ppuscroll =   {value, true}; break;}
        case constants::PPUADDR:   { ppuaddr   =   {value, true}; break;}
        case constants::PPUDATA:   { ppudata   =   {value, true}; break;}
        case constants::OAMDMA : {
            dma_address = value * 256;
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
        case constants::PPUCTRL: return ppuctrl.reg;
        case constants::PPUMASK: return ppumask.reg;
        case constants::PPUSTATUS: return ppustatus.reg;
        case constants::OAMADDR: return oamaddr.reg;
        case constants::OAMDATA: return oamdata.reg;
        case constants::PPUSCROLL: return ppuscroll.reg;
        case constants::PPUADDR: return ppuaddr.reg;
        case constants::PPUDATA: return ppudata.reg;
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
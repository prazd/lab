#include "Bus.hpp"
#include "Constants.hpp"

void Bus::write(unsigned short addr, unsigned char value) {
    if (addr == constants::PPUCTRL)   {ppuctrl   =   {value, true}; return;}
    if (addr == constants::PPUMASK)   {ppumask   =   {value, true}; return;}
    if (addr == constants::OAMADDR)   {oamaddr   =   {value, true}; return;}
    if (addr == constants::OAMDATA)   {oamdata   =   {value, true}; return;}
    if (addr == constants::PPUSCROLL) {ppuscroll =   {value, true}; return;}
    if (addr == constants::PPUADDR)   {ppuaddr   =   {value, true}; return;}
    if (addr == constants::PPUDATA)   {ppudata   =   {value, true}; return;}
    if (addr == constants::OAMDMA)    {dma_address = value * 256; dma_active = true; dma_index = 0;  return;}
}

unsigned char Bus::read(unsigned short addr) {
    if (addr == constants::PPUSTATUS) return ppustatus.reg;
    if (addr == constants::OAMDATA) return oamdata.reg;
    if (addr == constants::PPUDATA) return ppudata.reg;
    // dummy
    return 0xff;
}

unsigned char* Bus::read_ptr(unsigned short addr) {
    if (addr == constants::PPUSTATUS) return &ppustatus.reg;
    if (addr == constants::OAMDATA) return &oamdata.reg;
    if (addr == constants::PPUDATA) return &ppudata.reg;
    // dummy
    return nullptr;
}

void Bus::dma_tick(unsigned char val) {
    if (dma_active){
        oam[dma_index++] = val;
    }
    if (dma_index == 256) {
        dma_active = false;
    }
}
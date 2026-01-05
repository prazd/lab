#include "Memory.hpp"
#include "Constants.h"

void Memory::cpu_write(unsigned short addr, unsigned char value) {
    cpu_adjust_if_mirror(addr);
    if (addr == constants::PPUCTRL)   {ppuctrl =   value; return;}
    if (addr == constants::PPUMASK)   {ppumask =   value; return;}
    if (addr == constants::OAMADDR)   {oamaddr =   value; return;}
    if (addr == constants::OAMDATA)   {oamdata =   value; return;}
    if (addr == constants::PPUSCROLL) {ppuscroll = value; return;}
    if (addr == constants::PPUADDR)   {ppuaddr =   value; return;}
    if (addr == constants::PPUDATA)   {ppudata =   value; return;}
    if (addr == constants::OAMDMA)    {dma_source_high_byte =   value; return;}
    cpu_map[addr] = value;
}

unsigned char Memory::cpu_read(unsigned short addr) const {
    cpu_adjust_if_mirror(addr);
    if (addr == constants::PPUSTATUS) return ppustatus;
    if (addr == constants::OAMDATA) return oamdata;
    if (addr == constants::PPUDATA) return ppudata;
    return cpu_map[addr];
}

unsigned char* Memory::cpu_read_addr(unsigned short addr) {
    cpu_adjust_if_mirror(addr);
    if (addr == constants::PPUSTATUS) return &ppustatus;
    if (addr == constants::OAMDATA) return &oamdata;
    if (addr == constants::PPUDATA) return &ppudata;
    return cpu_map + addr;
}

void Memory::cpu_adjust_if_mirror(unsigned short& addr) {
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

void Memory::ppu_write(unsigned short addr, unsigned char value) {
    ppu_adjust_if_mirror(addr);
    ppu_map[addr] = value;
}

unsigned char Memory::ppu_read(unsigned short addr) const {
    ppu_adjust_if_mirror(addr);
    return ppu_map[addr];
}

void Memory::ppu_adjust_if_mirror(unsigned short& addr) {
    // TODO: check palettes
    if (addr >= constants::PALETTES_MIRROR_BEGIN && addr <= constants::PALETTES_MIRROR_END){
        addr = constants::IMAGE_PALETTE_BEGIN + (addr % 32);
    } else if (addr >= constants::TABLES_MIRROR_BEGIN && addr <= constants::TABLES_MIRROR_END){
        addr = constants::NAME_TABLE0_BEGIN + (addr - constants::TABLES_MIRROR_BEGIN);
    }
}
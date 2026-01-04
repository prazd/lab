#include "Memory.hpp"
#include "Constants.h"

void Memory::write(unsigned short addr, unsigned char value) {
    adjust_if_mirror(addr);
    map[addr] = value;
}

unsigned char Memory::read(unsigned short addr) const {
    adjust_if_mirror(addr);
    return map[addr];
}

void Memory::adjust_if_mirror(unsigned short& addr) {
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

unsigned char* Memory::read_addr(unsigned short addr) {
    adjust_if_mirror(addr);
    return map + addr;
}
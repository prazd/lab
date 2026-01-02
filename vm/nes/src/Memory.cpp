#include "Memory.hpp"

void Memory::write(unsigned short addr, unsigned char value) {
    map[addr] = value;
}

unsigned char Memory::read(unsigned short addr) const {
    return map[addr];
}

unsigned char* Memory::read_addr(unsigned short addr) {
    return map + addr;
}
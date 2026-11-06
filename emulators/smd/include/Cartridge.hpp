#ifndef SEGAMEGADRIVE_CARTRIDGE_H
#define SEGAMEGADRIVE_CARTRIDGE_H
#include <vector>
#include <string_view>
#include <span>
#include <CPU.hpp>
#include "Simple.hpp"
// https://segaretro.org/images/1/18/GenesisTechnicalOverview.pdf - page 83
struct Cartridge{
    // 0x100: 'SEGA MEGA DRIVE '
    // 0x110: '(C)SEGA 1988.JUL'
    // 0x120: GAME NAME (DOMESTIC)
    // 0x150: GAME NAME (OVERSEAS)
    // 0x180: 'GM XXXXXXX-XX'
    // 0x18E: $XXXX
    // 0x190: CONTROL DATA
    // 0x1A0: $000000, $XXXXXX
    // 0x1A8: $FFOOOO, $FFFFFF
    // 0xlB0: EXTERNAL RAM DATA
    // 0x1BC: MODEM DATA
    // 0x1C8: MEMO
    // 0x1F0: Country in which the product can be released.
    std::vector<char> buffer;
    size_t rom_sz{};
    unsigned m68k_entry_point{};
    inline Cartridge(std::string_view sv){
        read_file(sv, buffer);
        memcpy(&rom_sz, buffer.data() + 0x1A0, sizeof(rom_sz));
        rom_sz += 1;
        m68k_entry_point = M68K::read_mem(M68K::LONG, (unsigned char*)(buffer.data() + 0x4));
    }
};
#endif

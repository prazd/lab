#ifndef NES_CONSTANTS_H
#define NES_CONSTANTS_H
#include <iostream>
namespace constants {
    constexpr unsigned short RAM_BEGIN = 0x0000;
    constexpr unsigned short RAM_END = 0x07FF;

    constexpr unsigned short RAM_MIRROR1_BEGIN = 0x800;
    constexpr unsigned short RAM_MIRROR1_END = 0xFFF;
    constexpr unsigned short RAM_MIRROR2_BEGIN = 0x1000;
    constexpr unsigned short RAM_MIRROR2_END = 0x17FF;
    constexpr unsigned short RAM_MIRROR3_BEGIN = 0x1800;
    constexpr unsigned short RAM_MIRROR3_END = 0x1FFF;


// PPU
    constexpr unsigned short PPU_REGS_BEGIN = 0x2000;
    constexpr unsigned short PPU_REGS_END = 0x2007;

    constexpr unsigned short PPU_REGS_MIRROR_BEGIN = 0x2008;
    constexpr unsigned short PPU_REGS_MIRROR_END = 0x3FFF;

    constexpr unsigned short PPUCTRL = 0x2000;
    constexpr unsigned short PPUMASK = 0x2001;
    constexpr unsigned short PPUSTATUS = 0x2002;
    constexpr unsigned short OAMADDR = 0x2003;
    constexpr unsigned short OAMDATA = 0x2004;
    constexpr unsigned short PPUSCROLL = 0x2005;
    constexpr unsigned short PPUADDR = 0x2006;
    constexpr unsigned short PPUDATA = 0x2007;
    constexpr unsigned short OAMDMA = 0x4014;

}
#endif //NES_CONSTANTS_H

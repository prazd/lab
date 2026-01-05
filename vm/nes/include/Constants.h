#ifndef NES_CONSTANTS_H
#define NES_CONSTANTS_H
#include <iostream>
namespace constants {
// CPU map
    constexpr unsigned short RAM_BEGIN = 0x0000;
    constexpr unsigned short RAM_END = 0x07FF;

    constexpr unsigned short RAM_MIRROR1_BEGIN = 0x800;
    constexpr unsigned short RAM_MIRROR1_END = 0xFFF;
    constexpr unsigned short RAM_MIRROR2_BEGIN = 0x1000;
    constexpr unsigned short RAM_MIRROR2_END = 0x17FF;
    constexpr unsigned short RAM_MIRROR3_BEGIN = 0x1800;
    constexpr unsigned short RAM_MIRROR3_END = 0x1FFF;

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

// PPU map
    constexpr unsigned short PATTERN_TABLE0_BEGIN = 0x0000;
    constexpr unsigned short PATTERN_TABLE0_END = 0x0FFF;
    constexpr unsigned short PATTERN_TABLE1_BEGIN = 0x1000;
    constexpr unsigned short PATTERN_TABLE1_END = 0x1FFF;
    constexpr unsigned short NAME_TABLE0_BEGIN = 0x2000;
    constexpr unsigned short NAME_TABLE0_END = 0x23BF;
    constexpr unsigned short ATTRIBUTE_TABLE0_BEGIN = 0x23C0;
    constexpr unsigned short ATTRIBUTE_TABLE0_END = 0x23FF;
    constexpr unsigned short NAME_TABLE1_BEGIN = 0x2400;
    constexpr unsigned short NAME_TABLE1_END = 0x27BF;
    constexpr unsigned short ATTRIBUTE_TABLE1_BEGIN = 0x27C0;
    constexpr unsigned short ATTRIBUTE_TABLE1_END = 0x27FF;
    constexpr unsigned short NAME_TABLE2_BEGIN = 0x2800;
    constexpr unsigned short NAME_TABLE2_END = 0x2BBF;
    constexpr unsigned short ATTRIBUTE_TABLE2_BEGIN = 0x2BC0;
    constexpr unsigned short ATTRIBUTE_TABLE2_END = 0x2BFF;
    constexpr unsigned short NAME_TABLE3_BEGIN = 0x2C00;
    constexpr unsigned short NAME_TABLE3_END = 0x2FBF;
    constexpr unsigned short ATTRIBUTE_TABLE3_BEGIN = 0x2FC0;
    constexpr unsigned short ATTRIBUTE_TABLE3_END = 0x2FFF;

    constexpr unsigned short TABLES_MIRROR_BEGIN = 0x3000;
    constexpr unsigned short TABLES_MIRROR_END = 0x3EFF;

    constexpr unsigned short IMAGE_PALETTE_BEGIN = 0x3F00;
    constexpr unsigned short IMAGE_PALETTE_END = 0x3F0F;
    constexpr unsigned short SPRITE_PALETTE_BEGIN = 0x3F10;
    constexpr unsigned short SPRITE_PALETTE_END = 0x3F1F;

    constexpr unsigned short PALETTES_MIRROR_BEGIN = 0x3F20;
    constexpr unsigned short PALETTES_MIRROR_END = 0x3FFF;
}
#endif //NES_CONSTANTS_H

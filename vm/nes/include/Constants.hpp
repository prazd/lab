#ifndef NES_CONSTANTS_HPP
#define NES_CONSTANTS_HPP
#include <iostream>
#define ROM_SELECT_ON 1
#define APU_TICK_ON 1
#define APU_ON 1
#define HIDE_OVERSCAN 1
#define DEBUG_MODE 1
#define GAME_GENIE_MODE 0
#define MULTI_TEST_MODE 0

#if !MULTI_TEST_MODE
#define VEC_MODE static inline
#else
#define VEC_MODE
#undef DEBUG_MODE
#undef GAME_GENIE_MODE
#define DEBUG_MODE 0
#define GAME_GENIE_MODE 0
#endif

#define HEX_COUT(val, n) std::hex << std::setw(n) << std::setfill('0') << val
#define HEX_COUT_W2(val) HEX_COUT((int)val, 2)

namespace constants {
    constexpr float WIDTH = 600.f;
    constexpr float HEIGHT = 600.f;

    constexpr float DEBUG_WIDTH = 1600.f;
    constexpr float DEBUG_HEIGHT = 1200.f;

    constexpr size_t DISPLAY_PIXELS_WIDTH = 256;
    constexpr size_t DISPLAY_PIXELS_HEIGHT = 240;

    constexpr unsigned short CONTROLLER1 = 0x4016;

    // - read after latch (4016 write) - controller 2
    // - write - apu frame counter
    constexpr unsigned short CONTROLLER2_AND_APU_FC = 0x4017;

    constexpr size_t CPU_SPEED = 1790000;
    constexpr size_t CPU_FRAME_CYCLE = CPU_SPEED/60;
    constexpr size_t PPU_FRAME_CYCLE = CPU_FRAME_CYCLE * 3;
    constexpr float CPU_SPEED_f = 1790000.f;

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

    constexpr unsigned short CARTRIDGE_RAM_BEGIN = 0x6000;

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

    constexpr unsigned short NMI_INTERRUPT_HANDLER_L = 0xFFFA;
    constexpr unsigned short RESET_INTERRUPT_HANDLER_L = 0xFFFC;
    constexpr unsigned short IRQ_INTERRUPT_HANDLER_L = 0xFFFE;

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

// APU
    constexpr unsigned short PULSE1_DUTY_ENVELOPE = 0x4000;
    constexpr unsigned short PULSE1_SWEEP = 0x4001;
    constexpr unsigned short PULSE1_TIMER_LOW = 0x4002;
    constexpr unsigned short PULSE1_TIMER_HIGH = 0x4003;

    constexpr unsigned short PULSE2_DUTY_ENVELOPE = 0x4004;
    constexpr unsigned short PULSE2_SWEEP = 0x4005;
    constexpr unsigned short PULSE2_TIMER_LOW = 0x4006;
    constexpr unsigned short PULSE2_TIMER_HIGH = 0x4007;

    constexpr unsigned short TRIANGLE_CHAN_LINEAR_COUNTER = 0x4008;
    constexpr unsigned short TRIANGLE_CHAN_TIMER_LOW = 0x400a;
    constexpr unsigned short TRIANGLE_CHAN_TIMER_HIGH = 0x400b;

    constexpr unsigned short NOISE_CHAN_ENVELOPE = 0x400c;
    constexpr unsigned short NOISE_CHAN_LOOP = 0x400e;
    constexpr unsigned short NOISE_CHAN_LENGTH_COUNTER = 0x400f;

    constexpr unsigned short DMC_CHAN_FREQ = 0x4010;
    constexpr unsigned short DMC_CHAN_DIRECT_LOAD = 0x4011;
    constexpr unsigned short DMC_CHAN_SAMPLE_ADDRESS = 0x4012;
    constexpr unsigned short DMC_CHAN_SAMPLE_LENGTH = 0x4013;

    constexpr unsigned short APU_CONTROL = 0x4015;
}
#endif //NES_CONSTANTS_HPP

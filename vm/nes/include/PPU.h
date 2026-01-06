#ifndef NES_PPU_H
#define NES_PPU_H
#include <array>
#include "Memory.hpp"
struct PPU {
    // Current VRAM address (15 bits)
    unsigned short v{};
    // Temporary VRAM address (15 bits); can also be thought of as the address of the top left onscreen tile.
    unsigned short t{};
    // Fine X scroll (3 bits)
    unsigned char x{};
    // First or second write toggle (1 bit)
    bool w{};

    union PPUCTRL {
        unsigned char reg;
        struct {
            unsigned char base_name_table_addr: 2;
            unsigned char addr_inc:1;
            unsigned char pattern_table_addr8x8:1;
            unsigned char bg_pattern_table_addr:1;
            unsigned char sprite_sz:1;
            unsigned char master_slave_select:1;
            unsigned char vblank_nmi_enable:1;
        } fields;
    } ppuctrl;

    union PPUMASK{
        unsigned char reg;
        struct {
            unsigned char greyscale: 1;
            unsigned char show_bg_leftmost_8_pixels:1;
            unsigned char show_sprite_leftmost_8_pixels:1;
            unsigned char enable_background_rendering:1;
            unsigned char enable_sprite_rendering:1;
            unsigned char emphasize_red:1;
            unsigned char emphasize_green:1;
            unsigned char emphasize_blue:1;
        } fields;
    } ppumask;

    union PPUSTATUS{
        unsigned char reg;
        struct {
            unsigned char ppu_identifier: 5;
            unsigned char sprite_overflow_flag:1;
            unsigned char sprite_zero_hit_flag:1;
            unsigned char vblank_flag:1;
        } fields;
    } ppustatus;


    unsigned short base_name_table_address{};
    bool address_increment_on_read_ppudata = false; //    RAM address increment per CPU read/write of PPUDATA
    unsigned short sprite_pattern_table_address8x8{};
    enum SpriteMode {
        m8x8,
        m8x16
    } sprite_mode = m8x8;
    bool vblank_nmi_enable = false;

    unsigned char x_scroll{};
    unsigned char y_scroll{};

// REGS
    void read_ppuctrl(Memory& mem);
    void read_ppuscroll(Memory& mem);
    void read_ppuaddr(Memory& mem);
    void update_state(Memory& mem);


    struct RGB {
        unsigned char r{};
        unsigned char g{};
        unsigned char b{};
    };

    std::array<std::array<RGB, 256>, 240> display{};

    std::array<RGB, 64> palette{
        RGB{0x75, 0x75,  0x75},
        RGB{0x27, 0x1B,  0x8F},
        RGB{0x00, 0x00,  0xAB},
        RGB{0x47,0x00,0x9F},
        RGB{0x8F,0x00,0x77},
        RGB{0xAB, 0x00, 0x13},
        RGB{0xA7, 0x00, 0x00},
        RGB{0x7F, 0x0B, 0x00},
        RGB{0x43, 0x2F, 0x00},
        RGB{0x00, 0x47, 0x00 },
        RGB{0x00, 0x51, 0x00 },
        RGB{0x00, 0x3F, 0x17},
        RGB{0x1B, 0x3F, 0x5F},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
        RGB{0xBC, 0xBC, 0xBC},
        RGB{0x00, 0x73, 0xEF},
        RGB{0x23, 0x3B, 0xEF},
        RGB{0x83, 0x00, 0xF3},
        RGB{0xBF, 0x00, 0xBF},
        RGB{0xE7, 0x00, 0x5B},
        RGB{0xDB, 0x2B, 0x00},
        RGB{0xCB, 0x4F, 0x0F},
        RGB{0x8B, 0x73, 0x00},
        RGB{0x00, 0x97, 0x00},
        RGB{0x00, 0xAB, 0x00},
        RGB{0x00, 0x93, 0x3B},
        RGB{0x00, 0x83, 0x8B},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
        RGB{0xFF, 0xFF, 0xFF},
        RGB{0x3F, 0xBF, 0xFF},
        RGB{0x5F, 0x97, 0xFF},
        RGB{0xA7, 0x8B, 0xFD},
        RGB{0xF7, 0x7B, 0xFF},
        RGB{0xFF, 0x77, 0xB7},
        RGB{0xFF, 0x77, 0x63},
        RGB{0xFF, 0x9B, 0x3B},
        RGB{0xF3, 0xBF, 0x3F},
        RGB{0x83, 0xD3, 0x13},
        RGB{0x4F, 0xDF, 0x4B},
        RGB{0x58, 0xF8, 0x98},
        RGB{0x00, 0xEB, 0xDB},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
        RGB{0xFF, 0xFF, 0xFF},
        RGB{0xAB, 0xE7, 0xFF},
        RGB{0xC7, 0xD7, 0xFF},
        RGB{0xD7, 0xCB, 0xFF},
        RGB{0xFF, 0xC7, 0xFF},
        RGB{0xFF, 0xC7, 0xDB},
        RGB{0xFF, 0xBF, 0xB3},
        RGB{0xFF, 0xDB, 0xAB},
        RGB{0xFF, 0xE7, 0xA3},
        RGB{0xE3, 0xFF, 0xA3},
        RGB{0xAB, 0xF3, 0xBF},
        RGB{0xB3, 0xFF, 0xCF},
        RGB{0x9F, 0xFF, 0xF3},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
        RGB{0x00, 0x00, 0x00},
    };

//  Pattern table
    template <typename = void>
    auto pattern_table(std::array<unsigned char, 8> l,  std::array<unsigned char, 8> r){
        std::array<std::array<unsigned char, 8>, 8> res{};
        for (int i = 0 ; i < 8; ++i){
            for (int j = 0; j < 8; ++j){
                res[i][7-j] = ((l[i] >> j) & 1) | (((r[i] >> j) & 1) << 1);
            }
        }
        return res;
    }
};
#endif //NES_PPU_H

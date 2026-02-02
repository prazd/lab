#ifndef NES_PPU_HPP
#define NES_PPU_HPP
#include <array>
#include "Bus.hpp"
#include "Constants.hpp"
#include "raylib.h"
#include <tuple>

// TODO: https://www.nesdev.org/wiki/Mirroring + Attributes
inline bool operator==(const Color& lhs, const Color& rhs){
    return std::tuple(rhs) == std::tuple(lhs);
}

inline Color color_by_id(unsigned char id) {
    switch (id) {
        case 0b01: return BLACK;
        case 0b10: return GRAY;
        case 0b11: return DARKGRAY;
        default: return WHITE;
    }
}

inline unsigned char id_by_color(Color c) {
    if (c == BLACK){
        return 0b01;
    } else if (c == GRAY) {
        return 0b10;
    } else if (c == DARKGRAY){
        return 0b11;
    }
    return 0b00;
}

struct PPU {
    struct Memory {
// https://www.nesdev.org/wiki/PPU_memory_map
//  $0000-$0FFF	$1000	Pattern table 0	Cartridge
//  $1000-$1FFF	$1000	Pattern table 1	Cartridge
//  $2000-$23BF	$03c0	Nametable 0	Cartridge
//  $23C0-$23FF	$0040	Attribute table 0 Cartridge
//  $2400-$27BF	$03c0	Nametable 1	Cartridge
//  $27C0-$27FF	$0040	Attribute table 1 Cartridge
//  $2800-$2BBF	$03c0	Nametable 2	Cartridge
//  $2BC0-$2BFF	$0040	Attribute table 2 Cartridge
//  $2C00-$2FBF	$03c0	Nametable 3	Cartridge
//  $2FC0-$2FFF	$0040	Attribute table 3 Cartridge
//  $3000-$3EFF	$0F00	Table Mirror $2000-$2EFF
//  $3F00-$3F1F	$0020	Palette RAM indexes	Internal to PPU
//  $3F20-$3FFF	$00E0	Mirrors of $3F00-$3F1F	Internal to PPU

// Pattern table - Tiles (2BPP) (256 * 16(byte) = 4096)  The pattern tables store the least significant two bits of the 4-bit
// number needed to identify the image or sprite palette entry used by that pixel

// Nametable - 32x30 tiles numbers (0 - 255)
// Attribute - Attribute tables hold the upper two bits of
// the colours for the tiles
        unsigned char map[0x4000]{};
        unsigned char read(Bus& bus, unsigned short addr) const;
        void write(Bus& bus, unsigned short addr, unsigned char value);
        static void adjust_if_mirror(unsigned short&);
    } mem;

    // Current VRAM address (15 bits)
    //  yyy NN YYYYY XXXXX
    //  ||| || ||||| +++++-- coarse X (NT)
    //  ||| || +++++-------- coarse Y (NT)
    //  ||| ++--------------- nametable
    //  +++------------------ fine Y
    unsigned short v{};

    // Temporary VRAM address (15 bits); can also be thought of as the address of the top left onscreen tile.
    unsigned short t{};
    // Fine X scroll (3 bits)
    unsigned char x{};
    // First or second write toggle (1 bit)
    bool w{};

    size_t current_cycle{};

    enum FetchStep {
        NT, AT, PTL, PTH
    } current_step = NT;

    void tick(Bus&);

    struct Sprite {
        unsigned char coord{}; //  Stores the y-coordinate of the top left of the sprite minus 1
        unsigned char index{}; //  Index number of the sprite in the pattern tables.
        struct {
            unsigned char color: 2{}; // Most significant two bits of the colour
            unsigned char: 3;
            unsigned char priority_flag:1{}; // Indicates whether this sprite has priority over the background
            unsigned char flip_horizontally:1{};
            unsigned char flip_vertically:1{};
            inline void set(unsigned char val){
                color = val & 0x3;
                priority_flag = (val >> 5) & 1;
                flip_horizontally = (val >> 6) & 1;
                flip_vertically = (val >> 7) & 1;
            };
        } attrib;
    };

    struct PPUCTRL {
        unsigned char base_name_table_addr: 2;
        unsigned char addr_inc:1;
        unsigned char pattern_table_addr8x8:1;
        unsigned char bg_pattern_table_addr:1;
        unsigned char sprite_sz:1;
        unsigned char master_slave_select:1;
        unsigned char vblank_nmi_enable:1;
        inline void set(unsigned char val){
            base_name_table_addr = val & 0x3;
            addr_inc = (val >> 2) & 1;
            pattern_table_addr8x8 = (val >> 3) & 1;
            bg_pattern_table_addr = (val >> 4) & 1;
            sprite_sz = (val >> 5) & 1;
            master_slave_select = (val >> 6) & 1;
            vblank_nmi_enable = (val >> 7) & 1;
        };
    } ppuctrl;

    struct PPUMASK{
        unsigned char greyscale: 1;
        unsigned char show_bg_leftmost_8_pixels:1;
        unsigned char show_sprite_leftmost_8_pixels:1;
        unsigned char enable_background_rendering:1;
        unsigned char enable_sprite_rendering:1;
        unsigned char emphasize_red:1;
        unsigned char emphasize_green:1;
        unsigned char emphasize_blue:1;
        inline void set(unsigned char val){
            greyscale = val & 1;
            show_bg_leftmost_8_pixels = (val >> 1) & 1;
            show_sprite_leftmost_8_pixels = (val >> 2) & 1;
            enable_background_rendering = (val >> 3) & 1;
            enable_sprite_rendering = (val >> 4) & 1;
            emphasize_red = (val >> 5) & 1;
            emphasize_green = (val >> 6) & 1;
            emphasize_blue = (val >> 7) & 1;
        };
    } ppumask;

    struct PPUSTATUS{
        unsigned char ppu_identifier: 5;
        unsigned char sprite_overflow_flag:1;
        unsigned char sprite_zero_hit_flag:1;
        unsigned char vblank_flag:1;
        inline void set(unsigned char val) {
            ppu_identifier = val & 0x1f;
            sprite_overflow_flag = (val >> 5) & 1;
            sprite_zero_hit_flag = (val >> 6) & 1;
            vblank_flag = (val >> 7) & 1;
        }
    } ppustatus;

    unsigned short base_name_table_address{};
    unsigned short bg_pattern_table_address{};
    unsigned short base_attrib_address{};
    unsigned short sprite_pattern_table_address8x8{};

    unsigned short shift_register_low{};
    unsigned short shift_register_high{};

    bool address_increment_on_read_ppudata = false; //    RAM address increment per CPU read/write of PPUDATA

    enum SpriteMode {
        m8x8,
        m8x16
    } sprite_mode = m8x8;
    bool vblank_nmi_enable = false;

    unsigned char x_scroll{};
    unsigned char y_scroll{};

    void reset();

// REGS
    void read_ppuctrl(Bus& mem);
    void read_ppuscroll(Bus& mem);
    void read_ppuaddr(Bus& mem);
    void update_state(Bus& mem);

    std::array<Color, 256 * 240> internal_display{};
    size_t current_x = 0;
    size_t current_y = 261;
    inline void update_internal_display(Color pixel){
        internal_display[current_x++] = pixel;
    }
    std::array<Color, 256 * 240> display{};

    std::array<Color, 64> palette{
        Color{0x75, 0x75,  0x75, 0xFF},
        Color{0x27, 0x1B,  0x8F, 0xFF},
        Color{0x00, 0x00,  0xAB, 0xFF},
        Color{0x47,0x00,   0x9F, 0xFF},
        Color{0x8F,0x00,   0x77, 0xFF},
        Color{0xAB, 0x00, 0x13,  0xFF},
        Color{0xA7, 0x00, 0x00,  0xFF},
        Color{0x7F, 0x0B, 0x00,  0xFF},
        Color{0x43, 0x2F, 0x00,  0xFF},
        Color{0x00, 0x47, 0x00,  0xFF},
        Color{0x00, 0x51, 0x00,  0xFF},
        Color{0x00, 0x3F, 0x17,  0xFF},
        Color{0x1B, 0x3F, 0x5F,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0xBC, 0xBC, 0xBC,  0xFF},
        Color{0x00, 0x73, 0xEF,  0xFF},
        Color{0x23, 0x3B, 0xEF,  0xFF},
        Color{0x83, 0x00, 0xF3,  0xFF},
        Color{0xBF, 0x00, 0xBF,  0xFF},
        Color{0xE7, 0x00, 0x5B,  0xFF},
        Color{0xDB, 0x2B, 0x00,  0xFF},
        Color{0xCB, 0x4F, 0x0F,  0xFF},
        Color{0x8B, 0x73, 0x00,  0xFF},
        Color{0x00, 0x97, 0x00,  0xFF},
        Color{0x00, 0xAB, 0x00,  0xFF},
        Color{0x00, 0x93, 0x3B,  0xFF},
        Color{0x00, 0x83, 0x8B,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0xFF, 0xFF, 0xFF,  0xFF},
        Color{0x3F, 0xBF, 0xFF,  0xFF},
        Color{0x5F, 0x97, 0xFF,  0xFF},
        Color{0xA7, 0x8B, 0xFD,  0xFF},
        Color{0xF7, 0x7B, 0xFF,  0xFF},
        Color{0xFF, 0x77, 0xB7,  0xFF},
        Color{0xFF, 0x77, 0x63,  0xFF},
        Color{0xFF, 0x9B, 0x3B,  0xFF},
        Color{0xF3, 0xBF, 0x3F,  0xFF},
        Color{0x83, 0xD3, 0x13,  0xFF},
        Color{0x4F, 0xDF, 0x4B,  0xFF},
        Color{0x58, 0xF8, 0x98,  0xFF},
        Color{0x00, 0xEB, 0xDB,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0xFF, 0xFF, 0xFF,  0xFF},
        Color{0xAB, 0xE7, 0xFF,  0xFF},
        Color{0xC7, 0xD7, 0xFF,  0xFF},
        Color{0xD7, 0xCB, 0xFF,  0xFF},
        Color{0xFF, 0xC7, 0xFF,  0xFF},
        Color{0xFF, 0xC7, 0xDB,  0xFF},
        Color{0xFF, 0xBF, 0xB3,  0xFF},
        Color{0xFF, 0xDB, 0xAB,  0xFF},
        Color{0xFF, 0xE7, 0xA3,  0xFF},
        Color{0xE3, 0xFF, 0xA3,  0xFF},
        Color{0xAB, 0xF3, 0xBF,  0xFF},
        Color{0xB3, 0xFF, 0xCF,  0xFF},
        Color{0x9F, 0xFF, 0xF3,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
        Color{0x00, 0x00, 0x00,  0xFF},
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

    unsigned char tile_byte_from_addr(Bus& bus, unsigned short addr);
    //    0HNNNN NNNNPyyy
    unsigned short to_pattern_addr(unsigned char y, unsigned char tile_n, bool plane, bool h);
};
#endif //NES_PPU_HPP

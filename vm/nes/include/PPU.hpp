#ifndef NES_PPU_HPP
#define NES_PPU_HPP
#include <array>
#include "Bus.hpp"
#include "Constants.hpp"
#include "raylib.h"
#include <tuple>
#include "Simple.hpp"

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
    unsigned short last_address = constants::PATTERN_TABLE1_BEGIN;

    size_t current_cycle{};
    size_t frames_count{};

#if DEBUG_MODE
    bool frame_start = false;
    bool tile_start = false;
    bool update_shifts = false;
    bool line_start = false;
    size_t last_line_x{};
    size_t current_line_x{};
    bool is_sprite_off = false;
    bool run = false;
    bool fill_white_background = true;


    struct PixelMixingDebugView {
        enum Mode: unsigned char {
            Bg = 0,
            Sp = 1,
            BgSp = 2,
            NBgSp = 3,
            SpriteOnly = 4,
            BgOnly = 5,
        };
        unsigned char data{};
        inline void set(bool val, Mode m){ data &= ((~(1 << m)) & 0x7); data |= (val << m);}
        inline void m_switch(Mode m){ data ^= (1 << m);}
        inline bool get(Mode m){return (data >> m) & 1;}
    } pixel_mixing_debug_view{};

#endif

    enum FetchStep {
        NT, AT, PTL, PTH
    } current_step = NT;

    void tick(Bus&);

    void secondary_oam_fill(Bus& bus);

    struct Sprite {
        unsigned char y_pos{};
        unsigned char pt_index{}; //  Index number of the sprite in the pattern tables.
        unsigned char attrib{};
        unsigned char x_pos{};
        // internal
        unsigned char sprite_index{};
    };

    struct SpriteData {
        unsigned char shift_l{};
        unsigned char shift_h{};
        unsigned char x_pos{};
        unsigned char attrib{};
        unsigned char oam_index{};
    };

    SimpleDequeTC<Sprite, 8> secondary_oam_buffer{};
    SimpleDequeTC<SpriteData, 8> secondary_oam{};

    void draw_pixel(Bus& bus);

    unsigned short current_scanline_cycle = 0;
    unsigned char nt_byte = 0;
    unsigned char attrib_value = 0;
    unsigned char pattern_table_l = 0;
    unsigned char pattern_table_h = 0;

    void fetch_tile_data_step(Bus& bus);
    void tile_data_fetch(Bus& bus);
    void tile_data_dummy_fetch(Bus& bus);

    unsigned short base_name_table_address{};

    enum SpriteEvaluationState {
        SpriteCopy,
        SpriteIncrement,
        SpriteOverflow,
        SpriteFinish
    } current_sprite_evaluation_step = SpriteCopy;
    bool copy_in_progress = false;
    unsigned char tmp_oam_value = 0;

    size_t copy_step_count = 0;
    Sprite tmp_sprite;

    size_t current_oam_index = 0;
    size_t sprite_evaluation_index = 0;
    void sprite_evaluate_tick(Bus& bus);

    void bg_tick(Bus& bus);

    unsigned short shift_register_low{};
    unsigned short shift_register_high{};

    unsigned short attributes_low{};
    unsigned short attributes_high{};

    void reset();
// REGS
    VEC_MODE std::array<std::array<Color, 256 * 240>, 2> displays{};
#if MULTI_TEST_MODE
    std::atomic<bool> update_display = false;
    std::atomic<int> display_in_progress_num = 0;
    size_t current_display_index = 0;
    inline void update_current_display(){
        current_display_index = display_in_progress_num.fetch_xor(1, std::memory_order_acq_rel);
        update_display.store(true, std::memory_order_release);
    }
    inline auto& get_active_display(){
        return displays[display_in_progress_num.load(std::memory_order_acquire)];
    }
#else
    size_t current_display_index = 0;
    inline void update_current_display(){
        current_display_index ^= 1;
    }
    inline auto& get_active_display(){
#if DEBUG_MODE && !MULTI_TEST_MODE
        return displays[current_display_index];
#else
        return displays[current_display_index ^ 1];
#endif
    }
#endif

    size_t current_x = 0;
    size_t current_y = 0;

    inline void update_internal_display(Color pixel){
        displays[current_display_index][current_x++] = pixel;
#if DEBUG_MODE
        ++current_line_x;
#endif
    }

    static constexpr inline std::array<Color, 64> palette{
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

    void update_base_nt(Bus& bus);

    unsigned char tile_byte_from_addr(Bus& bus, unsigned short addr);

    //    0HNNNN NNNNPyyy
    unsigned short to_pattern_addr(unsigned char y, unsigned char tile_n, bool plane, bool h);
};
#endif //NES_PPU_HPP

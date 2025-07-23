#ifndef GAMEBOY_PPU_H
#define GAMEBOY_PPU_H
#include "RAM.h"
#include <raylib.h>
#include <iostream>
#include "Constants.h"
#include <queue>

bool operator==(Color lhs, Color rhs);

void print_segment(unsigned short address, auto n, RAM& ram) {
    std::cout << "\n=============================\n";
    for (int i = 0; i < n; ++i){
        std::cout << std::hex << address + i <<  ":" << (unsigned)ram.mem[address + i] << '\n';
    }
    std::cout << "\n=============================\n";
    std::cout << std::dec;
}

void print_bytes(unsigned short address, auto n, RAM& ram) {
    std::cout << "\n=============================\n";
    for (int i = 0; i < n; ++i){
        std::cout << std::hex << "0x" << (unsigned)ram.mem[address + i] << ',';
    }
    std::cout << "\n=============================\n";
    std::cout << std::dec;
}

struct Sprite {
    unsigned char y_pos{};
    unsigned char x_pos{};
    unsigned char index_tile{};
    unsigned char attrib{};
};

struct Pixel {
    bool is_sprite;
    Color color{};      // The color number (IGNORING the palette, this value is the color value from the tile data)
    unsigned char attrib{};     // Palette: OBP0 / OBP1 for DMG, a value between 0 and 7 on CGB
    // Sprite Priority: Only relevant for Sprites on CGB
    bool bg_priority{}; // Only relevant for Sprites, keeps the value of Bit 7 (OBJ-to-BG Priority) of the Attributes byte of the sprite

    static Color get_pixel_color(unsigned char id);
    static int get_color_id(Color c);
    static std::queue<Pixel> tile_row_to_pixel_arr(unsigned short row, bool, unsigned char, unsigned char);
    static std::queue<Pixel> tile_row_to_pixel_arr(unsigned short row, bool, unsigned char, unsigned char current_pixel, const Sprite&);
};

template <typename Fetcher, typename ... Args>
struct BaseFetcher {
    enum class FetcherState {
        GetTile, Low, High, ToPixels, Push
    } current_state = FetcherState::GetTile;

    unsigned short tile_address{};
    unsigned char fetcher_current_tile_x{};
    unsigned char low {};
    unsigned char high {};

    unsigned char current_pixel_x = 0;
    bool is_sprite = false;
    int pixel_offset = 0;
    unsigned char pallete = 0;

    std::queue<Pixel> pixels{};
    std::queue<Pixel> fifo{};
    unsigned char line_in_tile{};
    bool is_window = false;
    unsigned char scx_offset{};

    // todo: add to static class like ram ranges
    auto get_window_tile_map_area(const RAM& ram) {
        if (((ram.read(constants::LCDC_ADDRESS) >> 6) & 1))
            return 0x9C00;
        return 0x9800;
    }

    auto get_bg_tile_map_area(const RAM& ram) {
        if (((ram.read(constants::LCDC_ADDRESS) >> 3) & 1))
            return 0x9C00;
        return 0x9800;
    }

    bool window_visible(const RAM& ram) {
        return (((ram.read(constants::LCDC_ADDRESS)) >> 5) & 1);
    }

    void reset() {
        fifo = {};
        pixels = {};
        current_state = FetcherState::GetTile;
        fetcher_current_tile_x = 0;
        tile_address = 0;
//        scx_offset = 0;
    }

    void step(RAM& ram, Args&&... args) {
        switch(current_state) {
            case FetcherState::GetTile:  {tile_address = static_cast<Fetcher*>(this)->update_tile_base(ram, std::forward<Args>(args)...); break;}
            case FetcherState::Low:      {low  = ram.read(tile_address + (line_in_tile * 2));      break;}
            case FetcherState::High:     {high = ram.read(tile_address + (line_in_tile * 2) + 1);  break;}
            case FetcherState::ToPixels: {
                pixels = Pixel::tile_row_to_pixel_arr((high << 8 | low), is_sprite, pallete, args...);
//                SCX at a Sub-Tile-Layer
                if ((!is_window && !is_sprite) && fetcher_current_tile_x == 0) {
                    for (int i = 0; i < scx_offset; ++i){
                        pixels.pop();
                    }
                }
                break;}
            case FetcherState::Push:     {
                if (fifo.size() < 8 && !pixels.empty()) {
                    fifo.push(pixels.front());
                    pixels.pop();
                }
                break;}
        }
    }

    void fetch(RAM& ram, Args&&... args) {
        switch(current_state) {
            case FetcherState::GetTile:  {step(ram, std::forward<Args>(args)...); current_state = FetcherState::Low;      break;}
            case FetcherState::Low:      {step(ram, std::forward<Args>(args)...); current_state = FetcherState::High;     break;}
            case FetcherState::High:     {step(ram, std::forward<Args>(args)...); current_state = FetcherState::ToPixels; break;}
            case FetcherState::ToPixels: {step(ram, std::forward<Args>(args)...); current_state = FetcherState::Push;     break;}
            case FetcherState::Push:     {step(ram, std::forward<Args>(args)...); if (pixels.empty()){
                fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 20;
                current_state = FetcherState::GetTile;
            } break;}
        }
    }
};

struct PixelFetcher: BaseFetcher<PixelFetcher, unsigned char> {
    unsigned char current_window_line{};
    unsigned short update_tile_base(auto& ram, unsigned char current_pixel_x) {
        auto wy = ram.read(constants::WY_ADDRESS);
        auto wx = ram.read(constants::WX_ADDRESS);
        auto scy = ram.read(constants::SCY_ADDRESS);
        auto ly  = ram.read(constants::LY_ADDRESS);
        auto scx = ram.read(constants::SCX_ADDRESS);
        scx_offset = ram.read(constants::SCX_ADDRESS) % 8;
        unsigned short tile_base = ((ram.read(constants::LCDC_ADDRESS) >> 4) & 1) ? 0x8000 : 0x8800; // TODO: check

        bool inside_window_y = (ly >= wy);
        bool inside_window_x = (wx >= 7 && current_pixel_x >= wx - 7);

        unsigned short tilemap_base{};
        unsigned char  tilemap_x{};
        unsigned char  tilemap_y{};

        if (window_visible(ram) && inside_window_x && inside_window_y) {
            tilemap_base = get_window_tile_map_area(ram);
            current_window_line = ly - wy; // local window line
            tilemap_x = (current_pixel_x - (wx - 7)) / 8; // todo: check
            tilemap_y = current_window_line/8;
            line_in_tile = current_window_line % 8;
            is_window = true;
        } else {
            tilemap_base = get_bg_tile_map_area(ram);                  // (SCX / 8) + fetcher’s X coordinate) & $1F
            tilemap_x = (fetcher_current_tile_x + (scx / 8)) & 0x1f;   // Because of this formula, fetcherX can be between 0 and 31 - todo: update fetcher X
            tilemap_y = ((ly + scy) & 0xff) / 8;                       // Because of this formula, fetcherY can be between 0 and 255
            line_in_tile = (ly + scy) % 8;
            is_window = false;
        }
        unsigned short tilemap_address = (tilemap_x + tilemap_y * 32) + tilemap_base;
        unsigned char tile_index = ram.read(tilemap_address);

        unsigned short res{};
        if (tile_base == 0x8000)
            res = (tile_base + (tile_index * 16));
        else
            res = (tile_base + ((char)tile_index * 16));
        return res;
    }
};

struct SpriteFetcher: BaseFetcher<SpriteFetcher, unsigned char, const Sprite&> {
    unsigned short update_tile_base(auto& ram, unsigned char cpx, const Sprite& sprite) {
        is_sprite = true;
        pallete = sprite.attrib;
        line_in_tile = ram.read(constants::LY_ADDRESS) - (sprite.y_pos - 16);
        return (sprite.index_tile * 16) + constants::VRAM_BEGIN;
    }
};

class PPU {
public:
    enum Mode {
        HBlank =  0b00,    // H-Blank
        VBlank =  0b01,    // V-Blank
        OAM =     0b10,    // OAM Search
        Drawing = 0b11     // Pixel Transfer
    };
//public:
    Mode current_mode = OAM;
    Color buffer[144][160]{};

    bool lcd_enable = true;
    bool window_visible = false;
    bool obj_enable = false; // sprites
    bool bg_enable = false;
    size_t window_line_counter = 0;
    bool oam_scan_finished = false;

    // todo: optimize
    std::vector<Sprite> sprite_buffer;

    // coordinates
    size_t current_pixel_x = 0;
    size_t current_window_line = 0;
    unsigned char current_line = {};
    int current_t_cycle_count = {};
    int current_v_blank_t_cycle_count = {};
    bool longest_line = false;

    PixelFetcher pixel_fetcher;
    SpriteFetcher sprite_fetcher;

    void set_ppu_reg(unsigned short address, unsigned char value, RAM& ram);
    void set_ppu_mode(RAM& ram);

    /* TODO: set 2 bit of STAT_ADDRESS to LY == LYC */
    void update_ly_lyc_compare_flag(RAM& ram);

    void update_lcd(const RAM& ram);
    void update_window(const RAM& ram);
    void update_obj(const RAM& ram);
    void update_bg(const RAM& ram);
    void update_regs(const RAM& ram);

    static Color get_pallete(const RAM& ram, unsigned short index);

    unsigned char get_obj_size(const RAM& ram);

    unsigned short get_window_tile_map_area(const RAM& ram);
    unsigned short get_bg_tile_map_area(const RAM& ram);
    unsigned short get_tile_data_area(const RAM& ram);

    void oam_scan(RAM& ram);
    Pixel pixel_mixing(RAM& ram);
    std::optional<Sprite> sprite_on_x(RAM& ram);
    void draw(RAM& ram);
public:
    auto  read_buffer() const -> const Color(&)[144][160];
    void handle_t_cycle(int t_cycle, RAM& ram);
    Mode get_current_mode() const;
    void print_buffer() const;
};

#endif //GAMEBOY_PPU_H

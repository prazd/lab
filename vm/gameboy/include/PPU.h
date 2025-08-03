#ifndef GAMEBOY_PPU_H
#define GAMEBOY_PPU_H
#include "RAM.h"
#include <raylib.h>
#include <iostream>
#include "Constants.h"
#include <queue>

constexpr size_t NEXT_T_CYCLE_VAL = 2;

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
    auto operator<=>(const Sprite&) const = default;
};

struct Pixel {
    bool is_sprite;
    Color color{};      // The color number (IGNORING the palette, this value is the color value from the tile data)
    unsigned char attrib{};     // Palette: OBP0 / OBP1 for DMG, a value between 0 and 7 on CGB
    // Sprite Priority: Only relevant for Sprites on CGB
    bool bg_priority{}; // Only relevant for Sprites, keeps the value of Bit 7 (OBJ-to-BG Priority) of the Attributes byte of the sprite

    static Color get_pixel_color(unsigned char id);
    static int get_color_id(Color c);
    static std::deque<Pixel> tile_row_to_pixel_arr(unsigned short row, bool, unsigned char, unsigned char);
    static std::deque<Pixel> tile_row_to_pixel_arr(unsigned short row, bool, unsigned char, unsigned char current_pixel, const Sprite&);
};

template <typename Fetcher, typename ... Args>
struct BaseFetcher {
    enum class FetcherState {
        GetTile, Low, High, Push
    } current_state = FetcherState::GetTile;

    unsigned short tile_address{};
    unsigned char fetcher_current_tile_x{};
    unsigned char low {};
    unsigned char high {};

    bool is_sprite = false;
    unsigned char pallete = 0;
    bool paused = false;
    bool finished = false;

    size_t next_t_cycle = NEXT_T_CYCLE_VAL; // Start GetTile 2 T-cycle

    std::deque<Pixel> pixels{};
    std::deque<Pixel> fifo{};
    unsigned char line_in_tile{};
    bool is_window = false;

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

    void reset_state() {
        current_state = FetcherState::GetTile;
    }
    void reset_pixels() {
        pixels = {};
    }

    void reset() {
        fifo = {};
        pixels = {};
        current_state = FetcherState::GetTile;
        tile_address = 0;
        next_t_cycle = NEXT_T_CYCLE_VAL;
    }

    void step(RAM& ram, Args&&... args) {
        switch(current_state) {
            case FetcherState::GetTile:  {tile_address = static_cast<Fetcher*>(this)->update_tile_base(ram, std::forward<Args>(args)...); next_t_cycle = NEXT_T_CYCLE_VAL; break;} // Low
            case FetcherState::Low:      {low = ram.read(tile_address + (line_in_tile * 2)); next_t_cycle = NEXT_T_CYCLE_VAL; break;}
            case FetcherState::High:     {
                high = ram.read(tile_address + (line_in_tile * 2) + 1);
                pixels = Pixel::tile_row_to_pixel_arr((high << 8 | low), is_sprite, pallete, args...);
                next_t_cycle = NEXT_T_CYCLE_VAL;
                if ((!is_window && !is_sprite) && fetcher_current_tile_x == 0) {
                    auto scx_offset= (ram.read(constants::SCX_ADDRESS) % 8);
                    for (int i = 0; i < scx_offset; ++i){
                        pixels.pop_front();
                    }
                    next_t_cycle += scx_offset;
                }
                if (fifo.size() < 8 && !pixels.empty()) {
                    fifo.push_back(pixels.front());
                    pixels.pop_front();
                }
                break;
            }
            case FetcherState::Push: {
                if (fifo.size() < 8 && !pixels.empty()) {
                    fifo.push_back(pixels.front());
                    pixels.pop_front();
                }
                next_t_cycle = 1;
                break;}
        }
    }

    void fetch(RAM& ram, Args&&... args) {
        switch(current_state) {
            case FetcherState::GetTile:  {step(ram, std::forward<Args>(args)...); current_state = FetcherState::Low;  break;}
            case FetcherState::Low:      {step(ram, std::forward<Args>(args)...); current_state = FetcherState::High; break;}
            case FetcherState::High:     {step(ram, std::forward<Args>(args)...); current_state = FetcherState::Push; if (pixels.empty()){
                    fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 20;
                    current_state = FetcherState::GetTile;
                    next_t_cycle = NEXT_T_CYCLE_VAL;
                } break;}
            case FetcherState::Push:     {step(ram, std::forward<Args>(args)...); if (pixels.empty()){
                fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 20;
                current_state = FetcherState::GetTile;
            } next_t_cycle = NEXT_T_CYCLE_VAL; break;}
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

    void fetch(RAM& ram, unsigned char cpx, const Sprite& sprite) {
        switch(current_state) {
            case FetcherState::GetTile:  {step(ram, cpx, sprite); next_t_cycle = NEXT_T_CYCLE_VAL; current_state = FetcherState::High; break;}
            case FetcherState::High:     {step(ram, cpx, sprite); next_t_cycle = NEXT_T_CYCLE_VAL;
                    pixels = {};
                    fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 20;
                    current_state = FetcherState::GetTile;
                    finished = true;
                    next_t_cycle = NEXT_T_CYCLE_VAL;
                 break;}
        }
    }

    void step(RAM& ram, unsigned char cpx, const Sprite& sprite) {
        switch(current_state) {
            case FetcherState::GetTile:  {tile_address = update_tile_base(ram, cpx, sprite); low  = ram.read(tile_address + (line_in_tile * 2)); break;}
            case FetcherState::High:     {
                high = ram.read(tile_address + (line_in_tile * 2) + 1);
                pixels = Pixel::tile_row_to_pixel_arr((high << 8 | low), is_sprite, pallete,cpx, sprite);
                if (fifo.empty()) {
                    while (fifo.size() < 8 && !pixels.empty()) {
                        fifo.push_back(pixels.front());
                        pixels.pop_front();
                    }
                } else {
                    for (int i = 0; i < fifo.size(); ++i) {
                        if (!pixels.empty()) {
                            if (Pixel::get_color_id(fifo[i].color) == 0 &&
                                Pixel::get_color_id(pixels.front().color) != 0) {
                                fifo[i] = pixels.front();
                            }
                            pixels.pop_front();
                        }
                    }
                    while (fifo.size() < 8 && !pixels.empty()) {
                        fifo.push_back(pixels.front());
                        pixels.pop_front();
                    }
                }
                break;
            }
        }
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
    bool pixel_fetcher_paused = false;
    size_t oam_next_t_cycle = 2;
    unsigned short current_oam_obj = 0;

    std::priority_queue<std::pair<Sprite, int>, std::vector<std::pair<Sprite, int>>, decltype([](auto& a, auto& b){
        return !(a.first.x_pos < b.first.x_pos || (a.first.x_pos == b.first.x_pos && a.second < b.second));
    })> sprite_buffer;
    size_t current_sprite_index = 0;

    // coordinates
    size_t current_pixel_x = 0;
    size_t current_window_line = 0;
    unsigned char current_line = {};
    int current_t_cycle_count = {};
    int current_v_blank_t_cycle_count = {};
    bool longest_line = false;
    size_t remaining_pixel_count = 0;
    bool sprite_render = false;

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
    void tick(RAM& ram);
    Mode get_current_mode() const;
    void print_buffer() const;
};

#endif //GAMEBOY_PPU_H

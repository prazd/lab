#ifndef GAMEBOY_PPU_H
#define GAMEBOY_PPU_H
#include "RAM.h"
#include <raylib.h>
#include <iostream>
#include "Constants.h"
#include <queue>
#include <optional>
#include "Resumable.h"

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

void print_external_ram(unsigned short address, auto n, RAM& ram) {
    std::cout << "\n=============================\n";
    for (int i = 0; i < n; ++i){
        std::cout << std::hex << address + i <<  ":" << (unsigned)ram.cart_ram[address + i] << '\n';
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
    static Color get_pixel_color(unsigned char id);
    static int get_color_id(Color c);
    static std::deque<Pixel> tile_row_to_pixel_arr(unsigned short row, bool, unsigned char);
};

template <typename Fetcher, typename ... Args>
struct BaseFetcher {
    enum class FetcherState {
        GetTile, Low, High, Sleep, Push
    } current_state = FetcherState::GetTile;

    unsigned short tile_address{};
    unsigned char fetcher_current_tile_x{};
    unsigned char windows_current_tile_x{};
    unsigned char low {};
    unsigned char high {};

    bool is_sprite = false;
    unsigned char pallete = 0;
    bool paused = false;
    bool finished = false;
    bool is_windows_drawing = false;
    size_t next_t_cycle = NEXT_T_CYCLE_VAL; // Start GetTile 2 T-cycle
    std::deque<Pixel> pixels{};
    std::deque<Pixel> fifo{};
    unsigned char line_in_tile{};
    bool is_window = false;

    auto get_window_tile_map_area(const RAM& ram) {
        if (((ram.read_unsafe(constants::LCDC_ADDRESS) >> 6) & 1))
            return 0x9C00;
        return 0x9800;
    }

    auto get_bg_tile_map_area(const RAM& ram) {
        if (((ram.read_unsafe(constants::LCDC_ADDRESS) >> 3) & 1))
            return 0x9C00;
        return 0x9800;
    }

    void try_to_fill_fifo() {
        if (fifo.empty()) {
          while (!pixels.empty()) {
            fifo.push_back(pixels.front());
            pixels.pop_front();
          }
        }
    }

    void reset() {
        is_window = false;
        fifo = {};
        pixels = {};
        current_state = FetcherState::GetTile;
        tile_address = 0;
        next_t_cycle = NEXT_T_CYCLE_VAL;
    }

    void step(RAM& ram, Args&&... args) {
        switch(current_state) {
            case FetcherState::GetTile:  {tile_address = static_cast<Fetcher*>(this)->update_tile_base(ram, std::forward<Args>(args)...); break;} // Low
            case FetcherState::Low:      {low = ram.read_unsafe(tile_address + (line_in_tile * 2)); break;}
            case FetcherState::High:     {
                high = ram.read_unsafe(tile_address + (line_in_tile * 2) + 1);
                pixels = Pixel::tile_row_to_pixel_arr((high << 8 | low), is_sprite, pallete);
                next_t_cycle = 1;
                auto scx_offset= (ram.read_unsafe(constants::SCX_ADDRESS) % 8);
                if (fetcher_current_tile_x == 0 || fetcher_current_tile_x == 1) {
                   next_t_cycle += 6;
                }
                if ((!is_window && !is_sprite) && fetcher_current_tile_x == 0 && scx_offset > 0) {
                    for (int i = 0; i < scx_offset; ++i){
                        pixels.pop_front();
                    }
                    next_t_cycle += scx_offset; // todo: check
                    break;
                }
                try_to_fill_fifo();
                break;}
            case FetcherState::Sleep:    {
                try_to_fill_fifo();
                next_t_cycle = 1;
                break;}
            case FetcherState::Push: {
                try_to_fill_fifo();
                next_t_cycle = 1;
                break;
            }
        }
    }

    void fetch(RAM& ram, Args&&... args) {
        switch(current_state) {
            case FetcherState::GetTile:  {step(ram, std::forward<Args>(args)...); next_t_cycle = NEXT_T_CYCLE_VAL; current_state = FetcherState::Low;  break;}
            case FetcherState::Low:      {step(ram, std::forward<Args>(args)...); next_t_cycle = NEXT_T_CYCLE_VAL; current_state = FetcherState::High; break;}
            case FetcherState::High:     {step(ram, std::forward<Args>(args)...);
                if (fifo.size() == 8 && pixels.empty()) {
                    if (is_window) {
                        windows_current_tile_x = (windows_current_tile_x + 1) % 21;
                    } else {
                        fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 21;
                    }
                    current_state = FetcherState::GetTile;
                } else {
                    next_t_cycle = 1;
                    current_state = FetcherState::Sleep;
                }
                break;}
            case FetcherState::Sleep:    {
                step(ram, std::forward<Args>(args)...);
                if (fifo.size() == 8 && pixels.empty()) {
                    if (is_window) {
                        windows_current_tile_x = (windows_current_tile_x + 1) % 21;
                    } else {
                        fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 21;
                    }
                    next_t_cycle = NEXT_T_CYCLE_VAL;
                    current_state = FetcherState::GetTile;
                } else {
                    next_t_cycle = 1;
                    current_state = FetcherState::Push;
                }
                break;}
            case FetcherState::Push:     {
                step(ram, std::forward<Args>(args)...);
                next_t_cycle = 1;
                if (pixels.empty()){
                    if (is_window) {
                        windows_current_tile_x = (windows_current_tile_x + 1) % 21;
                    } else {
                        fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 21;
                    }
                    current_state = FetcherState::GetTile;
                    next_t_cycle = NEXT_T_CYCLE_VAL;
                } break;}
        }
    }
};

struct PixelFetcher: BaseFetcher<PixelFetcher, unsigned char> {
    unsigned char current_window_line{};
    unsigned short update_tile_base(auto& ram, unsigned char current_pixel_x) {
        auto wy = ram.read_unsafe(constants::WY_ADDRESS);
        auto wx = ram.read_unsafe(constants::WX_ADDRESS);
        auto scy = ram.read_unsafe(constants::SCY_ADDRESS);
        auto ly  = ram.read_unsafe(constants::LY_ADDRESS);
        auto scx = ram.read_unsafe(constants::SCX_ADDRESS);
        unsigned short tile_base = ((ram.read_unsafe(constants::LCDC_ADDRESS) >> 4) & 1) ? 0x8000 : 0x9000;

        unsigned short tilemap_base{};
        unsigned char  tilemap_x{};
        unsigned char  tilemap_y{};

        if (is_windows_drawing) {
            tilemap_base = get_window_tile_map_area(ram);
            tilemap_x = windows_current_tile_x;
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
        unsigned char tile_index = ram.read_unsafe(tilemap_address);

        unsigned short res{};
        if (tile_base == 0x8000)
            res = (tile_base + (tile_index * 16));
        else
            res = (tile_base + (((char)tile_index) * 16));
        return res;
    }
};

struct SpriteFetcher: BaseFetcher<SpriteFetcher, unsigned char, const Sprite&> {
    unsigned short update_tile_base(auto& ram, unsigned char cpx, const Sprite& sprite) {
        is_sprite = true;
        pallete = sprite.attrib;
        auto lcdc_mode = (ram.read_unsafe(constants::LCDC_ADDRESS) >> 2) & 1;
        line_in_tile = ram.read_unsafe(constants::LY_ADDRESS) - (sprite.y_pos - 16);
        if ((sprite.attrib >> 6) & 1)
            line_in_tile = ((lcdc_mode ? 16 : 8)) - line_in_tile - 1;
//        In this mode, this byte specifies the index of the first (top) tile of the object.
//        This is enforced by the hardware: the least significant bit of the tile index is ignored; that is, the top 8×8 tile is “NN & $FE”,
//        and the bottom 8×8 tile is “NN | $01”.
// todo: check
        return ((lcdc_mode ? sprite.index_tile & 0xfe : sprite.index_tile) * 16) + constants::VRAM_BEGIN;
    }

    void fetch(RAM& ram, unsigned char cpx, const Sprite& sprite) {
        switch(current_state) {
            case FetcherState::GetTile:  {step(ram, cpx, sprite); next_t_cycle = NEXT_T_CYCLE_VAL; current_state = FetcherState::High; break;}
            case FetcherState::High:     {step(ram, cpx, sprite);
                    pixels = {};
                    fetcher_current_tile_x = (fetcher_current_tile_x + 1) % 21;
                    current_state = FetcherState::GetTile;
                    finished = true;
                    next_t_cycle = NEXT_T_CYCLE_VAL;
                 break;}
        }
    }

    void step(RAM& ram, unsigned char cpx, const Sprite& sprite) {
        switch(current_state) {
            case FetcherState::GetTile:  {tile_address = update_tile_base(ram, cpx, sprite); low  = ram.read_unsafe(
                        tile_address + (line_in_tile * 2)); break;}
            case FetcherState::High:     {
                high = ram.read_unsafe(tile_address + (line_in_tile * 2) + 1);

                pixels = Pixel::tile_row_to_pixel_arr((high << 8 | low), is_sprite, pallete);

                // X-flip
                if (sprite.attrib >> 5 & 1) std::reverse(pixels.begin(), pixels.end());

                int x_in_tile = cpx - (sprite.x_pos - 8);
                for (int i = 0; i < x_in_tile; ++i) pixels.pop_front();

                while(pixels.size() < 8) pixels.push_back(Pixel{.color = WHITE});

                if (fifo.empty()) {
                    while (fifo.size() < 8 && !pixels.empty()) {
                        fifo.push_back(pixels.front());
                        pixels.pop_front();
                    }
                } else {
                    for (auto &el: fifo) {
                        if (!pixels.empty()) {
                            if (Pixel::get_color_id(el.color) == 0 &&
                                Pixel::get_color_id(pixels.front().color) != 0) {
                                el = pixels.front();
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
    enum Mode {
        HBlank =  0b00,    // H-Blank
        VBlank =  0b01,    // V-Blank
        OAM =     0b10,    // OAM Search
        Drawing = 0b11     // Pixel Transfer
    };
    Mode current_mode = OAM;

    Color buffer[144][160]{};

    size_t last_t_cycles = 0;
    size_t prev_t_cycles = 0;
    unsigned char sprite_x = 0;
    Sprite current_sprite{};
    size_t internal_t_cycles = 0;
    size_t ly_increment_delay = 0;
    bool ly_increment_done = false;
    bool start_oam = false;
    bool oam_start = true;

    enum OamMode {
        ReadSprite,
        Gap
    } current_oam_mode = ReadSprite;
    int read_sprite_count = 2;
    int oam_gap_count = 2;

    bool lcd_enable = true;
    bool window_visible = false;
    bool obj_enable = false; // sprites
    bool bg_enable = false;
    bool pixel_fetcher_paused = false;
    int oam_next_t_cycle = 2;
    int oam_sprite_get = 2;
    size_t oam_next_t_sprite_t_cycle = 2;
    unsigned short current_oam_obj = 0;
    bool win_disabled = false;

    std::priority_queue<std::pair<Sprite, int>, std::vector<std::pair<Sprite, int>>, decltype([](auto& a, auto& b){
        return !(a.first.x_pos < b.first.x_pos || (a.first.x_pos == b.first.x_pos && a.second < b.second));
    })> sprite_buffer;

    // coordinates
    int current_pixel_x = 0;
    unsigned char current_line = {};
    int current_t_cycle_count = {};
    int current_v_blank_t_cycle_count = {};
    bool sprite_render = false;

    PixelFetcher pixel_fetcher;
    SpriteFetcher sprite_fetcher;

    void set_ppu_reg(unsigned short address, unsigned char value, RAM& ram);
    void set_ppu_mode(RAM& ram);
    void update_lcd(const RAM& ram);
    void update_window(const RAM& ram);
    void update_obj(const RAM& ram);
    void update_bg(const RAM& ram);
    void update_regs(const RAM& ram);

    bool is_win_init = false;
    bool use_saved_window_ly = false;
    unsigned char save_window_ly{};
    bool is_windows_fetching_ready(RAM& ram);

    unsigned char get_obj_size(const RAM& ram);
    Pixel pixel_mixing(RAM& ram);
    std::optional<Sprite> sprite_on_x(RAM& ram);
    void draw(RAM& ram);
    void set_bg_palette(RAM& ram, Pixel&);
    void set_sprite_palette(RAM& ram, Pixel&);
public:
    void tick(RAM& ram);
    void set_start_mode();
    void lyc_ly_compare(RAM& ram);
    void reset(RAM&);
    unsigned short get_bg_fetcher_tile_address() const;
    unsigned short get_sprite_fetcher_tile_address() const;
    auto  read_buffer() const -> const Color(&)[144][160];
    Mode get_current_mode() const;
    void print_buffer() const;
};
#endif //GAMEBOY_PPU_H

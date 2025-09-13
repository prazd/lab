#include "PPU.h"
#include <cassert>

Color Pixel::get_pixel_color(unsigned char id)  {
    switch(id){
        case 0: return WHITE;
        case 1: return LIGHTGRAY;
        case 2: return DARKGRAY;
        default:
            return BLACK;
    };
}

int Pixel::get_color_id(Color c)  {
    if (c == WHITE)
        return 0;
    if (c == LIGHTGRAY)
        return 1;
    if (c == DARKGRAY)
        return 2;
    return 3;
}

std::deque<Pixel> Pixel::tile_row_to_pixel_arr(unsigned short row, bool is_sprite, unsigned char pallete, unsigned char pcx, const Sprite& s)  {
    unsigned char l = row & 0xff;
    unsigned char h = (row >> 8) & 0xff;
    std::deque<Pixel> result{};
    unsigned char lc{};
    unsigned char hc{};
    Color color;

    int x_in_tile = pcx - (s.x_pos - 8);
    for (int i = x_in_tile; i < 8; ++i) {
        lc = (l >> (7 - i)) & 1;
        hc = (h >> (7 - i)) & 1;
        color = Pixel::get_pixel_color((hc << 1 | lc));
        result.push_back(Pixel{.color = color, .is_sprite = is_sprite, .attrib = pallete});
    }
    return result;
}

std::deque<Pixel> Pixel::tile_row_to_pixel_arr(unsigned short row, bool is_sprite, unsigned char pallete, unsigned char)  { // todo: maybe refactoring
    unsigned char l = row & 0xff;
    unsigned char h = (row >> 8) & 0xff;
    std::deque<Pixel> result{};
    unsigned char lc{};
    unsigned char hc{};
    Color color;
    for (int i = 0; i < 8; ++i) {
        lc = (l >> (7 - i)) & 1;
        hc = (h >> (7 - i)) & 1;
        color = Pixel::get_pixel_color((hc << 1 | lc));
        result.push_back(Pixel{.color = color, .is_sprite = is_sprite, .attrib = pallete});
    }
    return result;
}

bool operator==(Color lhs, Color rhs) {
    return lhs.r == rhs.r &&
           lhs.g == rhs.g &&
           lhs.b == rhs.b &&
           lhs.a == rhs.a;
}

PPU::Mode PPU::get_current_mode() const {
    return current_mode;
}

void PPU::set_ppu_reg(unsigned short address, unsigned char value, RAM& ram) {
    ram.write_unsafe(address, value);
}

void PPU::set_ppu_mode(RAM &ram) {
    ram.write_unsafe(constants::STAT_ADDRESS, (ram.read_unsafe(constants::STAT_ADDRESS) & 0xFC) | current_mode);
}
void PPU::update_lcd(const RAM &ram) {
    lcd_enable = (((ram.read_unsafe(constants::LCDC_ADDRESS)) >> 7) & 1);
}
void PPU::update_window(const RAM &ram) {
    window_visible = (((ram.read_unsafe(constants::LCDC_ADDRESS)) >> 5) & 1);
}
void PPU::update_obj(const RAM &ram) {
    obj_enable = (((ram.read_unsafe(constants::LCDC_ADDRESS)) >> 1) & 1);
}
void PPU::update_bg(const RAM &ram)  {
    bg_enable = ((ram.read_unsafe(constants::LCDC_ADDRESS)) & 1);
}
void PPU::update_regs(const RAM &ram) {
//    update_lcd(ram);
    update_window(ram);
    update_obj(ram);
    update_bg(ram);
}

unsigned char PPU::get_obj_size(const RAM &ram)  {
    return (((ram.read_unsafe(constants::LCDC_ADDRESS)) >> 2) & 1) ? 16 : 8;
}

void PPU::oam_scan(RAM &ram)  {
    if (sprite_buffer.size() == 10) {
        current_oam_obj += 4;
        return;
    }
    auto obj_sz = get_obj_size(ram);
    auto val= ram.read_unsafe(constants::OAM_BEGIN + current_oam_obj);
    int top = val - 16;
    if (current_line >= top && current_line < top + obj_sz){
        sprite_buffer.emplace(
            Sprite{
                .y_pos = val,
                .x_pos = static_cast<unsigned char>(ram.read_unsafe(constants::OAM_BEGIN + current_oam_obj + 1)),
                .index_tile = ram.read_unsafe(constants::OAM_BEGIN + current_oam_obj + 2),
                .attrib = ram.read_unsafe(constants::OAM_BEGIN + current_oam_obj + 3),
        }, current_oam_obj);
    }
    current_oam_obj += 4;
}


Pixel PPU::pixel_mixing(RAM &ram) {
    auto bg_pixel = Pixel{.color = WHITE}; // Default
    if (!pixel_fetcher.fifo.empty()){
        bg_pixel = pixel_fetcher.fifo.front(); pixel_fetcher.fifo.pop_front();
    }
    auto bgp = ram.read_unsafe(constants::BGP);
    switch (Pixel::get_color_id(bg_pixel.color)) {
        case 1:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 2) & 3);  break;}
        case 2:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 4) & 3);  break;}
        case 3:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 6) & 3);  break;}
        default:
            {bg_pixel.color = Pixel::get_pixel_color((bgp & 3));  break;}
    }
    if (!(ram.read_unsafe(constants::LCDC_ADDRESS) & 0x2)) {
        return bg_pixel;
    }
    if (sprite_fetcher.fifo.empty()) {
        return bg_pixel;
    }
    auto sprite_pixel = sprite_fetcher.fifo.front();  sprite_fetcher.fifo.pop_front();
    //  If the color number of the Sprite Pixel is 0, the Background Pixel is pushed to the buffer.
    if (Pixel::get_color_id(sprite_pixel.color) == 0){
        return bg_pixel;
    }

    //  If the BG-to-OBJ-Priority bit is 1 and the color number of the Background Pixel is anything other than 0,
    //  the Background Pixel is pushed to the buffer
    if ((sprite_pixel.attrib >> 7 & 1) && Pixel::get_color_id(bg_pixel.color) != 0){
        return bg_pixel;
    }

    unsigned char obp{};
    if ((sprite_pixel.attrib >> 4 & 1))
        obp = ram.read_unsafe(constants::OBP1);
    else
        obp = ram.read_unsafe(constants::OBP0);

    switch (Pixel::get_color_id(sprite_pixel.color)) {
        case 1:  {sprite_pixel.color = Pixel::get_pixel_color((obp >> 2) & 3);  break;}
        case 2:  {sprite_pixel.color = Pixel::get_pixel_color((obp >> 4) & 3);  break;}
        case 3:  {sprite_pixel.color = Pixel::get_pixel_color((obp >> 6) & 3);  break;}
    }
    return sprite_pixel;
}

std::optional<Sprite> PPU::sprite_on_x(RAM &ram)  {
   if (!sprite_buffer.empty()) {
       auto current = sprite_buffer.top();
       if (current.first.x_pos - 8 <= current_pixel_x && current_pixel_x < current.first.x_pos) {
           sprite_buffer.pop();
           return current.first;
       }
   }
    return std::nullopt;
}

void PPU::draw(RAM &ram)  {
    buffer[current_line][current_pixel_x++] = pixel_mixing(ram).color;
    if (!is_windows_drawing && is_windows_fetching_ready(ram)) {
        pixel_fetcher.reset();
        is_windows_drawing = true;
        pixel_fetcher.windows_current_tile_x = 0;
        pixel_fetcher.is_windows_drawing = true;
    }
}

void PPU::tick(RAM &ram)  {
#define DEBUG_T_CYCLE 0
#define PRINT_T_CYCLE(x, t_cycles) std::cout << x << ": " << t_cycles << '\n'; t_cycles = 0;
#define PRINT_DOTS(n) for (int i = 0; i < n; ++i) std::cout << '-'; std::cout << '\n';
#define PRINT_LINE(line) std::cout << "Line: " << (unsigned)line << '\n';

    static size_t debug_t_cycles = 0;
#if 0
    ram.write_unchecked(constants::LY_ADDRESS, 0x90);
    return;
#endif
    ++internal_t_cycles;
    ++current_t_cycle_count;

    ram.ppu_internal_t_cycle = current_t_cycle_count;

    ++last_t_cycles;
    update_regs(ram);
    set_ppu_mode(ram);
    lyc_ly_compare(ram);

//    std::cout <<  "Stat:" << std::bitset<8>(ram.read_unsafe(constants::STAT_ADDRESS)).to_string() << '\n';
    if (current_mode == OAM){
#if DEBUG_T_CYCLE
        ++debug_t_cycles;
#endif
        if (current_t_cycle_count != 80) {
            // 1Mcycle (4t-cycle) -> read 1 row (8byte ==> 2 sprite)
            // 2t-cycle read row + 2t-cycle gap
            if (oam_start) {
                ram.ppu_oam_usage = true;
                oam_start = false;
            }
//            std::cout << current_t_cycle_count << " CurrentMode:" << ram.ppu_oam_usage << '\n';
            switch (current_oam_mode) {
                case ReadSprite: {
                        --read_sprite_count;
                        if (sprite_buffer.size() != 10) {
                            auto obj_sz = get_obj_size(ram);
                            auto val = ram.read_unsafe(constants::OAM_BEGIN + current_oam_obj);
                            int top = val - 16;
                            if (current_line >= top && current_line < top + obj_sz) {
                                sprite_buffer.emplace(
                                        Sprite{
                                                .y_pos = val,
                                                .x_pos = static_cast<unsigned char>(ram.read_unsafe(
                                                        constants::OAM_BEGIN + current_oam_obj + 1)),
                                                .index_tile = ram.read_unsafe(constants::OAM_BEGIN + current_oam_obj + 2),
                                                .attrib = ram.read_unsafe(constants::OAM_BEGIN + current_oam_obj + 3),
                                        }, current_oam_obj);
                            }
                            current_oam_obj += 4;
                        }
                        if (!read_sprite_count) {
                            current_oam_mode = Gap;
                            ram.ppu_oam_usage = false;
                        }
                        break;
                }
                case Gap: {
                    --oam_gap_count;
                    if (oam_gap_count == 0) {
                        current_oam_mode = ReadSprite;
                        oam_gap_count = 2;
                        read_sprite_count = 2;
                        ram.ppu_oam_usage = true;
                    }
                }
            }
        } else {
            oam_start = true;
            current_oam_mode = ReadSprite;
            ram.ppu_oam_usage = false;
            read_sprite_count = 2;
            oam_gap_count = 2;

            oam_next_t_cycle = 2;
            current_oam_obj = 0;
            current_pixel_x = 0;
            current_mode = Drawing;

#if DEBUG_T_CYCLE
            PRINT_T_CYCLE("OAM", debug_t_cycles);
#endif
//                set_ppu_mode(ram);
        }
    } else if (current_mode == Drawing) {
#if DEBUG_T_CYCLE
        ++debug_t_cycles;
//            std::cout << "debug_t_cycles:" << debug_t_cycles << '\n';
#endif
        if (current_pixel_x < 160) {
            if (((ram.read_unsafe(constants::LCDC_ADDRESS) >> 5) & 1)
                && (ram.read_unsafe(constants::WY_ADDRESS) == ram.read_unsafe(constants::LY_ADDRESS))
                && !window_line_is_triggered) {
                window_line_is_triggered = true;
            }
            if (!pixel_fetcher.paused) {
                --pixel_fetcher.next_t_cycle;
                if (pixel_fetcher.next_t_cycle == 0) {
                    pixel_fetcher.fetch(ram,  current_pixel_x);
                }
            }

            if (!sprite_render) {
                auto sprite = sprite_on_x(ram);
                if (sprite.has_value()) {
                    current_sprite = sprite.value();
                    sprite_render = true;
                    pixel_fetcher.paused = true;
                    sprite_fetcher.finished = false;
                }
            }

            if (sprite_render) {
                --sprite_fetcher.next_t_cycle;
                if (sprite_fetcher.next_t_cycle == 0) {
                    sprite_fetcher.fetch(ram, current_pixel_x, current_sprite);
                    if (sprite_fetcher.finished) {
                        pixel_fetcher.paused = false;
                        pixel_fetcher.next_t_cycle = 2;
                        sprite_render = false;
                        sprite_fetcher.finished = false;
                    }
                }
            }

            if ((!pixel_fetcher.fifo.empty() || !sprite_fetcher.fifo.empty()) &&
                !pixel_fetcher.paused) {
                draw(ram);
            }
        } else {
            sprite_fetcher.reset();
            pixel_fetcher.reset();
//                    internal_t_cycles = 0;
            if (ram.read_unsafe(constants::STAT_ADDRESS) & (1 << 3))
                ram.write_unsafe(constants::IF_ADDRESS, (ram.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));

            switch(ram.read_unsafe(constants::SCX_ADDRESS) % 8){
                case 0: {ly_increment_delay = 51; break;}
                case 1:
                case 2:
                case 3:
                case 4:
                {ly_increment_delay = 50; break;}
                case 5:
                case 6:
                case 7:
                {ly_increment_delay = 49; break;}
            }
            ly_increment_done = false;
#if DEBUG_T_CYCLE
            if (debug_t_cycles != 172){
                       debug_t_cycles += 0;
                   }
                   PRINT_T_CYCLE("Drawing", debug_t_cycles)
#endif
            current_mode = HBlank;
//                    set_ppu_mode(ram);
        }
    } else if (current_mode == HBlank) {
#if DEBUG_T_CYCLE
        ++debug_t_cycles;
#endif
//            if (!ly_increment_done && --ly_increment_delay == 0) {
//                ly_increment_done = true;
//
//                if (current_line == 70) {
//                    current_line+=0;
//                }
//            }
//            std::cout << "ly_increment_delay:" << ly_increment_delay << '\n';
        if (current_t_cycle_count >= 456){
#if DEBUG_T_CYCLE
            //                assert(current_t_cycle_count == 456);
#endif
            pixel_fetcher.fetcher_current_tile_x = 0;
            sprite_fetcher.fetcher_current_tile_x = 0;
            current_pixel_x = 0;
            current_t_cycle_count = 0;

            // LYC == LY

            if (current_line == 143){
#if DEBUG_T_CYCLE
                PRINT_T_CYCLE("HBlank", debug_t_cycles)
#endif
                current_mode = VBlank;
//                    set_ppu_mode(ram);
                // VBlank Interrupt
                ram.write_unsafe(constants::IF_ADDRESS, ram.read_unsafe(constants::IF_ADDRESS) | 1);
                // LCDC
                if (ram.read_unsafe(constants::STAT_ADDRESS) & (1 << 4))
                    ram.write_unsafe(constants::IF_ADDRESS, (ram.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
                current_v_blank_t_cycle_count = 0;
            } else {
#if DEBUG_T_CYCLE
                PRINT_T_CYCLE("HBlank", debug_t_cycles)
                    PRINT_LINE(current_line)
                    PRINT_DOTS(20)
#endif
                current_mode = OAM;
//                    set_ppu_mode(ram);
                pixel_fetcher.is_windows_drawing = false;
                pixel_fetcher.windows_current_tile_x = 0;
                // LCD Interrupt
                if (ram.read_unsafe(constants::STAT_ADDRESS) & (1 << 5))
                    ram.write_unsafe(constants::IF_ADDRESS, (ram.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
                sprite_buffer = {};
                oam_next_t_cycle = 2;
                current_oam_obj = 0;
            }
            set_ppu_reg(constants::LY_ADDRESS, ++current_line, ram);
            window_line_counter = 0;
        }
    } else if (current_mode == VBlank) {
#if DEBUG_T_CYCLE
        ++debug_t_cycles;
#endif
        if (++current_v_blank_t_cycle_count == 456){
            ++current_line;
            current_v_blank_t_cycle_count = 0;
            if (current_line == 154) {
#if DEBUG_T_CYCLE
                PRINT_T_CYCLE("VBlank", debug_t_cycles)
                    PRINT_DOTS(20)
#endif
                last_t_cycles = 0;
                current_mode = OAM;
//                    set_ppu_mode(ram);
                // LCD Interrupt
                if (ram.read_unsafe(constants::STAT_ADDRESS) & (1 << 5))
                    ram.write_unsafe(constants::IF_ADDRESS, (ram.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
                current_oam_obj = 0;
                oam_next_t_cycle = 2;
//                    set_ppu_mode(ram);
                current_line = 0;
#if DEBUG_T_CYCLE
                std::cout << "internal_t_cycles:" << internal_t_cycles << '\n';
#endif
                internal_t_cycles = 0;
                set_ppu_reg(constants::LY_ADDRESS, current_line, ram);
                current_pixel_x = 0;
                current_t_cycle_count = 0;
                window_line_is_triggered = false;
                pixel_fetcher.is_windows_drawing = false;
                is_windows_drawing = false;
            } else {
                set_ppu_reg(constants::LY_ADDRESS, current_line, ram);
                prev_t_cycles = last_t_cycles;
            }
        }
    }
}


auto PPU::read_buffer() const -> const Color(&)[144][160] {
    return buffer;
}

void PPU::print_buffer() const {
    std::cout << "\n=======================================\n";
    for (auto&& row: buffer){
        for (auto& el: row) {
            std::cout << Pixel::get_color_id(el);
        }
        std::cout << '\n';
    }
    std::cout << "\n=======================================\n";
}

bool PPU::is_windows_fetching_ready(RAM &ram) {
    return window_line_is_triggered && ((ram.read_unsafe(constants::LCDC_ADDRESS) >> 5) & 1) && (ram.read_unsafe(
            constants::LCDC_ADDRESS) & 1)
           && current_pixel_x >= (ram.read_unsafe(constants::WX_ADDRESS) - 7);
}

unsigned short PPU::get_sprite_fetcher_tile_address() const {
    return sprite_fetcher.tile_address;
}

unsigned short PPU::get_bg_fetcher_tile_address() const {
    return pixel_fetcher.tile_address;
}

void PPU::reset(RAM& ram) {
    lyc_ly_compare(ram);
    *this = PPU{};
    current_mode = HBlank;
    set_ppu_reg(constants::LY_ADDRESS, current_line, ram);
    ram.ppu_internal_t_cycle = internal_t_cycles;
    set_ppu_mode(ram);
}

void PPU::set_start_mode() {
    current_mode = OAM;

}

void PPU::lyc_ly_compare(RAM& ram) {
    auto lyc = ram.read_unsafe(constants::LYC_ADDRESS);
    unsigned char stat = ram.read_unsafe(constants::STAT_ADDRESS);
    if(current_line == lyc) {
        if (!((stat >> 2) & 1)) {
            ram.write_unsafe(constants::STAT_ADDRESS, stat | (1 << 2));
            if (stat & (1 << 6)){
                ram.write_unsafe(constants::IF_ADDRESS, (ram.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
            }
        }
    } else {
        if ((stat >> 2) & 1) {
            ram.write_unsafe(constants::STAT_ADDRESS,
                             (ram.read_unsafe(constants::STAT_ADDRESS) & (~(1 << 2))));
        }
    }
}
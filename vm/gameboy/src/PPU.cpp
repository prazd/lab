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

std::deque<Pixel> Pixel::tile_row_to_pixel_arr(unsigned short row, bool is_sprite, unsigned char pallete)  {
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

void PPU::set_ppu_reg(unsigned short address, unsigned char value, Memory& mem) {
    mem.write_unsafe(address, value);
}

void PPU::set_ppu_mode(Memory &mem) {
    mem.write_unsafe(constants::STAT_ADDRESS, (mem.read_unsafe(constants::STAT_ADDRESS) & 0xFC) | current_mode);
}
void PPU::update_lcd(const Memory &mem) {
    lcd_enable = (((mem.read_unsafe(constants::LCDC_ADDRESS)) >> 7) & 1);
}
void PPU::update_window(const Memory &mem) {
    window_visible = (((mem.read_unsafe(constants::LCDC_ADDRESS)) >> 5) & 1);
}
void PPU::update_obj(const Memory &mem) {
    obj_enable = (((mem.read_unsafe(constants::LCDC_ADDRESS)) >> 1) & 1);
}
void PPU::update_bg(const Memory &mem)  {
    bg_enable = ((mem.read_unsafe(constants::LCDC_ADDRESS)) & 1);
}
void PPU::update_regs(const Memory &mem) {
//    update_lcd(mem);
    update_window(mem);
    update_obj(mem);
    update_bg(mem);
}

unsigned char PPU::get_obj_size(const Memory &mem)  {
    return (((mem.read_unsafe(constants::LCDC_ADDRESS)) >> 2) & 1) ? 16 : 8;
}

void PPU::set_bg_palette(Memory& mem, Pixel& bg_pixel) {
//    LCDC.0 has different meanings depending on Game Boy type and Mode:
//    Non-CGB Mode (DMG, SGB and CGB in compatibility mode): BG and Window display
//    When Bit 0 is cleared, both background and window become blank (white), and the Window Display Bit is ignored in that case. Only objects may still be displayed (if enabled in Bit 1).
    if (!(mem.read_unsafe(constants::LCDC_ADDRESS) & 0x1)){
        bg_pixel.color = Pixel::get_pixel_color(0);
        return;
    }
    auto bgp = mem.read_unsafe(constants::BGP);
    switch (Pixel::get_color_id(bg_pixel.color)) {
        case 1:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 2) & 3);  break;}
        case 2:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 4) & 3);  break;}
        case 3:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 6) & 3);  break;}
        default:
        {bg_pixel.color = Pixel::get_pixel_color((bgp & 3));  break;}
    }
}

void PPU::set_sprite_palette(Memory &mem, Pixel& sprite_pixel) {
    unsigned char obp{};
    if ((sprite_pixel.attrib >> 4 & 1))
        obp = mem.read_unsafe(constants::OBP1);
    else
        obp = mem.read_unsafe(constants::OBP0);

    switch (Pixel::get_color_id(sprite_pixel.color)) {
        case 1:  {sprite_pixel.color = Pixel::get_pixel_color((obp >> 2) & 3);  break;}
        case 2:  {sprite_pixel.color = Pixel::get_pixel_color((obp >> 4) & 3);  break;}
        case 3:  {sprite_pixel.color = Pixel::get_pixel_color((obp >> 6) & 3);  break;}
    }
}


Pixel PPU::pixel_mixing(Memory &mem) {
    auto bg_pixel = Pixel{.color = WHITE}; // Default
    if (!pixel_fetcher.fifo.empty()){
        bg_pixel = pixel_fetcher.fifo.front(); pixel_fetcher.fifo.pop_front();
    }

    if (!(mem.read_unsafe(constants::LCDC_ADDRESS) & 0x2)) {
        set_bg_palette(mem, bg_pixel);
        return bg_pixel;
    }

    if (sprite_fetcher.fifo.empty()) {
        set_bg_palette(mem, bg_pixel);
        return bg_pixel;
    }

    auto sprite_pixel = sprite_fetcher.fifo.front();  sprite_fetcher.fifo.pop_front();
    //  If the color number of the Sprite Pixel is 0, the Background Pixel is pushed to the buffer.
    if (Pixel::get_color_id(sprite_pixel.color) == 0){
        set_bg_palette(mem, bg_pixel);
        return bg_pixel;
    }

    //  If the BG-to-OBJ-Priority bit is 1 and the color number of the Background Pixel is anything other than 0,
    //  the Background Pixel is pushed to the buffer
    if ((sprite_pixel.attrib >> 7 & 1) && Pixel::get_color_id(bg_pixel.color) != 0){
        set_bg_palette(mem, bg_pixel);
        return bg_pixel;
    }

    set_sprite_palette(mem, sprite_pixel);
    return sprite_pixel;
}

std::optional<Sprite> PPU::sprite_on_x(Memory &mem)  {
   if (!sprite_buffer.empty()) {
       auto current = sprite_buffer.top();
       if (current.first.x_pos - 8 <= current_pixel_x) {
           sprite_buffer.pop();
           return current.first;
       }
   }
    return std::nullopt;
}

void PPU::draw(Memory &mem)  {
    buffer[current_line][current_pixel_x++] = pixel_mixing(mem).color;
    if (!pixel_fetcher.is_windows_drawing && is_windows_fetching_ready(mem)) {
        pixel_fetcher.reset();
        if (!is_win_init){
            pixel_fetcher.current_window_line = 0;
            is_win_init = true;
        }
        pixel_fetcher.is_windows_drawing = true;
        pixel_fetcher.windows_current_tile_x = 0;
    }
}

void PPU::tick(Memory &mem)  {
#define DEBUG_T_CYCLE 0
#define PRINT_T_CYCLE(x, t_cycles) std::cout << x << ": " << t_cycles << '\n'; t_cycles = 0;
#define PRINT_DOTS(n) for (int i = 0; i < n; ++i) std::cout << '-'; std::cout << '\n';
#define PRINT_LINE(line) std::cout << "Line: " << (unsigned)line << '\n';

    static size_t debug_t_cycles = 0;
#if 0
    mem.write_unchecked(constants::LY_ADDRESS, 0x90);
    return;
#endif
    ++internal_t_cycles;
    ++current_t_cycle_count;

    mem.ppu_internal_t_cycle = current_t_cycle_count;

    ++last_t_cycles;
    update_regs(mem);
    set_ppu_mode(mem);
    lyc_ly_compare(mem);

    if (current_mode == OAM){
#if DEBUG_T_CYCLE
        ++debug_t_cycles;
#endif
        if (current_t_cycle_count != 80) {
            // 1Mcycle (4t-cycle) -> read 1 row (8byte ==> 2 sprite)
            // 2t-cycle read row + 2t-cycle gap
            if (oam_start) {
                mem.ppu_oam_usage = true;
                oam_start = false;
            }
            switch (current_oam_mode) {
                case ReadSprite: {
                        --read_sprite_count;
                        if (sprite_buffer.size() != 10) {
                            auto obj_sz = get_obj_size(mem);
                            auto val = mem.read_unsafe(constants::OAM_BEGIN + current_oam_obj);
                            int top = val - 16;
                            if (current_line >= top && current_line < top + obj_sz) {
                                sprite_buffer.emplace(
                                        Sprite{
                                                .y_pos = val,
                                                .x_pos = static_cast<unsigned char>(mem.read_unsafe(
                                                        constants::OAM_BEGIN + current_oam_obj + 1)),
                                                .index_tile = mem.read_unsafe(constants::OAM_BEGIN + current_oam_obj + 2),
                                                .attrib = mem.read_unsafe(constants::OAM_BEGIN + current_oam_obj + 3),
                                        }, current_oam_obj);
                            }
                            current_oam_obj += 4;
                        }
                        if (!read_sprite_count) {
                            current_oam_mode = Gap;
                            mem.ppu_oam_usage = false;
                        }
                        break;
                }
                case Gap: {
                    --oam_gap_count;
                    if (oam_gap_count == 0) {
                        current_oam_mode = ReadSprite;
                        oam_gap_count = 2;
                        read_sprite_count = 2;
                        mem.ppu_oam_usage = true;
                    }
                }
            }
        } else {
            oam_start = true;
            current_oam_mode = ReadSprite;
            mem.ppu_oam_usage = false;
            read_sprite_count = 2;
            oam_gap_count = 2;

            oam_next_t_cycle = 2;
            current_oam_obj = 0;
            current_pixel_x = 0;
            current_mode = Drawing;

#if DEBUG_T_CYCLE
            PRINT_T_CYCLE("OAM", debug_t_cycles);
#endif
//                set_ppu_mode(mem);
        }
    } else if (current_mode == Drawing) {
#if DEBUG_T_CYCLE
        ++debug_t_cycles;
//            std::cout << "debug_t_cycles:" << debug_t_cycles << '\n';
#endif
        if (current_pixel_x < 160) {
            if (!pixel_fetcher.paused) {
                --pixel_fetcher.next_t_cycle;
                if (pixel_fetcher.next_t_cycle == 0) {
                    pixel_fetcher.fetch(mem,  current_pixel_x);
                }
            }

            if (!sprite_render && mem.read_unsafe(constants::LCDC_ADDRESS) & (1 << 1)){
                auto sprite = sprite_on_x(mem);
                if (sprite.has_value()) {
                    current_sprite = sprite.value();
                    sprite_render = true;
                    // todo: check
//                    pixel_fetcher.current_state = decltype(pixel_fetcher)::FetcherState::GetTile;
                    pixel_fetcher.paused = true;
                    sprite_fetcher.finished = false;
                }
            }

            if (sprite_render) {
                --sprite_fetcher.next_t_cycle;
                if (sprite_fetcher.next_t_cycle == 0) {
                    sprite_fetcher.fetch(mem, current_pixel_x, current_sprite);
                    if (sprite_fetcher.finished) {
                        pixel_fetcher.paused = false;
                        sprite_render = false;
                        sprite_fetcher.finished = false;
                    }
                }
            }

            if ((!pixel_fetcher.fifo.empty()) &&
                !pixel_fetcher.paused) {
                draw(mem);
            }
        } else {
            sprite_fetcher.reset();
            pixel_fetcher.reset();
            pixel_fetcher.windows_current_tile_x = 0;
            if (mem.read_unsafe(constants::STAT_ADDRESS) & (1 << 3))
                mem.write_unsafe(constants::IF_ADDRESS, (mem.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));

            switch(mem.read_unsafe(constants::SCX_ADDRESS) % 8){
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
//                    set_ppu_mode(mem);
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
//            pixel_fetcher.current_window_line = 0;
            pixel_fetcher.windows_current_tile_x = 0;
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
                // VBlank Interrupt
                mem.write_unsafe(constants::IF_ADDRESS, mem.read_unsafe(constants::IF_ADDRESS) | 1);
                // LCDC
                if (mem.read_unsafe(constants::STAT_ADDRESS) & (1 << 4))
                    mem.write_unsafe(constants::IF_ADDRESS, (mem.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
                current_v_blank_t_cycle_count = 0;
            } else {
#if DEBUG_T_CYCLE
                PRINT_T_CYCLE("HBlank", debug_t_cycles)
                    PRINT_LINE(current_line)
                    PRINT_DOTS(20)
#endif
                current_mode = OAM;
                pixel_fetcher.windows_current_tile_x = 0;
                // LCD Interrupt
                if (mem.read_unsafe(constants::STAT_ADDRESS) & (1 << 5))
                    mem.write_unsafe(constants::IF_ADDRESS, (mem.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
                sprite_buffer = {};
                oam_next_t_cycle = 2;
                current_oam_obj = 0;
            }
            if (pixel_fetcher.is_windows_drawing) {
                ++pixel_fetcher.current_window_line;
                pixel_fetcher.is_windows_drawing = false;
            }
            set_ppu_reg(constants::LY_ADDRESS, ++current_line, mem);
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

                // LCD Interrupt
                if (mem.read_unsafe(constants::STAT_ADDRESS) & (1 << 5))
                    mem.write_unsafe(constants::IF_ADDRESS, (mem.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
                current_oam_obj = 0;
                oam_next_t_cycle = 2;
                current_line = 0;
#if DEBUG_T_CYCLE
                std::cout << "internal_t_cycles:" << internal_t_cycles << '\n';
#endif
                internal_t_cycles = 0;
                set_ppu_reg(constants::LY_ADDRESS, current_line, mem);
                current_pixel_x = 0;
                use_saved_window_ly = false;
                is_win_init = false;
                save_window_ly = 0;
                current_t_cycle_count = 0;
                pixel_fetcher.is_windows_drawing = false;
            } else {
                set_ppu_reg(constants::LY_ADDRESS, current_line, mem);
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

bool PPU::is_windows_fetching_ready(Memory &mem) {
    return
            ((mem.read_unsafe(constants::LCDC_ADDRESS) >> 5) & 1) &&
            (mem.read_unsafe(constants::LCDC_ADDRESS) & 1) &&
           current_line >= mem.read_unsafe(constants::WY_ADDRESS) &&
           current_pixel_x >= ((int)(mem.read_unsafe(constants::WX_ADDRESS) - 7));
}

unsigned short PPU::get_sprite_fetcher_tile_address() const {
    return sprite_fetcher.tile_address;
}

unsigned short PPU::get_bg_fetcher_tile_address() const {
    return pixel_fetcher.tile_address;
}

void PPU::reset(Memory& mem) {
    lyc_ly_compare(mem);
    *this = PPU{};
    current_mode = HBlank;
    set_ppu_reg(constants::LY_ADDRESS, current_line, mem);
    mem.ppu_internal_t_cycle = internal_t_cycles;
    set_ppu_mode(mem);
}

void PPU::set_start_mode() {
    current_mode = OAM;
}

void PPU::lyc_ly_compare(Memory& mem) {
    auto lyc = mem.read_unsafe(constants::LYC_ADDRESS);
    unsigned char stat = mem.read_unsafe(constants::STAT_ADDRESS);
    if(current_line == lyc) {
        if (!((stat >> 2) & 1)) {
            mem.write_unsafe(constants::STAT_ADDRESS, stat | (1 << 2));
            if (stat & (1 << 6)){
                mem.write_unsafe(constants::IF_ADDRESS, (mem.read_unsafe(constants::IF_ADDRESS) | (1 << 1)));
            }
        }
    } else {
        if ((stat >> 2) & 1) {
            mem.write_unsafe(constants::STAT_ADDRESS,
                             (mem.read_unsafe(constants::STAT_ADDRESS) & (~(1 << 2))));
        }
    }
}
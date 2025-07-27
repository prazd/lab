#include "PPU.h"

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

std::queue<Pixel> Pixel::tile_row_to_pixel_arr(unsigned short row, bool is_sprite, unsigned char pallete, unsigned char pcx, const Sprite& s)  {
    unsigned char l = row & 0xff;
    unsigned char h = (row >> 8) & 0xff;
    std::queue<Pixel> result{};
    unsigned char lc{};
    unsigned char hc{};
    Color color;

    int x_in_tile = pcx - (s.x_pos - 8);
    for (int i = x_in_tile; i < 8; ++i) {
        lc = (l >> (7 - i)) & 1;
        hc = (h >> (7 - i)) & 1;
        color = Pixel::get_pixel_color((hc << 1 | lc));
        result.push(Pixel{.color = color, .is_sprite = is_sprite, .attrib = pallete});
    }
    return result;
}

std::queue<Pixel> Pixel::tile_row_to_pixel_arr(unsigned short row, bool is_sprite, unsigned char pallete, unsigned char)  { // todo: maybe refactoring
    unsigned char l = row & 0xff;
    unsigned char h = (row >> 8) & 0xff;
    std::queue<Pixel> result{};
    unsigned char lc{};
    unsigned char hc{};
    Color color;
    for (int i = 0; i < 8; ++i) {
        lc = (l >> (7 - i)) & 1;
        hc = (h >> (7 - i)) & 1;
        color = Pixel::get_pixel_color((hc << 1 | lc));
        result.push(Pixel{.color = color, .is_sprite = is_sprite, .attrib = pallete});
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
    ram.write(address, value);
}

void PPU::set_ppu_mode(RAM &ram) {
    ram.write(constants::STAT_ADDRESS, (current_mode | ram.read(constants::STAT_ADDRESS)) & 0xFC);
}
void PPU::update_lcd(const RAM &ram) {
    lcd_enable = (((ram.read(constants::LCDC_ADDRESS)) >> 7) & 1);
}
void PPU::update_window(const RAM &ram) {
    window_visible = (((ram.read(constants::LCDC_ADDRESS)) >> 5) & 1);
}
void PPU::update_obj(const RAM &ram) {
    obj_enable = (((ram.read(constants::LCDC_ADDRESS)) >> 1) & 1);
}
void PPU::update_bg(const RAM &ram)  {
    bg_enable = ((ram.read(constants::LCDC_ADDRESS)) & 1);
}
void PPU::update_regs(const RAM &ram) {
//    update_lcd(ram);
    update_window(ram);
    update_obj(ram);
    update_bg(ram);
}

unsigned char PPU::get_obj_size(const RAM &ram)  {
    return (((ram.read(constants::LCDC_ADDRESS)) >> 2) & 1) ? 16 : 8;
}

unsigned short PPU::get_window_tile_map_area(const RAM &ram) {
    if (((ram.read(constants::LCDC_ADDRESS) >> 6) & 1))
        return 0x9C00;
    return 0x9800;
}
unsigned short PPU::get_bg_tile_map_area(const RAM &ram)  {
    if (((ram.read(constants::LCDC_ADDRESS) >> 3) & 1))
        return 0x9C00;
    return 0x9800;
}
unsigned short PPU::get_tile_data_area(const RAM &ram)  {
    if (((ram.read(constants::LCDC_ADDRESS) >> 4) & 1))
        return 0x8800;
    return 0x8000;
}

void PPU::oam_scan(RAM &ram)  {
    std::vector<std::pair<Sprite, int>> result{};
    size_t count{};
    auto obj_sz = get_obj_size(ram);
    for (int i = 0; i < 160; i+=4){
        auto val= ram.read(constants::OAM_BEGIN + i);
        int top = val - 16;
        if (current_line >= top && current_line < top + obj_sz){
            result.emplace_back(Sprite{
                    .y_pos = val,
                    .x_pos = static_cast<unsigned char>(ram.read(constants::OAM_BEGIN + i + 1)),
                    .index_tile = ram.read(constants::OAM_BEGIN + i + 2),
                    .attrib = ram.read(constants::OAM_BEGIN + i + 3),
            }, i/4);
            ++count;
        }
        if (count == 10) break;
    }
    sprite_buffer.clear();
    std::sort(result.begin(), result.end(), [](auto& a, auto& b){
        return a.first.x_pos < b.first.x_pos || (a.first.x_pos == b.first.x_pos && a.second < b.second);
    });
    for (auto& el: result)
        sprite_buffer.push_back(el.first);
    current_sprite_index = 0;
}


Pixel PPU::pixel_mixing(RAM &ram) {
    auto bg_pixel = Pixel{.color = WHITE}; // Default
    if (!pixel_fetcher.fifo.empty()){
        bg_pixel = pixel_fetcher.fifo.front(); pixel_fetcher.fifo.pop();
    }
    auto bgp = ram.read(constants::BGP);
    switch (Pixel::get_color_id(bg_pixel.color)) {
        case 1:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 2) & 3);  break;}
        case 2:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 4) & 3);  break;}
        case 3:  {bg_pixel.color = Pixel::get_pixel_color((bgp >> 6) & 3);  break;}
        default:
            {bg_pixel.color = Pixel::get_pixel_color((bgp & 3));  break;}
    }
    if (!(ram.read(constants::LCDC_ADDRESS) & 0x2)) {
        return bg_pixel;
    }
    if (sprite_fetcher.fifo.empty()) {
        return bg_pixel;
    }
    auto sprite_pixel = sprite_fetcher.fifo.front();  sprite_fetcher.fifo.pop();
    //  If the color number of the Sprite Pixel is 0, the Background Pixel is pushed to the buffer.
    if (Pixel::get_color_id(sprite_pixel.color) == 0){
        return bg_pixel;
    }

    //  If the BG-to-OBJ-Priority bit is 1 and the color number of the Background Pixel is anything other than 0,
    //  the Background Pixel is pushed to the buffer
    if ((sprite_pixel.attrib >> 7 & 1) && Pixel::get_color_id(bg_pixel.color) != 0){
        return bg_pixel;
    }

    unsigned char pallete = 0;
    if ((sprite_pixel.attrib >> 4 & 1))
        pallete = ram.read(constants::OBP1);
    else
        pallete = ram.read(constants::OBP0);

    switch (Pixel::get_color_id(sprite_pixel.color)) {
        case 1:  {sprite_pixel.color = Pixel::get_pixel_color((pallete >> 2) & 3);  break;}
        case 2:  {sprite_pixel.color = Pixel::get_pixel_color((pallete >> 4) & 3);  break;}
        case 3:  {sprite_pixel.color = Pixel::get_pixel_color((pallete >> 6) & 3);  break;}
    }
    return sprite_pixel;
}

std::optional<Sprite> PPU::sprite_on_x(RAM &ram)  {
   if (current_sprite_index < sprite_buffer.size()) {
       auto current = sprite_buffer[current_sprite_index];
       if (current.x_pos - 8 <= current_pixel_x && current_pixel_x < current.x_pos) {
           ++current_sprite_index;
           return current;
       }
   }
    return {};
}

void PPU::draw(RAM &ram)  {
    buffer[current_line][current_pixel_x++] = pixel_mixing(ram).color;
}

void PPU::handle_t_cycle(int t_cycle, RAM &ram)  {
    static size_t last_t_cycles = 0;
    static size_t prev_t_cycles = 0;
    static bool sprite_render = false;
    static unsigned char sprite_x = 0;
    static Sprite current_sprite{};
    static size_t internal_t_cycles = 0;
//#if 1
//    ram.write(constants::LY_ADDRESS, 0x90);
//    return;
//#endif

//    std::cout << "CurrentLine:" << (unsigned)current_line << '\n';
    while(t_cycle--){
        ++internal_t_cycles;
        ++current_t_cycle_count;
        ++last_t_cycles;
        update_regs(ram);
//    std::cout << "\n=========================\n";
//    std::cout << "PPU:\n";
//    std::cout << "T-Cycle: " << current_t_cycle_count << '\n';
//    std::cout << "CURRENT_LINE:" << current_line << '\n';
//    std::cout << "CURRENT_MODE:" << current_mode << '\n';
//    std::cout << "\n=========================\n";
//    std::cout << "BG fifo:" << pixel_fetcher.fifo.size() << " Sprite fifo: " << sprite_fetcher.fifo.size() <<  '\n';
        if (current_mode == OAM){ //
            if (current_t_cycle_count <= 80) {
                if (!oam_scan_finished) {
                    oam_scan(ram);
                    oam_scan_finished = true;
                }
            } else {
                current_pixel_x = 0;
                internal_t_cycles = 0;
                current_mode = Drawing;
                set_ppu_mode(ram);
            }
        } else if (current_mode == Drawing) {
                if (current_pixel_x < 160) {
                    if (!pixel_fetcher.paused) --pixel_fetcher.next_t_cycle;
                    if (pixel_fetcher.next_t_cycle == 0) {
                        pixel_fetcher.fetch(ram,  current_pixel_x);
                    }
                    if (!sprite_render) {
                        auto sprite = sprite_on_x(ram);
                        if (sprite.has_value()) {
                            current_sprite = sprite.value();
                            sprite_render = true;
                            pixel_fetcher.paused = true;
                            pixel_fetcher.current_state = decltype(pixel_fetcher)::FetcherState::GetTile;
                            pixel_fetcher.reset();
                            sprite_fetcher.finished = false;
                        }
                    }

                    if (sprite_render) {
                        --sprite_fetcher.next_t_cycle;
                        if (sprite_fetcher.next_t_cycle == 0) {
                            sprite_fetcher.fetch(ram, current_pixel_x, current_sprite);
                            if (sprite_fetcher.finished) {
                                pixel_fetcher.paused = false;
                                sprite_render = false;
                                sprite_fetcher.finished = false;
                            }
                        }
                    }
                    if (!pixel_fetcher.fifo.empty() || !sprite_fetcher.fifo.empty()) draw(ram);
                } else {
//                    std::cout << "Internal t cycles:" << internal_t_cycles << '\n';
                    sprite_fetcher.reset();
                    pixel_fetcher.reset();
                    current_sprite_index = 0;
                    current_mode = HBlank;
                    set_ppu_mode(ram);
            }
        } else if (current_mode == HBlank) {
            if (current_t_cycle_count >= 456){
                set_ppu_reg(constants::LY_ADDRESS, ++current_line, ram);
                // LYC
                auto lyc = ram.read(constants::LYC_ADDRESS);
                unsigned char stat = ram.read(constants::STAT_ADDRESS);
                if(current_line == lyc) {
                    ram.write(constants::STAT_ADDRESS, (ram.read(constants::STAT_ADDRESS) | (1 << 2)));
                    if (stat & (1 << 6)) ram.write(constants::IF_ADDRESS, (ram.read(constants::IF_ADDRESS) | 1 << 1));
                } else {
                    ram.write(constants::STAT_ADDRESS, (ram.read(constants::STAT_ADDRESS) & (~(1 << 2))));
                }

                pixel_fetcher.fetcher_current_tile_x = 0; // todo: check
                current_pixel_x = 0;
                current_t_cycle_count = 0;
                if (current_line == 144){
                    current_mode = VBlank;
                    ram.write(constants::IF_ADDRESS, ram.read(constants::IF_ADDRESS) | 1);
                    current_v_blank_t_cycle_count = 0;
                }
                    // request_interrupt(VBLANK_INTERRUPT); // todo:
                else{
                    current_mode = OAM;
                    oam_scan_finished = false;
                }
                window_line_counter = 0;
                set_ppu_mode(ram);
            }
        } else if (current_mode == VBlank) {
            if (current_v_blank_t_cycle_count++ == 456){
                ++current_line;
                if (current_line == 154) {
                    last_t_cycles = 0;
                    current_mode = OAM;
                    set_ppu_mode(ram);
                    current_line = 0;
                    set_ppu_reg(constants::LY_ADDRESS, current_line, ram);
//                    std::cout << std::dec <<"T_cycles:" << last_t_cycles - prev_t_cycles << '\n';
                    current_pixel_x = 0;
                    current_window_line = 0;
                    current_t_cycle_count = 0;
                    current_v_blank_t_cycle_count = 0;
                } else {
                    set_ppu_reg(constants::LY_ADDRESS, current_line, ram);
//                    std::cout << std::dec <<"T_cycles:" << last_t_cycles - prev_t_cycles << '\n';
                    prev_t_cycles = last_t_cycles;
                    current_v_blank_t_cycle_count = 0;
                }
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

#include "PPU.h"

Color Pixel::get_pixel_color(unsigned char id)  {
    switch(id){
        case 0: return BLACK;
        case 1: return LIGHTGRAY;
        case 2: return DARKGRAY;
        default:
            return WHITE;
    };
}

int Pixel::get_color_id(Color c)  {
    if (c == BLACK)
        return 0;
    if (c == LIGHTGRAY)
        return 1;
    if (c == DARKGRAY)
        return 2;
    return 3;
}

std::queue<Pixel> Pixel::tile_row_to_pixel_arr(unsigned short row)  {
    auto l = row & 0xff;
    auto h = (row >> 8) & 0xff;
    std::queue<Pixel> result{};
    unsigned char lc{};
    unsigned char hc{};
    Color color;
    for (int i = 0; i < 8; ++i) {
        lc = (l >> (7 - i)) & 1;
        hc = (h >> (7 - i)) & 1;
        color = Pixel::get_pixel_color((hc << 1 | lc));
        result.push(Pixel{.color = color, .is_sprite = false});
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
    return (((ram.read(constants::LCDC_ADDRESS)) >> 2) & 1) ? 8 : 16;
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
    std::vector<Sprite> result{};
    result.reserve(10);
    size_t count{};
    auto obj_sz = get_obj_size(ram);
    for (int i = 0; i < 160; i+=4){
        auto val = ram.read(constants::OAM_BEGIN + i);
        int top = val - 16;
        if (current_line >= top && current_line < top + obj_sz){
            result[count++] = Sprite{
                    .y_pos = val,
                    .x_pos = static_cast<unsigned char>(ram.read(constants::OAM_BEGIN + i + 1) - 8), // todo: check
                    .index_tile = ram.read(constants::OAM_BEGIN + i + 2),
                    .attrib = ram.read(constants::OAM_BEGIN + i + 3),
            };
        }
        if (count == 10) break;
    }
//    std::sort(result.begin(), result.begin() + count, [](auto& l, auto& r){return l.x_pos < r.x_pos;});
    sprite_buffer = std::move(result); // todo: check
}

Pixel PPU::pixel_mixing(RAM &ram)  {
    if (pixel_fetcher.fifo.empty() && sprite_fetcher.fifo.empty()){
        return Pixel{.color = WHITE};
    }

    auto bg_pixel = pixel_fetcher.fifo.front();
    pixel_fetcher.fifo.pop();
    if (sprite_fetcher.fifo.empty()) {
        return bg_pixel;
    }
    auto sprite_pixel = sprite_fetcher.fifo.front();sprite_fetcher.fifo.pop();

    //  If the color number of the Sprite Pixel is 0, the Background Pixel is pushed to the buffer.
    if (Pixel::get_color_id(sprite_pixel.color) == 0){
        return bg_pixel;
    }

    //  If the BG-to-OBJ-Priority bit is 1 and the color number of the Background Pixel is anything other than 0,
    //  the Background Pixel is pushed to the buffer
    if (bg_pixel.bg_priority && Pixel::get_color_id(bg_pixel.color) != 0){
        return bg_pixel;
    }

    return sprite_pixel;
}

std::optional<Sprite> PPU::sprite_on_x(RAM &ram)  {
    for (auto& sprite: sprite_buffer) {
        if (sprite.x_pos <= current_pixel_x + 8) {
            return sprite;
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
#if 1
    ram.write(constants::LY_ADDRESS, 0x90);
    return;
#endif

    while(t_cycle--){
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
                current_mode = Drawing;
                set_ppu_mode(ram);
            }
        } else if (current_mode == Drawing) {
                if (current_pixel_x < 160) {
                    if (pixel_fetcher.fifo.size() < 8){
                        pixel_fetcher.fetch(ram, current_pixel_x);
                    }
                    auto sprite = sprite_on_x(ram);
                    if (sprite.has_value()) {
                        sprite_fetcher.fetch(ram, sprite.value());
                    }

                    if (!pixel_fetcher.fifo.empty()) {
                        draw(ram);
                    }
                } else {
                    current_mode = HBlank;
                    set_ppu_mode(ram);
            }
        } else if (current_mode == HBlank) {
            if (current_t_cycle_count == 456){
                set_ppu_reg(constants::LY_ADDRESS, current_line++, ram);
//                std::cout << std::dec <<"T_cycles:" << last_t_cycles - prev_t_cycles << '\n';
//                prev_t_cycles = last_t_cycles;
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

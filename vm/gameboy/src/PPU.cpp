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
    update_lcd(ram);
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
//        std::cout << "bg_pixel1 return" << '\n' << " Color:" << Pixel::get_color_id(bg_pixel.color) << "\n";
        return bg_pixel;
    }
    auto sprite_pixel = sprite_fetcher.fifo.front();sprite_fetcher.fifo.pop();

    //  If the color number of the Sprite Pixel is 0, the Background Pixel is pushed to the buffer.
    if (Pixel::get_color_id(sprite_pixel.color) == 0){
//        std::cout << "bg_pixel2 return" << '\n';
        return bg_pixel;
    }

    //  If the BG-to-OBJ-Priority bit is 1 and the color number of the Background Pixel is anything other than 0,
    //  the Background Pixel is pushed to the buffer
    if (bg_pixel.bg_priority && Pixel::get_color_id(bg_pixel.color) != 0){
//        std::cout << "bg_pixel3 return" << '\n';
        return bg_pixel;
    }
//    std::cout << "SPRITE PIXEL RETURN" << '\n';
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
    auto res = pixel_mixing(ram).color;
    buffer[current_line][current_pixel_x++] = res;
}

void PPU::handle_t_cycle(int t_cycle, RAM &ram)  {
    update_regs(ram);
//    std::cout << "\n=========================\n";
//    std::cout << "PPU:\n";
//    std::cout << "T-Cycle: " << current_t_cycle_count << '\n';
//    std::cout << "CURRENT_LINE:" << current_line << '\n';
//    std::cout << "CURRENT_MODE:" << current_mode << '\n';
//    std::cout << "\n=========================\n";
//    std::cout << "BG fifo:" << pixel_fetcher.fifo.size() << " Sprite fifo: " << sprite_fetcher.fifo.size() <<  '\n';
    if (current_mode == VBlank)
        current_v_blank_t_cycle_count += t_cycle;
    current_t_cycle_count += t_cycle;
    if (current_mode == OAM){ //
        if (current_t_cycle_count < 80) {
            if (!oam_scan_finished) {
                oam_scan(ram);
                oam_scan_finished = true;
            }
        } else {
            current_mode = Drawing;
            set_ppu_mode(ram);
        }
        return;
    }

    if (current_mode == Drawing) {
//        if (((longest_line && current_t_cycle_count < 289) || (!longest_line && current_t_cycle_count < 172))) {
        if (current_pixel_x < 160){
//            std::cout << "CurrentLine drawing:" << current_line << '\n';
//            std::cout << "CurrentPixel drawing:" << current_pixel_x << '\n';
            pixel_fetcher.fetch(ram, current_pixel_x);
            auto sprite = sprite_on_x(ram);
            if (sprite.has_value())
                sprite_fetcher.fetch(ram, sprite.value());
            draw(ram);
        } else {
            current_mode = HBlank;
            set_ppu_mode(ram);
        }
        return;
    }

    if (current_mode == HBlank) {
    //        if (((!longest_line && current_t_cycle_count >= 204) || (longest_line && current_t_cycle_count >= 87))) {
    if (current_t_cycle_count > 456){
            ++current_line;
            current_pixel_x = 0;
            set_ppu_reg(constants::LY_ADDRESS, current_line, ram);
            current_t_cycle_count = 0;
            if (current_line == 144)
                current_mode = VBlank;
                // request_interrupt(VBLANK_INTERRUPT); // todo:
            else{
                current_mode = OAM;
                oam_scan_finished = false;
            }
            window_line_counter = 0;
            set_ppu_mode(ram);
        }
        return;
    }

    if (current_mode == VBlank) {
        if (current_v_blank_t_cycle_count >= 456){
            ++current_line;
            current_v_blank_t_cycle_count = 0;
            set_ppu_reg(constants::LY_ADDRESS, current_line, ram);
        }
        if (current_t_cycle_count >= 4560) {
            current_mode = OAM;
            current_line = 0;
            current_pixel_x = 0;
            current_window_line = 0;
            current_t_cycle_count = 0;
            set_ppu_mode(ram);
        }
    }

}

auto PPU::read_lcd() const -> const Color(&)[144][160] {
    return buffer;
}

void PPU::print_lcd(const RAM& ram) const {
    auto scy = ram.read(constants::SCY_ADDRESS);
    auto scx = ram.read(constants::SCX_ADDRESS);
    std::cout << "\n---------------------------------------\n";
    for (int i = 0; i < 144; ++i) {
        for (int j = 0; j < 160; ++j) {
            std::cout << Pixel::get_color_id(buffer[(scy + i) % 144][(scx + j) % 160]);
        }
        std::cout << '\n';
    }
    std::cout << "\n---------------------------------------\n";
}


#ifndef MAIN_CPP_DEBUG_H
#define MAIN_CPP_DEBUG_H
#include <array>
#include <iostream>
#include "Memory.h"
#include "PPU.h"
#include "rlgl.h"

using Tile = std::array<std::array<int, 8>, 8>;

struct DebugTools {
    static Tile get_sprite_tile(const Memory& mem, const Sprite& sprite) {
        auto lcdc_mode = (mem.read_unsafe(constants::LCDC_ADDRESS) >> 2) & 1;
        return get_tile_by_address(mem, ((lcdc_mode ? sprite.index_tile & 0xfe : sprite.index_tile) * 16) + constants::VRAM_BEGIN);
    }
    static Tile get_tile_by_address(const Memory& mem, unsigned short address){
        std::array<unsigned char, 16> arr{};
        for (int i = 0; i < 16; ++i){
            arr[i] = mem.read_unsafe(address + i);
        }
        std::array<std::array<int, 8>, 8> colors{};
        std::array<int, 8> tmp{};
        size_t current{};
        for (size_t i = 0; i < 16; i+=2){
            auto l = arr[i];
            auto h = arr[i+1];
            for (int j = 0; j < 8; ++j) {
                auto lc = (l >> (7 - j)) & 1;
                auto hc = (h >> (7 - j)) & 1;
                tmp[j] = (hc << 1 | lc);
            }
            colors[current++] = tmp;
        }
        return colors;
    };

    static void print_segment(const Memory& mem, unsigned short address, int n) {
        std::cout << "\n=============================\n";
        for (int i = 0; i < n; ++i){
            std::cout << std::hex << address + i <<  ":" << (unsigned)mem.read_unsafe(address + i) << '\n';
        }
        std::cout << "\n=============================\n";
        std::cout << std::dec;
    }

    static void draw_n_tiles(const Memory& mem, unsigned short begin_address, Rectangle rec, int n_tiles) {
        std::stringstream ss;
        ss << "Address:" << std::hex << begin_address;
        auto str = ss.str();
        float x_begin = rec.x;
        float y_begin = rec.y;
        DrawRectangleLinesEx(rec, 1, BLACK);
        y_begin+=10.f;
        DrawText(str.c_str(), x_begin, y_begin, 30.f, BLACK);
        y_begin += 30.f;
        std::vector<Tile> vec{};
        for (int i = 0; i < n_tiles; ++i)
            vec.push_back(get_tile_by_address(mem, begin_address + (16 * i)));

        float xstep = (rec.width/20)/8;
        float ystep = (rec.height/20)/8;
        float x = x_begin;
        float y = y_begin;
        size_t count = 0;
        float current_x = x;
        float current_y = y;
        auto prev_x = x;
        for (auto& colors: vec) {
            for (auto&& row: colors){
                for (auto el: row) {
                    DrawRectangle(x, y, xstep, ystep, Pixel::get_pixel_color(el));
                    x+=xstep;
                }
                y += ystep;
                prev_x = std::exchange(x, current_x);
            }
            if ((prev_x + (xstep) >= rec.x + rec.width)){
                y = current_y = y + ystep;
                x = current_x = x_begin;
            } else {
                y = current_y;
                x = current_x = prev_x;
            }
        }
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct DebugWindow {
    using Buffer = std::array<std::array<Color, pwidth>, pheight>;
    Rectangle rec;
    Texture2D target;
    RenderTexture2D target_renderer;
    Buffer buffer{};
    std::string header;
    DebugWindow(float x, float y):
            rec({x, y, width, height}), target_renderer(LoadRenderTexture(width, height)){
        Image img(GenImageColor(pwidth, pheight, WHITE));
        target = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    void update() {
        UpdateTexture(target, buffer.data());
    }
    void move_check() {
        if (CheckCollisionPointRec(GetMousePosition(), rec)){
            if (IsMouseButtonDown(0)) {
                rec.x = GetMousePosition().x - (width/2);
                rec.y = GetMousePosition().y - (height/2);
            }
        }
    }
    void draw(float font_sz = 30.f){
        move_check();
        DrawRectangleLinesEx(rec, 1, BLACK);
        DrawText(header.c_str(), rec.x, rec.y, font_sz, BLACK);
        DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{rec.x+1, rec.y+30, width-20, height-50}, {0, 0}, 0.0f, WHITE);
    }
    void draw_text(float font_sz = 30.f) {
        move_check();
        DrawRectangleLinesEx(rec, 1, BLACK);
        DrawText(header.c_str(), rec.x, rec.y, font_sz, BLACK);
    }
    void draw_render_texture() {
        DrawTexturePro(target_renderer.texture, {0, 0, (float)target_renderer.texture.width, -(float)target_renderer.texture.height}, Rectangle{rec.x, rec.y, width, height}, {0, 0}, 0.0f, WHITE);
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct RectsPrint: public DebugWindow<0, 0, width + 150, height + 150>{
    using DW = DebugWindow<0, 0, width + 150, height + 150>;
    std::array<Rectangle, pwidth * pheight> res{};
    RectsPrint(float x, float y):
            DW(x, y){
    }
    void update(const Memory& mem, float add_x, float add_y){
        float x_pos = 0.0f;
        float y_pos = 0.0f;

        float step_x = width/pwidth;
        float step_y = height/pheight;
        int z = 0;
        for (int i = 0; i < pheight; ++i){
            for (int j = 0; j < pwidth; ++j) {
                res[z++] = Rectangle{x_pos, y_pos, step_x, step_y};
                x_pos += step_x;
            }
            x_pos = 0;
            y_pos += step_y;
        }
        std::stringstream ss;
        int count = 0;
        auto mouse_pos = GetMousePosition();
        BeginTextureMode(DW::target_renderer);
        ClearBackground(BLANK);
            for (const auto& el: res){
                if (CheckCollisionPointRec(mouse_pos, {el.x, el.y, el.width, el.height})){
                    DrawLine(el.x, el.y, el.x + el.width, el.y, YELLOW);
                    DrawLine(el.x+1.f, el.y+1.f, el.x+1.f + el.width, el.y+1.f, YELLOW);
                    DrawLine(el.x, el.y, el.x, el.y + el.height, YELLOW);
                    DrawLine(el.x + 1.f, el.y + 1.f, el.x + 1.f, el.y + 1.f + el.height, YELLOW);

                    DrawRectangle(mouse_pos.x + add_x ,  mouse_pos.y + add_y, 100,50,  WHITE);
                    ss << " X:" << count % pwidth << " Y:" << count / pwidth << '\n';
                    auto str = ss.str();
                    DrawText(str.c_str(), mouse_pos.x + add_x , mouse_pos.y + add_y, 20.f, RED);
                }
                ++count;
            }
        EndTextureMode();
    }

    void handle(Color c, float add_x, float add_y) {
        rlBegin(RL_LINES);
        rlColor4ub(c.r, c.g, c.b, c.a);
        auto mouse_pos = GetMousePosition();
        auto snap = [](float val){return std::floor(val) + 0.5f;};
        for (const auto& el: res){
            if ((CheckCollisionPointRec({el.x, el.y}, {mouse_pos.x, mouse_pos.y, 100 + add_x, 50.f + add_y})
                              || CheckCollisionRecs({el.x, el.y, el.width, el.height},{mouse_pos.x , mouse_pos.y, 100, 50.f + add_y})
            ))
                continue;
            rlVertex2f(snap(el.x), snap(el.y));
            rlVertex2f(snap(el.x) + snap(el.width), snap(el.y));
            rlVertex2f(snap(el.x), snap(el.y));
            rlVertex2f(snap(el.x), snap(el.y) + snap(el.height));
        }
        rlEnd();
        DW::draw_render_texture();
    }
};

template <size_t width, size_t height>
struct APUChansActive: public DebugWindow<0, 0, width, height>{
    using DW = DebugWindow<0, 0, width, height>;
    APUChansActive(float x, float y):
            DW(x, y){
    }
    void update(Memory& mem) {
        Rectangle ch1rec = {0, 0, 100, 50};
        Rectangle ch2rec = {0,  55, 100, 50};
        std::stringstream ss;
        auto draw_channel_button = [&](Rectangle rec, std::string_view text, Color c) {
            DrawRectangleRec(rec,  c);
            ss << text;
            auto s = ss.str();
            DrawText(s.c_str(), rec.x, rec.y, 20.f, BLACK);
            ss.str("");
        };
        BeginTextureMode(DW::target_renderer);
            ClearBackground(BLANK);
            if (CheckCollisionPointRec(GetMousePosition(), {DW::rec.x, DW::rec.y, 100, 50}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                mem.custom_ch1_active = !mem.custom_ch1_active;

            if (CheckCollisionPointRec(GetMousePosition(), {DW::rec.x, DW::rec.y + 55, 100, 50}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                mem.custom_ch2_active = !mem.custom_ch2_active;

            draw_channel_button(ch1rec, "CH1", mem.custom_ch1_active ? GREEN : RED);
            draw_channel_button(ch2rec, "CH2", mem.custom_ch2_active ? GREEN : RED);

        EndTextureMode();
    }
    void handle() {
        DW::draw_render_texture();
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height, size_t n_tiles>
struct TileWindow: public DebugWindow<pwidth, pheight, width, height>{
    using DW = DebugWindow<pwidth, pheight,width,height>;
    unsigned short base_address{};
    std::array<Tile, n_tiles> tiles;
    TileWindow(float x, float y, unsigned short base_address):
            DebugWindow<pwidth, pheight,width,height>(x, y),
            base_address(base_address){
    }
    void update_tiles(const Memory& mem){
        std::stringstream ss;
        ss << "Tiles address:" << std::hex << base_address;
        auto str = ss.str();
        int x_begin = 0;
        int y_begin = 0;
        DW::header = ss.str();

        for (int i = 0; i < n_tiles; ++i)
            tiles[i] = DebugTools::get_tile_by_address(mem, base_address + (16 * i));

        int x = x_begin;
        int y = y_begin;
        size_t count = 0;
        int current_x = x;
        int current_y = y;
        auto prev_x = x;
        for (auto& colors: tiles) {
            for (auto&& row: colors){
                for (auto el: row) {
                    DW::buffer[y][x++] = Pixel::get_pixel_color(el);
                }
                ++y;
                prev_x = std::exchange(x, current_x);
            }
            if ((prev_x + 1 >= pwidth)){
                y = current_y = y;
                x = current_x = x_begin;
            } else {
                y = current_y;
                x = current_x = prev_x;
            }
        }
        DW::update();
    }
    void handle(const Memory& mem){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec)){
            auto mouse_wheel_move = GetMouseWheelMove();
            if (mouse_wheel_move > 0)
                base_address += (20 * 16);
            else if (mouse_wheel_move < 0)
                base_address -= (20 * 16);
        }
        update_tiles(mem);
        DW::draw(20.f);
    }
};

template <size_t width, size_t height>
struct BgTileMapWindow: public DebugWindow<256, 256, width, height>{
    using DW = DebugWindow<256, 256,width,height>;
    unsigned short base_address = constants::BG_TILE_MAP1;
    std::array<Tile, 1024> tiles;
    BgTileMapWindow(float x, float y): DW(x, y){}
    void update_tiles(const Memory& mem, unsigned char scx, unsigned char scy){
        std::stringstream ss;
        ss << "Tiles address:" << std::hex << base_address;
        auto str = ss.str();
        int x_begin = 0;
        int y_begin = 0;
        DW::header = ss.str();

        unsigned short tile_base = ((mem.read_unsafe(constants::LCDC_ADDRESS) >> 4) & 1) ? 0x8000 : 0x9000;

        size_t count = 0;
        for (int i = 0; i < 32; ++i)
            for (int j = 0; j < 32; ++j)
                 tiles[count++] = DebugTools::get_tile_by_address(mem,  tile_base == 0x8000 ?
                 tile_base + (mem.read_unsafe((base_address + (j + 32 * i))) * 16):
                 tile_base + ((char)mem.read_unsafe((base_address + (j + 32 * i))) * 16));


        unsigned char scx_value = mem.read_unsafe(constants::SCX_ADDRESS);
        if (scx_value)
            scx = scx_value;
        unsigned char scy_value = mem.read_unsafe(constants::SCY_ADDRESS);
        if (scy_value)
            scy = scx_value;

        int x = x_begin;
        int y = y_begin;
        int current_x = x;
        int current_y = y;
        auto prev_x = x;
        for (auto& colors: tiles) {
            for (auto&& row: colors){
                for (auto el: row) {
                    DW::buffer[y][x++] = (
                            (((y == scy || y == (scy + 144) % 256) &&
                                    (scx >= 95 ? (x >= scx || x <= (scx + 160) % 256) : (x >= scx && x <= (scx + 160) % 256))) ||
                                    (scx == x || x == (scx + 160) % 256) && (scy < 112 ? (y >= scy && y <= (scy + 144) % 256) : (y >= scy || y <= (scy + 144) % 256)))
                                    ) ? RED : Pixel::get_pixel_color(el);
                }
                ++y;
                prev_x = std::exchange(x, current_x);
            }
            if ((prev_x + 1 >= 256)){
                y = current_y = y;
                x = current_x = x_begin;
            } else {
                y = current_y;
                x = current_x = prev_x;
            }
        }
        DW::update();
    }
    void handle(const Memory& mem,  unsigned char scx, unsigned char scy){
        update_tiles(mem, scx, scy);
        DW::draw(20.f);
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct SpriteWindow: public DebugWindow<pwidth, pheight, width, height>{
    using DW = DebugWindow<pwidth, pheight,width,height>;
    std::array<Sprite, 40> sprites;
    std::array<Tile, 40> tiles;
    SpriteWindow(float x, float y):
            DebugWindow<pwidth, pheight,width,height>(x, y){
    }
    void update_tiles(const Memory& mem){
        std::stringstream ss;
        ss << "Tiles address:" << std::hex << constants::OAM_BEGIN;
        auto str = ss.str();
        int x_begin = 0;
        int y_begin = 0;
        DW::header = ss.str();

        unsigned sprites_counter{};
        for (int i = 0; i < 160; i+=4) {
            sprites[sprites_counter++] = (
                    Sprite{
                            .y_pos = mem.read_unsafe(constants::OAM_BEGIN + i),
                            .x_pos = static_cast<unsigned char>(mem.read_unsafe(
                                    constants::OAM_BEGIN + i + 1)),
                            .index_tile = mem.read_unsafe(constants::OAM_BEGIN + i + 2),
                            .attrib = mem.read_unsafe(constants::OAM_BEGIN + i + 3),
                    });
        }

        for (int i = 0; i < 40; ++i) tiles[i] = DebugTools::get_sprite_tile(mem, sprites[i]);

        int x = x_begin;
        int y = y_begin;
        int current_x = x;
        int current_y = y;
        auto prev_x = x;
        for (auto& colors: tiles) {
            for (auto&& row: colors){
                for (auto el: row) {
                    DW::buffer[y][x++] = Pixel::get_pixel_color(el);
                }
                ++y;
                prev_x = std::exchange(x, current_x);
            }
            if ((prev_x + 1 >= pwidth)){
                y = current_y = y;
                x = current_x = x_begin;
            } else {
                y = current_y;
                x = current_x = prev_x;
            }
        }
        DW::update();
    }

    void handle(const Memory& mem){
        update_tiles(mem);
        DW::draw();
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct SpriteWindowText: public DebugWindow<pwidth, pheight, width, height>{
    using DW = DebugWindow<pwidth, pheight,width,height>;
    SpriteWindowText(float x, float y):
            DebugWindow<pwidth, pheight,width,height>(x, y){
    }
    void handle(const auto& sprite_window){
        std::stringstream ss;
        for (int i = 0; i < 40; i += 5){
            for (int j = 0; j < 5; ++j){
                ss << "[ " <<  (i + j) << " X:" << (unsigned)sprite_window.sprites[i + j].x_pos << " Y:" << (unsigned)sprite_window.sprites[i + j].y_pos << " ]";
            }
            ss << '\n';
        }
        DW::header = ss.str();
        DW::draw_text(18.f);
    }
};


template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct RegWindow: public DebugWindow<pwidth, pheight, width, height> {
    using DW = DebugWindow<pwidth, pheight,width,height>;
    RegWindow(float x, float y): DW(x, y){}
    void handle(const Memory& mem, const CPU& cpu){
        std::stringstream ss;
        ss << std::hex << " Registers:\n" <<
           " A:" << (unsigned)cpu.regs[CPU::A] << "\n" <<
           " F:" <<  (unsigned)cpu.regs[CPU::F] << "\n" <<
           " B:" <<  (unsigned)cpu.regs[CPU::B] << "\n" <<
           " C:" <<  (unsigned)cpu.regs[CPU::C] << "\n" <<
           " D:" <<  (unsigned)cpu.regs[CPU::D] << "\n" <<
           " E:" <<  (unsigned)cpu.regs[CPU::E] << "\n" <<
           " H:" <<  (unsigned)cpu.regs[CPU::H] << "\n" <<
           " L:" <<  (unsigned)cpu.regs[CPU::L] << "\n" <<
           " SP:" <<  cpu.sp << "\n" <<
           " PC:" <<  cpu.pc << "\n" <<
           " ZeroFlag:" << (unsigned)cpu.get_flag(CPU::Zf) << '\n' <<
           " NegFlag:" << (unsigned)cpu.get_flag(CPU::Nf) << '\n' <<
           " HalfFlag:" << (unsigned)cpu.get_flag(CPU::Hf) << '\n' <<
           " CarryFlag:" << (unsigned)cpu.get_flag(CPU::Cf) << '\n' <<
           " IE: " << std::bitset<8>(mem.map[constants::IE_ADDRESS]).to_string() << '\n' <<
           " IF: " << std::bitset<8>(mem.map[constants::IF_ADDRESS]).to_string() << '\n' <<
           " STAT: " << std::bitset<8>(mem.map[constants::STAT_ADDRESS]).to_string();
        DW::header = ss.str();
        DW::draw_text(24.f);
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct PPURegistersText: public DebugWindow<pwidth, pheight, width, height> {
    using DW = DebugWindow<pwidth, pheight,width,height>;
    PPURegistersText(float x, float y): DW(x, y){}
    void handle(const Memory& mem, unsigned char scx, unsigned char scy){
        std::stringstream ss;
        ss << std::hex << " PPU REGS:\n" <<
           " SCX:" << (unsigned)scx << "\n" <<
           " SCY:" <<  (unsigned)scy << "\n" <<
           " LYC:" <<  (unsigned)mem.read_unsafe(constants::LYC_ADDRESS) << "\n" <<
           " LY:" <<  (unsigned)mem.read_unsafe(constants::LY_ADDRESS) << "\n";
        DW::header = ss.str();
        DW::draw_text(24.f);
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct APURegistersText: public DebugWindow<pwidth, pheight, width, height> {
    using DW = DebugWindow<pwidth, pheight,width,height>;
    APURegistersText(float x, float y): DW(x, y){}
    void handle(const Memory& mem){
        std::stringstream ss;
        ss << std::hex << " APU REGS:\n" <<
           " NR11:" <<  std::bitset<8>(mem.read_unsafe(constants::NR11_ADDRESS)).to_string() << "\n" <<
           " NR12:" <<  std::bitset<8>(mem.read_unsafe(constants::NR12_ADDRESS)).to_string() << "\n" <<
           " NR13:" <<  std::bitset<8>(mem.read_unsafe(constants::NR13_ADDRESS)).to_string() << "\n" <<
           " NR14:" <<  std::bitset<8>(mem.read_unsafe(constants::NR14_ADDRESS)).to_string() << "\n" <<
           " NR52:" <<  std::bitset<8>(mem.read_unsafe(constants::NR52_ADDRESS)).to_string() << "\n";
        DW::header = ss.str();
        DW::draw_text(24.f);
    }
};


template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct JoypadWindow: public DebugWindow<pwidth, pheight, width, height> {
    using DW = DebugWindow<pwidth, pheight,width,height>;
    JoypadWindow(float x, float y): DW(x, y){}
    void handle(const Memory& mem, Joypad& jp){
        std::stringstream ss;
        auto current = jp.current_pressed_values(false);
        ss << std::hex << " Register: " << std::hex << constants::JOYPAD << '\n' <<
           " Start: " << ((current.first >> 3) & 1)  << "\n" <<
           " Select: " << ((current.first >> 2) & 1)  << "\n" <<
           " B: " << ((current.first >> 1) & 1)  << "\n" <<
           " A: " << ((current.first) & 1)  << "\n";
        current = jp.current_pressed_values(true);
        ss <<
           " Down: " << ((current.first >> 3) & 1)  << "\n" <<
           " Up: " << ((current.first >> 2) & 1)  << "\n" <<
           " Left: " << ((current.first >> 1) & 1)  << "\n" <<
           " Right: " << ((current.first) & 1)  << "\n";
        DW::header = ss.str();
        DW::draw_text(24.f);
    }
};

void print_segment(unsigned short address, auto n, const Memory& mem) {
    std::cout << "\n=============================\n";
    for (int i = 0; i < n; ++i){
        std::cout << std::hex << address + i << ":" << (unsigned)mem.map[address + i] << '\n';
    }
    std::cout << "\n=============================\n";
    std::cout << std::dec;
}

void print_external_ram(unsigned short address, auto n, const Memory& mem) {
    std::cout << "\n=============================\n";
    for (int i = 0; i < n; ++i){
        std::cout << std::hex << address + i << ":" << (unsigned)mem.external_ram[address + i] << '\n';
    }
    std::cout << "\n=============================\n";
    std::cout << std::dec;
}

void print_bytes(unsigned short address, auto n, const Memory& mem) {
    std::cout << "\n=============================\n";
    for (int i = 0; i < n; ++i){
        std::cout << std::hex << "0x" << (unsigned)mem.map[address + i] << ',';
    }
    std::cout << "\n=============================\n";
    std::cout << std::dec;
}

#endif //MAIN_CPP_DEBUG_H

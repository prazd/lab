#ifndef MAIN_CPP_DEBUG_H
#define MAIN_CPP_DEBUG_H
#include <array>
#include <iostream>
#include "Memory.h"
#include "CPU.h"
#include "Joypad.h"
#include "PPU.h"
#include "rlgl.h"
#include <iomanip>
#include "Cheats.h"
#include <charconv>

using Tile8x8 = std::array<std::array<int, 8>, 8>;
using Tile8x16 = std::array<std::array<int, 8>, 16>;


struct DebugTools {
    static Tile8x8 get_sprite_tile8x8(const Memory& mem, const Sprite& sprite) {
        auto lcdc_mode = (mem.read_unsafe(constants::LCDC_ADDRESS) >> 2) & 1;
        return get_tile8x8_by_address(mem, ((lcdc_mode ? sprite.index_tile & 0xfe : sprite.index_tile) * 16) + constants::VRAM_BEGIN);
    }

    static Tile8x8 get_tile8x8_by_address(const Memory& mem, unsigned short address){
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

    static Tile8x16 get_sprite_tile16(const Memory& mem, const Sprite& sprite) {
        auto lcdc_mode = (mem.read_unsafe(constants::LCDC_ADDRESS) >> 2) & 1;
        return get_tile_by_address16(mem, ((lcdc_mode ? sprite.index_tile & 0xfe : sprite.index_tile) * 16) + constants::VRAM_BEGIN);
    }

    static Tile8x16 get_tile_by_address16(const Memory& mem, unsigned short address){
        std::array<unsigned char, 32> arr{};
        std::copy(mem.map + address, mem.map + address + 32, arr.begin());
        std::array<std::array<int, 8>, 16> colors{};
        std::array<int, 8> tmp{};
        size_t current{};
        for (size_t i = 0; i < 32; i+=2){
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
        std::vector<Tile8x8 > vec{};
        for (int i = 0; i < n_tiles; ++i)
            vec.push_back(get_tile8x8_by_address(mem, begin_address + (16 * i)));

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
    bool m_pressed = false;
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
            if (IsMouseButtonPressed(0)) {
                m_pressed = true;
            } else if (IsMouseButtonReleased(0)) {
                m_pressed = false;
            }
            if (IsMouseButtonDown(0) && m_pressed) {
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

    static void draw_button(Rectangle rec, Color c, const char* text) {
        DrawRectangle(rec.x, rec.y, rec.width, rec.height,
                      (CheckCollisionPointRec(GetMousePosition(), rec) && IsMouseButtonPressed(0)) ? c : WHITE);
        DrawRectangleLinesEx(rec, 1.f, c);
        DrawText(text, rec.x, rec.y, 12.f, BLACK);
    }

    static void add_circle(auto& ctr,auto value, size_t sz){
        if (ctr.size() == sz){
            ctr.pop_front();
        }
        ctr.push_back(value);
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
        Rectangle ch3rec = {0,  110, 100, 50};
        Rectangle ch4rec = {0,  170, 100, 50};

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
#if DEBUG_MODE
            if (CheckCollisionPointRec(GetMousePosition(), {DW::rec.x, DW::rec.y, 100, 50}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                mem.custom_ch1_active = !mem.custom_ch1_active;

            if (CheckCollisionPointRec(GetMousePosition(), {DW::rec.x, DW::rec.y + 55, 100, 50}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                mem.custom_ch2_active = !mem.custom_ch2_active;

            if (CheckCollisionPointRec(GetMousePosition(), {DW::rec.x, DW::rec.y + 110, 100, 50}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                mem.custom_ch3_active = !mem.custom_ch3_active;

             if (CheckCollisionPointRec(GetMousePosition(), {DW::rec.x, DW::rec.y + 170, 100, 50}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                 mem.custom_ch4_active = !mem.custom_ch4_active;

            draw_channel_button(ch1rec, "CH1", mem.custom_ch1_active ? GREEN : RED);
            draw_channel_button(ch2rec, "CH2", mem.custom_ch2_active ? GREEN : RED);
            draw_channel_button(ch3rec, "CH3", mem.custom_ch3_active ? GREEN : RED);
            draw_channel_button(ch4rec, "CH4", mem.custom_ch4_active ? GREEN : RED);
#endif
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
    std::array<Tile8x8, n_tiles> tiles;
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
            tiles[i] = DebugTools::get_tile8x8_by_address(mem, base_address + (16 * i));

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
struct HexViewer: public DebugWindow<0, 0, width, height> {
    using DW = DebugWindow<0, 0,width,height>;
    unsigned short base = 0x00;
    enum Speed {
        Low,
        Med,
        High,
        SpeedCount
    } current_speed = Low;
    static constexpr int speed_vals[] = {16, 128, 512};
    HexViewer(float x, float y): DW(x, y){}
    void handle(const Memory& mem){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec)){
            auto mouse_wheel_move = GetMouseWheelMove();
            if (mouse_wheel_move > 0)
                base += speed_vals[current_speed];
            else if (mouse_wheel_move < 0)
                base -= speed_vals[current_speed];

            if (IsKeyPressed(KEY_EQUAL))
                current_speed = static_cast<Speed>((current_speed + 1) % SpeedCount);
        }
        std::stringstream ss_addr;
        ss_addr << '\n';
        char ascii_buffer[16]{};
        unsigned short current = base;
        for (int i = 0; i < 28; ++i){
            ss_addr << ' ' << std::hex << std::setw(4) << std::setfill('0') << (unsigned)current << "  ";
            for (int j = 0; j < 16; ++j) {
                auto val = mem.read_unsafe(current + j);
                ascii_buffer[j] = (val >= 32 && val <= 127  ? (char)val : '.');
                ss_addr  << std::setw( 2) <<  std::setfill('0') << (unsigned)val << ' ';
            }
            current += 16;
            ss_addr << "   ";
            for (auto el: ascii_buffer)  ss_addr << el;
            ss_addr << '\n';
        }
        DW::header = ss_addr.str();
        DW::draw_text(15.f);
    }
};

template <size_t width, size_t height>
struct InstructionViewer: public DebugWindow<0, 0, width, height> {
    using DW = DebugWindow<0, 0,width,height>;
    unsigned short base_pc = 0x100;
    std::string user_input_syms;
    bool typing = false;
    Rectangle input_rec;
    Rectangle button_arg;
    Rectangle button_addr;
    std::deque<unsigned char> prev_instruction_sz{};
    InstructionViewer(float x, float y): DW(x, y),
    input_rec({x,y, width/2, height/12}),
    button_arg{x + width/2,y, 40, 40},
    button_addr{x + width/2 + 40,y, 40, 40}
    {}
    void handle(Memory& mem, CPU& cpu){
        typing = (CheckCollisionPointRec(GetMousePosition(), input_rec));
        if (typing){
            auto key = GetKeyPressed();
            if (key) {
                if (key == KEY_BACKSPACE){
                    if(!user_input_syms.empty()) {
                        user_input_syms.back() = ' ';
                        user_input_syms.pop_back();
                    }
                }
            }
            auto sym = GetCharPressed();
            if (sym) {
                user_input_syms.push_back(sym);
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), button_arg) && IsMouseButtonPressed(0)){
            std::deque<unsigned char> tmp{};
            unsigned val{};
            if (std::from_chars(user_input_syms.data(), user_input_syms.data() + user_input_syms.size(), val, 16).ec == std::errc{}) {
                bool stop = false;
                auto current_pc = base_pc;
                for (int i = base_pc; i < constants::ROM_BANK_01_END; ++i) {
                    auto decoded = CPU::decode(mem.read_unsafe(current_pc));
                    auto arg1 = mem.read_unsafe(current_pc + 1);
                    auto arg2 = mem.read_unsafe(current_pc + 2);
                    switch (decoded) {
                        case CPU::LD_0b11111010:
                        case CPU::LD_0b11101010:
                        case CPU::LD_0b00xx0001:
                        case CPU::LD_0b00001000: {
                            if ((arg1 | (arg2 << 8)) == val && base_pc != current_pc) {
                                stop = true;
                            }
                            break;
                        }
                        case CPU::LDH_0b11110000:
                        case CPU::LDH_0b11100000: {
                            if ((0xff00 | (arg1 & 0xff)) == val && base_pc != current_pc) {
                                stop = true;
                            }
                            break;
                        }
                    }
                    if (stop) {
                        prev_instruction_sz = std::move(tmp);
                        base_pc = current_pc;
                        break;
                    }
                    // todo: check
                    auto sz = cpu.instruction_sz(decoded);
                    DW::add_circle(tmp, sz, 15);
                    current_pc += sz;
                }
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), button_addr) && IsMouseButtonPressed(0)){
            unsigned val{};
            if (std::from_chars(user_input_syms.data(), user_input_syms.data() + user_input_syms.size(), val, 16).ec == std::errc{}) {
                auto current_pc = 0;
                for (int i = 0; i < constants::ROM_BANK_01_END; ++i){
                    auto sz = cpu.instruction_sz(CPU::decode(mem.read_unsafe(current_pc)));
                    prev_instruction_sz.push_back(sz);
                    current_pc+=sz;
                    if (current_pc >= val) {
                        base_pc = current_pc;
                        break;
                    }
                }
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), DW::rec)){
            auto mouse_wheel_move = GetMouseWheelMove();
            if (mouse_wheel_move > 0){
                auto sz = cpu.instruction_sz(CPU::decode(mem.read_unsafe(base_pc)));
                prev_instruction_sz.push_back(sz);
                base_pc+=sz;

            } else if (mouse_wheel_move < 0) {
                if (!prev_instruction_sz.empty()) {
                    base_pc-=prev_instruction_sz.back();
                    prev_instruction_sz.pop_back();
                }
            }
        }

        std::stringstream ss;
        std::stringstream input_ss;

        auto current_pc = base_pc;
        for (int i = 0; i < 25; ++i){
            ss << std::hex << std::setw(4) << std::setfill('0') << (unsigned)current_pc << "  ";
            Rectangle button_rec = {input_rec.x, input_rec.y + 40 + (i * 18), 20, 15};
            DW::draw_button(button_rec, GREEN, "nop");
            if (CheckCollisionPointRec(GetMousePosition(), button_rec) && IsMouseButtonPressed(0)){
                auto decoded = CPU::decode(mem.read_unsafe(current_pc));
                auto sz = cpu.instruction_sz(decoded);
                for (int j = 0; j < sz; ++j) mem.write_unsafe(current_pc + j, 0x00);
            }
            auto decoded = CPU::decode(mem.read_unsafe(current_pc));
            auto current_ins_str = cpu.instr_to_str(mem,decoded,current_pc);
            ss << current_ins_str << " ";
            current_pc += cpu.instruction_sz(decoded);
            ss << "\n";
        }
        input_ss << user_input_syms;
        input_ss << (typing ? " |" : "");
        auto input_str = input_ss.str();
        DrawRectangleLinesEx(input_rec,1.f, BLACK);


        DW::draw_button(button_arg, GREEN, "arg");
        DW::draw_button(button_addr, GREEN, "addr");


        auto str = ss.str();
        DrawText(str.c_str(), input_rec.x + 30, input_rec.y + 40, 16.f, BLACK);
        DrawText(input_str.c_str(), input_rec.x, input_rec.y, 20.f, BLACK);
    }
};

template <size_t width, size_t height>
struct BgTileMapWindow: public DebugWindow<256, 256, width, height>{
    using DW = DebugWindow<256, 256,width,height>;
    unsigned short base_address = constants::BG_TILE_MAP1;
    std::array<Tile8x8, 1024> tiles;
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
                 tiles[count++] = DebugTools::get_tile8x8_by_address(mem,  tile_base == 0x8000 ?
                 tile_base + (mem.read_unsafe((base_address + (j + 32 * i))) * 16):
                 tile_base + ((char)mem.read_unsafe((base_address + (j + 32 * i))) * 16));


        unsigned char scx_value = mem.read_unsafe(constants::SCX_ADDRESS);
        if (scx_value)
            scx = scx_value;
        unsigned char scy_value = mem.read_unsafe(constants::SCY_ADDRESS);
        if (scy_value)
            scy = scy_value;

        int x = x_begin;
        int y = y_begin;
        int current_x = x;
        int current_y = y;
        auto prev_x = x;
        for (auto& colors: tiles) {
            for (auto&& row: colors){
                for (auto el: row) {
                    DW::buffer[y][x++] = (
                            // up + down lines
                            ((y == scy || y == (scy + 144) % 256) && (scx >= 95 ? (x >= scx || x <= (scx + 160) % 256) : (x >= scx && x <= (scx + 160) % 256))) ||
                            // left + right lines
                            (scx == x || x == (scx + 160) % 256) && (scy < 112 ? (y >= scy && y <= (scy + 144) % 256) : (y >= scy || y <= (scy + 144) % 256))
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
    std::array<Tile8x8, 40> tiles8x8;
    std::array<Tile8x16, 40> tiles8x16;
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

        auto update_buffer = [x_begin, y_begin](auto& mem, auto& sprites, auto& buffer, auto& tiles, auto get_sprite_func){
            for (int i = 0; i < 40; ++i)
                tiles[i] = get_sprite_func(mem, sprites[i]);

                int x = x_begin;
                int y = y_begin;
                int current_x = x;
                int current_y = y;
                auto prev_x = x;
                for (auto& colors: tiles) {
                    for (auto&& row: colors){
                        for (auto el: row) {
                            buffer[y][x++] = Pixel::get_pixel_color(el);
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
        };

        if ((mem.read_unsafe(constants::LCDC_ADDRESS) >> 2) & 1)
            update_buffer(mem, sprites, DW::buffer, tiles8x16, DebugTools::get_sprite_tile16);
        else
            update_buffer(mem, sprites, DW::buffer, tiles8x8, DebugTools::get_sprite_tile8x8);

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
           " NR10:" <<  std::bitset<8>(mem.read_unsafe(constants::NR10_ADDRESS)).to_string() << "\n" <<
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

template <size_t width, size_t height>
class CheatsWin : public DebugWindow<0, 0, width, height> {
    using DW = DebugWindow<0, 0,width,height>;
    bool typing = false;

    std::string user_input_syms;

    Rectangle input_value_rec;
    Rectangle input_rec;
    Rectangle output_rec;
    Rectangle search_button;
    Rectangle clean_button;
    Rectangle add_inspect_button;
    Rectangle clear_inspect_button;
    Rectangle was_updated_button;
    Rectangle was_not_updated_button;

    Rectangle update_button;

    std::vector<std::pair<unsigned short, unsigned char>> search_buffer{};

    enum Mode {
        Search,
        Update,
        ModeCount
    } mode = Search;

    unsigned short selected_address = 0;
    size_t update_base = 0;

public:
    CheatsWin(float x, float y): DW(x, y),
                                 user_input_syms{' '},
                                 input_rec({x,y, width/2, height/12}),
                                 output_rec({x,y+height/12, width/2, height/12}),
                                 search_button{x+width/2,y, 50, height/12},
                                 clean_button{x+width/2 + 50,y, 50, height/12},
                                 add_inspect_button{x+width/2 + 100,y, 50, height/12},
                                 clear_inspect_button{x+width/2 + 150,y, 50, height/12},
                                 input_value_rec{x,y+40, width/4, height/12},
                                 update_button{x+width/4,y+40, 30, 30},
                                 was_updated_button{x+width/2,y+40, 30, 30},
                                 was_not_updated_button{x+width/2 + 30,y+40, 30, 30}
    {}
    void check_switch_mode() {
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec)) {
            if (IsKeyPressed(KEY_C)) mode = static_cast<Mode>((mode + 1) % ModeCount);
        }
    }
    void handle_update_mode(Memory& mem) {
        typing = (CheckCollisionPointRec(GetMousePosition(), input_value_rec));
        if (typing){
            auto key = GetKeyPressed();
            if (key) {
                if (key == KEY_BACKSPACE){
                    if(user_input_syms.size()>1) {
                        user_input_syms.back() = ' ';
                        user_input_syms.pop_back();
                    }
                }
            }
            auto sym = GetCharPressed();
            if (sym) {
                user_input_syms.push_back(sym);
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), update_button) && IsMouseButtonPressed(0)){
            unsigned val{};
            if (std::from_chars(user_input_syms.data(), user_input_syms.data() + user_input_syms.size(), val, 16).ec == std::errc{}) {
                if (val <= 0xff){
                    mem.write_unsafe(selected_address, val);
                } else if (val <= 0xffff) {
                    mem.write_unsafe(selected_address, val & 0xff);
                    mem.write_unsafe(selected_address + 1, (val >> 8) & 0xff);
                } else {
                    mem.write_unsafe(selected_address, val & 0xff);
                    mem.write_unsafe(selected_address + 1, (val >> 8) & 0xff);
                    mem.write_unsafe(selected_address + 2, (val >> 16) & 0xff);
                    mem.write_unsafe(selected_address + 3, (val >> 24) & 0xff);
                }
            }

        }
        std::stringstream frozen_addresses_ss;
        unsigned count = 0;
        for (int max = 10; auto el: CheatCore::indexes) {
            frozen_addresses_ss << std::hex << std::setw(4) << std::setfill('0') << (unsigned)el << "  " << (unsigned)mem.read_unsafe(el) << '\n';
            if (!(--max)) break;
            ++count;
        }
        for (int i = 0; i < count; ++i){
            bool frozen = CheatCore::is_frozen(CheatCore::indexes[update_base + i]);
            Rectangle freeze_button_rec = {input_rec.x + 120, input_rec.y + 90 + (i * 25), 20, 20};
            DW::draw_button(freeze_button_rec, frozen ? RED : GREEN, frozen ? "OFF" : "ON");
            Rectangle update_button_rec = {input_rec.x + 140, input_rec.y + 90 + (i * 25), 40, 20};
            DW::draw_button(update_button_rec, GREEN, "select");

            if (CheckCollisionPointRec(GetMousePosition(), freeze_button_rec) && IsMouseButtonPressed(0)){
                if (!frozen){
                    CheatCore::freeze_index(CheatCore::indexes[update_base + i]);
                } else {
                    CheatCore::unfreeze_index(CheatCore::indexes[update_base + i]);
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), update_button_rec) && IsMouseButtonPressed(0)){
                selected_address = CheatCore::indexes[update_base + i];
            }
        }

        std::stringstream input_ss;
        input_ss << user_input_syms.data();
        input_ss << ((typing) ? " |" : " ");

        auto i_str = input_ss.str();
        DrawText(i_str.c_str(), input_value_rec.x, input_value_rec.y, 20.f, BLACK);

        std::stringstream ss;
        ss << "Selected address:\n";
        ss << std::hex << std::setw(4) << std::setfill('0') << selected_address;
        auto s_str = ss.str();
        DrawText(s_str.c_str(), input_rec.x, input_rec.y, 20.f, BLACK);
        auto indexes = frozen_addresses_ss.str();
        DrawText(indexes.c_str(), input_rec.x, input_rec.y + 90, 20.f, BLACK);
        DrawRectangleLinesEx(input_value_rec,1.f, BLACK);
        DW::draw_button(update_button, GREEN, "update");
        check_switch_mode();
    }

    void handle_search_mode(Memory& mem) {
        typing = (CheckCollisionPointRec(GetMousePosition(), input_rec));
        if (typing){
            auto key = GetKeyPressed();
            if (key) {
                if (key == KEY_BACKSPACE){
                    if(user_input_syms.size()>1) {
                        user_input_syms.back() = ' ';
                        user_input_syms.pop_back();
                    }
                }
            }
            auto sym = GetCharPressed();
            if (sym) {
                user_input_syms.push_back(sym);
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), search_button) && IsMouseButtonPressed(0)){
            unsigned char val{};
            if (std::from_chars(user_input_syms.data(), user_input_syms.data() + user_input_syms.size(), val, 16).ec == std::errc{}) {
                if (search_buffer.empty()){
                    for (int i = constants::WRAM1_BEGIN + 1; i < 0xffff; ++i){
                        if (mem.read_unsafe(i) == val){
                            search_buffer.emplace_back(i, val);
                        }
                    }
                } else {
                    search_buffer.erase(std::remove_if(search_buffer.begin(), search_buffer.end(), [&mem, val](auto cur){
                        return (mem.read_unsafe(cur.first) != val);
                    }), search_buffer.end());
                }
            }
        }


        if (CheckCollisionPointRec(GetMousePosition(), was_updated_button) && IsMouseButtonPressed(0)){
            if (search_buffer.empty()){
                for (int i = constants::WRAM1_BEGIN + 1; i < 0xffff; ++i){
                    search_buffer.emplace_back(i, mem.read_unsafe(i));
                }
            } else {
                search_buffer.erase(std::remove_if(search_buffer.begin(), search_buffer.end(), [&mem](auto cur){
                    return (mem.read_unsafe(cur.first) == cur.second);
                }), search_buffer.end());
                for (auto& [el, value]: search_buffer)
                    value = mem.read_unsafe(el);
            }
        }


        if (CheckCollisionPointRec(GetMousePosition(), was_not_updated_button) && IsMouseButtonPressed(0)){
            if (search_buffer.empty()){
                for (int i = constants::WRAM1_BEGIN + 1; i < 0xffff; ++i){
                    search_buffer.emplace_back(i, mem.read_unsafe(i));
                }
            } else {
                search_buffer.erase(std::remove_if(search_buffer.begin(), search_buffer.end(), [&mem](auto cur){
                    return (mem.read_unsafe(cur.first) != cur.second);
                }), search_buffer.end());
                for (auto& [el, value]: search_buffer)
                    value = mem.read_unsafe(el);

            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), clean_button) && IsMouseButtonPressed(0)){
            search_buffer.clear();
        }

        if (CheckCollisionPointRec(GetMousePosition(), add_inspect_button) && IsMouseButtonPressed(0)){
            for (auto [index,value]: search_buffer){
                CheatCore::indexes.push_back(index);
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), clear_inspect_button) && IsMouseButtonPressed(0)){
            CheatCore::indexes.clear();
        }

        std::stringstream input_ss;
        std::stringstream result_ss;
        std::stringstream search_addresses_ss;
        std::stringstream frozen_addresses_ss;
        search_addresses_ss << "Found:\n";
        for (int max = 10; auto [el, val]: search_buffer) {
            search_addresses_ss << std::hex << std::setw(4) << std::setfill('0') << (unsigned)el << "  " << (unsigned)mem.read_unsafe(el) << '\n';
            if (!(--max)) break;
        }

        frozen_addresses_ss << "Inspect:\n";
        for (int max = 10; auto el: CheatCore::indexes) {
            frozen_addresses_ss << std::hex << std::setw(4) << std::setfill('0') << (unsigned)el << "  " << (unsigned)mem.read_unsafe(el) << '\n';
            if (!(--max)) break;
        }

        result_ss << "Result:" << search_buffer.size() <<  '\n';
        input_ss << user_input_syms.data();


        DW::draw_button(search_button, GREEN, "search");
        DW::draw_button(clean_button, GREEN, "clean");
        DW::draw_button(add_inspect_button, BLUE, "add");
        DW::draw_button(clear_inspect_button, RED, "delete");
        DW::draw_button(was_updated_button, GREEN, "updated");
        DW::draw_button(was_not_updated_button, GREEN, "same");

        DrawRectangleLinesEx(input_rec,1.f, BLACK);

        input_ss << ((typing) ? " |" : " ");
        auto i_str = input_ss.str();
        DrawText(i_str.c_str(), input_rec.x, input_rec.y, 20.f, BLACK);
        auto res_str = result_ss.str();
        DrawText(res_str.c_str(), output_rec.x, output_rec.y, 20.f, BLACK);
        auto search_addr = search_addresses_ss.str();
        DrawText(search_addr.c_str(), output_rec.x, output_rec.y + 20, 20.f, BLACK);
        auto indexes = frozen_addresses_ss.str();
        DrawText(indexes.c_str(), output_rec.x + width/2, output_rec.y + 20, 20.f, BLACK);
        check_switch_mode();
    }
    void handle(Memory& mem) {
        switch (mode) {
            case Search: {handle_search_mode(mem);break;}
            case Update: {handle_update_mode(mem);break;}
        }
    }
};


#if DEBUG_MODE
size_t ALLOCS_COUNT = 0;
void* operator new(size_t sz){
    ++ALLOCS_COUNT;
    return malloc(sz);
}

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct GeneralInfo: public DebugWindow<pwidth, pheight, width, height> {
    using DW = DebugWindow<pwidth, pheight,width,height>;
    GeneralInfo(float x, float y): DW(x, y){}
    void handle(const Memory& mem, unsigned long long internal_frame_ms, unsigned long long main_loop_frame_ms){
        std::stringstream ss;
        ss << std::hex << " Allocs: " << std::dec << ALLOCS_COUNT << '\n'
        << std::hex << " Frame ms: " << std::dec << internal_frame_ms << '\n'
        << std::hex << " Main loop ms: " << std::dec << main_loop_frame_ms << '\n';
        DW::header = ss.str();
        DW::draw_text(24.f);
    }
};
#endif


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

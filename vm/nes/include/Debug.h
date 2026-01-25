#ifndef NES_DEBUG_H
#define NES_DEBUG_H
#include <sstream>
#include "raylib.h"
#include "PPU.hpp"
#include <iomanip>

template <size_t x, size_t y, size_t w, size_t h>
struct DebugWindow {
    Rectangle rec;
    bool in_motion = false;
    DebugWindow(): rec{x,y,w,h}{}

    void move() {
        if (IsMouseButtonReleased(0)) in_motion = false;
        auto mouse_pos = GetMousePosition();
        if (CheckCollisionPointRec(mouse_pos, rec)){
            if (IsMouseButtonPressed(0) && !in_motion) in_motion = true;
            if (in_motion){
                rec.x = mouse_pos.x - rec.width/2;
                rec.y = mouse_pos.y - rec.height/2;
            }
        }
    }

    template<size_t n>
    static void move_recs(std::array<Rectangle*, n> recs) {
        auto mouse_pos = GetMousePosition();
        for (auto r: recs) {
            if (CheckCollisionPointRec(mouse_pos, *r) && IsMouseButtonDown(0)){
                r->x = mouse_pos.x - r->width/2;
                r->y = mouse_pos.y - r->height/2;
            }
        }
    }

    void draw_win(){
        DrawRectangleLinesEx(rec, 1.f, BLACK);
    }

    static void draw_text_button(Rectangle rec, const char* text, Color c, float font_sz = 10.1f){
        DrawRectangleLinesEx(rec, 1.f, BLACK);
        DrawText(text, rec.x, rec.y, font_sz, c);
    }

    static void draw_button(Rectangle rec, Color c){
        DrawRectangle(rec.x, rec.y, rec.width, rec.height, c);
    }

};

template<size_t x, size_t y, size_t w, size_t h, size_t t_w, size_t t_h>
struct DebugWindowWithTexture : DebugWindow<x,y,w,h> {
    using DW = DebugWindow<x,y,w,h>;
    std::array<Color, t_w * t_h> buffer{};
    Rectangle texture_rec;
    Texture2D target;
    DebugWindowWithTexture(): texture_rec{x, y, w, h} {
        Image img = GenImageColor(t_w, t_h, YELLOW);
        target = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    void handle() {
        DW::template move_recs<2>({&(DW::rec), &texture_rec});

        UpdateTexture(target, buffer.data());
        DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{texture_rec.x, texture_rec.y, w, h}, {0, 0}, 0.0f, WHITE);
        DW::draw_win();
    }
};

template<size_t x, size_t y, size_t w, size_t h>
struct DebugWindowText: DebugWindow<x,y,w,h>{
    using DW = DebugWindow<x,y,w,h>;
    void draw(std::string_view str, float font_sz = 10.1f, Color color = BLACK) {
        DrawText(str.data(), DW::rec.x, DW::rec.y, font_sz, color);
        DW::draw_win();
    }
};

template <size_t x, size_t y, size_t w, size_t h, size_t t_w, size_t t_h>
struct TilesView: DebugWindowWithTexture<x, y, w, h, t_w + (t_w/8), t_h + (t_h/8)> {
    using DW = DebugWindowWithTexture<x, y, w, h, t_w + (t_w/8), t_h + (t_h/8)>;
    using TileRow = std::array<Color, 8>;

    short base_index = 0;
    short current_index = 0;

    size_t tiles_w{};
    size_t tiles_h{};

    TileRow get_tile_row(unsigned char a, unsigned char b) {
        std::array<Color, 8> res{};
        for (int i = 0; i < 8; ++i){
            res[i] = color_by_id((((a >> (7-i)) & 1) << 1) | ((b >> (7-i)) & 1));
        }
        return res;
    }

    TilesView(size_t tiles_w, size_t tiles_h):
    tiles_w(tiles_w),
    tiles_h(tiles_h){}

    void handle(PPU& ppu){
      if (CheckCollisionPointRec(GetMousePosition(), DW::rec)){
          if (auto m = GetMouseWheelMove(); m != 0.f){
              if (m > 0.f){
                  base_index = (base_index + (t_w/8 * 16)) % 8192;
              } else if (m < 0.f) {
                  base_index = (base_index - (t_w/8 * 16) % 8192);
                  if (base_index < 0) base_index = 8192+base_index;
              }
          }

          if (IsKeyPressed(KEY_DOWN)){
              base_index = (base_index + (t_w/8 * 16)) % 8192;
          }
          if (IsKeyPressed(KEY_UP))  {
              base_index = (base_index - (t_w/8 * 16) % 8192);
              if (base_index < 0) base_index = 8192+base_index;
          }
      }

        current_index = base_index;
        size_t current = 0;
        for (int s = 0; s < tiles_h; s+=8){
            auto it = (ppu.mem.map + current_index);
            for (int z = 0; z < 8; ++z) {
                for (int i = 0; i < t_w/8; ++i){
                    auto l_tile = *(it + i * 16);
                    auto h_tile = *(it + i * 16 + 8);
                    auto tile_row = get_tile_row(h_tile, l_tile);
                    for (int k = 0; k < 8; ++k){
                        DW::buffer[current++] = tile_row[k];
                    }
                    DW::buffer[current++] = RED;
                }
                ++it;
            }
            for (int i = 0; i < t_w + (t_w/8); i++){
                DW::buffer[current++] = RED;
            }
            current_index = (current_index + (t_w/8 * 16)) % 8192;
        }

        DW::handle();
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct UserInput {
    std::string s;
    Rectangle rec;
    std::stringstream ss;
    UserInput(): rec{x, y, w, h} {}
    void push_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        auto c = GetCharPressed();
        if (!c) return;
        s.push_back((char)c);
    }
    void pop_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        if (IsKeyPressed(KEY_BACKSPACE) && !s.empty())
            s.pop_back();
    }
    void draw() {
        push_handler();
        pop_handler();
        DrawRectangleLinesEx({rec.x, rec.y, rec.width, rec.height}, 1.f, BLACK);
        ss << std::hex << s;
        if (CheckCollisionPointRec(GetMousePosition(), rec))
            ss << " |";
        auto r_str = ss.str();
        DrawText(r_str.data(), rec.x, rec.y, 15.f, BLACK);
        ss.str("");
    }
};

template<size_t x, size_t y, size_t w, size_t h>
struct Disassembler: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    bool next = false;
    bool selected_move_active = false;
    unsigned short selected_line_pc = 0;
    unsigned short base{};
    unsigned short pc{};
    std::stringstream ss;

    Rectangle line_button;
    UserInput<x, y - 30, w, 30> u_input;

    Rectangle next_button;
    Rectangle select_jump_button;
    Rectangle input_address_jump_button;

    Disassembler(CPU& cpu, Bus& bus): base(cpu.pc),
                                      line_button{DW::rec.x - 20, DW::rec.y, 10, 10},
                                      next_button{DW::rec.x, DW::rec.y + h, 40, 40},
                                      select_jump_button{DW::rec.x + 40, DW::rec.y + h, 40, 40},
                                      input_address_jump_button{DW::rec.x + 40 + 40, DW::rec.y + h, 40, 40}
                                      {}

    void handle(CPU& cpu) {
        base = cpu.pc;
        next = (CheckCollisionPointRec(GetMousePosition(), next_button) && IsMouseButtonPressed(0));
    }

    void draw(CPU& cpu, Bus& bus) {
        line_button = {DW::rec.x - 20, DW::rec.y, 10, 10};
        next_button = {DW::rec.x, DW::rec.y + h, 40, 40};
        select_jump_button = {DW::rec.x + 40, DW::rec.y + h, 40, 40};
        input_address_jump_button = {DW::rec.x + 40 + 40, DW::rec.y + h, 40, 40};
        u_input.rec = {DW::rec.x, DW::rec.y-30, w, 30};

        pc = base;
        if (CheckCollisionPointRec(GetMousePosition(), select_jump_button) && IsMouseButtonPressed(0)){
            selected_move_active = true;
        }

        if (CheckCollisionPointRec(GetMousePosition(), input_address_jump_button) && IsMouseButtonPressed(0)) {
            auto val = std::stoul(u_input.s, nullptr, 16);
            selected_line_pc = (unsigned short)val;
            selected_move_active = true;
        }

        for (int i = 0; i < 10; ++i){
            auto r = Rectangle{line_button.x, line_button.y + i * (line_button.height + 25.f), line_button.width, line_button.height};
            if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(0)){
                selected_line_pc = pc;
            }
            DW::draw_button(r,  selected_line_pc == pc ? RED: GREEN);
            if (pc == cpu.pc) ss << ">>";
            auto asm_info = cpu.disassemble(bus, pc);
            ss << std::hex << "  "  << pc << " " << asm_info.mnemonic << " ";
            asm_info.is_accumulator ? (ss << "A") : (asm_info.no_operand ? ss : (ss << asm_info.operand));
            ss << "\n\n";
            pc += cpu.decode(cpu.mem.read(bus, pc)).bytes;
        }
        DW::move();
        auto str = ss.str();
        DW::draw(str.data(), 15.f);
        DW::draw_text_button(next_button, " next", GREEN, 15.f);
        DW::draw_text_button(select_jump_button, " jmp", GREEN, 15.f);
        DW::draw_text_button(input_address_jump_button, " tjmp", GREEN, 15.f);
        u_input.draw();
        ss.str("");
    }
};

template<size_t x, size_t y, size_t w, size_t h>
struct RegsInfo: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::stringstream ss;
    void handle(CPU& cpu, PPU& ppu) {
        ss  << std::dec << " Cycles count:" << cpu.cycles_count << '\n'
        << std::hex << " A:" << (unsigned)cpu.a << '\n'
        << " X:" << (unsigned)cpu.x << '\n'
        << " Y:" << (unsigned)cpu.y << '\n'
        << " PC:" << cpu.pc << '\n'
        << " SP:" << (unsigned )cpu.sp << '\n'
        << "  Flags:" << (unsigned)(cpu.sr.reg & 0xcf) << '\n'
        << "  Cf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::C)) << '\n'
        << "  Zf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::Z)) << '\n'
        << "  If:" << (unsigned)(cpu.sr.get(CPU::StatusReg::I)) << '\n'
        << "  Df:" << (unsigned)(cpu.sr.get(CPU::StatusReg::D)) << '\n'
        << "  Vf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::V)) << '\n'
        << "  Nf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::N)) << '\n';
        auto s = ss.str();
        DW::draw(s.c_str(), 15.f);
        ss.str("");
        DW::move();
    }
};

#endif //NES_DEBUG_H

#ifndef NES_DEBUG_H
#define NES_DEBUG_H
#include <sstream>
#include <iomanip>
#include "raylib.h"
#include "PPU.hpp"
#include "Cartridge.h"

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

    using TileRow = std::array<Color, 8>;
    static TileRow get_tile_row(unsigned char a, unsigned char b) {
        std::array<Color, 8> res{};
        for (int i = 0; i < 8; ++i){
            res[i] = color_by_id((((a >> (7-i)) & 1) << 1) | ((b >> (7-i)) & 1));
        }
        return res;
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
struct PatternTableView: DebugWindowWithTexture<x, y, w, h, t_w + (t_w / 8), t_h + (t_h / 8)> {
    using DW = DebugWindowWithTexture<x, y, w, h, t_w + (t_w/8), t_h + (t_h/8)>;

    short base_index = 0;
    short current_index = 0;

    size_t tiles_w{};
    size_t tiles_h{};

    PatternTableView(size_t tiles_w, size_t tiles_h):
    tiles_w(tiles_w),
    tiles_h(tiles_h){}

    void handle(PPU& ppu, Bus& bus){
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
        if (!bus.mapper.chr.empty()) {
            for (int s = 0; s < tiles_h; s+=8){
                auto it =  (bus.mapper.chr.data() + current_index); // todo: check
                for (int z = 0; z < 8; ++z) {
                    for (int i = 0; i < t_w/8; ++i){
                        auto l_tile = *(it + i * 16);
                        auto h_tile = *(it + i * 16 + 8);
                        auto tile_row = DW::get_tile_row(h_tile, l_tile);
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
        }
        DW::handle();
    }
};


template <size_t x, size_t y, size_t w, size_t h, size_t t_w = (256 * 2) + 1, size_t t_h = (240 * 2) + 1>
struct NameTableView: DebugWindowWithTexture<x, y, w, h, t_w, t_h> {
    using DW = DebugWindowWithTexture<x, y, w, h, t_w, t_h>;
    using TileRow = std::array<Color, 8>;
    enum PatternTable{
        PT0,
        PT1,
        PatternTableCount
    } current_pt = PT0;

    short base_pt_address{};

    enum ViewMode {
        ColorMode,
        TBPPMode,
        ViewModeCount,
    } view_mode = TBPPMode;

    TileRow get_tile_row(unsigned char a, unsigned char b) {
        std::array<Color, 8> res{};
        for (int i = 0; i < 8; ++i){
            res[i] = color_by_id((((a >> (7-i)) & 1) << 1) | ((b >> (7-i)) & 1));
        }
        return res;
    }

    void draw_color(Bus& bus, PPU& ppu){
        size_t current = 0;
        size_t current_nt = 0;
        for (int s = 0; s < 61; ++s){
            if (s == 30){
                for (int i = 0; i < 64; ++i) {
                    for (int k = 0; k < 8; ++k){
                        DW::buffer[current++] = RED;
                    }
                }
                current_nt += 2;
            } else {
                for (int z = 0; z < 8; ++z) {
                    for (int i = 0; i < 64; ++i){
                        auto nt =  ppu.get_nt_by_mirror_mode(bus.mapper.current_screen_mirroring, current_nt);
                        auto x_tile_n = (i % 32);
                        auto y_tile_n = (s > 30 ? ((s-1) % 30) : s % 30);
                        auto pt_addr = ((base_pt_address + (16 * *(ppu.mem.map + nt + (y_tile_n * 32) + x_tile_n))));
                        auto l_tile = bus.read_chr(pt_addr + z);
                        auto h_tile = bus.read_chr(pt_addr + z + 8);
                        auto val = ppu.mem.read(bus,   nt + 0x3c0 + (y_tile_n/4 * 8) + x_tile_n/4);
                        auto shift_y = (y_tile_n/2) % 2;
                        auto shift_x = (x_tile_n/2) % 2;
                        auto attrib_value = (val >> (2 * ((shift_y * 2) + shift_x))) & 0x3;
                        for (int k = 0; k < 8; ++k){
                            auto res = ((((h_tile >> 7) & 1) << 1) | (l_tile >> 7) & 1);
                            h_tile <<= 1;
                            l_tile <<= 1;
                            auto attrib_high_bit = (attrib_value >> 1) & 1;
                            auto attrib_low_bit = attrib_value & 1;
                            unsigned char index = (((attrib_high_bit << 1) | attrib_low_bit) << 2) | res;
                            auto pixel_index = ppu.mem.read(bus, res == 0 ? constants::IMAGE_PALETTE_BEGIN :  constants::IMAGE_PALETTE_BEGIN + index);
                            DW::buffer[current++] = ppu.palette[pixel_index];
                        }
                        if (i == 32) {
                            DW::buffer[current++] = RED;
                            ++current_nt;
                        }
                    }
                    --current_nt;
                }
            }
        }
    }

    void draw_tbpp(Bus& bus, PPU& ppu){
        size_t current = 0;
        size_t current_nt = 0;
        for (int s = 0; s < 61; ++s){
            if (s == 30){
                for (int i = 0; i < 64; ++i) {
                    for (int k = 0; k < 8; ++k){
                        DW::buffer[current++] = RED;
                    }
                }
                current_nt += 2;
            } else {
                for (int z = 0; z < 8; ++z) {
                    for (int i = 0; i < 64; ++i){
                        auto pt_addr = ((base_pt_address + (16 *
                                                            *(ppu.mem.map + (ppu.get_nt_by_mirror_mode(bus.mapper.current_screen_mirroring, current_nt) +
                                                                             ((s > 30 ? ((s-1) % 30) : s % 30) * 32) + (i % 32))))));
                        auto l_tile = bus.read_chr(pt_addr + z);
                        auto h_tile = bus.read_chr(pt_addr + z + 8);
                        auto tile_row = get_tile_row(h_tile, l_tile);
                        for (int k = 0; k < 8; ++k){
                            DW::buffer[current++] = tile_row[k];
                        }
                        if (i == 32) {
                            DW::buffer[current++] = RED;
                            ++current_nt;
                        }
                    }
                    --current_nt;
                }
            }
        }
    }

    void handle(PPU& ppu, Bus& bus){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_ONE))
            view_mode = static_cast<ViewMode>((view_mode + 1) % ViewModeCount);
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_TWO))
            current_pt = static_cast<PatternTable>((current_pt + 1) % PatternTableCount);
        base_pt_address = current_pt == PT0 ? constants::PATTERN_TABLE0_BEGIN : constants::PATTERN_TABLE1_BEGIN;
        if (!bus.mapper.chr.empty()){
            if (view_mode == ColorMode)
                draw_color(bus, ppu);
            else
                draw_tbpp(bus, ppu);
        }
        DW::handle();
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct NameTableNumView: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::stringstream ss;
    bool ascii_print = false;
    // 32 x 30
    short base_nt_address = constants::NAME_TABLE0_BEGIN;
    void handle(PPU& ppu){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_ONE))
            ascii_print = !ascii_print;
        ss << std::hex;
        for (int s = 0; s < 30; ++s){
            for (int i = 0; i < 32; ++i){
                auto v = *(ppu.mem.map + (base_nt_address + (s * 32) + i));
                if (ascii_print && v != '\0') ss << v; else ss << (unsigned)v;
                ss << " ";
            }
            ss << '\n';
        }
        auto s = ss.str();
        DW::move();
        DW::draw(s.c_str());
        ss.str("");
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

    bool pressed_mode = true;
    bool should_sync = false;
    bool trigger_search = false;

    enum RunMode {
        CPU_M, PPU_M, RunModeCount
    } run_mode = CPU_M;

    bool next = false;
    bool selected_move_active = false;
    unsigned short selected_line_pc = 0;
    unsigned short base{};
    unsigned short pc{};
    std::stringstream ss;

    bool is_frame_jump_active = false;
    bool is_tile_line_jump_active = false;
    bool is_line_jump_active = false;
    bool is_one_tick_ppu_active = false;

    Rectangle line_button;
    UserInput<x, y - 30, w, 30> u_input;

    Rectangle button1; // next
    Rectangle button2; //
    Rectangle button3;
    Rectangle button4;
    Rectangle button5;
    Rectangle button6;
    Rectangle button7;
    Rectangle button8;
    Rectangle button9;
    Rectangle button10;
    Rectangle button11;
    Rectangle button12;

    Rectangle sync_button;
    Rectangle change_mode_button;

    Disassembler(CPU& cpu, Bus& bus): base(cpu.pc),
                                      line_button{DW::rec.x - 20, DW::rec.y, 10, 10},
                                      button1{DW::rec.x, DW::rec.y + h, 40, 40},
                                      button2{DW::rec.x + 40, DW::rec.y + h, 40, 40},
                                      button3{DW::rec.x + 40 * 2, DW::rec.y + h, 40, 40},
                                      button4{DW::rec.x + 40 * 3, DW::rec.y + h, 40, 40},
                                      button5{DW::rec.x + 40 * 4, DW::rec.y + h, 40, 40},
                                      button6{DW::rec.x + 40 * 5, DW::rec.y + h, 40, 40},
                                      button7{DW::rec.x + 40 * 6, DW::rec.y + h, 40, 40},
                                      button8{DW::rec.x + 40 * 7, DW::rec.y + h, 40, 40},
                                      sync_button{DW::rec.x, DW::rec.y + h + 40, 40, 40},
                                      change_mode_button{DW::rec.x + 40, DW::rec.y + h + 40, 40, 40},
                                      button9{DW::rec.x + 40*2, DW::rec.y + h + 40, 40, 40},
                                      button10{DW::rec.x + 40*3, DW::rec.y + h + 40, 40, 40},
                                      button11{DW::rec.x + 40*4, DW::rec.y + h + 40, 40, 40},
                                      button12{DW::rec.x + 40*5, DW::rec.y + h + 40, 40, 40}
                                      {}

    void handle(CPU& cpu) {
        base = cpu.pc;
        next = (CheckCollisionPointRec(GetMousePosition(), button1) && IsMouseButtonPressed(0));
    }

    void draw(CPU& cpu, PPU& ppu, Bus& bus) {
        line_button = {DW::rec.x - 20, DW::rec.y, 10, 10};
        button1 = {DW::rec.x, DW::rec.y + h, 40, 40};
        button2 = {DW::rec.x + 40, DW::rec.y + h, 40, 40};
        button3 = {DW::rec.x + 40 * 2, DW::rec.y + h, 40, 40};
        button4 = {DW::rec.x + 40 * 3, DW::rec.y + h, 40, 40};
        button5 = {DW::rec.x + 40 * 4, DW::rec.y + h, 40, 40};
        button6 = {DW::rec.x + 40 * 5, DW::rec.y + h, 40, 40};
        button7 = {DW::rec.x + 40 * 6, DW::rec.y + h, 40, 40};
        button8 = {DW::rec.x + 40 * 7, DW::rec.y + h, 40, 40};
        button9 = {DW::rec.x + 40*2, DW::rec.y + h + 40, 40, 40};
        button10 = {DW::rec.x + 40*3, DW::rec.y + h + 40, 40, 40};
        button11 = {DW::rec.x + 40*4, DW::rec.y + h + 40, 40, 40};
        button12 = {DW::rec.x + 40*5, DW::rec.y + h + 40, 40, 40};
        u_input.rec = {DW::rec.x, DW::rec.y-30, w, 30};

        change_mode_button = {DW::rec.x + 40, DW::rec.y + h + 40, 40, 40};
        sync_button = {DW::rec.x, DW::rec.y + h + 40, 40, 40};

        pc = base;

        auto mouse_button_pred = [](bool pressed_mode){
            return pressed_mode ?  IsMouseButtonPressed(0) :  IsMouseButtonDown(0);
        };

        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_M))
            pressed_mode = !pressed_mode;
        else if (CheckCollisionPointRec(GetMousePosition(), button2) && run_mode == CPU_M && mouse_button_pred(pressed_mode)){
            selected_move_active = true;
        } else if (CheckCollisionPointRec(GetMousePosition(), button3)  && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            auto val = std::stoul(u_input.s, nullptr, 16);
            selected_line_pc = (unsigned short)val;
            selected_move_active = true;
        } else if (CheckCollisionPointRec(GetMousePosition(), button4) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            next = true;
            auto val = std::stoul(u_input.s, nullptr, 16);
            selected_line_pc = (unsigned short) val;
            selected_move_active = true;
        // trigger search
        } else if (CheckCollisionPointRec(GetMousePosition(), button5) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            cpu.mem.write_trigger_addresses.push_back(std::stoul(u_input.s, nullptr, 16));
        }   else if (CheckCollisionPointRec(GetMousePosition(), button6) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            cpu.mem.read_trigger_addresses.push_back(std::stoul(u_input.s, nullptr, 16));
        }   else if (CheckCollisionPointRec(GetMousePosition(), button7) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            cpu.mem.write_trigger_addresses.clear();
            cpu.mem.read_trigger_addresses.clear();
        } else if (CheckCollisionPointRec(GetMousePosition(), button8) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            trigger_search = !trigger_search;
            cpu.mem.triggered_on_write = !trigger_search;
            cpu.mem.triggered_on_read = !trigger_search;
            ppu.fill_white_background = !trigger_search;
        } else if (CheckCollisionPointRec(GetMousePosition(), button1) && run_mode == PPU_M && mouse_button_pred(pressed_mode))
            is_frame_jump_active = true;
        else if (CheckCollisionPointRec(GetMousePosition(), button2) && run_mode == PPU_M && mouse_button_pred(pressed_mode))
            is_tile_line_jump_active = true;
        else if (CheckCollisionPointRec(GetMousePosition(), button3) && run_mode == PPU_M && mouse_button_pred(pressed_mode))
            is_line_jump_active = true;
        else if (CheckCollisionPointRec(GetMousePosition(), button4) && run_mode == PPU_M && mouse_button_pred(pressed_mode))
            is_one_tick_ppu_active = true;
        else if (CheckCollisionPointRec(GetMousePosition(), button5) && run_mode == PPU_M && IsMouseButtonPressed(0)){
            ppu.run = !ppu.run;
            ppu.fill_white_background = !ppu.run;
        }
        // trigger search
        else if (CheckCollisionPointRec(GetMousePosition(), button9) && run_mode == PPU_M  && IsMouseButtonPressed(0)) {
            ppu.mem.write_trigger_addresses.push_back(std::stoul(u_input.s, nullptr, 16));
        }   else if (CheckCollisionPointRec(GetMousePosition(), button10) && run_mode == PPU_M && IsMouseButtonPressed(0)) {
            ppu.mem.read_trigger_addresses.push_back(std::stoul(u_input.s, nullptr, 16));
        }   else if (CheckCollisionPointRec(GetMousePosition(), button11) && run_mode == PPU_M && IsMouseButtonPressed(0)) {
            ppu.mem.write_trigger_addresses.clear();
            ppu.mem.read_trigger_addresses.clear();
        } else if (CheckCollisionPointRec(GetMousePosition(), button12) && run_mode == PPU_M && IsMouseButtonPressed(0)) {
            trigger_search = !trigger_search;
            ppu.mem.triggered_on_write = !trigger_search;
            ppu.mem.triggered_on_read = !trigger_search;
            ppu.fill_white_background = !trigger_search;
        }
        else if (CheckCollisionPointRec(GetMousePosition(), sync_button) && IsMouseButtonPressed(0))
            should_sync = true;
        else if (CheckCollisionPointRec(GetMousePosition(), change_mode_button) && IsMouseButtonPressed(0))
            run_mode = static_cast<RunMode>((run_mode + 1) % RunModeCount);

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
            if (asm_info.show_address){
                ss << " [" << asm_info.arg_addr <<"] ";
            }
            ss << "\n\n";
            pc += cpu.decode(cpu.mem.read(bus, pc)).bytes;
        }
        ss << "RunMode:";
        if (run_mode == CPU_M)
            ss << "CPU";
        else
            ss << "PPU";
        ss << ' ' << (pressed_mode ? "PressedMode" : "DownMode");

        const auto& write_trigger_address = run_mode == CPU_M ?  cpu.mem.write_trigger_addresses :  ppu.mem.write_trigger_addresses;
        const auto& read_trigger_address =  run_mode == CPU_M ?  cpu.mem.read_trigger_addresses  :  ppu.mem.read_trigger_addresses;
        ss << "\nWriteTrigger:";
        for (auto el: write_trigger_address)
            ss << std::hex << el << ',';
        ss << "\nReadTrigger:";
        for (auto el: read_trigger_address)
            ss << std::hex << el << ',';

        DW::move();
        auto str = ss.str();
        DW::draw(str.data(), 15.f);
        if (run_mode == CPU_M) {
            DW::draw_text_button(button1, " next", GREEN, 15.f);
            DW::draw_text_button(button2, " jmp", GREEN, 15.f);
            DW::draw_text_button(button3, " tjmp", GREEN, 15.f);
            DW::draw_text_button(button4, " ntjmp", GREEN, 15.f);

            DW::draw_text_button(button5, " wadd", GREEN, 15.f);
            DW::draw_text_button(button6, " radd", GREEN, 15.f);
            DW::draw_text_button(button7, " clear", GREEN, 15.f);
            DW::draw_text_button(button8, " trun", GREEN, 15.f);

        } else {
            DW::draw_text_button(button1, "frame", GREEN, 15.f);
            DW::draw_text_button(button2, " tile", GREEN, 15.f);
            DW::draw_text_button(button3, " line", GREEN, 15.f);
            DW::draw_text_button(button4, " dot", GREEN, 15.f);
            DW::draw_text_button(button5, " run", GREEN, 15.f);

            DW::draw_text_button(button9, " wadd", GREEN, 15.f);
            DW::draw_text_button(button10, " radd", GREEN, 15.f);
            DW::draw_text_button(button11, " clear", GREEN, 15.f);
            DW::draw_text_button(button12, " trun", GREEN, 15.f);
        }
        DW::draw_text_button(sync_button, " sync", GREEN, 15.f);
        DW::draw_text_button(change_mode_button, " mode", GREEN, 15.f);
        u_input.draw();
        ss.str("");
    }
};

template<size_t x, size_t y, size_t w, size_t h>
struct RegsInfo: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    UserInput<x, y - 30, w, 30> u_input;
    Rectangle button_rec;
    enum RegNum {
        A, CPU_X, Y, PC, SP, Flags,
        V, PPU_X, T, W,  RegsNumCount
    };
    std::array<float, RegsNumCount> set_button_coords_offset = {
            55.0f,
            70.0f,
            85.0f,
            105.0f,
            120.0f,
            135.0f,
            225.0f,
            290.0f,
            305.0f,
            320.0f,
    };
    Rectangle mirroring_switch_button = {DW::rec.x-30, DW::rec.y+DW::rec.height-30, 30, 30};
    Rectangle sprite_off_button = {DW::rec.x-30, DW::rec.y+DW::rec.height, 30, 30};
    std::stringstream ss;
    void update_reg(CPU& cpu, PPU& ppu, RegNum rn, unsigned short value){
        switch (rn) {
            case A:     {cpu.a = value & 0xff; break;}
            case CPU_X: {cpu.x = value & 0xff; break;}
            case Y:     {cpu.y = value & 0xff; break;}
            case PC:    {cpu.pc = value; break;}
            case SP:    {cpu.sp = value; break;}
            case V:     {ppu.v = value; break;}
            case PPU_X: {ppu.x = value & 0x7; break;}
            case T:     {ppu.t = value; break;}
            case W:     {ppu.w = (value); break;}
        }
    }
    void handle(CPU& cpu, PPU& ppu, Bus& bus) {
        u_input.rec = {DW::rec.x, DW::rec.y-30, w, 30};
        mirroring_switch_button = {DW::rec.x-30, DW::rec.y+DW::rec.height-30, 30, 30};
        sprite_off_button = {DW::rec.x-30, DW::rec.y+DW::rec.height, 30, 30};
        if (CheckCollisionPointRec(GetMousePosition(), mirroring_switch_button) && IsMouseButtonPressed(0))
            bus.mapper.current_screen_mirroring =
                    static_cast<Mapper::ScreenMirroring>((bus.mapper.current_screen_mirroring + 1) % Mapper::ScreenMirroring::MirroringCount);
        if (CheckCollisionPointRec(GetMousePosition(), sprite_off_button) && IsMouseButtonPressed(0))
            ppu.is_sprite_off = !ppu.is_sprite_off;


        ss  << std::dec << " Cycles count:" << cpu.cycles_count << '\n'
        << " CPU:\n\n"
        << std::hex << " A:" << (unsigned)cpu.a << '\n'
        << " X:" << (unsigned)cpu.x << '\n'
        << " Y:" << (unsigned)cpu.y << '\n'
        << " PC:" << cpu.pc << '\n'
        << " SP:" << (unsigned )cpu.sp << '\n'
        << "  Flags:" << (unsigned)(cpu.sr.reg & 0xcf) << '\n'
        << "  Cf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::C)) << ' '
        << "  Zf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::Z)) << ' '
        << "  If:" << (unsigned)(cpu.sr.get(CPU::StatusReg::I)) << '\n'
        << "  Df:" << (unsigned)(cpu.sr.get(CPU::StatusReg::D)) << ' '
        << "  Vf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::V)) << ' '
        << "  Nf:" << (unsigned)(cpu.sr.get(CPU::StatusReg::N)) << ' '
        << " \n PPU:\n\n"
        << "  V:" << (unsigned)(ppu.v) << '\n'
        << " Coarse(X):" << (unsigned)(ppu.v & 0x1f) << " Fine(X):" <<  (unsigned)(ppu.x_scroll) << '\n'
        << " Coarse(Y):" << (unsigned)((ppu.v >> 5) & 0x1f) << " Fine(Y):" <<  (unsigned)((ppu.v >> 12) & 0x7) << '\n'
        << "  NT:" << (unsigned)((ppu.v >> 10) & 0x3) << '\n'
        << "  X:" << (unsigned)(ppu.x) << '\n'
        << "  T:" << (unsigned)(ppu.t) << '\n'
        << "  W:" << (unsigned)(ppu.w) << '\n'
        << "  VBLank:" << (unsigned)(ppu.ppustatus.vblank_flag) << '\n'
        << "  Bus VBLank:" << (unsigned)(bus.ppustatus.reg & 0x80) << '\n'
        << std::dec << "  Cycles:" << ppu.current_scanline_cycle
        << "  Scanline:" << ppu.current_y;
        ss << "\nScreenMirroring:";
        switch (bus.mapper.current_screen_mirroring) {
            case Mapper::ScreenMirroring::Horizontal: ss << "Horizontal"; break;
            case Mapper::ScreenMirroring::Vertical: ss << "Vertical"; break;
            case Mapper::ScreenMirroring::SingleScreenLower: ss << "SingleScreenLower"; break;
            case Mapper::ScreenMirroring::SingleScreenUpper: ss << "SingleScreenUpper"; break;
        }
        for (int i = 0; i < RegsNumCount; ++i){
            auto current_button_rec = Rectangle{DW::rec.x-20,DW::rec.y+set_button_coords_offset[i], 20, 10};
            DW::draw_text_button(current_button_rec, "set", BLACK);
            if (CheckCollisionPointRec(GetMousePosition(), current_button_rec) && IsMouseButtonPressed(0) && !u_input.s.empty())
                update_reg(cpu, ppu, static_cast<RegNum>(i), std::stoul(u_input.s, nullptr, 16));
        }
        auto s = ss.str();
        DW::draw(s.c_str(), 15.f);
        DW::draw_text_button(mirroring_switch_button, "MS", BLACK);
        DW::draw_text_button(sprite_off_button, "S_OFF", BLACK);
        ss.str("");
        DW::move();
        u_input.draw();
    }
};

template <size_t x, size_t y, size_t w, size_t h, size_t t_w = 4 * 2 + 5, size_t t_h = 8 * 2 + 8>
struct PaletteViewer: DebugWindowWithTexture<x, y, w, h, t_w, t_h> {
    using DW = DebugWindowWithTexture<x, y, w, h, t_w, t_h>;
    enum View {
        Num,
        Colors,
        ViewCount
    } current_view = Num;
    std::stringstream ss;

    void handle(Bus& bus, PPU& ppu){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) &&  IsKeyPressed(KEY_ONE))
            current_view = static_cast<View>((current_view + 1) % ViewCount);
        if (current_view == Num) {
            DW::buffer.fill(WHITE);
            DW::handle();
            for (int i = 0; i < 0x20; ++i){
                ss << std::hex << (unsigned)ppu.mem.map[constants::IMAGE_PALETTE_BEGIN + i] << ' ';
                if (i % 4 == 3) ss << '\n';
            }
            auto str = ss.str();
            DrawText(str.c_str(), DW::rec.x, DW::rec.y,  12.f, BLACK);
            ss.str("");
            return;
        }
        size_t current_buffer_index = 0;
        size_t current_palette_index = 0;
        for (int i = 0; i < t_h; ++i){
            if (i > 0 && i % 3 != 2) {
                current_palette_index -= 4;
            } else if (i > 0){
                current_palette_index += 4;
            }
            for (int j = 0; j < t_w; ++j) {
                if (i % 3 == 2) {
                    DW::buffer[current_buffer_index++] = WHITE;
                } else {
                    if (j % 3 == 0) {
                        if (j > 0) ++current_palette_index;
                        DW::buffer[current_buffer_index++] = WHITE;
                    } else {
                        unsigned char c_index = ppu.mem.read(bus,
                                                             constants::IMAGE_PALETTE_BEGIN + current_palette_index);
                        DW::buffer[current_buffer_index++] = ppu.palette[c_index];
                    }
                }
            }
        }
        DW::handle();
    }
};

template <size_t x, size_t y, size_t w, size_t h, size_t t_w = 8 * 2 + 9, size_t t_h = 8 * 2 + 8>
struct ColorsViewer: DebugWindowWithTexture<x, y, w, h, t_w, t_h> {
    using DW = DebugWindowWithTexture<x, y, w, h, t_w, t_h>;
    std::stringstream ss;
    void handle(Bus& bus, PPU& ppu){
        size_t current_buffer_index = 0;
        size_t current_palette_index = 0;
        for (int i = 0; i < t_h; ++i){
            if (i > 0 && i % 3 != 2) {
                current_palette_index -= 8;
            } else if (i > 0){
                current_palette_index += 8;
            }
            for (int j = 0; j < t_w; ++j) {
                if (i % 3 == 2) {
                    DW::buffer[current_buffer_index++] = RAYWHITE;
                } else {
                    if (j % 3 == 0) {
                        if (j > 0) ++current_palette_index;
                        DW::buffer[current_buffer_index++] = RAYWHITE;
                    } else {
                        DW::buffer[current_buffer_index++] = ppu.palette[current_palette_index];
                    }
                }
            }
        }
        DW::handle();
        ss << "\n     ";
        for (int i = 0; i < 64; ++i){
            ss << std::hex << i;
            if (i > 0 &&  i % 8 == 7){
                if (i == 0x1f || i == 0x37) {ss << "\n\n\n    ";}
                else if (i <= 0xe)          {ss << "\n\n\n     ";}
                else if (i < 0x26)          {ss <<  "\n\n\n\n    ";}
                else                        {ss <<  "\n\n\n\n       ";}
            } else {
                if (i == 0x1f || i == 0x37) {ss << "         ";}
                else if (i <= 0xe)          {ss << "           ";}
                else if (i < 0x1f)          {ss <<  "          ";}
                else                        {ss <<  "         ";}
            }
        }
        auto str = ss.str();
        DrawText(str.c_str(), DW::rec.x, DW::rec.y,  13.5f, RAYWHITE);
        ss.str("");
    }
};


template <size_t x, size_t y, size_t w, size_t h>
struct PPUInfo: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::stringstream ss;

    void handle(PPU& ppu){
        ss << "PPU:\n"
#if UI_DEBUGGER
        << "  FramesCount:"    <<   std::dec << ppu.frames_count << '\n'
        << "  LastLineX:"      <<   std::dec << ppu.last_line_x << '\n'
        << "  CurrentLineX:"   <<   std::dec << ppu.current_line_x << '\n' << std::hex
#endif
        << "  ShiftRegL:"      <<   std::bitset<16>(ppu.shift_register_low).to_string()  << '\n' << std::hex
        << "  ShiftRegH:"      <<   std::bitset<16>(ppu.shift_register_high).to_string() << '\n' << std::hex
        << "  AttributesRegL:" <<   std::bitset<16>(ppu.attributes_low).to_string()      << '\n' << std::hex
        << "  AttributesRegH:" <<   std::bitset<16>(ppu.attributes_high).to_string()     << '\n' << std::hex;
        auto s = ss.str();
        DW::move();
        DW::draw(s.c_str(), 13.f);
        ss.str("");
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct CartridgeInfo: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::stringstream ss;
    void handle(Cartridge& c){
        ss << "CartridgeInfo:\n"
           << "  MapperNum:" << std::hex   << c.mapper_num << '\n'
           << "  CHR_ROM_SZ:" << std::dec  << c.chr_rom_sz << '\n'
           << "  PRG_ROM_SZ:" << std::dec  << c.prg_rom_sz << '\n'
           << "  Battery:"  << c.is_battery << '\n'
           << " Is alternative nt:" << c.is_alternative_nametable << '\n'
           << " Is horizontal arrangement:" << c.is_horizontal_arrangement << '\n';
        auto s = ss.str();
        DW::move();
        DW::draw(s.c_str(), 13.f);
        ss.str("");
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct MemoryViewer: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    unsigned short target_address{};
    std::stringstream ss;
    Rectangle update_target_button;
    Rectangle write_button;
    Rectangle mode_button;
    Rectangle pc_watch_address_button{};
    UserInput<x, y - 30, w, 30> u_input;
    enum Mode {
        CPUm, PPUm, OAMm, ModeCount
    } mode = CPUm;
    MemoryViewer():
            update_target_button{DW::rec.x, DW::rec.y+DW::rec.height, 30,30},
            mode_button{DW::rec.x+30, DW::rec.y+DW::rec.height, 30,30},
            write_button{DW::rec.x+30*2, DW::rec.y+DW::rec.height, 30,30},
            pc_watch_address_button{DW::rec.x+30*3, DW::rec.y+DW::rec.height, 30,30}
            {}
    void handle(Bus& bus, CPU& cpu, PPU& ppu){
        update_target_button = {DW::rec.x, DW::rec.y+DW::rec.height, 30,30};
        mode_button   = {DW::rec.x+30, DW::rec.y+DW::rec.height, 30,30};
        write_button = {DW::rec.x+30*2, DW::rec.y+DW::rec.height, 30,30};
        pc_watch_address_button = {DW::rec.x+30*3, DW::rec.y+DW::rec.height, 30,30};
        u_input.rec   = {DW::rec.x, DW::rec.y-30, DW::rec.width,30};

        auto mem_print = [](auto& bus, auto& ss, auto& r, unsigned short target_address){
            auto start_address = target_address - (target_address % 16);
            ss << std::hex << "\n "<< start_address << " | ";
            for (int i = 0; i < 32; ++i){
                if (i == 16){ ss << "\n "; ss << start_address + i << " | ";}
                if (start_address + i == target_address)
                    ss << " [ " << (unsigned)r.mem.read(bus, start_address + i) << " ] " << ' ';
                else
                    ss  << (unsigned)r.mem.read(bus, start_address + i) << ' ';
            }
        };
        if (CheckCollisionPointRec(GetMousePosition(), mode_button) && IsMouseButtonPressed(0))
            mode = static_cast<Mode>((mode + 1) % ModeCount);
        else if (CheckCollisionPointRec(GetMousePosition(), update_target_button) && IsMouseButtonPressed(0))
            target_address = std::stoul(u_input.s, nullptr, 16);
        else if (CheckCollisionPointRec(GetMousePosition(), write_button) && IsMouseButtonPressed(0)){
            if (mode == CPUm)
                cpu.mem.unsafe_write(bus, target_address, std::stoul(u_input.s, nullptr, 16));
            else
                ppu.mem.write(bus, target_address, std::stoul(u_input.s, nullptr, 16));
        } else if (CheckCollisionPointRec(GetMousePosition(), pc_watch_address_button) && IsMouseButtonPressed(0)){
            cpu.pc_watch_address = std::stoul(u_input.s, nullptr, 16);
            cpu.pc_watch_count = 0;
        }

        switch (mode){
            case CPUm: mem_print(bus, ss, cpu, target_address); ss << "\nCPU"; break;
            case PPUm: mem_print(bus, ss, ppu, target_address); ss << "\nPPU"; break;
            case OAMm: {
                auto i = target_address * 4;
                auto e = i + (16 * 4);
                ss << "\n Start :" << target_address << " " << " End:" << target_address + 16 << '\n';
                ss << " Y    PT   AT   X\n";
                for (; i != e; i+=4)
                    ss << (int)bus.oam[i] << ' ' << (int)bus.oam[i+1] << ' ' << (int)bus.oam[i+2] << ' ' << (int)bus.oam[i+3] << '\n';
                ss << "OAM";
                break;
            }
        }
        ss << '\n';
        ss << "\nPC watch address:" << cpu.pc_watch_address << " , count:"  << std::dec << cpu.pc_watch_count << '\n';
        auto s = ss.str();
        DW::move();
        DW::draw(s.c_str(), 13.f);
        DW::draw_text_button(update_target_button, "update", BLACK);
        DW::draw_text_button(mode_button, "mode", BLACK);
        DW::draw_text_button(write_button, "write", BLACK);
        DW::draw_text_button(pc_watch_address_button, "pc_watch", BLACK);
        u_input.draw();
        ss.str("");
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct SpriteInfo: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::array<unsigned char, 64> y_values{};

    std::stringstream ss;
    enum ViewVersion {
        Dec, Hex, ViewVersionCount
    } view_version = Dec;

    enum InfoVersion {
        OAM_YV, SecondaryOAMV, FullOAMV, InfoVersionCount
    } info_version = OAM_YV;

    bool sorted = false;
    void handle(Bus& bus, PPU& ppu){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_ONE))
            view_version = static_cast<ViewVersion>((view_version + 1) % ViewVersionCount);
        else if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_S))
            sorted = !sorted;
        else if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_M))
            info_version = static_cast<InfoVersion>((info_version + 1) % InfoVersionCount);

        ss << "SpriteInfo:\n"
            << " SpriteMode:" << (ppu.sprite_mode == PPU::SpriteMode::m8x8 ? 8 : 16) << '\n'
           << "  SecondaryOAMSize:"    <<   std::dec << ppu.secondary_oam.size() << '\n'
           << "  OAMBufferSize:"      <<   std::dec << ppu.secondary_oam_buffer.size() << '\n'
        << (view_version == Dec ? std::dec : std::hex);

        if (info_version == OAM_YV) {
            ss << "OAM_Y:\n";
            for (int i = 0; i < 64; ++i)
                y_values[i] = bus.oam[(i * 4)];
            if (sorted) std::sort(y_values.begin(), y_values.end());

            for (int i = 0; i < 64; ++i) {
                ss << (unsigned)y_values[i] << ' ';
                if (i % 8 == 7) ss << '\n';
            }
        } else if (info_version == SecondaryOAMV) {
            ss << "SecondaryOAMBuffer:\n";
            for (int i = 0; i < 8; ++i){
                ss
                << (unsigned)ppu.secondary_oam_buffer[i].y_pos << ' '
                << (unsigned)ppu.secondary_oam_buffer[i].pt_index << ' '
                << (unsigned)ppu.secondary_oam_buffer[i].attrib << ' '
                << (unsigned)ppu.secondary_oam_buffer[i].x_pos << '\n';
            }
        } else if (info_version == FullOAMV) {
            ss << "OAM:\n";
            for (int i = 0; i < 256; ++i){
                ss << (unsigned)bus.oam[i] << ' ';
                if (i % 16 == 15) ss << '\n';
            }
        }
        auto s = ss.str();
        DW::move();
        DW::draw(s.c_str(), 13.f);
        ss.str("");
    }
};

template <size_t x, size_t y, size_t w, size_t h, size_t t_w = 8 * 8 + 8, size_t t_h = (8 * 8 * 2) + 8>
struct SpriteTilesViewer: DebugWindowWithTexture<x, y, w, h, t_w, t_h> {
    using DW = DebugWindowWithTexture<x, y, w, h, t_w, t_h>;
    enum View {
        Tile, Num, ViewCount
    } current_view = Tile;

    std::stringstream ss;
    void handle(Bus& bus, PPU& ppu){
        size_t buffer_index = 0;
        size_t oam_index = 0;
        size_t k = 0;
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_ONE))
            current_view = static_cast<View>((current_view + 1) % ViewCount);

        if (current_view == Tile) {
            for (int i = 0 ; i < t_h; ++i){
                if (i % 17 == 16){
                    for (int z = 0; z < t_w; z++){
                        DW::buffer[buffer_index++] = RED;
                    }
                    k = 0;
                    oam_index += 8;
                } else {
                    for (int j = 0; j < 8; ++j) {
                        auto pt_index =  *(bus.oam + ((oam_index + j) * 4) + 1);
                        auto attrib =    *(bus.oam + ((oam_index + j) * 4) + 2);

                        unsigned char current_index = k < 8 ?
                                (ppu.sprite_mode ==  PPU::SpriteMode::m8x8 ? pt_index : (pt_index & 0xfe)) :
                                ((pt_index & 0xfe) + 1);

                        bool is_horizontal_flip = ((attrib >> 6) & 1);
                        auto pt_base =
                                ppu.sprite_mode == PPU::SpriteMode::m8x8 ?
                                       (ppu.ppuctrl.pattern_table_addr8x8 & 1 ? constants::PATTERN_TABLE1_BEGIN : constants::PATTERN_TABLE0_BEGIN) :
                                (pt_index & 1 ? constants::PATTERN_TABLE1_BEGIN : constants::PATTERN_TABLE0_BEGIN);

                        unsigned char shift_l = ppu.mem.read(bus,
                                                             pt_base + (current_index * 16) + (k % 8));
                        unsigned char shift_h = ppu.mem.read(bus,
                                                             pt_base + (current_index * 16) + (k % 8) + 8);

                        unsigned char attrib_high_bit = (attrib >> 3) & 1;
                        unsigned char attrib_low_bit = (attrib >> 2) & 1;
                        for (int l = 0; l < 8; ++l) {
                            auto res =
                                    is_horizontal_flip ?
                                    ((shift_h & 1) << 1)  | (shift_l  & 1)
                                                       : (((shift_h >> 7) & 1) << 1)  | (shift_l >> 7) & 1;
                            unsigned char index = (((attrib_high_bit << 1) | attrib_low_bit) << 2) | res;
                            unsigned char pixel_index = ppu.mem.read(bus, constants::SPRITE_PALETTE_BEGIN + index);
                            DW::buffer[buffer_index++] = ppu.palette[pixel_index];
                            if (is_horizontal_flip){
                                shift_l >>= 1;
                                shift_h >>= 1;
                            } else {
                                shift_l <<= 1;
                                shift_h <<= 1;
                            }
                        }
                        DW::buffer[buffer_index++] = RED;
                    }
                    if ((ppu.sprite_mode == PPU::SpriteMode::m8x8 && i % 2 == 1) || ppu.sprite_mode  == PPU::SpriteMode::m8x16) ++k;
                }
            }
            DW::move();
            DW::handle();
        } else {
            for (int i = 0; i < 64; ++i) {
                ss <<  (unsigned)bus.oam[(i * 4) + 1] << " ";
                if (i % 8 == 7) ss << '\n';
            }
            auto s = ss.str();
            DrawText(s.c_str(), DW::rec.x, DW::rec.y, 12.3f, BLACK);
            ss.str("");
            DW::move();
        }
    }
};

template <size_t x, size_t y>
struct DummyTilesGrid {
    void draw(float cell_w, float cell_h) {
        float cur_x = x;
        float cur_y = y;
        for (size_t i = 0; i < 30; ++i){
            for (size_t j = 0; j < 32; ++j){
                DrawRectangleLinesEx({cur_x, cur_y, cell_w, cell_h}, 1.f, RED);
                cur_x += cell_w;
            }
            cur_x = x;
            cur_y += cell_h;
        }
    }
};
#endif //NES_DEBUG_H

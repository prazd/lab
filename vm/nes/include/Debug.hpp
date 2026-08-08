#ifndef NES_DEBUG_HPP
#define NES_DEBUG_HPP
#if DEBUG_MODE
#include <sstream>
#include <iomanip>
#include "raylib.h"
#include "PPU.hpp"
#include "APU.hpp"
#include "Cartridge.hpp"
#include "Simple.hpp"
#include <algorithm>

template <size_t x, size_t y, size_t w, size_t h>
struct DebugWindow {
    Rectangle rec;
    DebugWindow(): rec{x,y,w,h}{}

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

    template <typename PredF, typename HandlerF, typename ... Args>
    static void cpu_search_step(Bus& bus, CPU& cpu, PredF pred_f, HandlerF handler_f, unsigned& l_pc, Args&&... args) {
        auto val = bus.unsafe_read_prg(l_pc);
        if (val == 0xff){++l_pc; return;}  // also skip ISC (0xff)
        auto decoded = cpu.decode(val);
        if (!decoded.bytes) {++l_pc; return;}
        auto asm_info = cpu.disassemble(bus, l_pc);
        if (pred_f(l_pc, decoded, asm_info))
            handler_f(l_pc, decoded, asm_info, std::forward<Args>(args)...);
        l_pc += decoded.bytes;
    }

    template <typename PredF, typename HandlerF, typename ... Args>
    static void cpu_search(Bus& bus, CPU& cpu, PredF pred_f, HandlerF handler_f, Args&&... args) {
        unsigned l_pc = 0x8000;
        while(l_pc < 0xffff)
            cpu_search_step(bus, cpu,  pred_f, handler_f, l_pc, std::forward<Args>(args)...);
    }

    template <typename PredF, typename HandlerF, typename ... Args>
    static void cpu_search_pc(Bus& bus, CPU& cpu, PredF pred_f, HandlerF handler_f, Args&&... args) {
        unsigned l_pc = 0x8000;
        while(l_pc < 0xffff) {
            if (!Bus::cpu_pc_read_set_addresses.test(l_pc)) {++l_pc; continue;}
            cpu_search_step(bus, cpu, pred_f, handler_f, l_pc, std::forward<Args>(args)...);
        }
    }

    using TileRow = std::array<Color, 8>;
    static TileRow get_tile_row(unsigned char a, unsigned char b) {
        std::array<Color, 8> res{};
        for (int i = 0; i < 8; ++i){
            res[i] = color_by_id((((a >> (7-i)) & 1) << 1) | ((b >> (7-i)) & 1));
        }
        return res;
    }

    static void save_data_to_file(const std::string& filename, const char* ptr, long sz){
        std::ofstream f(std::string("/Users/prazd/Desktop/books/cpp/cpp/vm/nes/build/") + filename);
        if (!f.is_open()) {std::abort();};
        f.write(ptr, sz);
        f.close();
    }

    static std::vector<unsigned char> read_data_from_file(const std::string& filename){
        std::vector<unsigned char> res;
        auto full_path = std::string("/Users/prazd/Desktop/books/cpp/cpp/vm/nes/build/") + filename;
        auto sz = std::filesystem::file_size(full_path);
        res.resize(sz);
        auto fs = std::ifstream(full_path, std::ios::binary);
        if (!fs.is_open()) std::abort();
        fs.read(reinterpret_cast<char*>(res.data()),sz);
        return res;
    }
    static inline unsigned short hex_to_ushort(const std::string& str, int base){
       return str.empty() ? 0 : std::stoul(str, nullptr, base);
    }

    static inline auto split_by_c(std::string& s, char c){
        std::vector<std::string_view> res;
        size_t it = 0;
        for (size_t i = 0, e = s.size(); i != e; ++i){
            if (s[i] == c){
                res.push_back(std::string_view{s.begin() + it, s.begin() + i});
                it = i+1;
            }
        }
        res.push_back(std::string_view{s.begin() + it, s.end()});
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

    static inline size_t lines_h[32]{};
    static inline size_t lines_y1{};
    static inline size_t lines_y2{};

    static inline size_t lines_v[30]{};
    static inline size_t lines_x1{};
    static inline size_t lines_x2{};

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

    bool show_screen = true;

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
                auto current_nt_y = s - (s > 30);
                for (int z = 0; z < 8; ++z) {
                    for (int i = 0; i < 64; ++i){
                        if (i == 32) {
                            DW::buffer[current++] = RED;
                            ++current_nt;
                        }
                        auto nt =  bus.get_nt_by_mirror_mode(bus.mapper.current_screen_mirroring, current_nt);
                        auto x_tile_n = (i % 32);
                        auto y_tile_n = current_nt_y % 30;
                        auto pt_addr = ((base_pt_address + (16 * *(bus.ppu_map + nt + (y_tile_n * 32) + x_tile_n))));
                        auto l_tile = bus.unsafe_read_chr(pt_addr + z);
                        auto h_tile = bus.unsafe_read_chr(pt_addr + z + 8);
                        auto val = bus.ppu_map[nt + 0x3c0 + (y_tile_n/4 * 8) + x_tile_n/4];
                        auto shift_y = (y_tile_n/2) % 2;
                        auto shift_x = (x_tile_n/2) % 2;
                        auto attrib_value = (val >> (2 * ((shift_y * 2) + shift_x))) & 0x3;
                        if (show_screen && is_in_screen(i, current_nt_y))
                        {
                            for (int k = 0; k < 8; ++k){
                                DW::buffer[current++] = GREEN;
                            }
                        } else {
                            for (int k = 0; k < 8; ++k) {
                                auto res = ((((h_tile >> 7) & 1) << 1) | (l_tile >> 7) & 1);
                                h_tile <<= 1;
                                l_tile <<= 1;
                                auto attrib_high_bit = (attrib_value >> 1) & 1;
                                auto attrib_low_bit = attrib_value & 1;
                                unsigned char index = (((attrib_high_bit << 1) | attrib_low_bit) << 2) | res;
                                unsigned short r_index = constants::IMAGE_PALETTE_BEGIN;
                                r_index += ((res != 0) * index);
                                auto pixel_index = bus.ppu_map[r_index];
                                DW::buffer[current++] = ppu.palette[pixel_index];
                            }
                        }
                    }
                    --current_nt;
                }
            }
        }
    }

    static void sync_screen_pos(Bus& bus){
        size_t start_nt = ((bus.start_v_value >> 10) & 0x3);
        lines_x1 = (((bus.start_v_value & 0x1f) + (start_nt == 1 || start_nt == 3) * 31)) % 64;
        lines_y1 = (((bus.start_v_value >> 5) & 0x1f) + (start_nt == 2 || start_nt == 3) * 29) % 60;
        lines_x2 = (lines_x1 + 31) % 64;
        lines_y2 = (lines_y1 + 29) % 60;

        for (size_t i = 0; i < 32; ++i)
            lines_h[i] = (i + lines_x1) % 64;

        for (size_t i = 0; i < 30; ++i)
            lines_v[i] = (i + lines_y1) % 60;
    }

    static bool is_in_screen(size_t s_x, size_t s_y){
        return
        std::any_of(std::begin(lines_h), std::end(lines_h), [s_x, s_y](auto el){
            return el == s_x && (s_y == lines_y1 || s_y == lines_y2);
        }) ||
        std::any_of(std::begin(lines_v), std::end(lines_v), [s_x, s_y](auto el){
            return el == s_y && (s_x == lines_x1 || s_x == lines_x2);
        });
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
                auto current_nt_y = s - (s > 30);
                for (int z = 0; z < 8; ++z) {
                    for (int i = 0; i < 64; ++i){
                        if (i == 32) {
                            DW::buffer[current++] = RED;
                            ++current_nt;
                        }
                        auto pt_addr = ((base_pt_address + (16 *
                                                            *(bus.ppu_map + (bus.get_nt_by_mirror_mode(bus.mapper.current_screen_mirroring, current_nt) +
                                                                             ((current_nt_y % 30) * 32) + (i % 32))))));
                        auto l_tile = bus.unsafe_read_chr(pt_addr + z);
                        auto h_tile = bus.unsafe_read_chr(pt_addr + z + 8);
                        auto tile_row = get_tile_row(h_tile, l_tile);
                        if (show_screen && is_in_screen(i, current_nt_y))
                         {
                            for (int k = 0; k < 8; ++k){
                                DW::buffer[current++] = GREEN;
                            }
                        } else {
                            for (int k = 0; k < 8; ++k){
                                DW::buffer[current++] = tile_row[k];
                            }
                        }
                    }
                    --current_nt;
                }
            }
        }
    }

    void handle(PPU& ppu, Bus& bus){
        sync_screen_pos(bus);
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_M))
            show_screen = !show_screen;
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
    void handle(Bus& bus){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_ONE))
            ascii_print = !ascii_print;
        ss << std::hex;
        for (int s = 0; s < 30; ++s){
            for (int i = 0; i < 32; ++i){
                auto v = *(bus.ppu_map + (base_nt_address + (s * 32) + i));
                if (ascii_print && v != '\0') ss << v; else ss << (unsigned)v;
                ss << " ";
            }
            ss << '\n';
        }
        auto s = ss.str();
        DW::draw(s.c_str());
        ss.str("");
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct UserInput {
    std::stringstream ss;
    std::string s;
    std::string selected_sub;
    std::string offset_sub;
    Rectangle rec;
    size_t cursor_pos = 0;
    float wsize = 15.f;

    int selected_begin = -1;
    int selected_end = -1;
    UserInput(): rec{x, y, w, h} {}

    void flush_select() {
        selected_begin = -1; selected_end = -1; selected_sub.clear(); offset_sub.clear();
    }
    void push_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        if (IsKeyPressed(KEY_LEFT))  {flush_select(); cursor_pos += cursor_pos ? -1 : 0;}
        if (IsKeyPressed(KEY_RIGHT)) {flush_select(); cursor_pos += cursor_pos < s.size() ? 1 : 0;}
        if (IsKeyPressed(KEY_V) && IsKeyDown(KEY_LEFT_SUPER)) {s = GetClipboardText(); cursor_pos = s.size();}

        if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_SUPER)) {if (!s.empty()) {selected_begin = 0; selected_end = s.size() + 1; selected_sub = s.substr(selected_begin, selected_end); offset_sub.clear();}}

        if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_LEFT)) {if (!s.empty())  {selected_begin = 0; selected_end = cursor_pos + 1; cursor_pos = 0; selected_sub = s.substr(selected_begin, selected_end);}}
        else if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_LEFT) && !IsKeyPressed(KEY_LEFT_SHIFT)) {if (!s.empty()) {flush_select(); cursor_pos = 0;}}

        if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_RIGHT)) {if (!s.empty()) {selected_begin = cursor_pos ? cursor_pos-1 : 0; selected_end = s.size() + 1; cursor_pos = s.size(); selected_sub = s.substr(selected_begin, selected_end);  offset_sub = s.substr(0, selected_begin);}}
        else if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_RIGHT) && !IsKeyPressed(KEY_LEFT_SHIFT)) {if (!s.empty()) {flush_select(); cursor_pos = s.size();}}

        if (IsKeyPressed(KEY_C) && IsKeyDown(KEY_LEFT_SUPER)) {SetClipboardText(selected_sub.c_str());}

        auto c = GetCharPressed();
        if (!c) return;
        if (selected_begin != -1 && selected_end != -1){
            cursor_pos = 0;
            flush_select();
            s.clear();
        }
        s.insert(cursor_pos, 1, (char)c);
        cursor_pos+=1;
    }
    void pop_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        if (IsKeyPressed(KEY_BACKSPACE) && !s.empty()){
            if (selected_begin != -1 && selected_end != -1){
                s.erase(selected_begin, selected_end-selected_begin);
                if (selected_begin != 0)
                    cursor_pos = s.size();
                else
                    cursor_pos = 0;
                flush_select();
            } else {
                if (cursor_pos){
                    s.erase(cursor_pos - (cursor_pos ? 1 : 0), 1);
                    --cursor_pos;
                    flush_select();
                }
            }
        }
    }
    void handle() {
        push_handler();
        pop_handler();
        DrawRectangleLinesEx({rec.x, rec.y, rec.width, rec.height}, 1.f, BLACK);
        if (selected_begin != -1) {
            auto twidth = MeasureText(selected_sub.c_str(), wsize);
            auto offset_width = MeasureText(offset_sub.c_str(), wsize);
            DrawRectangle(rec.x + offset_width, rec.y, twidth, wsize, BLUE);
#if 0
            std::cout << "Selected_Begin:" << selected_begin << " SelectedEnd:" << selected_end <<
            " Twidth:" << twidth << " Diff:" << selected_end-selected_begin << std::endl;
#endif
        }

        for (size_t i = 0, e = s.size(); i != e; ++i){
            if (i == cursor_pos && CheckCollisionPointRec(GetMousePosition(), rec))
                ss << "|";

            ss << s[i];

            if (cursor_pos == i + 1 && cursor_pos == s.size() && CheckCollisionPointRec(GetMousePosition(), rec))
                ss << "|";
        }
        if (s.empty() && CheckCollisionPointRec(GetMousePosition(), rec)){
            ss << " |";
        }
        auto r_str = ss.str();
        DrawText(r_str.data(), rec.x, rec.y, wsize, BLACK);
        ss.str("");
    }
};

template<size_t x, size_t y, size_t w, size_t h>
struct Disassembler: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::stringstream ss;
    bool pressed_mode = true;
    bool should_sync = false;
    bool trigger_search = false;

    enum RunMode {
        CPU_M, PPU_M, RunModeCount
    } run_mode = PPU_M;

    bool next = false;
    bool selected_move_active = false;
    unsigned short selected_line_pc = 0;
    unsigned short base{};
    unsigned short pc{};
    unsigned char selected_scanline{};
    bool selected_scanline_active{};
    bool dump_state_request = false;

    bool is_frame_jump_active = false;
    bool is_tile_line_jump_active = false;
    bool is_line_jump_active = false;
    bool is_one_tick_ppu_active = false;
    unsigned char stress_speed = 0;
    bool cpu_to_fetch_req = false;

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
    Rectangle button13;
    Rectangle button14;

    Rectangle asm_dump_button;
    Rectangle state_dump_button;
    Rectangle state_import_button;
    Rectangle load_save_button;
    Rectangle save_button;
    Rectangle stress_button;


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
                                      button12{DW::rec.x + 40*5, DW::rec.y + h + 40, 40, 40},
                                      button14{DW::rec.x + 40 * 8, DW::rec.y + h, 40, 40},
                                      asm_dump_button{DW::rec.x + w, DW::rec.y + 40, 40, 40},
                                      state_dump_button{DW::rec.x + w, DW::rec.y + 40 * 2, 40, 40},
                                      state_import_button{DW::rec.x + w, DW::rec.y + 40 * 3, 40, 40},
                                      load_save_button{DW::rec.x + w, DW::rec.y + 40 * 4, 40, 40},
                                      save_button{DW::rec.x + w, DW::rec.y + 40 * 5, 40, 40},
                                      stress_button{DW::rec.x + w, DW::rec.y + 40 * 6, 40, 40}
                                      {}


    void dump_state(Bus& bus, CPU& cpu, PPU& ppu, APU& apu) {
        std::vector<unsigned char> res;
        res.resize(sizeof(ppu) + sizeof(cpu) + sizeof(apu) + sizeof(bus) +
                       sizeof(Bus::cpu_map) + sizeof(Bus::ppu_map) + sizeof(Bus::oam) +
                         Mapper::prg.size() + Mapper::chr.size()
                       );
        unsigned char* res_ptr = res.data();
        memcpy(res_ptr, &ppu,  sizeof(ppu));
        res_ptr += sizeof(ppu);
        memcpy(res_ptr, &cpu,  sizeof(cpu));
        res_ptr += sizeof(cpu);
        memcpy(res_ptr, &apu,  sizeof(apu));
        res_ptr += sizeof(apu);
        memcpy(res_ptr, &bus,  sizeof(bus));
        res_ptr += sizeof(bus);
        memcpy(res_ptr, Bus::ppu_map,  sizeof(Bus::ppu_map));
        res_ptr += sizeof(Bus::ppu_map);
        memcpy(res_ptr, Bus::cpu_map, sizeof(Bus::cpu_map));
        res_ptr += sizeof(Bus::cpu_map);
        memcpy(res_ptr,Bus::oam,  sizeof(Bus::oam));
        res_ptr += sizeof(Bus::oam);
        memcpy(res_ptr, Mapper::prg.data(), Mapper::prg.size());
        res_ptr += Mapper::prg.size();
        memcpy(res_ptr, Mapper::chr.data(), Mapper::chr.size());
        DW::save_data_to_file("state_dump", (const char*)res.data(), res.size());
        dump_state_request = false;
    }

    void import_state(Bus& bus, CPU& cpu, PPU& ppu, APU& apu) {
        auto res = DW::read_data_from_file("state_dump");
        unsigned char* res_ptr = res.data();

        memcpy(&ppu, res_ptr,  sizeof(ppu));
        res_ptr += sizeof(ppu);
        memcpy(&cpu, res_ptr,  sizeof(cpu));
        res_ptr += sizeof(cpu);
        memcpy(&apu, res_ptr,  sizeof(apu));
        res_ptr += sizeof(apu);
        memcpy(&bus, res_ptr,  sizeof(bus));
        res_ptr += sizeof(bus);
        memcpy(Bus::ppu_map, res_ptr,   sizeof(Bus::ppu_map));
        res_ptr += sizeof(Bus::ppu_map);
        memcpy(Bus::cpu_map, res_ptr, sizeof(Bus::cpu_map));
        res_ptr += sizeof(Bus::cpu_map);
        memcpy(Bus::oam,res_ptr,  sizeof(Bus::oam));
        res_ptr += sizeof(Bus::oam);

        memcpy(Mapper::prg.data(), res_ptr, Mapper::prg.size());
        res_ptr += Mapper::prg.size();
        memcpy(Mapper::chr.data(), res_ptr, Mapper::chr.size());
    }

    void reset(){
        selected_move_active = false;
        is_frame_jump_active = false;
        is_tile_line_jump_active = false;
        is_line_jump_active  = false;
        is_one_tick_ppu_active = false;
        should_sync = false;
        trigger_search = false;
        selected_line_pc = 0;
        pressed_mode = true;
        should_sync = false;
        run_mode = CPU_M;
        next = true;
    }

    bool mouse_button_pred(bool pm){
        return pm ?  IsMouseButtonPressed(0) :  IsMouseButtonDown(0);
    }

    void handle(CPU& cpu) {
        base = cpu.pc;
        next = (CheckCollisionPointRec(GetMousePosition(), button1) && mouse_button_pred(pressed_mode));
    }

    void snap_handle(CPU& cpu, PPU& ppu, Bus& bus, auto& snap_storage){
        if (CheckCollisionPointRec(GetMousePosition(), button14) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            snap_storage.step_back(bus, cpu, ppu);
            handle(cpu);
            cpu.debug_run = false;
        }
    }

    void draw(CPU& cpu, PPU& ppu, Bus& bus, APU& apu, Cartridge& cartridge) {
        line_button = {DW::rec.x - 20, DW::rec.y, 10, 10};
        button1 = {DW::rec.x, DW::rec.y + h, 40, 40};
        button2 = {DW::rec.x + 40, DW::rec.y + h, 40, 40};
        button3 = {DW::rec.x + 40 * 2, DW::rec.y + h, 40, 40};
        button4 = {DW::rec.x + 40 * 3, DW::rec.y + h, 40, 40};
        button5 = {DW::rec.x + 40 * 4, DW::rec.y + h, 40, 40};
        button6 = {DW::rec.x + 40 * 5, DW::rec.y + h, 40, 40};
        button7 = {DW::rec.x + 40 * 6, DW::rec.y + h, 40, 40};
        button8 = {DW::rec.x + 40 * 7, DW::rec.y + h, 40, 40};
        button14 = {DW::rec.x + 40 * 8, DW::rec.y + h, 40, 40};
        button9 = {DW::rec.x + 40*2, DW::rec.y + h + 40, 40, 40};
        button10 = {DW::rec.x + 40*3, DW::rec.y + h + 40, 40, 40};
        button11 = {DW::rec.x + 40*4, DW::rec.y + h + 40, 40, 40};
        button12 = {DW::rec.x + 40*5, DW::rec.y + h + 40, 40, 40};
        asm_dump_button = {DW::rec.x + w, DW::rec.y + 40, 40, 40};

        u_input.rec = {DW::rec.x, DW::rec.y-30, w, 30};

        change_mode_button = {DW::rec.x + 40, DW::rec.y + h + 40, 40, 40};
        sync_button = {DW::rec.x, DW::rec.y + h + 40, 40, 40};

        pc = base;

        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_M))
            pressed_mode = !pressed_mode;
        else if (CheckCollisionPointRec(GetMousePosition(), button2) && run_mode == CPU_M && mouse_button_pred(pressed_mode)){
            selected_move_active = true;
        } else if (CheckCollisionPointRec(GetMousePosition(), button3)  && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            selected_line_pc = pc + cpu.decode(bus.unsafe_read_prg(pc)).bytes;
            selected_move_active = true;
        } else if (CheckCollisionPointRec(GetMousePosition(), button4) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            next = true;
            auto val = DW::hex_to_ushort(u_input.s, 16);
            selected_line_pc = (unsigned short) val;
            selected_move_active = !selected_move_active;
            ppu.fill_white_background = !selected_move_active;
        // trigger search
        } else if (CheckCollisionPointRec(GetMousePosition(), button5) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            unsigned short addr = DW::hex_to_ushort(u_input.s, 16);
            bus.adjust_cpu_mirroring_unsafe(addr);
            bus.cpu_trigger_write_addresses[addr] = true;
        }   else if (CheckCollisionPointRec(GetMousePosition(), button6) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            unsigned short addr = DW::hex_to_ushort(u_input.s, 16);
            bus.adjust_cpu_mirroring_unsafe(addr);
            bus.cpu_trigger_read_addresses[addr] = true;
        }   else if (CheckCollisionPointRec(GetMousePosition(), button7) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            bus.cpu_trigger_read_addresses = 0;
            bus.cpu_trigger_write_addresses = 0;
        } else if (CheckCollisionPointRec(GetMousePosition(), button8) && run_mode == CPU_M && mouse_button_pred(pressed_mode)) {
            trigger_search = !trigger_search;
            bus.cpu_addr_triggered_on_write = !trigger_search;
            bus.cpu_addr_triggered_on_read = !trigger_search;
            ppu.fill_white_background = !trigger_search;
        } else if (CheckCollisionPointRec(GetMousePosition(), button1) && run_mode == PPU_M && mouse_button_pred(pressed_mode)){
            ppu.frame_start = false;
            is_frame_jump_active = true;
        }
        else if (CheckCollisionPointRec(GetMousePosition(), button2) && run_mode == PPU_M && mouse_button_pred(pressed_mode)){
            is_tile_line_jump_active = true;
        }
        else if (CheckCollisionPointRec(GetMousePosition(), button3) && run_mode == PPU_M && mouse_button_pred(pressed_mode)){
            ppu.line_start = false;
            is_line_jump_active = true;
        } else if (CheckCollisionPointRec(GetMousePosition(), button4) && run_mode == PPU_M && mouse_button_pred(pressed_mode)) {
            selected_scanline = (unsigned char)DW::hex_to_ushort(u_input.s, 10);
            selected_scanline_active = true;
        } else if (CheckCollisionPointRec(GetMousePosition(), button5) && run_mode == PPU_M && mouse_button_pred(pressed_mode))
            is_one_tick_ppu_active = true;
        else if (CheckCollisionPointRec(GetMousePosition(), button6) && run_mode == PPU_M && IsMouseButtonPressed(0)){
            ppu.run = !ppu.run;
            ppu.fill_white_background = !ppu.run;
            cpu_to_fetch_req = true;
        }
        // trigger search
        else if (CheckCollisionPointRec(GetMousePosition(), button9) && run_mode == PPU_M  && IsMouseButtonPressed(0)) {
            unsigned short addr = DW::hex_to_ushort(u_input.s, 16);
            bus.adjust_ppu_mirroring_unsafe(addr);
            bus.ppu_trigger_write_addresses[addr] = true;
        }   else if (CheckCollisionPointRec(GetMousePosition(), button10) && run_mode == PPU_M && IsMouseButtonPressed(0)) {
            unsigned short addr = DW::hex_to_ushort(u_input.s, 16);
            bus.adjust_ppu_mirroring_unsafe(addr);
            bus.ppu_trigger_read_addresses[std::stoul(u_input.s, nullptr, 16)] = true;
        }   else if (CheckCollisionPointRec(GetMousePosition(), button11) && run_mode == PPU_M && IsMouseButtonPressed(0)) {
            bus.ppu_trigger_read_addresses = 0;
            bus.ppu_trigger_write_addresses = 0;
        } else if (CheckCollisionPointRec(GetMousePosition(), button12) && run_mode == PPU_M && IsMouseButtonPressed(0)) {
            trigger_search = !trigger_search;
            bus.ppu_addr_triggered_on_write = !trigger_search;
            bus.ppu_addr_triggered_on_read = !trigger_search;
            ppu.fill_white_background = !trigger_search;
        }
        else if (CheckCollisionPointRec(GetMousePosition(), sync_button) && IsMouseButtonPressed(0))
            should_sync = true;
        else if (CheckCollisionPointRec(GetMousePosition(), change_mode_button) && IsMouseButtonPressed(0))
            run_mode = static_cast<RunMode>((run_mode + 1) % RunModeCount);
        else if (CheckCollisionPointRec(GetMousePosition(), asm_dump_button) && IsMouseButtonPressed(0)){
            std::vector<unsigned short> subroutines_addresses{};
            DW::cpu_search(bus, cpu,
            [](auto lpc, auto& decoded, auto& asm_info){
                return decoded.i == CPU::JSR;
            },
            [](auto lpc, auto& decoded, auto& asm_info, auto& subroutines_addresses){
                subroutines_addresses.push_back(asm_info.arg_addr);
            },
            subroutines_addresses
            );

            std::stringstream lss;
            DW::cpu_search_pc(bus, cpu,
                           [](auto&&...){return true;},
                           [](auto l_pc, auto& decoded, auto& asm_info, auto& lss, const auto& subroutines_addresses){
                               if (std::any_of(subroutines_addresses.begin(), subroutines_addresses.end(), [l_pc](auto el){return el == l_pc;}))
                                   lss << "-------------------------BEGIN-----------------------------\n";
                               lss << std::hex << l_pc << ' ';
                               lss << asm_info.mnemonic << ' ';
                               asm_info.is_accumulator ? (lss << "A") : (asm_info.no_operand ? lss : lss << asm_info.operand);
                               if (asm_info.show_address)
                                   lss << std::hex << ' ' << '[' << asm_info.arg_addr << ']';
                               lss << '\n';

                               if (decoded.i == CPU::RTS)
                                   lss << "-------------------------END-----------------------------\n";
                           },
                           lss, subroutines_addresses
            );

            auto s = lss.str();
            DW::save_data_to_file(u_input.s, (char*)s.c_str(), (long)s.size());
        }  else if (CheckCollisionPointRec(GetMousePosition(), state_dump_button) && IsMouseButtonPressed(0)){
            dump_state_request = true;
        } else if  (CheckCollisionPointRec(GetMousePosition(), state_import_button) && IsMouseButtonPressed(0)){
            import_state(bus, cpu, ppu, apu);
        } else if (CheckCollisionPointRec(GetMousePosition(), load_save_button) && IsMouseButtonPressed(0)){
            cartridge.update_game_state<false>((char*)(bus.cpu_map + constants::CARTRIDGE_RAM_BEGIN));
        }  else if (CheckCollisionPointRec(GetMousePosition(), save_button) && IsMouseButtonPressed(0)){
            cartridge.update_game_state<true>((char*)(bus.cpu_map + constants::CARTRIDGE_RAM_BEGIN));
        } else if (CheckCollisionPointRec(GetMousePosition(), stress_button) && IsMouseButtonPressed(0)){
            stress_speed ^= 1;
        }

        if (dump_state_request && cpu.istep == CPU::Fetch && cpu.iop.empty())
            dump_state(bus, cpu, ppu, apu);

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
            pc += cpu.decode(bus.unsafe_read_prg(pc)).bytes;
        }
        ss << "RunMode:";
        if (run_mode == CPU_M)
            ss << "CPU";
        else
            ss << "PPU";
        ss << ' ' << (pressed_mode ? "PressedMode" : "DownMode");

        auto print_trigger_list = [](auto& ss, auto& read_trigger_address, auto& write_trigger_address){
            ss << "\nWriteTrigger:";
            for (size_t i = 0, e = std::size(write_trigger_address); i != e; ++i){
                if (write_trigger_address[i]) ss << std::hex << i << ',';
            }
            ss << "\nReadTrigger:";
            for (size_t i = 0, e = std::size(read_trigger_address); i != e; ++i){
                if (read_trigger_address[i]) ss << std::hex << i << ',';
            }
        };

        if (run_mode == CPU_M)
            print_trigger_list(ss,  bus.cpu_trigger_read_addresses, bus.cpu_trigger_write_addresses);
        else if (run_mode == PPU_M)
            print_trigger_list(ss, bus.ppu_trigger_read_addresses, bus.ppu_trigger_write_addresses);

        auto str = ss.str();
        DW::draw(str.data(), 15.f);
        if (run_mode == CPU_M) {
            DW::draw_text_button(button1, " next", GREEN, 15.f);
            DW::draw_text_button(button2, " jmp", GREEN, 15.f);
            DW::draw_text_button(button3, " step", GREEN, 15.f);
            DW::draw_text_button(button4, " ntjmp", GREEN, 15.f);

            DW::draw_text_button(button5, " wadd", GREEN, 15.f);
            DW::draw_text_button(button6, " radd", GREEN, 15.f);
            DW::draw_text_button(button7, " clear", GREEN, 15.f);
            DW::draw_text_button(button8, " trun", GREEN, 15.f);
            DW::draw_text_button(button14, " prev", GREEN, 15.f);

        } else {
            DW::draw_text_button(button1, "frame", GREEN, 15.f);
            DW::draw_text_button(button2, " tile", GREEN, 15.f);
            DW::draw_text_button(button3, " line", GREEN, 15.f);
            DW::draw_text_button(button4, " linej", GREEN, 15.f);
            DW::draw_text_button(button5, " dot", GREEN, 15.f);
            DW::draw_text_button(button6, " run", GREEN, 15.f);

            DW::draw_text_button(button9, " wadd", GREEN, 15.f);
            DW::draw_text_button(button10, " radd", GREEN, 15.f);
            DW::draw_text_button(button11, " clear", GREEN, 15.f);
            DW::draw_text_button(button12, " trun", GREEN, 15.f);
        }
        DW::draw_text_button(asm_dump_button, " d_asm", GREEN, 15.f);
        DW::draw_text_button(state_dump_button, " d_state", GREEN, 15.f);
        DW::draw_text_button(state_import_button, " i_state", GREEN, 15.f);
        DW::draw_text_button(load_save_button, " load_save", GREEN, 15.f);
        DW::draw_text_button(save_button, " save", GREEN, 15.f);
        DW::draw_text_button(stress_button, " stress", stress_speed ? GREEN : RED, 15.f);
        DW::draw_text_button(sync_button, " state", GREEN, 15.f);
        DW::draw_text_button(change_mode_button, " mode", GREEN, 15.f);
        u_input.handle();
        ss.str("");
    }
};


struct SnapStorage {
    struct CPUs {
        // TODO: check
        SimpleDequeTC<std::pair<unsigned short, unsigned char>,10> write_addresses;
        CPU cpu_state;
        inline void from_cpu(const CPU& cpu, Bus& bus){
            cpu_state = cpu;
            write_addresses = bus.cpu_write_addresses;
        }
        inline void to_cpu(Bus& bus, CPU& cpu){
            while(!write_addresses.empty()){
                auto cur = write_addresses.back();
                bus.unsafe_write(cur.first, cur.second);
                write_addresses.pop_back();
            }
            cpu = cpu_state;
        }
    };

    struct PPUs{
        SimpleDequeTC<std::pair<unsigned short, unsigned char>, 10> write_addresses;
        PPU ppu_state;
        inline void from_ppu(const PPU& ppu, Bus& bus){
            ppu_state = ppu;
            write_addresses = bus.ppu_write_addresses;
        }
        inline void to_ppu(Bus& bus, PPU& ppu){
            while(!write_addresses.empty()){
                auto cur = write_addresses.back();
                bus.write_chr(cur.first, cur.second);
                write_addresses.pop_back();
            }
            ppu = ppu_state;
        }
    };

    struct State {
        CPUs cpus;
        PPUs ppus;
        Bus bus;
    };
    State next;
    CircleStack<State,10> prev_states;
    inline void reset(){
        prev_states.clear();
    }
    inline void step_back(Bus& bus, CPU& cpu, PPU& ppu){
        if (!prev_states.empty()){
            auto& [c,p,b] = prev_states.top();
            bus = b;
            c.to_cpu(bus, cpu);
            p.to_ppu(bus, ppu);
            prev_states.pop();
        }
    }

    inline void update_state(Bus& bus, CPU& cpu, PPU& ppu){
        next.cpus.from_cpu(cpu, bus);
        next.ppus.from_ppu(ppu, bus);
        next.bus = bus;
    }

    inline void push_state(Bus& bus){
        next.cpus.write_addresses = bus.cpu_write_addresses;
        next.ppus.write_addresses = bus.ppu_write_addresses;
        bus.cpu_write_addresses.reset();
        bus.ppu_write_addresses.reset();
        prev_states.push(std::move(next)); // todo: check
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
    enum Mode {
        Set, Watch , ModeCount
    } mode = Set;
    unsigned short watch_status{};
    void watch_status_switch(RegNum rn) {
        watch_status ^= (1 << rn);
    }
    bool get_watch_status(RegNum rn){
        return (watch_status >> rn) & 1;
    }
    bool is_cpu_watching(){
        return watch_status & 0x3f;
    }
    bool is_ppu_watching(){
        return watch_status & 0x03c0;
    }

    void watch_update_check(CPU& cpu, Bus& bus, PPU& ppu){
        for (int i = 0; i < 10; ++i){
            auto rn = static_cast<RegNum>(i);
            if ((get_watch_status(rn) && get_reg_value(rn, bus, ppu, cpu) != previous_reg_values[i])){
                watch_status = 0;
                ppu.fill_white_background = true;
                break;
            }
        }
    }

    std::array<unsigned char, RegsNumCount> previous_reg_values = {};

    unsigned char get_reg_value(RegNum rn, Bus& bus, PPU& ppu, CPU& cpu){
        switch (rn) {
            case A: return cpu.a;
            case CPU_X: return cpu.x;
            case Y: return cpu.y;
            case PC: return cpu.pc;
            case SP: return cpu.sp;
            case Flags: return cpu.sr.reg;
            case V: return bus.ppu_v;
            case PPU_X: return bus.ppu_x;
            case T: return bus.ppu_t;
            default: return bus.ppu_w;
        }
    }


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
    void update_reg(Bus& bus, CPU& cpu, RegNum rn, unsigned short value){
        switch (rn) {
            case A:     {cpu.a = value & 0xff; break;}
            case CPU_X: {cpu.x = value & 0xff; break;}
            case Y:     {cpu.y = value & 0xff; break;}
            case PC:    {cpu.pc = value; break;}
            case SP:    {cpu.sp = value; break;}
            case V:     {bus.ppu_v = value; break;}
            case PPU_X: {bus.ppu_x = value & 0x7; break;}
            case T:     {bus.ppu_t = value; break;}
            case W:     {bus.ppu_w = (value); break;}
        }
    }
    void handle(CPU& cpu, PPU& ppu, Bus& bus) {
        u_input.rec = {DW::rec.x, DW::rec.y-30, w, 30};
        mirroring_switch_button = {DW::rec.x-30, DW::rec.y+DW::rec.height-30, 30, 30};
        sprite_off_button = {DW::rec.x-30, DW::rec.y+DW::rec.height, 30, 30};
        if (watch_status) watch_update_check(cpu, bus, ppu);

        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_M))
            mode = static_cast<Mode>((mode + 1) % ModeCount);
        if (CheckCollisionPointRec(GetMousePosition(), mirroring_switch_button) && IsMouseButtonPressed(0))
            bus.mapper.current_screen_mirroring =
                    static_cast<Mapper::ScreenMirroring>((bus.mapper.current_screen_mirroring + 1) % Mapper::ScreenMirroring::MirroringCount);
        if (CheckCollisionPointRec(GetMousePosition(), sprite_off_button) && IsMouseButtonPressed(0))
            ppu.is_sprite_off = !ppu.is_sprite_off;

        ss  << std::dec << " CPU Cycles count:" << cpu.cycles_count << '\n'
        << " CPU:" << (int)bus.ins_cycle << '\n'
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
                << " \n\n PPU:\n"
                << "  V:" << (unsigned)(bus.ppu_v) << '\n'
                << " Coarse(Y):" << (unsigned)((bus.ppu_v >> 5) & 0x1f) << " Fine(Y):" << (unsigned)((bus.ppu_v >> 12) & 0x7) << '\n'
                << "  NT:" << (unsigned)((bus.ppu_v >> 10) & 0x3) << '\n'
                << "  X:" << (unsigned)(bus.ppu_x) << '\n'
                << "  T:" << (unsigned)(bus.ppu_t) << '\n'
                << "  W:" << (unsigned)(bus.ppu_w) << '\n'
        << "  VBLank:" << (unsigned)((bus.ppustatus>> 7) & 1) << "   Bus VBLank:" << (unsigned)(bus.ppustatus & 0x80) << '\n'
        << "  OpenBus:" <<  (int)bus.ppu_open_bus_value << '\n'
        << std::dec << "  Cycles:" << ppu.current_scanline_cycle
        << "  Scanline:" << ppu.current_y << '\n'
        << " DrawX:" << ppu.current_x%256 << '\n'
        << " DrawY:" << ppu.current_y << '\n';
        ss << "\nScreenMirroring:";
        switch (bus.mapper.current_screen_mirroring) {
            case Mapper::ScreenMirroring::Horizontal: ss << "Horizontal"; break;
            case Mapper::ScreenMirroring::Vertical: ss << "Vertical"; break;
            case Mapper::ScreenMirroring::SingleScreenLower: ss << "SingleScreenLower"; break;
            case Mapper::ScreenMirroring::SingleScreenUpper: ss << "SingleScreenUpper"; break;
        }
        for (int i = 0; i < RegsNumCount; ++i){
            auto rn = static_cast<RegNum>(i);
            auto current_button_rec = Rectangle{DW::rec.x-20,DW::rec.y+set_button_coords_offset[i], 20, 10};
            if (mode == Set)
                DW::draw_text_button(current_button_rec,  "set", BLACK);
            else
                DW::draw_button(current_button_rec,   get_watch_status(rn) ? GREEN : RED);

            if (CheckCollisionPointRec(GetMousePosition(), current_button_rec) && mode == Set && IsMouseButtonPressed(0) && !u_input.s.empty())
                update_reg(bus, cpu,  rn, std::stoul(u_input.s, nullptr, 16));
            else if (CheckCollisionPointRec(GetMousePosition(), current_button_rec) && mode == Watch && IsMouseButtonPressed(0)){
                watch_status_switch(rn);
                if (get_watch_status(rn)){
                    previous_reg_values[i] = get_reg_value(rn, bus, ppu, cpu);
                    ppu.fill_white_background = false;
                }
            }
        }
        auto s = ss.str();
        DW::draw(s.c_str(), 15.f);
        DW::draw_text_button(mirroring_switch_button, "MS", BLACK);
        DW::draw_text_button(sprite_off_button, "S_OFF", BLACK);
        ss.str("");
        u_input.handle();
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
                ss << std::hex << (unsigned)bus.ppu_map[constants::IMAGE_PALETTE_BEGIN + i] << ' ';
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
                        unsigned char c_index = bus.unsafe_read_chr(constants::IMAGE_PALETTE_BEGIN + current_palette_index);
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
struct GeneralInfo: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    enum View: unsigned char {
        Bin, Hex, ViewCount
    } view = Bin;
    enum Mode: unsigned char {
        CartridgeM,
        PPUM,
        MMIORegsM,
        Interrupts,
#if GAME_GENIE_MODE
        GameGenieM,
#endif
        ModeCount
    } mode = CartridgeM;

    std::stringstream ss;
    void handle(Bus& bus, Cartridge& c, PPU& ppu, APU& apu){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_M))
            mode = static_cast<Mode>((mode + 1) % ModeCount);
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_ONE))
            view = static_cast<View>((view + 1) % ViewCount);

        switch (mode) {
            case CartridgeM: {
                ss << "CartridgeInfo:\n"
                   << "  MapperNum:" << std::dec << c.mapper_num << '\n'
                   << "  CHR_ROM_SZ:" << std::dec << c.chr_rom_sz << '\n'
                   << "  PRG_ROM_SZ:" << std::dec << c.prg_rom_sz << '\n'
                   << "  CHR RAM:"  << bus.mapper.is_chr_ram << '\n'
                   << "  Battery:" << c.is_battery << '\n'
                   << " Is alternative nt:" << c.is_alternative_nametable << '\n'
                   << " Is horizontal arrangement:" << c.is_horizontal_arrangement << '\n';

                switch (bus.mapper.mtype) {
                    case Mapper::MMC3:{
                        ss << "MMC3:\n" <<
                           "IRQ enable:" << (int) bus.mapper.mmc3.irq_enable << '\n' <<
                           "IRQ counter:" << (int) bus.mapper.mmc3.irq_counter << '\n' <<
                           "IRQ latch:" << (int) bus.mapper.mmc3.irq_latch << '\n' <<
                           "IRQ requested:" << (int) bus.mapper.mmc3.irq_requested << '\n' <<
                           "Reload flag:" << (int) bus.mapper.mmc3.reload_flag;
                        break;
                    }
                    case Mapper::AXROM: {
                        ss << "AXROM:\n" <<
                        "Bank:" << (int)bus.mapper.axrom.bank << '\n' <<
                        "MirrorMode:" << (int)bus.mapper.axrom.single_screen_nt << '\n';
                        break;
                    }
                }
                break;
            }
            case PPUM: {
                ss << "PPU:\n"
                   << "  FramesCount:" << std::dec << ppu.frames_count << '\n'
                   << "  LastLineX:" << std::dec << ppu.last_line_x << '\n'
                   << "  CurrentLineX:" << std::dec << ppu.current_line_x << '\n' << std::hex
                   << "  PPU_X:" << std::dec << (int)bus.ppu_x << '\n' << std::hex
                   << "  ShiftRegL:" << std::bitset<16>(ppu.shift_register_low).to_string() << '\n' << std::hex
                   << "  ShiftRegH:" << std::bitset<16>(ppu.shift_register_high).to_string() << '\n' << std::hex
                   << "  AttributesRegL:" << std::bitset<16>(ppu.attributes_low).to_string() << '\n' << std::hex
                   << "  AttributesRegH:" << std::bitset<16>(ppu.attributes_high).to_string() << '\n' << std::hex;
                break;
            }
            case MMIORegsM: {

                auto ppuctrl_v = bus.unsafe_read_prg(constants::PPUCTRL);
                auto ppumask_v = bus.unsafe_read_prg(constants::PPUMASK);
                auto ppustatus_v = bus.unsafe_read_prg(constants::PPUSTATUS);
                auto ppuoamaddr_v = bus.unsafe_read_prg(constants::OAMADDR);
                auto ppuoamdata_v = bus.unsafe_read_prg(constants::OAMDATA);
                auto ppuscroll_v = bus.unsafe_read_prg(constants::PPUSCROLL);
                auto ppuaddr_v = bus.unsafe_read_prg(constants::PPUADDR);
                auto ppudata_v = bus.unsafe_read_prg(constants::PPUDATA);

                if (view == Bin) {
                    ss << "MMIO:\n"
                       << "  PPUCTRL(0x2000):"     <<   std::bitset<8>(ppuctrl_v).to_string()  << '\n'
                       << "  PPUMASK(0x2001):"     <<   std::bitset<8>(ppumask_v).to_string()<< '\n'
                       << "  PPUSTATUS(0x2002):"   <<    std::bitset<8>(ppustatus_v).to_string() << '\n'
                       << "  OAMADDR(0x2003):"     <<   std::bitset<8>(ppuoamaddr_v).to_string() << '\n'
                       << "  OAMDATA(0x2004):"     <<    std::bitset<8>(ppuoamdata_v).to_string() << '\n'
                       << "  PPUSCROLL(0x2005):"   <<      std::bitset<8>(ppuscroll_v).to_string() << '\n'
                       << "  PPUADDR(0x2006):" <<        std::bitset<8>(ppuaddr_v).to_string()<< '\n'
                       << "  PPUDATA(0x2007):" <<        std::bitset<8>(ppudata_v).to_string()<< '\n';
                } else if (view == Hex) {
                    ss << "MMIO:\n" << std::hex
                       << "  PPUCTRL(0x2000):"     <<   (int)(ppuctrl_v)  << '\n'
                       << "  PPUMASK(0x2001):"     <<   (int)(ppumask_v)<< '\n'
                       << "  PPUSTATUS(0x2002):"   <<    (int)(ppustatus_v) << '\n'
                       << "  OAMADDR(0x2003):"     <<   (int)(ppuoamaddr_v) << '\n'
                       << "  OAMDATA(0x2004):"     <<    (int)(ppuoamdata_v)<< '\n'
                       << "  PPUSCROLL(0x2005):"   <<     (int)(ppuscroll_v) << '\n'
                       << "  PPUADDR(0x2006):" <<        (int)(ppuaddr_v) << '\n'
                       << "  PPUDATA(0x2007):" <<        (int)(ppudata_v) << '\n';
                }

                break;
            }
            case Interrupts: {
                ss << "Interrupts req:\n"
                   << "  APU_IRQ:"     <<  (int)bus.apu_irq_interrupt_flip_flop  << '\n'
                   << "  PPU_IRQ:"     <<  (int)bus.ppu_irq_interrupt_flip_flop  << '\n'
                   << "  NMI:"     <<  (int)bus.nmi_interrupt_flip_flop  << '\n'
                   << "DMA req:\n"
                   << "  DMC:"     <<  (int)bus.dmc_dma_in_progress  << '(' << bus.dmc_dma_counter << ")\n"
                   << "  OAM:"     <<  (int)bus.dma_active  << '\n';
                break;
            }
#if GAME_GENIE_MODE
            case GameGenieM: {
                ss << "GameGenieInfo:\n";
                for (int i = 0; i < 3; ++i){
                    if (!bus.gg.data[i].active) continue;
                    ss << "I:" << i << '\n'
                       << "  Address:"    <<  std::hex  <<   int(bus.gg.data[i].address) << '\n'
                       << "  Value:"      <<  std::hex  <<   int(bus.gg.data[i].value) << '\n';
                    if (bus.gg.data[i].compare)
                        ss << "  Compare:"      <<  std::hex  <<  int(bus.gg.data[i].compare_value) << '\n';
                }
                break;
            }
#endif
        }
        auto s = ss.str();
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsMouseButtonPressed(0))
            SetClipboardText(s.c_str());
        DW::draw(s.c_str(), 13.f);
        ss.str("");
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct MemoryViewer: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    unsigned short target_address{};
    unsigned short start_address{};
    static constexpr unsigned char lines_count = 10;
    float t_x = 0;
    float t_y = 0;
    std::stringstream ss;
    std::string tmp_str;
    Rectangle update_target_button{};
    Rectangle write_button{};
    Rectangle mode_button{};
    Rectangle pc_watch_address_button{};
    Rectangle dump_button{};
    Rectangle clear_sets_button{};

    UserInput<x, y - 30, w, 30> u_input;
    enum Mode {
        CPUm, PPUm, CIRAM, OAMm, ModeCount
    } mode = CPUm;
    std::string_view mode_str[3] = {
            "/Users/prazd/Desktop/books/cpp/cpp/vm/nes/build/CPU.dump",
            "/Users/prazd/Desktop/books/cpp/cpp/vm/nes/build/PPU.dump",
               "/Users/prazd/Desktop/books/cpp/cpp/vm/nes/build/CIRAM.dump",
    };
    MemoryViewer():
            update_target_button{DW::rec.x, DW::rec.y+DW::rec.height, 30,30},
            mode_button{DW::rec.x+30, DW::rec.y+DW::rec.height, 30,30},
            write_button{DW::rec.x+30*2, DW::rec.y+DW::rec.height, 30,30},
            pc_watch_address_button{DW::rec.x+30*3, DW::rec.y+DW::rec.height, 30,30},
            dump_button{DW::rec.x+30*4, DW::rec.y+DW::rec.height, 30,30},
            clear_sets_button{DW::rec.x+30*5, DW::rec.y+DW::rec.height, 30,30}
            {}

    void dump(Bus& bus, auto& r, std::string_view path, size_t sz){
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return;
        for (int i = 0; i != sz; ++i){
            switch(mode) {
                case PPUm: {file.put(bus.unsafe_read_chr(i)); break;}
                case CPUm: {file.put(bus.unsafe_read_prg(i)); break;}
                case CIRAM: {file.put(bus.ppu_map[0x2000 + i]); break;}
            }
        }
    }

    void mem_print(Bus& bus, unsigned char* ptr, size_t sz){
        unsigned short address = start_address & (sz-1);
        unsigned short cur_addr{};
       for (size_t i = 0; i < 16 * lines_count; ++i){
           cur_addr = (address + i) & (sz-1);
           if (!(i & 0xf)) {
               ss << ' ' << HEX_COUT(cur_addr, 4) << "| ";
           }
           if (cur_addr == target_address) ss << " [ ";
           ss << HEX_COUT_W2(ptr[cur_addr]) << ' ';
           if (cur_addr == target_address) ss << " ] ";
           if ((i & 0xf) == 0xf) ss << '\n';
       }
    }

    void mem_print_color(Bus& bus, auto& r, size_t sz){
        t_x = DW::rec.x;
        t_y = DW::rec.y;
        ss << std::hex;
        auto current_address = start_address;
        unsigned short adjusted_address = current_address;
        for (int i = 0; i < 16 * lines_count; ++i){
            adjusted_address = current_address = (start_address + i) & (sz-1);
            if (mode == PPUm)
                bus.adjust_ppu_mirroring_unsafe(adjusted_address);
            else
                bus.adjust_cpu_mirroring_unsafe(adjusted_address);
            if (!(i & 0xf)){
                ss << "\n";
                tmp_str = ss.str();
                DrawText(tmp_str.c_str(), t_x, t_y, 10.f, BLACK);
                ss.str("");
                ss << std::hex;
                ss << "  " <<  HEX_COUT(current_address, 4) << " | ";
                t_y += 15.f;
                t_x = DW::rec.x;
            }
            ss << ' ';
            if (current_address == target_address) ss << " [ ";
            // TODO
            bool is_updated = mode == PPUm ? (bus.ppu_read_set_addresses[adjusted_address] || bus.ppu_write_set_addresses[adjusted_address]) :
                                             (bus.cpu_read_set_addresses[adjusted_address] || bus.cpu_write_set_addresses[adjusted_address]);
            if (is_updated){
                tmp_str = ss.str();
                size_t sz = tmp_str.size();
                DrawText(tmp_str.c_str(), t_x, t_y, 10.f, BLACK);
                ss.str("");
                t_x += (sz * 5.f);

                if (mode == PPUm) {
                    ss << HEX_COUT_W2(bus.unsafe_read_chr(current_address)) << ' ';
                } else {
                    ss << HEX_COUT_W2(bus.unsafe_read_prg( current_address)) << ' ';
                }
                tmp_str = ss.str();
                bool is_write = mode == PPUm ? (bus.ppu_write_set_addresses[adjusted_address]) : (bus.cpu_write_set_addresses[adjusted_address]);
                DrawText(tmp_str.c_str(), t_x, t_y, 10.f, is_write ? RED : GREEN);
                sz = tmp_str.size();
                t_x += (sz * 5.f);
                ss.str("");
                ss << std::hex;
            } else {
                if (mode == PPUm) {
                    ss  << HEX_COUT_W2(bus.unsafe_read_chr(current_address)) << ' ';
                } else {
                    ss  << HEX_COUT_W2(bus.unsafe_read_prg( current_address)) << ' ';
                }
            }

            if (current_address == target_address) ss << " ] ";
        }
        ss << "\n ";
        tmp_str = ss.str();
        DrawText(tmp_str.c_str(), t_x, t_y, 10.f, BLACK);
        ss.str("");
    }

    void handle(Bus& bus, CPU& cpu, PPU& ppu){
        update_target_button = {DW::rec.x, DW::rec.y+DW::rec.height, 30,30};
        mode_button   = {DW::rec.x+30, DW::rec.y+DW::rec.height, 30,30};
        write_button = {DW::rec.x+30*2, DW::rec.y+DW::rec.height, 30,30};
        pc_watch_address_button = {DW::rec.x+30*3, DW::rec.y+DW::rec.height, 30,30};
        dump_button = {DW::rec.x+30*4, DW::rec.y+DW::rec.height, 30,30};
        u_input.rec = {DW::rec.x, DW::rec.y-30, DW::rec.width,30};

        if (CheckCollisionPointRec(GetMousePosition(), mode_button) && IsMouseButtonPressed(0))
            mode = static_cast<Mode>((mode + 1) % ModeCount);
        else if (CheckCollisionPointRec(GetMousePosition(), update_target_button) && IsMouseButtonPressed(0)){
            target_address = DW::hex_to_ushort(u_input.s, 16);
            start_address = target_address - (target_address % 16);
        } else if (CheckCollisionPointRec(GetMousePosition(), write_button) && IsMouseButtonPressed(0)) {
            if (mode == CPUm)
                bus.unsafe_write( target_address, std::stoul(u_input.s, nullptr, 16));
            else
                bus.unsafe_write_chr(target_address, std::stoul(u_input.s, nullptr, 16));
        } else if (CheckCollisionPointRec(GetMousePosition(), dump_button) && IsMouseButtonPressed(0)){
            switch (mode) {
                case PPUm: dump(bus, ppu, mode_str[mode], sizeof(bus.ppu_map)); break;
                case CPUm: dump(bus, cpu, mode_str[mode], sizeof(bus.cpu_map)); break;
                case CIRAM: dump(bus, cpu, mode_str[mode], 0x800); break;
                default: break;
            }
        } else if (CheckCollisionPointRec(GetMousePosition(), pc_watch_address_button) && IsMouseButtonPressed(0)){
            cpu.pc_watch_address = DW::hex_to_ushort(u_input.s, 16);
            cpu.pc_watch_count = 0;
        } else if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyDown(KEY_UP)){
            start_address = (start_address - 16) & 0xffff;
        } else if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyDown(KEY_DOWN)){
            start_address = (start_address + 16) & 0xffff;
        } else if (CheckCollisionPointRec(GetMousePosition(), clear_sets_button) && IsMouseButtonPressed(0)){
            bus.cpu_write_set_addresses.reset();
            bus.cpu_read_set_addresses.reset();
            bus.ppu_write_set_addresses.reset();
            bus.ppu_read_set_addresses.reset();
        }

        switch (mode){
            case CPUm: mem_print_color(bus,  cpu, sizeof(bus.cpu_map)); ss << "\nCPU"; break;
            case PPUm: mem_print_color(bus, ppu, sizeof(bus.ppu_map)); ss << "\nPPU"; break;
            case OAMm: {
                auto i = target_address * 4;
                auto e = i + (16 * 4);
                ss << "\n Start :" << target_address << " " << " End:" << target_address + 16 << '\n';
                ss << " Y    PT   AT   X\n";
                for (; i != e; i+=4)
                    ss << (int)bus.oam[i] << ' ' << (int)bus.oam[i+1] << ' ' << (int)bus.oam[i+2] << ' ' << (int)bus.oam[i+3] << '\n';
                ss << "OAM";
                tmp_str = ss.str();
                DrawText(tmp_str.c_str(), DW::rec.x,  DW::rec.y, 11.2, BLACK);
                ss.str("");
                ss << std::hex;
                break;
            }
            case CIRAM: {
                mem_print(bus, bus.ppu_map + 0x2000, 0x800);
                ss << "CIRAM\n";
                tmp_str = ss.str();
                DrawText(tmp_str.c_str(), DW::rec.x,  DW::rec.y, 11.2, BLACK);
                ss.str("");
                ss << std::hex;
                break;
            }
        }
        ss << '\n';
        ss << "\nPC watch address:" << cpu.pc_watch_address << " , count:"  << std::dec << cpu.pc_watch_count << '\n';
        auto s = ss.str();
        DrawText(s.c_str(), DW::rec.x,  DW::rec.y + h - 200, 11.2, BLACK);
        DW::draw_text_button(update_target_button, "update", BLACK);
        DW::draw_text_button(mode_button, "mode", BLACK);
        DW::draw_text_button(write_button, "write", BLACK);
        DW::draw_text_button(pc_watch_address_button, "pc_watch", BLACK);
        DW::draw_text_button(dump_button, "dump", BLACK);
        DW::draw_text_button(clear_sets_button, "clear", BLACK);
        u_input.handle();
        DW::draw_win();
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
            << " SpriteMode:" << (bus.sprite_mode == Bus::SpriteMode::m8x8 ? 8 : 16) << '\n'
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
            for (int i = 0, e = ppu.secondary_oam_buffer.size(); i != e; ++i){
                ss
                << (unsigned)ppu.secondary_oam_buffer[i].y_pos << ' '
                << (unsigned)ppu.secondary_oam_buffer[i].pt_index << ' '
                << ((unsigned)ppu.secondary_oam_buffer[i].attrib & 0xe3)<< ' '
                << (unsigned)ppu.secondary_oam_buffer[i].x_pos << ' '
                << (unsigned)ppu.secondary_oam_buffer[i].sprite_index << '\n';
            }
        } else if (info_version == FullOAMV) {
            ss << "OAM:\n";
            unsigned val{};
            for (int i = 0; i < 256; ++i){
                val = bus.oam[i] & ((i & 0x3) == 2 ? 0xe3 : 0xff);
                ss << val << ' ';
                if (i % 16 == 15) ss << '\n';
            }
        }
        auto s = ss.str();
        DW::draw(s.c_str(), 13.f);
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsMouseButtonPressed(0))
            SetClipboardText(s.c_str());
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
                        bool is_vertical_flip = (attrib >> 7) & 1;
                        bool is_horizontal_flip = ((attrib >> 6) & 1);

                        unsigned char updated_pt_index =
                                bus.sprite_mode ==  Bus::SpriteMode::m8x8 ? pt_index : (pt_index & 0xfe);

                        unsigned char current_index = updated_pt_index;
                        if (k < 8){
                            current_index += (bus.sprite_mode == Bus::SpriteMode::m8x16 && is_vertical_flip);
                        } else {
                            current_index += (!is_vertical_flip);
                        }

                        auto pt_base =
                                bus.sprite_mode == Bus::SpriteMode::m8x8 ?
                                       ((bus.ppuctrl >> 3) & 1 ? constants::PATTERN_TABLE1_BEGIN : constants::PATTERN_TABLE0_BEGIN) :
                                (pt_index & 1 ? constants::PATTERN_TABLE1_BEGIN : constants::PATTERN_TABLE0_BEGIN);

                        unsigned char shift_l = bus.unsafe_read_chr(pt_base + (current_index * 16) + (is_vertical_flip ? (7 - (k % 8)) : (k % 8)));
                        unsigned char shift_h = bus.unsafe_read_chr(pt_base + (current_index * 16) + (is_vertical_flip ? (7 - (k % 8)) : (k % 8)) + 8);

                        unsigned char attrib_high_bit = (attrib >> 3) & 1;
                        unsigned char attrib_low_bit = (attrib >> 2) & 1;
                        for (int l = 0; l < 8; ++l) {
                            auto res =
                                    is_horizontal_flip ?
                                    ((shift_h & 1) << 1)  | (shift_l  & 1)
                                                       : (((shift_h >> 7) & 1) << 1)  | (shift_l >> 7) & 1;
                            unsigned char index = (((attrib_high_bit << 1) | attrib_low_bit) << 2) | res;
                            unsigned char pixel_index = bus.unsafe_read_chr(constants::SPRITE_PALETTE_BEGIN + index);
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
                    if ((bus.sprite_mode == Bus::SpriteMode::m8x8 && i % 2 == 1) || bus.sprite_mode  == Bus::SpriteMode::m8x16) ++k;
                }
            }
            DW::handle();
        } else {
            for (int i = 0; i < 64; ++i) {
                ss <<  (unsigned)bus.oam[(i * 4) + 1] << " ";
                if (i % 8 == 7) ss << '\n';
            }
            auto s = ss.str();
            DrawText(s.c_str(), DW::rec.x, DW::rec.y, 12.3f, BLACK);
            ss.str("");
        }
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct InstructionFinder: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::stringstream ss;
    size_t begin = 0;
    size_t end = 0;
    unsigned short target_addr = 0;
    unsigned char target_val = 0;
    SimpleDeque<CPU::Instruction, 10> instr_search_list;
    inline static unsigned short instr_search_address = 0;
    enum Mode {
        Instruction,
        Value,
        ModeCount,
    } mode = Value;

    struct AsmInfo {
        unsigned short pc{};
        std::string_view mnemonic{};
        unsigned short operand{};
        unsigned short arg_addr{};
    };
    static constexpr size_t view_count = 16;
    UserInput<x, y - 30, w, 30> u_input;
    Rectangle search_button{DW::rec.x, DW::rec.y + h, 30, 30};
    Rectangle clear_button{DW::rec.x + 30, DW::rec.y + h, 30, 30};
    Rectangle next_button{DW::rec.x + 30 * 2, DW::rec.y + h, 30, 30};

    Rectangle prev_button{DW::rec.x + 30 * 3, DW::rec.y + h, 30, 30};
    Rectangle add_instr_button{DW::rec.x, DW::rec.y + h + 30, 30, 30};
    Rectangle clear_instr_button{DW::rec.x + 30, DW::rec.y + h + 30, 30, 30};
    std::vector<AsmInfo> instructions{};

    void handle(Bus& bus, CPU& cpu){
        u_input.rec =   {DW::rec.x, DW::rec.y - 30, w, 30};
        search_button = {DW::rec.x, DW::rec.y + h, 30, 30};
        clear_button =  {DW::rec.x + 30, DW::rec.y + h, 30, 30};
        next_button = {DW::rec.x + 30 * 2, DW::rec.y + h, 30, 30};
        prev_button = {DW::rec.x + 30 * 3, DW::rec.y + h, 30, 30};
        add_instr_button = {DW::rec.x, DW::rec.y + h + 30, 30, 30};
        clear_instr_button = {DW::rec.x + 30, DW::rec.y + h + 30, 30, 30};

        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_ONE))
            mode = static_cast<Mode>((mode + 1) % ModeCount);

        if (CheckCollisionPointRec(GetMousePosition(), clear_button) &&  IsMouseButtonPressed(0)){
            instructions.clear();
            begin = 0;
            end = 0;
        }

        if (CheckCollisionPointRec(GetMousePosition(), next_button) && IsMouseButtonPressed(0)){
            if (begin + view_count < instructions.size()){
                begin += view_count;
                end = std::min(end + view_count, instructions.size());
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), prev_button) && IsMouseButtonPressed(0)){
            if (begin >= view_count){
                begin -= view_count;
                end = std::min(begin + view_count, instructions.size());;
            }
        }

        if (mode == Instruction) {
            if (CheckCollisionPointRec(GetMousePosition(), add_instr_button) && IsMouseButtonPressed(0)){
                auto instructions_sv = DW::split_by_c(u_input.s, ' ');
                for (auto& isv: instructions_sv){
                    for (size_t i = 0, e = std::size(CPU::instruction_str); i < e; ++i){
                        if (isv == CPU::instruction_str[i]){
                            instr_search_list.push_back(static_cast<CPU::Instruction>(i));
                            break;
                        }
                    }
                }
            } else if (CheckCollisionPointRec(GetMousePosition(), clear_instr_button) && IsMouseButtonPressed(0)) {
                instr_search_list.reset();
            }
        }

        if (CheckCollisionPointRec(GetMousePosition(), search_button) && IsMouseButtonPressed(0)){
            if (mode == Value){
                instructions.clear();
                target_addr = std::stoul(u_input.s, 0, 16);
                // TODO: check
                DW::cpu_search_pc(bus, cpu, [target_addr=target_addr](auto lpc, auto& decoded, auto& asm_info){
                    return asm_info.arg_addr == target_addr;
                },
        [](auto lpc, auto& decoded, auto& asm_info, auto& instructions){
                instructions.push_back(
                    {(unsigned short)lpc, asm_info.mnemonic, asm_info.operand, asm_info.arg_addr});
                },
                instructions);
            } else if (mode == Instruction){
                DW::cpu_search_pc(bus, cpu, [instr_search_list=instr_search_list](auto lpc, auto& decoded, auto& asm_info){
                       static int current_ins = 0;
                       if (decoded.i == instr_search_list[current_ins]){
                            if (current_ins == 0) instr_search_address = lpc;
                           ++current_ins;
                       } else {
                           current_ins = 0;
                           instr_search_address = 0;
                       }
                       return current_ins >= instr_search_list.size();
                   },
                   [](auto lpc, auto& decoded, auto& asm_info, auto& instructions){
                       instructions.push_back(
                               {(unsigned short)instr_search_address, asm_info.mnemonic, asm_info.operand, asm_info.arg_addr});
                   },
                   instructions);

            }
            begin = 0;
            end = std::min(instructions.size(), view_count);
        }

        for (size_t i = begin ; i != end; ++i){
            ss  << std::hex << ' ' << instructions[i].pc;
            if (mode == Value) ss << ' ' <<  instructions[i].mnemonic;
            ss <<'\n';
        }

        if (mode == Instruction) {
            for (size_t i = 0, e = instr_search_list.size(); i < e; ++i)
                ss << CPU::instruction_str[instr_search_list[i]] << ',';
            ss << '\n';
        }

        u_input.handle();
        ss << "Res:" << instructions.size() << '\n';
        auto s = ss.str();
        DrawText(s.c_str(), DW::rec.x, DW::rec.y, 13.f, BLACK);
        DW::draw_text_button(search_button, "search", BLACK);
        DW::draw_text_button(clear_button, " clear", BLACK);
        DW::draw_text_button(next_button, " next", BLACK);
        DW::draw_text_button(prev_button, " prev", BLACK);
        if (mode == Instruction) {
            DW::draw_text_button(add_instr_button, " addi", BLACK);
            DW::draw_text_button(clear_instr_button, " cleari", BLACK);
        }
        ss.str("");
        DW::draw_win();
    }
};

template <size_t x, size_t y, size_t w, size_t h>
struct APUView: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    Rectangle active_chan_buttons[5]{
            {DW::rec.x, DW::rec.y + DW::rec.height, 30, 30},
            {DW::rec.x + 30, DW::rec.y + DW::rec.height, 30, 30},
            {DW::rec.x + 60, DW::rec.y + DW::rec.height, 30, 30},
            {DW::rec.x + 90, DW::rec.y + DW::rec.height, 30, 30},
            {DW::rec.x + 120, DW::rec.y + DW::rec.height, 30, 30}
    };
    Rectangle active_sound_button = {DW::rec.x + 150, DW::rec.y + DW::rec.height, 30, 30};
    bool active_sound = true;

    Rectangle info_buttons[5]{
            {DW::rec.x - 30, DW::rec.y, 30, 30},
            {DW::rec.x - 30, DW::rec.y + 30, 30, 30},
            {DW::rec.x - 30, DW::rec.y + 60, 30, 30},
            {DW::rec.x - 30, DW::rec.y + 90, 30, 30},
            {DW::rec.x - 30, DW::rec.y + 120, 30, 30}
    };

    enum InfoMode: unsigned char {
        P1, P2, T, N, D, InfoModeCount,
    } info_mode = P1;

    Bus::APUChan chans[5]{Bus::APUChan::Pulse1,
                              Bus::APUChan::Pulse2,
                              Bus::APUChan::Triangle,
                              Bus::APUChan::Noise,
                              Bus::APUChan::DMC};
    std::stringstream  ss;

    void handle(Bus& bus, APU& apu){
        if (CheckCollisionPointRec(GetMousePosition(), active_sound_button) && IsMouseButtonPressed(0)){
            active_sound = !active_sound;
            apu.internal_enable = active_sound ? 0x1f : 0;
        }
        for (size_t i = 0, e = std::size(active_chan_buttons); i != e; ++i){
            if (CheckCollisionPointRec(GetMousePosition(), active_chan_buttons[i]) && IsMouseButtonPressed(0))
                apu.internal_enable ^= chans[i];
            if (CheckCollisionPointRec(GetMousePosition(), info_buttons[i]) && IsMouseButtonPressed(0))
                info_mode = (InfoMode)i;
        }

        // TODO: add more info
        ss << "APU:\n"
           << "  Enable :"      <<  std::bitset<8>(bus.apu_enable).to_string() << '\n'
           << "  FC mode:"      <<  (int)bus.frame_counter_mode  << '\n';
        switch (info_mode) {
            case P1: {
                ss  << "  duty:"     <<  (int)apu.pulse_ch1.current_duty_step  << '\n'
                    << " length counter:"     <<  (int)apu.pulse_ch1.length_counter  << '\n'
                    << "  sweep:"     <<  (int)apu.pulse_ch1.sweep_shift  << '\n'
                    << "  sweep neg:"     <<  (int)apu.pulse_ch1.sweep_is_neg  << '\n'
                    << "  sweep period:"     <<  (int)apu.pulse_ch1.sweep_period  << '\n'
                    << "  timer:"     <<  (int)apu.pulse_ch1.timer  << '\n';
                break;
            }
            case P2: {
                ss  << "  duty:"     <<  (int)apu.pulse_ch2.current_duty_step  << '\n'
                    << "  length counter:"     <<  (int)apu.pulse_ch2.length_counter  << '\n'
                    << "  sweep:"     <<  (int)apu.pulse_ch2.sweep_shift  << '\n'
                    << "  sweep neg:"     <<  (int)apu.pulse_ch2.sweep_is_neg  << '\n'
                    << "  sweep period:"     <<  (int)apu.pulse_ch2.sweep_period  << '\n'
                    << "  timer:"     <<  (int)apu.pulse_ch2.timer  << '\n';
                break;
            }
            case T: {break;}
            case N: {break;}
            case D: {
                ss << "  Rate counter:"    << std::hex <<  (int)bus.dmc_rate_counter << '\n'
                << "  Loop:"    << std::hex <<  (int)apu.dmc_ch.is_loop << '\n'
                << "  Output level:"    << std::hex <<  (int)bus.dmc_output_level << '\n'
                << "  IRQ enable:"    << std::hex <<  (int)apu.dmc_ch.irq_enable << '\n'
                << "  Bytes remaining:"    << std::hex <<  (int)bus.dmc_bytes_remaining << '\n'
                << "  DMA address:"   << std::hex <<  (int)bus.dmc_dma_address  << '\n';
                break;
            }
        }

        auto s = ss.str();
        DrawText(s.c_str(), DW::rec.x, DW::rec.y, 13.f, BLACK);
        DW::draw_text_button(active_chan_buttons[0], " P1", apu.internal_enable & chans[0] ? GREEN : RED);
        DW::draw_text_button(active_chan_buttons[1], " P2", apu.internal_enable & chans[1] ? GREEN : RED);
        DW::draw_text_button(active_chan_buttons[2], " T", apu.internal_enable & chans[2] ? GREEN : RED);
        DW::draw_text_button(active_chan_buttons[3], " N", apu.internal_enable & chans[3] ? GREEN : RED);
        DW::draw_text_button(active_chan_buttons[4], " D", apu.internal_enable & chans[4] ? GREEN : RED);
        DW::draw_text_button(active_sound_button, " ON", active_sound ? GREEN : RED);

        DW::draw_text_button(info_buttons[0], " P1", info_mode == P1 ? GREEN : BLACK);
        DW::draw_text_button(info_buttons[1], " P2", info_mode == P2 ? GREEN : BLACK);
        DW::draw_text_button(info_buttons[2], " T", info_mode == T ? GREEN : BLACK);
        DW::draw_text_button(info_buttons[3], " N", info_mode == N ? GREEN : BLACK);
        DW::draw_text_button(info_buttons[4], " D", info_mode == D ? GREEN : BLACK);

        ss.str("");
        DW::draw_win();
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

template <size_t x, size_t y, size_t w, size_t h>
struct CustomController: DebugWindowText<x,y,w,h> {
    using DW = DebugWindowText<x,y,w,h>;
    const char* dir_buttons_text[4]={" u"," d", " l", " r"};
    const char* select_buttons_text[4]={" a"," b", " s", " e"};
    std::stringstream ss;

    enum Mode {
        Pressed, Down, ModeCount
    } mode = Down;

    Rectangle direction_buttons{DW::rec.x, DW::rec.y-30,30,30};
    Rectangle select_buttons{DW::rec.x, DW::rec.y+h,30,30};

    void handle(Controller& c){
        direction_buttons = {DW::rec.x, DW::rec.y-30,30,30};
        select_buttons    = {DW::rec.x, DW::rec.y+h,30,30};
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec) && IsKeyPressed(KEY_M)){
            mode = static_cast<Mode>((mode + 1) % ModeCount);
        }

        auto key_handle = [](const Rectangle& rec, Controller& c, int i,  bool dir, Mode mode) {
            if (CheckCollisionPointRec(GetMousePosition(), rec)){
                if (mode == Pressed && IsMouseButtonPressed(0)){
                    c.cs_switch(c, i, dir);
                } else if (mode == Down) {
                    c.cs_set(c, i, dir, IsMouseButtonDown(0));
                }
            }
        };

        for (int i = 0; i < 4; ++i){
            auto d = Rectangle{direction_buttons.x + (i * direction_buttons.width), direction_buttons.y, direction_buttons.width, direction_buttons.height};
            key_handle(d, c, i, true, mode);
            DW::draw_text_button(d, dir_buttons_text[i], c.cs_read(c, i, true) ? GREEN : RED);

            auto s = Rectangle{select_buttons.x + (i * direction_buttons.width), select_buttons.y, select_buttons.width, select_buttons.height};
            key_handle(s, c, i, false, mode);
            DW::draw_text_button(s, select_buttons_text[i], c.cs_read(c, i, false) ? GREEN : RED);
        }
        ss << "Mode:" << (mode == Down ? "Down": "Pressed");
        auto s = ss.str();
        DrawText(s.c_str(), DW::rec.x, DW::rec.y, 12.f, BLACK);
        ss.str("");
        DW::draw_win();
    }
};

#if GAME_GENIE_MODE
template <size_t x, size_t y, size_t w, size_t h>
struct CustomGameGenie: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;
    std::stringstream ss;
    static constexpr size_t cheat_sz = 3;
    UserInput<x, y - 30, w, 30> u_input;

    std::array<int, 6> cheat_6_arr;
    std::array<int, 8> cheat_8_arr;

    Rectangle add_button{DW::rec.x, DW::rec.y + h, 30, 30};
    Rectangle clear_button{DW::rec.x , DW::rec.y, 30, 30};

    void handle(Bus& bus){
        u_input.rec  =   {DW::rec.x, DW::rec.y - 30, w, 30};
        clear_button =   {DW::rec.x, DW::rec.y+5, 30, 30};
        add_button   =   {DW::rec.x, DW::rec.y + h, 30, 30};

        if (CheckCollisionPointRec(GetMousePosition(), add_button) && IsMouseButtonPressed(0)){
                if(bus.gg.data.size() < cheat_sz) {
                    if (u_input.s.size() == 6){
                        for (int j = 0; j < 6; ++j){
                            if (auto s = GameGenie::get_syms_by_str(u_input.s[j]); s != GameGenie::Unknown){
                                cheat_6_arr[j] = s;
                            } else {
                                break;
                            }
                        }
                        auto r = bus.gg.custom_decode(cheat_6_arr);
                        r.str = u_input.s;
                        bus.gg.data.push_back(r);
                    } else {
                        for (int j = 0; j < 8; ++j){
                            if (auto s = GameGenie::get_syms_by_str(u_input.s[j]); s != GameGenie::Unknown){
                                cheat_8_arr[j] = s;
                            } else {
                                break;
                            }
                        }
                        auto r = bus.gg.custom_decode(cheat_8_arr);
                        r.str = u_input.s;
                        bus.gg.data.push_back(r);
                    }
            }
        }

        ss << '\n';
        for (int i = 0, e = bus.gg.data.size();i < e; ++i){
            auto r = Rectangle{clear_button.x-30, clear_button.y + (i * 42), 30,30};
            DW::draw_text_button(r, "del", RED);
            ss << std::hex << ' ' << bus.gg.data[i].str << " Address: " << bus.gg.data[i].address << " Value:" << (int)bus.gg.data[i].value;
            (bus.gg.data[i].compare ? (ss << " Compare:" << (int)bus.gg.data[i].compare_value) : ss);
            ss << "\n\n\n";
            if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(0)){
                auto t = std::remove(bus.gg.data.begin(),  bus.gg.data.end(), bus.gg.data[i]);
                bus.gg.data.erase(t, bus.gg.data.end());
            }
        }

        DW::draw_text_button(add_button, "add", BLACK);
        auto s = ss.str();
        DrawText(s.c_str(), DW::rec.x, DW::rec.y, 12.f, BLACK);
        u_input.handle();
        ss.str("");
        DW::draw_win();
    }
};
#endif

template <size_t x, size_t y, size_t w, size_t h>
struct Snapshot: DebugWindowText<x, y, w, h> {
    using DW = DebugWindowText<x, y, w, h>;

    enum SnapValue {
        CPU_X, CPU_Y, CPU_A,
        CPU_FLAG, PPU_V, PPU_T, PPU_X,
        PPU_SHIFT_L,
        PPU_SHIFT_H,
        PPU_CTRL_REG,
        PPU_STATUS_REG,
        SnapValuesCount
    };

    static constexpr std::string_view snap_values_str[SnapValuesCount] = {
        "CPU_X", "CPU_Y", "CPU_A",
        "CPU_FLAG", "PPU_V", "PPU_T", "PPU_X",
        "PPU_SHIFT_L", "PPU_SHIFT_H",
        "PPU_CTRL",
        "PPU_STATUS",
    };

    unsigned short s1[SnapValuesCount]{};
    unsigned short s2[SnapValuesCount]{};

    CPU cpu_s;
    PPU ppu_s;

    std::chrono::system_clock::time_point cpu_saved_time;
    std::chrono::system_clock::time_point ppu_saved_time;

    std::stringstream ss;
    bool first = true;

    Rectangle take_fast_snapshot{DW::rec.x, DW::rec.y + h, 30, 30};
    Rectangle clear_s{DW::rec.x + 30, DW::rec.y + h, 30, 30};
    Rectangle cpu_snap{DW::rec.x + 30 * 2, DW::rec.y + h, 30, 30};
    Rectangle cpu_restore{DW::rec.x + 30 * 3, DW::rec.y + h, 30, 30};
    Rectangle ppu_snap{DW::rec.x + 30 * 4, DW::rec.y + h, 30, 30};
    Rectangle ppu_restore{DW::rec.x + 30 * 5, DW::rec.y + h, 30, 30};

    Rectangle both_snap{DW::rec.x + 30 * 6, DW::rec.y + h, 30, 30};
    Rectangle both_restore{DW::rec.x + 30 * 7, DW::rec.y + h, 30, 30};

    void handle(Bus& bus, CPU& cpu, PPU& ppu){
        take_fast_snapshot =   {DW::rec.x, DW::rec.y + h, 30, 30};
        clear_s     =  {DW::rec.x + 30, DW::rec.y + h, 30, 30};
        cpu_snap    =  {DW::rec.x + 30 * 2, DW::rec.y + h, 30, 30};
        cpu_restore =  {DW::rec.x + 30 * 3, DW::rec.y + h, 30, 30};
        ppu_snap    =  {DW::rec.x + 30 * 4, DW::rec.y + h, 30, 30};
        ppu_restore =  {DW::rec.x + 30 * 5, DW::rec.y + h, 30, 30};
        both_snap = {DW::rec.x + 30 * 6, DW::rec.y + h, 30, 30};
        both_restore = {DW::rec.x + 30 * 7, DW::rec.y + h, 30, 30};

        if (CheckCollisionPointRec(GetMousePosition(), take_fast_snapshot) && IsMouseButtonPressed(0)){
            auto& s = (std::exchange(first, !first)) ? s1 : s2;
            s[CPU_X] = cpu.x;
            s[CPU_Y] = cpu.y;
            s[CPU_A] = cpu.a;
            s[CPU_FLAG] = cpu.sr.reg;
            s[PPU_V] = bus.ppu_v;
            s[PPU_T]=  bus.ppu_t;
            s[PPU_X] = bus.ppu_x;
            s[PPU_SHIFT_L] = ppu.shift_register_low;
            s[PPU_SHIFT_H] = ppu.shift_register_high;
            s[PPU_CTRL_REG] = bus.ppuctrl;
            s[PPU_STATUS_REG] = bus.ppustatus;
        } else if (CheckCollisionPointRec(GetMousePosition(), clear_s) && IsMouseButtonPressed(0)){
            std::memset(s1, 0 , sizeof(s1));
            std::memset(s2, 0 , sizeof(s2));
            first = true;
        } else if (CheckCollisionPointRec(GetMousePosition(), cpu_snap) && IsMouseButtonPressed(0)){
            cpu_s = cpu;
            cpu_saved_time = std::chrono::system_clock::now();
        } else if (CheckCollisionPointRec(GetMousePosition(), cpu_restore) && IsMouseButtonPressed(0)){
            cpu = cpu_s;
        } else if (CheckCollisionPointRec(GetMousePosition(), ppu_snap) && IsMouseButtonPressed(0)){
            ppu_s = ppu;
// debug

//            std::vector<char> res;
//            res.resize(bus.mapper.chr.size());
//            memcpy(res.data(), bus.mapper.chr.data(), 0x2000);
//            auto res = bus.mapper.chr;
//            memcpy(ppu_s.mem.map + 0x3f00, bus.ppu_map + 0x3f00, 0xff);
//            ppu_s.run = ppu.run;
//            ppu_s.fill_white_background = ppu.fill_white_background;
//            static bool f = true;
//            DW::save_data_to_file(f ?  (f = false, "dump") : (f = true, "dump2"), (char*)(res.data()), res.size());
            ppu_saved_time = std::chrono::system_clock::now();
        } else if (CheckCollisionPointRec(GetMousePosition(), ppu_restore) && IsMouseButtonPressed(0)){
            ppu = ppu_s;
        } else if (CheckCollisionPointRec(GetMousePosition(), both_snap) && IsMouseButtonPressed(0)){
            cpu_s = cpu;
            cpu_saved_time = std::chrono::system_clock::now();
            ppu_s = ppu;
            ppu_saved_time = std::chrono::system_clock::now();
        } else if (CheckCollisionPointRec(GetMousePosition(), both_restore) && IsMouseButtonPressed(0)){
            cpu = cpu_s;
            ppu = ppu_s;
        }

        auto draw_diff = [](auto& ss, auto text, auto a, auto b, int rx, int ry){
            ss << ' ' << text << ':' << HEX_COUT(a, 4) << "         " << text << ':' << HEX_COUT(b, 4);
            auto s = ss.str();
            DrawText(s.c_str(), rx, ry,12.f, a == b ? BLACK : RED);
            ss.str("");
        };

        for (int i = 0; i < SnapValuesCount; ++i)
            draw_diff(ss,snap_values_str[i], s1[i], s2[i], DW::rec.x, DW::rec.y  + (15 * i));

        DW::draw_text_button(take_fast_snapshot, "snap", BLACK);
        DW::draw_text_button(clear_s, "clear1", BLACK);
        DW::draw_text_button(cpu_snap, "cpu_s", BLACK);
        DW::draw_text_button(cpu_restore, "cpu_r", BLACK);
        DW::draw_text_button(ppu_snap, "ppu_s", BLACK);
        DW::draw_text_button(ppu_restore, "ppu_r", BLACK);
        DW::draw_text_button(both_snap, "both_s", BLACK);
        DW::draw_text_button(both_restore, "both_r", BLACK);

        ss << "CPU saved:" << cpu_saved_time.time_since_epoch().count() << '\n';
        ss << "PPU saved:" << ppu_saved_time.time_since_epoch().count() << '\n';

        auto s = ss.str();
        DrawText(s.c_str(), DW::rec.x, DW::rec.y  + (15 * SnapValuesCount), 12.f, BLACK);
        ss.str("");
        DW::draw_win();
    }
};

#endif
#endif //NES_DEBUG_HPP

#ifndef NES_DEBUG_H
#define NES_DEBUG_H
#include "raylib.h"
#include "PPU.hpp"
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
        DW::draw_win();
        UpdateTexture(target, buffer.data());
        DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{texture_rec.x, texture_rec.y, w * 1.25f, h * 1.25f}, {0, 0}, 0.0f, WHITE);
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

#endif //NES_DEBUG_H

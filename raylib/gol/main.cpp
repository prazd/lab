#include <iostream>
#include <vector>
#include "raylib.h"

constexpr size_t DISPLAY_HEIGHT = 1000;
constexpr size_t DISPLAY_WIDTH = 1000;
size_t HEIGHT = 100;
size_t WIDTH = 100;

struct Cell {
    //     [4] state     
    // 0 0 0 1 ' 1 0 0 0
    //             [0-3] alive neighbours count(0-8)
    unsigned char state{};
    void set_ncount(unsigned char count){
        state &= 0xf0;
        state |= (count & 0xf);
    }
    void set_state(bool s){
        state &= 0xef;
        state |= (s << 4);
    }

    void next_gen() {
        bool alive = (state & 0x10);
        unsigned char alive_neighbours = state & 0xf;
        if (!alive && alive_neighbours == 3) {
            set_state(true);
        } else if (alive && (alive_neighbours < 2 || alive_neighbours > 3)) {
            set_state(false);
        }
    }
};

struct Field {
    std::vector<std::vector<Cell>> cells;
    void count_neighbors(size_t i, size_t j){
        unsigned char res = 0;
        res += bool(cells[i][(j - 1 + WIDTH) % WIDTH].state & 0x10);
        res += bool(cells[i][(j + 1) % WIDTH].state & 0x10);
        res += bool(cells[(i - 1 + HEIGHT) % HEIGHT][j].state & 0x10);
        res += bool(cells[(i + 1) % HEIGHT][j].state & 0x10);

        res += bool(cells[(i - 1 + HEIGHT) % HEIGHT][(j - 1 + WIDTH) % WIDTH].state & 0x10);
        res += bool(cells[(i - 1 + HEIGHT) % HEIGHT][(j + 1) % WIDTH].state & 0x10);
        res += bool(cells[(i + 1) % HEIGHT][(j - 1 + WIDTH) % WIDTH].state & 0x10);
        res += bool(cells[(i + 1) % HEIGHT][(j + 1) % WIDTH].state & 0x10);
        cells[i][j].set_ncount(res);
    }

    void resize(){
        cells.resize(HEIGHT);
        for (auto& r: cells) r.resize(WIDTH);
    }

    Field(){resize();}

    void next() {
        for (size_t i = 0; i < HEIGHT; ++i)
            for (size_t j = 0; j < WIDTH; ++j)
                count_neighbors(i, j);
                

        for(auto& row: cells)
            for(auto& c: row)
                c.next_gen();
    }
};

int main(){
    bool pause = true;

    InitWindow(DISPLAY_WIDTH, DISPLAY_HEIGHT, "GoL");
    Field f;

    float CELL_SZ_X = DISPLAY_WIDTH/WIDTH;
    float CELL_SZ_Y = DISPLAY_HEIGHT/HEIGHT;

     RenderTexture2D target = LoadRenderTexture(DISPLAY_WIDTH, DISPLAY_HEIGHT);
     auto update_texture_state = [&]{
        BeginTextureMode(target);
        ClearBackground(BLACK);
        float x = 0.f;
        float y = 0.f;
        for(auto& row: f.cells) {
            for(auto& c: row){
                DrawRectangle(x, y, CELL_SZ_X * 0.8f, CELL_SZ_Y * 0.8f, c.state & 0x10 ? BLACK : WHITE);
                x += CELL_SZ_X;
            }
            x = 0.f;
            y += CELL_SZ_Y;
        }
        EndTextureMode();
     };

     bool mode = true;

    while (!WindowShouldClose())
    {
        if (!pause) f.next();

        update_texture_state();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, static_cast<float>(target.texture.width), static_cast<float>(-target.texture.height)}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();

        if (IsKeyPressed(KEY_P)){
            pause = !pause;
        }
        if (IsKeyPressed(KEY_M)) {
            mode = !mode;
        }

        if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_W)){
            HEIGHT = std::min(HEIGHT + (IsKeyPressed(KEY_Q) ? 10 : -10), DISPLAY_HEIGHT/2);
            WIDTH = std::min(WIDTH + (IsKeyPressed(KEY_Q) ? 10 : -10), DISPLAY_WIDTH/2);
            CELL_SZ_X = DISPLAY_WIDTH/WIDTH;
            CELL_SZ_Y = DISPLAY_HEIGHT/HEIGHT;
            f.resize();
        }
            
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            size_t x = GetMouseX()/CELL_SZ_X;
            size_t y = GetMouseY()/CELL_SZ_Y;
            if (bool(f.cells[y][x].state & 0x10) != mode)
                f.cells[y][x].set_state(mode);
        }
    }
    
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
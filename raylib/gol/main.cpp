#include <iostream>
#include "raylib.h"

constexpr int CELL_SZ = 10;
constexpr int CELL_STEP = 5;
constexpr int STEP = CELL_STEP + CELL_SZ;
constexpr int HEIGHT = 40;
constexpr int WIDTH = 80;

struct Point {
    int x,y;
    Color color;
};

struct Cell {
    bool alive{};
    size_t alive_neighbours{};
    Point p;
    Cell* neighbours[8];

    void prepare() {
        size_t res{};
        for (auto el: neighbours) {
            if (el != nullptr && el->alive){
                ++res;
            }
        }
        alive_neighbours = res;
    }

    void next_gen() {
        if (!alive && alive_neighbours == 3) {
            set_state(true);
        } else if (alive && (alive_neighbours < 2 || alive_neighbours > 3)) {
            set_state(false);
        }
    }

    void set_state(bool state) {
        alive = state;
        if (alive) {
            p.color = BLACK;
        } else {
            p.color = WHITE;
        }
    }
};

template <size_t height, size_t width>
struct Field {
    static inline Cell cells[height][width];
    Field(){
        float current_x = 0.f;
        float current_y = 0.f;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                cells[i][j].neighbours[0] = &cells[i][(j - 1 + width) % width];
                cells[i][j].neighbours[1] = &cells[i][(j + 1) % width];
                cells[i][j].neighbours[2] = &cells[(i - 1 + height) % height][j];
                cells[i][j].neighbours[3] = &cells[(i + 1) % height][j];

                cells[i][j].neighbours[4] = &cells[(i - 1 + height) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[5] = &cells[(i - 1 + height) % height][(j + 1) % width];
                cells[i][j].neighbours[6] = &cells[(i + 1) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[7] = &cells[(i + 1) % height][(j + 1) % width];


                cells[i][j].p.color = BLACK;
                cells[i][j].p.x = current_x;
                cells[i][j].p.y = current_y;
                current_x += STEP;
            }
            current_x = 0;
            current_y += STEP;
        }
    }
    void next() {
        for(auto& row: cells)
            for(auto& c: row)
                c.prepare();
        for(auto& row: cells)
            for(auto& c: row)
                c.next_gen();
    }
};

template <typename T>
Cell* get_cell_by_mouse(T& f, int x, int y) {
    for(auto& r: f.cells){
        auto e = std::find_if(std::begin(r), std::end(r), [x,y](auto& el){
            return (x >= el.p.x && x < el.p.x + CELL_SZ) &&
                   (y >= el.p.y && y < el.p.y + CELL_SZ);
        });
        if (e != std::end(r)){
            return &(*e);
        }
    }
    return nullptr;
};


int main(){
    bool pause = true;

    InitWindow(800, 600, "GoL");
    Field<HEIGHT, WIDTH> f;

     RenderTexture2D target = LoadRenderTexture(800, 600);
     auto update_texture_state = [&]{
        BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        for(auto& row: f.cells) {
            for(auto& c: row){
                DrawRectangle(c.p.x, c.p.y, CELL_SZ, CELL_SZ, c.p.color);
                auto line_ex_color = c.alive ? BLACK : WHITE;
                DrawRectangleLinesEx((Rectangle){(float)c.p.x, (float)c.p.y, CELL_SZ, CELL_SZ}, CELL_STEP, line_ex_color);
            }
        }
        EndTextureMode();
     };
     auto switch_fps = [&]{
          enum class FPSMode: unsigned char {slow = 10, medium = 30, fast = 255};
          static FPSMode mode = FPSMode::slow;
          switch(mode){
            case FPSMode::slow:   {mode = FPSMode::medium; SetTargetFPS(static_cast<int>(FPSMode::medium)); break;}
            case FPSMode::medium: {mode = FPSMode::fast;   SetTargetFPS(static_cast<int>(FPSMode::fast));   break;}
            case FPSMode::fast:   {mode = FPSMode::slow;   SetTargetFPS(static_cast<int>(FPSMode::slow));   break;}
          }
     };

     Cell* last_found_cell = nullptr;
     bool mode = true;

    while (!WindowShouldClose())
    {
        if (!pause) {
            f.next();
            update_texture_state();
        }

        update_texture_state();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, static_cast<float>(target.texture.width), static_cast<float>(-target.texture.height)}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();

        if (IsKeyPressed(KEY_SPACE)) {
            switch_fps();
        }
        if (IsKeyPressed(KEY_P)){
            pause = !pause;
        }
        if (IsKeyPressed(KEY_M)) {
            mode = !mode;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            auto cell = get_cell_by_mouse(f, GetMouseX(), GetMouseY());
            if (cell && cell != last_found_cell){
                cell->set_state(mode);
                last_found_cell = cell;
            }
        }
    }
    
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
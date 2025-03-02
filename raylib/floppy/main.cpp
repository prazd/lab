#include "raylib.h"
#include <math.h>
#include <array>
#include <cassert>
#include <iostream>

constexpr float PLAYER_VEL_X = 220;
constexpr float PLAYER_VEL_Y = 100;
constexpr float HEIGHT = 600;
constexpr float WIDTH = 1200;
constexpr float WINDOW_SZ = 120.f;

template <size_t n>
struct Walls {
    Vector2 velocity{100,200};
    std::array<std::pair<Vector2, Vector2>, n> walls;
    Walls() {
        float start_x = 500.f;
        float start_y = 0.f;
        for (auto& el: walls) {
            el.first.x = start_x;
            el.first.y = start_y;
            auto r = (rand() % 250);
            el.second.x = el.first.x;
            el.second.y = el.first.y + r;
            start_x += 400.f;
        }
    }
    //todo: finish
    void move_left(float dt) {
        for (auto& el: walls){
            if (el.first.x <= -100.f) {
                el.first.x = WIDTH + 300.f;
                el.first.y = 0;
                float sy = 0;
                if (walls.back().first.y > HEIGHT - 10) {
                    sy = (walls.back().first.y + (rand() % 250)) - WINDOW_SZ;
                } else if (walls.back().first.y < 10) {
                    sy = (walls.back().first.y + (rand() % 250)) + WINDOW_SZ;
                } else {
                    sy = (rand() % 2) ? (walls.back().first.y + (rand() % 250)) - WINDOW_SZ : (walls.back().first.y + (rand() % 250)) + WINDOW_SZ;
                }
                el.second.x = el.first.x;
                el.second.y = sy;
                continue;
            }
            el.first.x -= velocity.x * dt;
            el.second.x -= velocity.x * dt;
        }
    }
};

struct Player {
    Vector2 pos{100,100};
    Vector2 velocity{PLAYER_VEL_X, PLAYER_VEL_Y};
    void move_up(float dt) {
        pos.y -= velocity.x * dt;
    }
    void move_down(float dt) {
        pos.y += velocity.y * dt;
    }
};

int main() {
    InitWindow(WIDTH, HEIGHT, "Floppy");
    SetTargetFPS(60);
    Player p;
    Walls<4> w;
    bool running = true;
    auto target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            auto dt = GetFrameTime();
            if (!running){
                DrawText("Game over. Press R to restart", (WIDTH/2)-200, HEIGHT/2, 30, BLACK);
                if (IsKeyPressed(KEY_R)) {
                    p = Player{};
                    w = Walls<4>{};
                    running = true;
                }
                EndTextureMode();
                return;
            }
            w.move_left(dt);
            if (IsKeyPressed(KEY_SPACE) || IsKeyDown(KEY_SPACE))
                p.move_up(dt);
            p.move_down(dt);

            for(auto& el: w.walls) {
                DrawRectangleRec((Rectangle){el.first.x, el.first.y, 100, el.second.y}, GRAY);
                DrawRectangleRec((Rectangle){el.second.x, el.second.y+WINDOW_SZ, 100, HEIGHT}, GRAY);
            }

            auto next_wall = std::find_if(w.walls.begin(), w.walls.end(), [&](auto& el){return std::abs(el.first.x - p.pos.x) <= 150;});
            if (next_wall != w.walls.end()) {
                DrawRectangleLinesEx((Rectangle){next_wall->first.x, next_wall->first.y, 100, next_wall->second.y}, 1, BLUE);
                DrawRectangleLinesEx((Rectangle){next_wall->second.x, next_wall->second.y+WINDOW_SZ, 100, HEIGHT - next_wall->second.y - WINDOW_SZ}, 1, RED);
                if (CheckCollisionRecs((Rectangle){p.pos.x, p.pos.y, 60,60},
                                       (Rectangle){next_wall->first.x, next_wall->first.y, 100, next_wall->second.y}) ||
                    CheckCollisionRecs((Rectangle){p.pos.x, p.pos.y, 60,60}, (Rectangle){next_wall->second.x, next_wall->second.y+WINDOW_SZ, 100, HEIGHT - next_wall->second.y - WINDOW_SZ})
                ){
                    running = false;
                }
            }
      
            DrawCircle(p.pos.x+30, p.pos.y + 30, 35, RED);
            // DrawRectangleRec((Rectangle){p.pos.x, p.pos.y, 60,60}, BLACK);
     
        EndTextureMode();
    };

    while(!WindowShouldClose()){
        render();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(1000, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
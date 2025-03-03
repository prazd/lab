#include "raylib.h"
#include <math.h>
#include <array>
#include <cassert>
#include <iostream>

constexpr float PLAYER_VEL_X = 300;
constexpr float PLAYER_VEL_Y = 150;
constexpr float HEIGHT = 600;
constexpr float WIDTH = 1200;
constexpr float WINDOW_SZ = 120.f;
constexpr float WALL_VELOCITY = 100.f;
constexpr float WALL_DISTANCE = 400.f;
constexpr float MAX_UP_DOWN = 15.f;

template <size_t n>
struct Walls {
    using WallPair = std::pair<Vector2, Vector2>;

    std::array<WallPair, n> walls;
    size_t current_last = n - 1;

    Walls() {
        float start_x = 500.f;
        float start_y = 0.f;
        // genesis
        auto& el = walls[0];
        el.first.x  = start_x;
        el.first.y = start_y;
        auto r = (rand() % 250);
        el.second.x = el.first.x;
        el.second.y = el.first.y + r;
        for (auto it = std::next(walls.begin()), ite = walls.end(); it != ite; ++it)
            *it = gen_next_wall(*std::prev(it));
    }

    size_t get_last() {
        return current_last++ % n;
    }

    float get_win_y_by_last(float l) {
        auto down_gen = [](float l) {
            return (l + (rand() % int(HEIGHT - l - WINDOW_SZ)));
        };

        auto up_gen = [](float l) {
            return  (l - (rand() % int((l - WINDOW_SZ))));
        };

        if (HEIGHT - l <= WINDOW_SZ) {
            return up_gen(l);
        } else if (l - WINDOW_SZ <= WINDOW_SZ) {
            return down_gen(l);
        }
        // todo: update
        return rand() % 2 ? down_gen(l) : up_gen(l);
    }

    WallPair gen_next_wall(const WallPair& wp) {
        WallPair el;
        el.first.x = wp.first.x + WALL_DISTANCE;
        el.first.y = 0;
        el.second.x = el.first.x;
        el.second.y = get_win_y_by_last(wp.second.y);
        return el;
    }

    void move_left(float dt) {
        for (auto& el: walls){
            if (el.first.x <= -100.f) {
                el = gen_next_wall(walls[get_last()]);
                continue;
            }
            el.first.x -= WALL_VELOCITY * dt;
            el.second.x -= WALL_VELOCITY * dt;
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
             if (IsKeyPressed(KEY_R)) {
                 p = Player{};
                 w = Walls<4>{};
                 running = true;
             }
             if (IsKeyPressed(KEY_A)) {
                std::cout << "123";
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
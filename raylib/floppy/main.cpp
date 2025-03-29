#include "raylib.h"
#include <array>
#include <iostream>
#include <random>
#include <sstream>

constexpr float PLAYER_VEL_X = 360;
constexpr float PLAYER_VEL_Y = 180;
constexpr float HEIGHT = 600;
constexpr float WIDTH = 1200;
constexpr float WINDOW_SZ = 120.f;
constexpr float WALL_VELOCITY = 100.f;
constexpr float WALL_DISTANCE = 400.f;
constexpr int START_POS_Y = 30;
constexpr int END_POS_Y = HEIGHT - WINDOW_SZ - START_POS_Y;
constexpr size_t SCORE = 10;

template <typename T, int a, int b>
class Random{
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<T> dis = std::uniform_int_distribution<T>(a,b);
public:
    static int roll() {
        return dis(gen);
    }
};

template <size_t n>
struct Walls {
    size_t scores = 0;
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
        auto r = get_win_y_by_last();
        el.second.x = el.first.x;
        el.second.y = el.first.y + r;
        for (auto it = std::next(walls.begin()), ite = walls.end(); it != ite; ++it)
            *it = gen_next_wall(*std::prev(it));
    }

    size_t get_last() {
        return current_last++ % n;
    }

    float get_win_y_by_last() {
        return float(Random<int, START_POS_Y, END_POS_Y>::roll());
    }

    WallPair gen_next_wall(const WallPair& wp) {
        WallPair el;
        el.first.x = wp.first.x + WALL_DISTANCE;
        el.first.y = 0;
        el.second.x = el.first.x;
        el.second.y = get_win_y_by_last();
        return el;
    }

    void move_left(float dt) {
        for (auto& el: walls){
            if (el.first.x <= -100.f) {
                el = gen_next_wall(walls[get_last()]);
                scores += SCORE;
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
    size_t scores = 0;
    bool running = true;
    std::stringstream ss_score;
    ss_score << "Scores:" << 0;
    auto stream_updater = [](auto& ss, auto&&... val){
        ss.str("");
        (ss << ... << val);
    };
    auto target = LoadRenderTexture(WIDTH, HEIGHT);
     auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            auto dt = GetFrameTime();
            if (!running){
                DrawText(ss_score.str().data(), (WIDTH/2)-200, (HEIGHT/2)-100, 30, BLACK);
                DrawText("Game over. Press R to restart", (WIDTH/2)-200, HEIGHT/2, 30, BLACK);
                if (IsKeyPressed(KEY_R)) {
                    p = Player{};
                    w = Walls<4>{};
                    scores = 0;
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
                #if 0
                  DrawRectangleLinesEx((Rectangle){next_wall->first.x, next_wall->first.y, 100, next_wall->second.y}, 1, BLUE);
                  DrawRectangleLinesEx((Rectangle){next_wall->second.x, next_wall->second.y+WINDOW_SZ, 100, HEIGHT - next_wall->second.y - WINDOW_SZ}, 1, RED);
                #endif
                if (CheckCollisionRecs((Rectangle){p.pos.x, p.pos.y, 60,60},
                                       (Rectangle){next_wall->first.x, next_wall->first.y, 100, next_wall->second.y}) ||
                    CheckCollisionRecs((Rectangle){p.pos.x, p.pos.y, 60,60}, (Rectangle){next_wall->second.x, next_wall->second.y+WINDOW_SZ, 100, HEIGHT - next_wall->second.y - WINDOW_SZ})
                ){
                    running = false;
                }
            }

            if (p.pos.y > HEIGHT || p.pos.y < -50.f) {
                running = false;
            }

             stream_updater(ss_score, "Scores:", w.scores);
             DrawText(ss_score.str().data(), 10, 10, 30, BLACK);
      
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

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
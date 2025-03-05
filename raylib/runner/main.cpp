#include "raylib.h"
#include <array>
#include <iostream>
#include <random>
#include <sstream>

constexpr float HEIGHT = 600;
constexpr float WIDTH = 1200;
constexpr float JUMP_VELOCITY = 400.f;
constexpr float WALL_VELOCITY = 200.f;


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

struct Wall {
    Vector2 pos{WIDTH + 100, float(Random<int, 400, 450>::roll())};
    void process(float dt) {
        if (pos.x < -100.f) {
            pos.x = WIDTH + 100;
            pos.y = float(Random<int, 400, 450>::roll());
            return;
        }
        pos.x -= WALL_VELOCITY * dt;
    }
};


struct Player {
    Vector2 pos{100,HEIGHT-150};
    enum class JumpState {none, up, down} jump_state = JumpState::none;
    bool in_process() const {
        return jump_state != JumpState::none;
    }
    void jump() {
        jump_state = JumpState::up;
    }
    void process(float dt) {
        switch(jump_state){
            case JumpState::up: {
                pos.y -= JUMP_VELOCITY * dt;
                if (pos.y <= 300) {
                    jump_state = JumpState::down;
                }
                break;
            }
            case JumpState::down: {
                pos.y += JUMP_VELOCITY * dt;
                if (pos.y >= HEIGHT-150) {
                    pos.y = HEIGHT-150;
                    jump_state = JumpState::none;
                }
                break;
            }
            default:{break;}
        }
    }
};

int main() {
    InitWindow(WIDTH, HEIGHT, "Runner");
    SetTargetFPS(60);
    Player p;
    Vector2 ground = {0, HEIGHT-100};
    Wall w;

    auto target = LoadRenderTexture(WIDTH, HEIGHT);
     auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            auto dt = GetFrameTime();
            
            if (IsKeyPressed(KEY_SPACE) && !p.in_process())
                p.jump();

            if (p.in_process())
                p.process(dt);

            w.process(dt);    

            DrawRectangleRec((Rectangle){p.pos.x, p.pos.y, 50,50}, RED);
            DrawRectangleRec((Rectangle){ground.x, ground.y, WIDTH, 100}, BLACK);
            DrawRectangleRec((Rectangle){w.pos.x, w.pos.y, 50, ground.y - w.pos.y}, BLACK);
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
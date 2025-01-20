#include <iostream>
#include "raylib.h"
#include <array>

constexpr int HEIGHT = 600;
constexpr int WIDTH = 1200;
constexpr int BLOCK_H = 60;
constexpr int BLOCK_W = 100;
constexpr int SPEED = 7;
constexpr int START_POS_X = 600;
constexpr int START_POS_Y = 550;

struct RectangleObject {
    int x, y, h, w;
    Color c;
};

struct MainStick: RectangleObject{};
struct HP: RectangleObject {bool active;};
struct BlockObj: RectangleObject{bool active;};

template <size_t sz>
struct HPBar {
    std::array<HP, sz> bar;
    size_t active_index = sz-1;
    HPBar() {
        int x = 60;
        int y = 580;
        int step = 60;
        for (auto& el: bar) {
            el = {{x, y, 40,10, GREEN}, true};
            x += step;
        }
    }
    auto begin() {
        return bar.begin();
    }
    auto end() {
        return bar.end();
    }
    void reset() {
        for(auto& el: bar)
            el.active = true;
        active_index = sz-1;
    }
    void min_hp() {
        if (active_index) bar[active_index--].active = false;
    }
};

template <size_t h, size_t w>
struct Blocks {
    std::vector<std::vector<BlockObj>> blocks;
    Blocks(int xstart, int ystart, int xstep, int ystep){
        blocks.resize(h);
        for(auto& el: blocks)
            el.resize(w);
        int x = xstart;
        int y = ystart;
        for (auto& row: blocks) {
            for(auto& el: row){
                el = {{x, y, BLOCK_W, BLOCK_H, GRAY}, true};
                x += xstep;
            }
            x = xstart;
            y += ystep;
        }  
    }
    auto begin() {
        return blocks.begin();
    }
    auto end() {
        return blocks.end();
    }
};

struct Ball {
    int x, y, r;
    Color c;
    bool active;
    Vector2 velocity;
};

struct StickManager{
    MainStick& ms;
    StickManager(MainStick& ms): ms(ms) {}
    void move() {
        if (IsKeyPressed(KEY_A) || IsKeyDown(KEY_A)){
            if (ms.x - ms.w > 0) {
                ms.x -= SPEED;
            }
        }
        if (IsKeyPressed(KEY_D) || IsKeyDown(KEY_D)){
             if (ms.x + ms.w < WIDTH-100) {
                ms.x += SPEED;
            }
        }
    }
};


struct BallManager {
    Ball& b;
    BallManager(Ball& b): b(b) {}
    void move() {
        if (!b.active){
            if (IsKeyPressed(KEY_A) || IsKeyDown(KEY_A)){
                if (b.x - b.r > 50) {
                    b.x -= SPEED;
                }
            }
            if (IsKeyPressed(KEY_D) || IsKeyDown(KEY_D)){
                if (b.x + b.r < WIDTH-50) {
                    b.x += SPEED;
                }
            }
        } else {
            update(WIDTH, HEIGHT, 2.f);
        }
    }
    // todo: update 
    void update(float screenWidth, float screenHeight, float bounceFactor) {
        b.x += b.velocity.x;
        b.y += b.velocity.y;

        if (b.x < 0 || b.x + b.r > screenWidth) {
            b.velocity.x *= -1;
            b.x = b.x < 0 ? 0 : screenWidth - b.r; 
        }

        if (b.y < 0 || b.y + b.r > screenHeight) {
            b.velocity.y *= -1; 
            b.y = b.y < 0 ? 0 : screenHeight - b.r;
        }
    }
};


int main(){
    InitWindow(WIDTH, HEIGHT, "Arkanoid");
    
    MainStick ms{{START_POS_X, START_POS_Y, 100, 10, BLACK}};
    Ball ball{START_POS_X + 50, START_POS_Y-11, 10,  RED, false, {0, -8}};

    StickManager sm(ms);
    BallManager bm(ball);
    SetTargetFPS(60);

    HPBar<5> bar;
    Blocks<4,8> blocks(150, 40, 105, 65);

    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        for(const auto& el: bar){
            if (el.active)
                DrawRectangle(el.x, el.y, el.h, el.w, el.c);   // hp bar
        }
        for(auto& row: blocks.blocks){
            for(const auto& el: row) {
                if (el.active) {
                    DrawRectangle(el.x, el.y, el.h, el.w, el.c); // blocks
                    DrawRectangleLinesEx((Rectangle){(float)el.x, (float)el.y, (float)el.h, (float)el.w}, 10, BLACK);
                }
            }
        }
        
        DrawCircle(ball.x, ball.y, ball.r, ball.c);    // ball
        DrawRectangle(ms.x,ms.y,ms.h,ms.w,ms.c);       // stick
        EndTextureMode();
    };
   
    while(!WindowShouldClose()){
        sm.move();
        bm.move();
        render();
        if (IsKeyPressed(KEY_SPACE)){
            if (!ball.active) ball.active = true;
        }
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
}

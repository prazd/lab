#include <iostream>
#include "raylib.h"
#include <array>

constexpr int HEIGHT = 600;
constexpr int WIDTH = 1200;
constexpr int BLOCK_H = 40;
constexpr int BLOCK_W = 60;
constexpr int SPEED = 9;
constexpr int STICK_SPEED = 9;
constexpr int BALL_SPEED = 7;
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

template <size_t w, size_t h>
struct Blocks {
    size_t active_blocks;
    std::vector<std::vector<BlockObj>> blocks;
    Blocks(int xstart, int ystart, int xstep, int ystep): active_blocks(w * h){
        blocks.resize(h);
        for(auto& el: blocks)
            el.resize(w);
        int x = xstart;
        int y = ystart;
        for (auto& row: blocks) {
            for(auto& el: row){
                el = {{x, y, BLOCK_H, BLOCK_W,GRAY}, true};
                x += xstep;
            }
            x = xstart;
            y += ystep;
        }  
    }
    void reset() {
        for(auto& row: blocks){
            for(auto& el: row) {
                el.active = true;
            }
        }
        active_blocks = w * h;
    }
    size_t height() const {
        return h;
    }

    size_t width() const {
        return w;
    }

    size_t active_blocks_count() const {
        return active_blocks;
    }

    void disable(BlockObj& obj) {
        obj.active = false;
        if (active_blocks) active_blocks--;
    }
    auto rbegin() {
        return blocks.rbegin();
    }
    auto rend() {
        return blocks.rend();
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


template <typename Blocks, typename HPBar>
class StateManager {
    Blocks& blocks;
    Ball& b;
    MainStick& ms;
    HPBar& hpbar;
    void move_stick() {
        // todo: fix ranges
        Vector2 mpos = GetMousePosition();
        ms.x = mpos.x;
        if (!b.active){
            b.x = ms.x + ms.w/2;
        }
    }

    bool collision(int cx, int cy, int r, int rx, int ry, int rw, int rh) {
        int nearestX = std::max(rx, std::min(cx, rx + rw));
        int nearestY = std::max(ry, std::min(cy, ry + rh));
        int deltaX = cx - nearestX;
        int deltaY = cy - nearestY;
        return (deltaX * deltaX + deltaY * deltaY) <= (r * r);
    }
    
    void move_ball() {
        b.x += b.velocity.x;
        b.y += b.velocity.y;

        if (b.x - b.r <= 0 || b.x + b.r >= WIDTH) {
            b.velocity.x *= -1;
        }

        if (b.y - b.r <= 0) {
            b.velocity.y *= -1;
        }
       
    }

    void check_stick_collision() {
        if ((b.y + b.r >= ms.y && b.x >= ms.x && b.x <= ms.x + ms.w) && collision(b.x, b.y, b.r, ms.x, ms.y, ms.w, ms.h)){                  
            if (b.x <= ms.x + ms.w / 3) { // left    
                b.velocity.x = -5;
                b.velocity.y = -std::abs(b.velocity.y);
            } else if (b.x <= ms.x + 2 * ms.w / 3) { // middle
                b.velocity.x = 0;
                b.velocity.y = -std::abs(b.velocity.y);
            } else { 
                b.velocity.x = 5;
                b.velocity.y = -std::abs(b.velocity.y); // right
            }  
        }
    }

    void check_block_collision() {
        for(auto it = blocks.rbegin(), ite = blocks.rend();it!=ite;++it){
            for(auto& el: *it) {
                if (el.active && collision(b.x, b.y, b.r, el.x, el.y, el.w, el.h)){
                     if (b.x < el.x || b.x > el.x + el.w) { 
                        b.velocity.x *= -1;
                    } else { 
                        b.velocity.y *= -1;
                    }
                    blocks.disable(el);
                    win_check();
                    return;
                }
            }
        }
    }

    void fall_check() {
        if (b.y + b.r >= HEIGHT) {
            if (!hpbar.active_index) {
                current = State::Loose;
            } else {
                hpbar.min_hp();
                current = State::Prepare;
                b.active = false;
                b.x = START_POS_X + 50;
                b.y = START_POS_Y - 11;
            }
        }
    }

    void win_check() {
        if (!blocks.active_blocks_count()) {
            current = State::Win;
        }
    }    
    enum class State: char {
        Prepare,
        Run,
        Loose,
        Win
    } current = State::Prepare;

public:

    StateManager(Ball& b, MainStick& ms, Blocks& blocks, HPBar& hpbar): b(b), ms(ms), blocks(blocks), hpbar(hpbar) {}
    void update() {
        if (IsKeyPressed(KEY_R)){
            current = State::Prepare;
            hpbar.reset();
            blocks.reset();
            b.active = false;
            b.x = START_POS_X + 50;
            b.y = START_POS_Y - 11;
            
        }
        if (IsKeyPressed(KEY_SPACE) && current == State::Prepare){
            if (!b.active) {
                b.active = true;
                current = StateManager::State::Run;
            }
        }

        move_stick();
        if (b.active && current == State::Run){
            move_ball();
            check_stick_collision();
            check_block_collision();
            fall_check();
        };    
    }
    bool loose() const {
        return current == State::Loose;
    }

    bool win() const {
        return current == State::Win;
    }
};


int main(){
    InitWindow(WIDTH, HEIGHT, "Arkanoid");
    
    MainStick ms{{START_POS_X, START_POS_Y, 10, 100, BLACK}};
    Ball ball{START_POS_X + 50, START_POS_Y-11, 10,  RED, false, {0, -BALL_SPEED}};
    HPBar<5> bar;
    Blocks<18,8> blocks(40, 0, 62, 40);

    StateManager sm(ball, ms, blocks, bar);
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        for(const auto& el: bar){
            if (el.active)
                DrawRectangle(el.x, el.y, el.h, el.w, el.c);       // hp bar
        }
        for(auto& row: blocks.blocks){
            for(const auto& el: row) {
                if (el.active) {
                    DrawRectangle(el.x, el.y, el.w, el.h, el.c);   // blocks
                    
                    DrawRectangleLines(el.x, el.y, el.w, el.h, RED);
                    // DrawRectangleLinesEx((Rectangle){(float)el.x, (float)el.y, (float)el.h, (float)el.w}, 10, BLACK);
                }
            }
        }
        
        DrawCircle(ball.x, ball.y, ball.r, ball.c); // ball
        DrawCircleLines(ball.x, ball.y, ball.r, BLUE);
        DrawRectangle(ms.x,ms.y,ms.w, ms.h, ms.c);    // stick
        if (sm.loose()) {
           DrawText("LOOSE", 600, 400, 30, RED);
        }
        if (sm.win()) {
            DrawText("Win", 600, 400, 30, GREEN);
        }
        EndTextureMode();
    };
   
    while(!WindowShouldClose()){
        sm.update();
        render();

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
}

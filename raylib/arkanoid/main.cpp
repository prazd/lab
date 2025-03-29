#include <iostream>
#include "raylib.h"
#include <array>

constexpr int HEIGHT = 600;
constexpr int WIDTH = 1200;
constexpr int BLOCK_H = 40;
constexpr int BLOCK_W = 60;
constexpr int BALL_SPEED = 7;
constexpr int START_POS_X = 600;
constexpr int START_POS_Y = 550;


struct MainStick {
    Rectangle rec;
    Color color;
};
struct HP{
    Rectangle rec;
    Color color;
    bool active;
};
struct BlockObj {
    Rectangle rec;
    Color color;
    bool active;
};

template <size_t sz>
struct HPBar {
    std::array<HP, sz> bar;
    size_t active_index = sz-1;
    HPBar() {
        float x = 60;
        float y = 580;
        float step = 60;
        for (auto& el: bar) {
            el = {{x, y, 40, 10}, GREEN, true};
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
    Blocks(float xstart, float ystart, float xstep, float ystep): active_blocks(w * h){
        blocks.resize(h);
        for(auto& el: blocks)
            el.resize(w);
        float x = xstart;
        float y = ystart;
        for (auto& row: blocks) {
            for(auto& el: row){
                el = {{x, y, BLOCK_W, BLOCK_H },GRAY,true};
                x += xstep;
            }
            x = xstart;
            y += ystep;
        }  
    }
    void seed() {
        size_t counter = 0;
        active_blocks = 0;
        for(auto& row: blocks) {
            for(auto& el: row) {
                if (counter & 1) {
                    ++active_blocks;
                    el.active = true;
                } else {
                    el.active = false;
                }
                ++counter;
            }
        }
    }
    void seedchess() {
        size_t counter = 0;
        active_blocks = 0;
        bool next_block = false;
        for(auto& row: blocks) {
            for(auto& el: row) {
                if (next_block) {
                    ++active_blocks;
                    el.active = true;
                } else {
                    el.active = false;
                }
                next_block = !next_block;
            }
            next_block = !next_block;
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
    bool moving;
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
        ms.rec.x = mpos.x;
        if (!b.moving){
            b.x = ms.rec.x + ms.rec.width / 2;
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
        if ((b.y + b.r >= ms.rec.y && b.x >= ms.rec.x && b.x <= ms.rec.x + ms.rec.width) && collision(b.x, b.y, b.r, ms.rec.x, ms.rec.y, ms.rec.width, ms.rec.height)){                  
            if (b.x <= ms.rec.x + ms.rec.width / 3) { // left    
                b.velocity.x = -5;
                b.velocity.y = -std::abs(b.velocity.y);
            } else if (b.x <= ms.rec.x + 2 * ms.rec.width / 3) { // middle
                b.velocity.x = 0;
                b.velocity.y = -std::abs(b.velocity.y);
            } else { 
                b.velocity.x = 5;
                b.velocity.y = -std::abs(b.velocity.y); // right
            }  
        }
    }

    void check_block_collision() {
        for(auto it = blocks.begin(), ite = blocks.end(); it != ite; ++it){
            for(auto& el: *it) {
                if (el.active && collision(b.x, b.y, b.r, el.rec.x, el.rec.y, el.rec.width, el.rec.height)){
                    int ballLeft = b.x - b.r;
                    int ballRight = b.x + b.r;
                    int ballTop = b.y - b.r;
                    int ballBottom = b.y + b.r;

                    int blockLeft = el.rec.x;
                    int blockRight = el.rec.x + el.rec.width;
                    int blockTop = el.rec.y;
                    int blockBottom = el.rec.y + el.rec.height;

                    bool hitFromLeft = ballRight >= blockLeft;
                    bool hitFromRight = ballLeft <= blockRight;
                    bool hitFromTop = ballBottom >= blockTop;
                    bool hitFromBottom = ballTop <= blockBottom;

                    // if ((hitFromLeft && hitFromRight) || (hitFromTop && hitFromBottom)) {
                    //     b.velocity.x *= -1;
                    //     b.velocity.y *= -1;
                    if (hitFromLeft || hitFromRight) {
                        b.velocity.y *= -1;
                    } else if (hitFromTop || hitFromBottom) {
                        b.velocity.x *= -1;
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
                b.moving = false;
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

    StateManager(Ball& b, MainStick& ms, Blocks& blocks, HPBar& hpbar): b(b), ms(ms), blocks(blocks), hpbar(hpbar) {
        blocks.seedchess();
    }
    void update() {
        if (IsKeyPressed(KEY_R)){
            current = State::Prepare;
            hpbar.reset();
            blocks.reset();
            b.moving = false;
            b.x = START_POS_X + 50;
            b.y = START_POS_Y - 11;
            
        }
        if (IsKeyPressed(KEY_SPACE) && current == State::Prepare){
            if (!b.moving) {
                b.moving = true;
                current = StateManager::State::Run;
            }
        }

        move_stick();
        if (b.moving && current == State::Run){
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
    
    MainStick ms{{START_POS_X, START_POS_Y, 100, 10}, BLACK};
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
                DrawRectangleRec(el.rec, el.color);       // hp bar
        }
        for(auto& row: blocks.blocks){
            for(const auto& el: row) {
                if (el.active) {
                    DrawRectangleRec(el.rec, el.color);   // blocks
                    DrawRectangleLinesEx(el.rec, 1, RED);
                }
            }
        }
        
        DrawCircle(ball.x, ball.y, ball.r, ball.c); // ball
        DrawCircleLines(ball.x, ball.y, ball.r, BLUE);
        DrawRectangleRec(ms.rec, ms.color);    // stick
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
    
    UnloadRenderTexture(target);
    CloseWindow();
}

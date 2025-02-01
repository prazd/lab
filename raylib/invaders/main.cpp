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

struct Spaceship {
    Rectangle rec;
    Color color;
};

struct Bullet {
    Rectangle rec;
    Vector2 velocity;
    Color color;
    bool active;
};

struct ObjectsManager {
    Spaceship& sp;
    Bullet& b;
    ObjectsManager(Spaceship& sp, Bullet& b): sp(sp), b(b) {}
    void ship_move() {
        auto mouse_pos = GetMousePosition();
        sp.rec.x = (mouse_pos.x-sp.rec.width/2);
    }
    void bullet_activate() {
        b.rec.x = sp.rec.x + sp.rec.width/2;
        b.rec.y = sp.rec.y;
        b.active = true;
    }
    void bullet_collision() {
        if (b.rec.y <= 0) {
            b.active = false;
            b.velocity.y = 0.f;
        }
    }
    void bullet_move() {
        b.rec.y += b.velocity.y;
        b.velocity.y -= 0.2f;
    }
    void update() {
        ship_move();
        if ((IsKeyPressed(KEY_SPACE)) && !b.active) {
            bullet_activate();
        }
        if (b.active){
            bullet_move();
            bullet_collision();
        };
    }
};

int main(){
    InitWindow(WIDTH, HEIGHT, "Ivaders");
    Spaceship sp{{START_POS_X, START_POS_Y, 120,40}, BLACK};
    Bullet b{{START_POS_X, START_POS_Y, 10,10},{0.f, 0.f}, RED, false};

    ObjectsManager om(sp, b);
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(WHITE);
            DrawRectangleRec(sp.rec, sp.color);    // spaceship
            if (om.b.active) {
                DrawRectangleRec(b.rec, b.color);    // bullet
            }
        EndTextureMode();
    };
   
  while(!WindowShouldClose()){
        om.update();
        render();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
}

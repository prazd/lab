#include <iostream>
#include <random>
#include "raylib.h"
#include <array>

constexpr int WIDTH = 1200;
constexpr int HEIGHT = 600;
constexpr int MAP_WIDTH = 24;
constexpr int MAP_HEIGHT = 12;

std::string smap;
void initmap() {
    smap += "########################";
    smap += "#......................#";
    smap += "#......................#";
    smap += "#......................#";
    smap += "#......#...............#";
    smap += "#......#...............#";
    smap += "#......##..............#";
    smap += "#......................#";
    smap += "#......................#";
    smap += "#..........#...........#";
    smap += "#..........###.........#";
    smap += "########################";
}

struct Block {
    Rectangle rec;
    bool iswall;
};

struct Map {
    std::vector<Block> blocks;
    Map(): blocks(MAP_WIDTH * MAP_HEIGHT) {
        float x = 0.0f;
        float y = 0.0f;
        int id = 0;
        for (auto el: smap) {
            blocks[id++] = {{x, y, 50, 50}, el == '#'};
            if (id % 24 == 0) {
                y += 50;
                x = 0;
            } else {
                x += 50;
            }
        }
    }
};


struct Player {
    Vector2 center;
    Vector2 velocity;
    Vector2 view_point;
    float r;
    float view_distance = 30.f;
    float angle = 0.f;
    void move() {
        if (IsKeyPressed(KEY_A)|| IsKeyDown(KEY_A)) {
            center.x -= velocity.x;
        }
        if (IsKeyPressed(KEY_D) || IsKeyDown(KEY_D)) {
            center.x += velocity.x;
        }
        if (IsKeyPressed(KEY_W)|| IsKeyDown(KEY_W)) {
            center.y -= velocity.y;
        }
        if (IsKeyPressed(KEY_S)|| IsKeyDown(KEY_S)) {
            center.y += velocity.y;
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyDown(KEY_LEFT)) {
            angle -= 0.1f;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyDown(KEY_RIGHT)) {
            angle += 0.1f;
        }
    }

};

int main(){
    initmap();
    InitWindow(WIDTH, HEIGHT, "Raycasting");
    Player p{{600, 300}, {4.6f, 4.6f}, {600, 300}, 10, 200.f};
    Map m;
    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(WHITE);
            for (auto& el: m.blocks){
                if(el.iswall) {
                    // DrawRectangleRec(el.rec, BLACK);
                    DrawRectangleLinesEx(el.rec, 1, RED);
                }
            }
                
            DrawCircle(p.center.x, p.center.y, p.r, RED);
            DrawLineV(Vector2{p.center.x, p.center.y}, Vector2{p.center.x + p.view_distance * std::cos(p.angle), p.center.y + p.view_distance * std::sin(p.angle)}, GREEN);
        EndTextureMode();
    };

    SetTargetFPS(60);
   
    while(!WindowShouldClose()){
        p.move();
        render();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
}

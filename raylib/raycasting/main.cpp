#include <iostream>
#include <random>
#include "raylib.h"
#include <array>
#include <set>

constexpr int WIDTH = 1200;
constexpr int HEIGHT = 600;
constexpr int MAP_WIDTH = 24;
constexpr int MAP_HEIGHT = 12;
constexpr float FOV = 3.14f/3;
constexpr float HALF_FOV = FOV/2;
constexpr int RAY_COUNT = 120;
constexpr int MAX_DEPTH = 600;
constexpr float DELTA_ANGLE = FOV/RAY_COUNT;
constexpr float VIEW_DISTANCE = 30.f;
constexpr int TILE = 50;
const int DIST = RAY_COUNT / (2 * std::tan(HALF_FOV));
const int PROJ_COEFF = DIST * TILE * 2;
const int SCALE = WIDTH / RAY_COUNT;
constexpr float C_DIS = 10.f;

std::array<std::string_view, MAP_HEIGHT> text_map = {
    "########################",
    "#..........#############",
    "#.#....................#",
    "#.#........#######.....#",
    "#.#..............#.....#",
    "#.#..............#.....#",
    "#.#....................#",
    "#......................#",
    "#..........#############",
    "#......................#",
    "#........#####.........#",
    "########################",
};

std::set<std::pair<int, int>> walls_map;
void init_walls_map() {
    for (size_t i = 0; i < MAP_HEIGHT; ++i) {
        for (size_t j = 0; j < MAP_WIDTH; ++j) {
            if (text_map[i][j] == '#') 
                walls_map.insert(std::pair<int, int>{j * TILE, i * TILE});
        } 
    }
}

bool wall_collision(float x, float y) {
    return walls_map.contains(std::pair<int, int>{((int)(x) / TILE * TILE), ((int)(y) / TILE * TILE)});
}


void raycasting(float x, float y, float angle) {
    auto cur_angle = angle - HALF_FOV;
    for (size_t i = 0; i < RAY_COUNT; ++i) {
        auto sina = std::sin(cur_angle);
        auto cosa = std::cos(cur_angle);
        for (int j = 0; j < MAX_DEPTH; ++j) {
            auto x0 = x + j * cosa;
            auto y0 = y + j * sina; 
            if (wall_collision(x0 / TILE * TILE, y0 / TILE * TILE)) {
//                 float tc = 255.f / (1.f + float(j * j) * 0.00001f);
//                 Color c = {(unsigned char)tc, (unsigned char)tc, (unsigned char)tc, 255};
                auto proj_height = PROJ_COEFF / (j);
                DrawRectangleRec(Rectangle{(float)(i * SCALE), (float)(300 - proj_height / 2), SCALE, (float)proj_height}, WHITE);
            }
        }
        cur_angle += DELTA_ANGLE;
    }
}


struct Player {
    Vector2 center;
    Vector2 velocity;
    Vector2 view_point;
    float r;
    float angle = 0.f;
    Vector2 front;
    Vector2 back;
    Vector2 left;
    Vector2 right;

    void update_sides() {
        front =   {center.x + C_DIS * std::cos(angle),center.y + C_DIS * std::sin(angle)};
        back  =   {center.x - C_DIS * std::cos(angle),center.y - C_DIS * std::sin(angle)};
        right =   {center.x - C_DIS * std::sin(angle),center.y + C_DIS * std::cos(angle)};
        left  =   {center.x + C_DIS * std::sin(angle),center.y - C_DIS * std::cos(angle)};
    }
    void move() {
        auto sina = std::sin(angle);
        auto cosa = std::cos(angle);

        auto x = center.x + r + 100.f;
        auto y = center.y + r + 100.f;
        if ((IsKeyPressed(KEY_W) || IsKeyDown(KEY_W)) && !wall_collision(front.x, front.y)) {
            center.x += velocity.x * cosa;
            center.y += velocity.y * sina;
        }
        if ((IsKeyPressed(KEY_S) || IsKeyDown(KEY_S)) && !wall_collision(back.x, back.y)) {
            center.x += -velocity.x * cosa;
            center.y += -velocity.y * sina;
        }
        if ((IsKeyPressed(KEY_A) || IsKeyDown(KEY_A)) && !wall_collision(left.x, left.y)) {
            center.x += velocity.x * sina;
            center.y += -velocity.y * cosa;
        }
        if ((IsKeyPressed(KEY_D) || IsKeyDown(KEY_D)) && !wall_collision(right.x, right.y)) {
            center.x += -velocity.x * sina;
            center.y += velocity.y * cosa;
        }

        // if (IsKeyPressed(KEY_LEFT) || IsKeyDown(KEY_LEFT)) {
        //     angle -= 0.1f;
        // }
        // if (IsKeyPressed(KEY_RIGHT) || IsKeyDown(KEY_RIGHT)) {
        //     angle += 0.1f;
        // }

        if (GetMouseDelta().x < 0) {
            angle -= 0.1f;
        }
        if (GetMouseDelta().x > 0) {
            angle += 0.1f;
        }
        update_sides();
    }

};

int main(){
    init_walls_map();
    InitWindow(WIDTH, HEIGHT, "Raycasting");
    Player p{{600, 300}, {4.6f, 4.6f}, {600, 300}, 10, 200.f};
    

    RenderTexture2D target3D = LoadRenderTexture(WIDTH, HEIGHT);
    RenderTexture2D target2D = LoadRenderTexture(WIDTH, HEIGHT);
    Rectangle sourceRec = {0, 0, (float)WIDTH, -(float)HEIGHT}; 
    Rectangle destRec = {0, 0, 400, 200};
    Vector2 origin = {0, 0};
   
   auto render3D = [&]{
        BeginTextureMode(target3D);
            ClearBackground(BLACK);
            raycasting(p.center.x, p.center.y, p.angle);
        EndTextureMode();
    };

    auto render_mini_map2D = [&]{
         BeginTextureMode(target2D);
            ClearBackground(BLACK);
            for (auto& el: walls_map) {
                DrawRectangleRec((Rectangle){(float)el.first, (float)el.second, (float)TILE, (float)TILE}, BLACK);
                DrawRectangleLinesEx((Rectangle){(float)el.first, (float)el.second, (float)TILE, (float)TILE}, 1, RED);
            }
            DrawCircle(p.center.x, p.center.y, p.r, RED);
            // raycasting_mini_map2D(p.center.x, p.center.y, p.angle); // 
            DrawLineV(Vector2{p.center.x, p.center.y}, Vector2{p.center.x + WIDTH * std::cos(p.angle), p.center.y + WIDTH * std::sin(p.angle)}, GREEN);
        EndTextureMode();
    };

    SetTargetFPS(60);

    bool show_minimap = true; 
   
    while(!WindowShouldClose()){
        p.move();
        render3D();
        
        if (IsKeyPressed(KEY_M)){
            show_minimap = !show_minimap;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target3D.texture, {0, 0, (float)target3D.texture.width, -(float)target3D.texture.height}, {0, 0}, WHITE);
            if (show_minimap) {
                render_mini_map2D();
                DrawTexturePro(target2D.texture, sourceRec, destRec, origin, 0.0f, WHITE);
            }
            DrawFPS(10, 10);
        EndDrawing();
    }
    CloseWindow();
}
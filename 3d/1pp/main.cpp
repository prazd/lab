#include <iostream>
#include "raylib.h"

constexpr size_t WIDTH = 800;
constexpr size_t HEIGHT = 800;

auto get_point_y(float y, float z, float h){
    return ((((y/z) * (HEIGHT/2)) * -1.f) + HEIGHT/2);
}

auto get_point_x(float x, float z, float w){
    return (((x/z) * (WIDTH/2)) + WIDTH/2);
}

struct Point {
    float x,y,z;
};

int main(){
    InitWindow(WIDTH,HEIGHT, "OnePointPerspective");
    Point cube []={
        { -0.5f, -0.5f, 1.f},
        {  0.5f, -0.5f, 1.f},
        {  0.5f,  0.5f, 1.f},
        { -0.5f,  0.5f, 1.f},

        { -0.5f, -0.5f, 2.f},
        {  0.5f, -0.5f, 2.f},
        {  0.5f,  0.5f, 2.f},
        { -0.5f,  0.5f, 2.f},
    };
    
    float cx = 0.f;
    float cy = 0.f;
    float cz = 1.5f;
    float a = 0.f;
    std::array<std::pair<float,float>, 8> lines{};

    SetTargetFPS(60);
    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WHITE);
            a += 0.1f;
            int count = 0;
            for (auto [x,y,z]: cube) {
                auto px = x - cx;
                auto py = y - cy;
                auto pz = z - cz;

                float x2 = px * std::cos(a) + pz * std::sin(a);
                float z2 = (-1.f * px) * std::sin(a) + pz * std::cos(a);

                x = x2 + cx;
                z = z2 + cz;
                auto res_x = get_point_x(x, z, 40);
                auto res_y = get_point_y(y, z, 40);
                lines[count++] = {res_x, res_y};

            }
            
            for (int i = 0; i < 4; ++i) {
                if (i == 3) {
                    DrawLine(lines[i].first, lines[i].second, lines[0].first, lines[0].second, BLACK);    
                    DrawLine(lines[i+4].first, lines[i+4].second, lines[4].first, lines[4].second, BLACK); 
                } else {
                    DrawLine(lines[i].first, lines[i].second, lines[i+1].first, lines[i+1].second, BLACK);
                    DrawLine(lines[i+4].first, lines[i+4].second, lines[i+5].first, lines[i+5].second, BLACK);
                }
                DrawLine(lines[i].first, lines[i].second, lines[i+4].first, lines[i+4].second, BLACK);
            }
#if 1
            DrawLine(0, HEIGHT/2, WIDTH, HEIGHT/2, BLACK);   
            DrawLine(WIDTH/2, 0, WIDTH/2, HEIGHT, BLACK); 
#endif            
            
        EndDrawing();
    }
    CloseWindow();
}

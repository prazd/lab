#include <iostream>
#include <random>
#include <sstream>
#include "raylib.h"

template <size_t begin, size_t end>
struct Rand {
    static inline std::random_device rd = std::random_device{};
    static inline std::uniform_int_distribution<size_t> ud = std::uniform_int_distribution<size_t>{begin, end};
    static inline size_t gen() {return ud(rd);}
};

constexpr size_t WIDTH = 1000;
constexpr size_t HEIGHT = 700;

constexpr size_t DISTR_X_BEGIN = 200;
constexpr size_t DISTR_X_END = 800;
constexpr size_t DISTR_Y_BEGIN = 200;
constexpr size_t DISTR_Y_END = 500;
constexpr size_t DISTRIBUTION_X = WIDTH/2;
constexpr size_t DISTRIBUTION_Y = HEIGHT/2;
constexpr size_t DISTRIBUTION_RADIUS = 200;
constexpr size_t COUNT = 100;

struct Planet {
    float x[COUNT]{};
    float y[COUNT]{};
    float mass[COUNT]{};
    static float disatnce(float p1x, float p1y, float p2x, float p2y){
        float d1 = p1x - p2x;
        float d2 = p1y - p2y;
        return std::sqrt(d1*d1 + d2*d2);
    }
    void seed(){
        for (int i = 0; i < COUNT; ++i){
            x[i] = Rand<0,WIDTH>::gen();
            y[i] = Rand<0,HEIGHT>::gen();
            if (auto d = disatnce(x[i], y[i], DISTRIBUTION_X, DISTRIBUTION_Y); d > DISTRIBUTION_RADIUS){
                float angle = atan2f(DISTRIBUTION_Y-y[i], DISTRIBUTION_X-x[i]);
                x[i] = DISTRIBUTION_X + DISTRIBUTION_RADIUS * std::cos(angle);
                y[i] = DISTRIBUTION_Y + DISTRIBUTION_RADIUS * std::sin(angle);
            }
            mass[i] = Rand<5, 15>::gen();
        }
    }
    Planet(){seed();}
} planet;


int main(){
    InitWindow(WIDTH, HEIGHT, "Gravity");
    bool show_distr_circle = false;
    bool show_coord = false;
    std::stringstream ss;
    std::string text;
    while(!WindowShouldClose()){
        if (IsKeyPressed(KEY_R))
            planet.seed();
        if (IsKeyPressed(KEY_X))
            show_distr_circle = !show_distr_circle;
        if (IsKeyPressed(KEY_V))
            show_coord = !show_coord;
        BeginDrawing();
            ClearBackground(WHITE);
            for (size_t i = 0; i < COUNT; ++i)
                DrawCircle(planet.x[i], planet.y[i], planet.mass[i], BLACK);

            if (show_distr_circle)
                DrawCircleLines(DISTRIBUTION_X, DISTRIBUTION_Y, DISTRIBUTION_RADIUS, RED);

            if (show_coord){
                auto x = GetMouseX();
                auto y = GetMouseY();
                ss << "X:" << x << " Y:" << y;
                text = ss.str();
                DrawText(text.c_str(), x + 10, y + 10, 13.f, RED);
                ss.str("");
            }

        EndDrawing();
    }
    CloseWindow();
}

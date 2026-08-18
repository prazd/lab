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
constexpr size_t DISTRIBUTION_X = WIDTH/2;
constexpr size_t DISTRIBUTION_Y = HEIGHT/2;
constexpr size_t DISTRIBUTION_RADIUS = 200;
constexpr size_t COUNT = 5;
constexpr size_t MAX_PRINT = 10;

constexpr float G = 6.67430e-11f;

// https://youtu.be/ryfbBB3pHfI?list=LL&t=632
// Fi = mi * ai = G * (mi*mj)/(d^2)
// ai = G * (mj)/d^2
// ai_v = (G * (mj)/d^2) * {dx/d, dy/d};
// ai_v = {G * (mj * dx)/d^3, G * (mj * dy)/d3};

// a - acceleration
// m - mass
// d - distance
// G - gravitational constant
struct Planet {
    float x1[COUNT]{};
    float y1[COUNT]{};
    
    float x2[COUNT]{};
    float y2[COUNT]{};

    bool first = true;
    
    float mass[COUNT]{};
    float velocity_x[COUNT]{};
    float velocity_y[COUNT]{};

    std::stringstream ss;
    std::string text;

    static float distance(float p1x, float p1y, float p2x, float p2y){
        float d1 = p1x - p2x;
        float d2 = p1y - p2y;
        return std::sqrt(d1*d1 + d2*d2);
    }
    void seed(){
        for (int i = 0; i < COUNT; ++i){
            x1[i] = Rand<0,WIDTH>::gen();
            y1[i] = Rand<0,HEIGHT>::gen();
            if (auto d = distance(x1[i], y1[i], DISTRIBUTION_X, DISTRIBUTION_Y); d > DISTRIBUTION_RADIUS){
                float r = atan2f(DISTRIBUTION_Y-y1[i], DISTRIBUTION_X-x1[i]);
                size_t rand_radius = Rand<0,DISTRIBUTION_RADIUS>::gen();
                x1[i] = DISTRIBUTION_X + rand_radius * std::cos(r);
                y1[i] = DISTRIBUTION_Y + rand_radius * std::sin(r);
            }
            mass[i] = Rand<20, 30>::gen();
            velocity_x[i] = Rand<10,20>::gen();
            velocity_y[i] = Rand<10,20>::gen();
        }
    }
    void print_coords(){
        for (size_t i = 0, e = std::min(COUNT, MAX_PRINT); i != e; ++i)
            ss << "X:" << (first ? x1[i] : x2[i]) << " Y:" << (first ? y1[i] : y2[i]) << '\n';
        text = ss.str();
        DrawText(text.c_str(), 0, 0, 13.f, BLACK);
        ss.str("");
    }
    void calc_gravity(){
        auto& xs = (first) ? x1 : x2;
        auto& ys = (first) ? y1 : y2;
        auto& xd = (first) ? x2 : x1;
        auto& yd = (first) ? y2 : y1;

        // TODO: debug
        auto dt = GetFrameTime();
        // ai_v = {G * (mj * dx)/d^3, G * (mj * dy)/d3};
        for (size_t i = 0; i < COUNT; ++i){
            float vx = 0.f;
            float vy = 0.f;
            for (size_t j = 0 ; j < COUNT; ++j){
                if (i == j) continue;
                float d = distance(xs[i], ys[i], xs[j], ys[j]);
                float d3 = d * d * d;
                float dx = xs[j] - xs[i];
                float dy = ys[j] - ys[i];
                vx += G * (mass[j] * dx)/d3;
                vy += G * (mass[j] * dy)/d3;
            }
            velocity_x[i] += vx * dt;
            velocity_y[i] += vy * dt;
            xd[i] = xs[i] + velocity_x[i] * dt;
            yd[i] = ys[i] + velocity_y[i] * dt;
        }
        first = !first;
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


        planet.calc_gravity();
        BeginDrawing();
            ClearBackground(WHITE);
            planet.print_coords();
            for (size_t i = 0; i < COUNT; ++i)
                DrawCircle(planet.first ? planet.x2[i] : planet.x1[i], planet.first ? planet.y2[i] : planet.y1[i], planet.mass[i], BLACK);

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

#include "raylib.h"
#include <random>
#include <complex>

using namespace std::literals::complex_literals;

constexpr size_t HEIGHT = 1000;
constexpr size_t WIDTH = 1200;
constexpr double HIGH_LEFT = 3.0;
constexpr double STEP = 0.01;
constexpr size_t POINTS_COUNT = 600;
constexpr size_t ITERATIONS = 120;
constexpr std::complex<double> C = -0.7 + 0.27015i;

std::complex<double> next(std::complex<double> val, std::complex<double> c) {
    return (val * val) + c;
}

Color test_point(std::complex<double> val, std::complex<double> c) {
    auto tmp = val;
    for (int i = 0; i < ITERATIONS; ++i){
        tmp = next(tmp, c);
        if (std::abs(tmp) > 3) {
            return WHITE;
        }
    }
    return BLACK;
}

auto get_points(size_t dots_count) {
    std::vector<std::complex<double>> result;
    auto begin = std::complex<double>{-HIGH_LEFT, HIGH_LEFT};
    for (int i = 0; i < dots_count; ++i){
        for (int j = 0; j < dots_count; ++j) {
            result.push_back(begin);
            begin += std::complex<double>(STEP, 0.0);
        }
        begin = std::complex<double>(-HIGH_LEFT, begin.imag() - STEP);
    }
    return result;
}

int main()
{
    InitWindow(WIDTH, HEIGHT, "Fractals");
    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto points = get_points(POINTS_COUNT);
    BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        for (auto el: points){
            auto color = test_point(el, C);
            DrawCircle(el.real() * 100 + WIDTH/2, el.imag()  * 100 + HEIGHT/2, 2.f, color);
        }
        // X
        DrawLine(0, HEIGHT/2, WIDTH, HEIGHT/2, GRAY);
        // Y
        DrawLine(WIDTH/2, 0, WIDTH/2, HEIGHT, GRAY);
    EndTextureMode();

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }
    
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

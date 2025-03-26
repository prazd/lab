#include <iostream>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>

// #define FULL_PRINT

constexpr int HEIGHT = 15;
constexpr int WIDTH = 30;

char lab[HEIGHT][WIDTH]{};

void print(auto& arr) {    
    for (auto&& row: lab){
        for (auto el: row){
            std::cout << (!el ? "#" : " ");
        }
        std::cout << '\n';
    }
}

std::pair<int, int> directins[4] = {{2, 0}, {-2, 0}, {0,2}, {0,-2}};
auto rd = std::mt19937(std::random_device()());

void generate(int x, int y, int xstart = 0, int ystart = 0, int w = WIDTH, int h = HEIGHT) {
    lab[y][x] = 1;
#ifdef FULL_PRINT
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    std::cout << "\x1B[H";
    print(lab);
    std::cout << std::flush;
#endif
    std::shuffle(std::begin(directins), std::end(directins), rd);
    for (auto [dx, dy]: directins) {
        auto nx = dx + x, ny = dy + y;
        if (nx >= xstart && nx < w && ny >= ystart && ny < h && !lab[ny][nx]) {
            lab[dy/2+y][dx/2 + x] = 1;
            generate(nx, ny, xstart, ystart, w, h);
        }
    }
}

int main() {
#ifdef FULL_PRINT    
    std::cout << "\x1B[2J\x1B[H";
#endif    
    generate(4,2,4,2,WIDTH-4, HEIGHT - 1);
#ifndef FULL_PRINT
    print(lab);
#endif
}
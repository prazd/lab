#include "metal_api/metal_api.h"
#include <cstdint>
#include <thread>
#include <chrono>
#include <atomic>

constexpr int PIXEL_W = 160;
constexpr int PIXEL_H = 144;
constexpr int WINDOW_W = 800;
constexpr int WINDOW_H = 720;

template <int w, int h>
struct GoL {
    struct Cell {
        unsigned char active: 1; 
        unsigned char neigbours: 4; // 0-8
    };
    Cell field[h][w]{};
    void active_neighbours_update() {
        for (int i = 0; i < h; ++i){
            for (int j = 0; j < w; ++j) {
                size_t count = 0;
                count += field[i][(j + 1) % w].active;
                count += field[i][(w + (j - 1)) % w].active;
                count += field[(i + 1)%h][j].active;
                count += field[(h + (i - 1)) % h][j].active;

                count += field[(h + (i - 1)) % h][(w + (j - 1)) % w].active;
                count += field[(h + (i - 1)) % h][(j + 1) % w].active;
                count += field[(i + 1)%h][(w + (j - 1)) % w].active;
                count += field[(i + 1)%h][(j + 1)%w].active;

                field[i][j].neigbours = count;
            }
        }
    }

    void next_gen() {
            for (int i = 0; i < h; ++i){
                for (int j = 0; j < w; ++j) {
                    auto& cell = field[i][j];
                    if (cell.neigbours == 3 && !cell.active)
                        cell.active = true;
                    else if (cell.active && (cell.neigbours < 2 || cell.neigbours > 3))
                        cell.active = false;
            }
        }
    }

    void step(){
        active_neighbours_update();
        next_gen();
    }   
};

std::atomic<bool> new_frame_ready{false};
// clang++ -fsanitize=thread -std=c++2b main.cpp metal_api/metal_api.mm -ObjC++ -framework Cocoa -framework Metal -framework QuartzCore
int main() {
    GoL<PIXEL_W, PIXEL_H> g;
    // seed glider
    [&]{
        g.field[PIXEL_H/2][PIXEL_W/2].active = true;
        g.field[PIXEL_H/2 - 1][PIXEL_W/2 + 1].active = true;
        g.field[PIXEL_H/2 - 2][PIXEL_W/2 + 1].active = true;
        g.field[PIXEL_H/2 - 2][PIXEL_W/2].active = true;
        g.field[PIXEL_H/2 - 2][PIXEL_W/2 - 1].active = true;
    }();
    auto ctx = metal_create_window_and_renderer(PIXEL_W, PIXEL_H, WINDOW_W, WINDOW_H);
    auto framebuffer = metal_get_framebuffer(ctx);

    std::thread main_loop([&]{
        while(true) {
            g.step();
            for (int i = 0; i < PIXEL_H; ++i) {
                for (int j = 0; j < PIXEL_W; ++j) {
                    if (g.field[i][j].active) {
                        framebuffer[(i * PIXEL_W) + j] = 0xffffff;
                    } else {
                        framebuffer[(i * PIXEL_W) + j] = 0x00;
                    }
                }
            }
            new_frame_ready.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    metal_start_main_loop(ctx);
    main_loop.join();
}

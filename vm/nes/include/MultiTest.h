#ifndef NES_MULTITEST_H
#define NES_MULTITEST_H
#include <iostream>
#include "raylib.h"
#include <vector>
#include <thread>
#include "Simple.h"
#include "Constants.hpp"
#include "PPU.hpp"
template <typename VM, size_t tests_count>
struct MultiTest{
    static constexpr float WIDTH = 1600.f;
    static constexpr float HEIGHT = 1200.f;
    static constexpr float DISPLAY_PIXELS_WIDTH = 256;
    static constexpr float DISPLAY_PIXELS_HEIGHT = 240;
    static constexpr size_t FRAME_SZ = 256;
    static constexpr size_t CYCLES_COUNT = constants::CPU_FRAME_CYCLES/2;

    struct FrameData {
        Texture2D texture{};
        Rectangle rec{};
        std::atomic<bool> in_progress = false;
    };

    static void step(VM& vm, FrameData& fm) {
        for (int i = 0; i < CYCLES_COUNT; ++i){
            for (int j = 0; j < 3; ++j) {
                vm.ppu.tick(vm.bus);
            }
            vm.cpu.tick_step(vm.bus);
            vm.bus.tick();
        }
        fm.in_progress.store(false);
    }

    void run(const auto& tests_paths){
        InitWindow(WIDTH, HEIGHT, "MultiTest");
        static FrameData frame_data[tests_count]{};
        static VM vms[tests_count];

        float x = 0.f;
        float y = 0.f;

        // init_textures
        for (int i = 0, e = tests_count; i < e; ++i){
            vms[i].reset(tests_paths[i]);
            auto img = GenImageColor(DISPLAY_PIXELS_WIDTH, DISPLAY_PIXELS_HEIGHT, WHITE);
            frame_data[i].texture = LoadTextureFromImage(img);
            frame_data[i].rec = {x, y,(float)FRAME_SZ, (float)FRAME_SZ};
            UnloadImage(img);
            x += FRAME_SZ + 20.f;
            if (x > WIDTH - FRAME_SZ){
                x = 0.f;
                y += FRAME_SZ + 20.f;
            }
        }

        WorkerPool<7, decltype(&step), VM&, FrameData&> wp;
        wp.run();

        while(!WindowShouldClose()) {
            for (int i = 0, e = tests_count; i < e; ++i){
                if (!frame_data[i].in_progress.exchange(true, std::memory_order_acquire)) {wp.add(&step, vms[i], frame_data[i]);}
                if (vms[i].ppu.update_display.exchange(false, std::memory_order_acquire))
                    UpdateTexture(frame_data[i].texture, vms[i].ppu.get_active_display().data());
            }
            BeginDrawing();
                ClearBackground(WHITE);
                for (int i = 0, e = tests_count; i < e; ++i) {
                    if (CheckCollisionPointRec(GetMousePosition(), frame_data[i].rec) && IsMouseButtonPressed(0))
                        SetClipboardText(tests_paths[i].data());
                    DrawTexturePro(frame_data[i].texture, {0, 0, (float)frame_data[i].texture.width, (float) frame_data[i].texture.height},
                                   {frame_data[i].rec}, {0, 0}, 0, WHITE);
                }
                DrawFPS(0,0);
            EndDrawing();
        }
        CloseWindow();
    }
};
#endif //NES_MULTITEST_H

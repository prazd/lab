#ifndef NES_MULTITEST_H
#define NES_MULTITEST_H
#include <iostream>
#include "raylib.h"
#include <vector>
#include <thread>
#include "Simple.h"
#include "Constants.hpp"
#include "PPU.hpp"

#if 1
// todo: Double buffering
template <typename VM, size_t tests_count>
struct MultiTest{
    static constexpr float WIDTH = 1600.f;
    static constexpr float HEIGHT = 1200.f;
    static constexpr float DISPLAY_PIXELS_WIDTH = 256;
    static constexpr float DISPLAY_PIXELS_HEIGHT = 240;
    static constexpr size_t FRAME_SZ = 256;
    static constexpr size_t CYCLES_COUNT = constants::CPU_FRAME_CYCLES;

    struct FrameData {
        Texture2D texture{};
        Rectangle rec{};
        std::atomic<bool> in_progress = false;
        std::atomic<bool> display_active = false;
        decltype(PPU::internal_display) display{};
    };

    static void step(VM& vm, FrameData& fm) {
        for (int i = 0; i < CYCLES_COUNT; ++i){
            for (int j = 0; j < 3; ++j) {
                vm.ppu.tick(vm.bus);
                if (vm.ppu.current_y == 241 && vm.ppu.current_scanline_cycle == 1 && !fm.display_active.exchange(true, std::memory_order_acquire)) {
                    std::copy(vm.ppu.internal_display.begin(), vm.ppu.internal_display.end(), fm.display.begin());
                    fm.display_active.store(false, std::memory_order_release);
                }
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
        std::atomic<bool> running;
        std::atomic<bool> stop;
        WorkerPool<7, decltype(&step), VM&, FrameData&> wp;
        auto vm_workers = [](decltype((wp)) wp, decltype((vms)) vms, decltype(frame_data) fd, std::atomic<bool>& running, std::atomic<bool>& stop){
            wp.run();
            while(running.load()){
                if (!stop.load()){
                    for (size_t i = 0; i < tests_count; ++i){
                        if (!fd[i].in_progress.load()) {fd[i].in_progress.store(true); wp.add(&step, vms[i], fd[i]);}
                    }
                }
            }
        };
        running.store(true);
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
        std::thread t(vm_workers, std::ref(wp), std::ref(vms), std::ref(frame_data), std::ref(running), std::ref(stop));
        while(!WindowShouldClose()) {
            for (int i = 0, e = tests_count; i < e; ++i){
                if (!frame_data[i].display_active.exchange(true, std::memory_order_acquire)) {
                    UpdateTexture(frame_data[i].texture, frame_data[i].display.data());
                    frame_data[i].display_active.store(false, std::memory_order_release);
                }
            }
            BeginDrawing();
                ClearBackground(WHITE);
                for (int i = 0, e = tests_count; i < e; ++i) {
                    DrawTexturePro(frame_data[i].texture, {frame_data[i].rec.x, frame_data[i].rec.y, (float)frame_data[i].texture.width, (float) frame_data[i].texture.height},
                                   {frame_data[i].rec.x, frame_data[i].rec.y, frame_data[i].rec.width, frame_data[i].rec.height}, {0, 0}, 0, WHITE);
                }
                DrawFPS(0,0);
            EndDrawing();
        }
        running.store(false);
        stop.store(true);
        t.join();
        CloseWindow();
    }
};
#else
template <typename VM, size_t tests_count>
struct MultiTest{
    struct alignas(64) FrameData {
        Texture2D texture{};
        Rectangle rec{};
        std::atomic<bool> running = true;
        std::atomic<bool> render = false;
    };
    constexpr static size_t frame_sz = 240;
    void run(const auto& tests_paths){
        auto worker = [](FrameData& fm, VM& vm){
            while(fm.running.load()){
                fm.render.wait(false);
                vm.frame_execute();
                fm.render.store(false);
                fm.render.notify_one();
            }
        };

        InitWindow(constants::DEBUG_WIDTH, constants::DEBUG_HEIGHT, "MultiTest");
        static FrameData frame_data[tests_count]{};
        static VM vms[tests_count];
        std::vector<std::thread> workers;
        workers.reserve(tests_count);
        for (int i = 0, e = tests_count; i < e; ++i)
            workers.emplace_back(worker, std::ref(frame_data[i]), std::ref(vms[i]));

        float x = 0.f;
        float y = 0.f;
        // init_textures
        for (int i = 0, e = tests_count; i < e; ++i){
            vms[i].reset(tests_paths[i]);
            auto img = GenImageColor(constants::DISPLAY_PIXELS_WIDTH, constants::DISPLAY_PIXELS_HEIGHT, WHITE);
            frame_data[i].texture = LoadTextureFromImage(img);
            frame_data[i].rec = {x, y,(float)frame_sz, (float)frame_sz};
            UnloadImage(img);
            x += frame_sz + 20.f;
            if (x > constants::DEBUG_WIDTH - frame_sz){
                x = 0.f;
                y += frame_sz + 20.f;
            }
        }
        while(!WindowShouldClose()) {
            for (auto& f: frame_data){
                f.render.store(true);
                f.render.notify_one();
            }
            for (int i = 0, e = tests_count; i < e; ++i){
                frame_data[i].render.wait(true);
                UpdateTexture(frame_data[i].texture, vms[i].ppu.display.data());
            }
            BeginDrawing();
                ClearBackground(WHITE);
                for (int i = 0, e = tests_count; i < e; ++i) {
                    DrawTexturePro(frame_data[i].texture, {frame_data[i].rec.x, frame_data[i].rec.y, (float)frame_data[i].texture.width, (float) frame_data[i].texture.height},
                                   {frame_data[i].rec.x, frame_data[i].rec.y, frame_data[i].rec.width, frame_data[i].rec.height}, {0, 0}, 0, WHITE);
                }
                DrawFPS(0,0);
            EndDrawing();
        }
        for (auto& f: frame_data) {f.running.store(false); f.render.store(true); f.render.notify_all(); f.running.notify_all();}
        for (auto&& w: workers) w.join();
        CloseWindow();
    }
};
#endif
#endif //NES_MULTITEST_H

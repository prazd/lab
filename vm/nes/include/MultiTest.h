#ifndef NES_MULTITEST_H
#define NES_MULTITEST_H
#include "raylib.h"
#include "Constants.hpp"
#include <vector>
#include <thread>
// todo: finish
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
#endif //NES_MULTITEST_H

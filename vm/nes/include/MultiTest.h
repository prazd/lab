#ifndef NES_MULTITEST_H
#define NES_MULTITEST_H
#include <iostream>
#include "raylib.h"
#include <vector>
#include <thread>
#include "Simple.h"
#include "Constants.hpp"
#include "PPU.hpp"

enum TestMode {
    CPUm, PPUm
};

constexpr static std::string_view cpu_tests[] = {
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_nes_cpu_test5/cpu.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/oam_read/oam_read.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_open_bus/ppu_open_bus.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_timing_test6/cpu_timing_test.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/instr_test-v5/all_instrs.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/branch_timing_tests/1.Branch_Basics.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/branch_timing_tests/2.Backward_Branch.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/branch_timing_tests/3.Forward_Branch.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/dmc_dma_during_read4/read_write_2007.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_ppumem.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_oam.nes",
};

constexpr static std::string_view ppu_tests[] = {
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/1.frame_basics.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/2.vbl_timing.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/3.even_odd_frames.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/4.vbl_clear_timing.nes",

        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_vbl_nmi/rom_singles/01-vbl_basics.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/01.basics.nes",
//        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/02.alignment.nes", todo: debug
//        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/03.corners.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/04.flip.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/05.left_clip.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/06.right_edge.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/11.edge_timing.nes"
};

template <typename VM, TestMode tm>
struct MultiTest{
    static constexpr inline size_t tests_count = []constexpr{
        switch (tm) {
            case CPUm: return std::size(cpu_tests);
            case PPUm: return std::size(ppu_tests);
            default: return 0ul;
        }
    }();
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

    static inline FrameData frame_data[tests_count]{};
    static inline VM vms[tests_count]{};

    static void step(VM& vm, FrameData& fm) {
        for (int i = 0; i < CYCLES_COUNT; ++i){
            for (int j = 0; j < 3; ++j) {
                vm.ppu_tick();
            }
            vm.cpu_tick();
            vm.bus.tick();
        }
        fm.in_progress.store(false);
    }

    void move(){
        if (IsKeyDown(KEY_DOWN)) {
            for (auto& f: frame_data) f.rec.y -= 1.f;
        } else if (IsKeyDown(KEY_UP)) {
            for (auto& f: frame_data) f.rec.y += 1.f;
        } else if (auto w = GetMouseWheelMove(); w > 0) {
            for (auto& f: frame_data) f.rec.y += 10.f;
        } else if (w < 0) {
            for (auto& f: frame_data) f.rec.y -= 10.f;
        }
    }

    void run(){
        InitWindow(WIDTH, HEIGHT, "MultiTest");
        const auto& tests_paths = tm == CPUm ? cpu_tests : ppu_tests;

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
            move();
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

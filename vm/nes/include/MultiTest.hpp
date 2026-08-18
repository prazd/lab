#ifndef NES_MULTITEST_HPP
#define NES_MULTITEST_HPP
#include <iostream>
#include "raylib.h"
#include <vector>
#include <thread>
#include "Simple.hpp"
#include "Constants.hpp"
#include "PPU.hpp"

enum TestMode {
    CPUm, PPUm, APUm
};

constexpr static std::string_view apu_tests[] = {
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/01.len_ctr.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/02.len_table.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/03.irq_flag.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/04.clock_jitter.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/05.len_timing_mode0.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/06.len_timing_mode1.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/07.irq_flag_timing.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/08.irq_timing.nes",
//          "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/09.reset_timing.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/10.len_halt_timing.nes",
//          "/Users/prazd/Desktop/dev/nes-test-roms/blargg_apu_2005.07.30/11.len_reload_timing.nes",
            "/Users/prazd/Desktop/dev/nes-test-roms/apu_test/rom_singles/7-dmc_basics.nes"
};

constexpr static std::string_view cpu_tests[] = {
        "/Users/prazd/Desktop/dev/nes-test-roms/blargg_nes_cpu_test5/cpu.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/oam_read/oam_read.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_timing_test6/cpu_timing_test.nes", // TODO: check
        "/Users/prazd/Desktop/dev/nes-test-roms/instr_test-v5/all_instrs.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/branch_timing_tests/1.Branch_Basics.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/branch_timing_tests/2.Backward_Branch.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/branch_timing_tests/3.Forward_Branch.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/dmc_dma_during_read4/read_write_2007.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_ppumem.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_writes/cpu_dummy_writes_oam.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_interrupts_v2/rom_singles/1-cli_latency.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes",
        // TODO: debug
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes"
};

constexpr static std::string_view ppu_tests[] = {
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/1.frame_basics.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/2.vbl_timing.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/3.even_odd_frames.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/4.vbl_clear_timing.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_vbl_nmi/rom_singles/01-vbl_basics.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_open_bus/ppu_open_bus.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/01.basics.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/02.alignment.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/03.corners.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/04.flip.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/05.left_clip.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/06.right_edge.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/sprite_hit_tests_2005.10.05/11.edge_timing.nes",
        "/Users/prazd/Desktop/dev/nes-test-roms/ppu_read_buffer/test_ppu_read_buffer.nes",
// TODO: debug
//        "/Users/prazd/Desktop/dev/nes-test-roms/mmc3_irq_tests/1.Clocking.nes",
//        "/Users/prazd/Desktop/dev/nes-test-roms/mmc3_irq_tests/2.Details.nes",
//        "/Users/prazd/Desktop/dev/nes-test-roms/mmc3_irq_tests/3.A12_clocking.nes",
//        "/Users/prazd/Desktop/dev/nes-test-roms/mmc3_test_2/rom_singles/5-MMC3.nes",
//        "/Users/prazd/Desktop/dev/nes-test-roms/mmc3_test_2/rom_singles/2-details.nes"
};

template <typename VM, TestMode tm>
struct MultiTest{
    static constexpr inline size_t tests_count = []constexpr{
        switch (tm) {
            case CPUm: return std::size(cpu_tests);
            case PPUm: return std::size(ppu_tests);
            case APUm: return std::size(apu_tests);
            default: return 0ul;
        }
    }();
    static constexpr float WIDTH = 1600.f;
    static constexpr float HEIGHT = 1200.f;
    static constexpr float DISPLAY_PIXELS_WIDTH = 256;
    static constexpr float DISPLAY_PIXELS_HEIGHT = 240;
    static constexpr size_t FRAME_SZ = 256;
    static constexpr size_t CYCLES_COUNT = constants::CPU_SPEED/120;

    struct FrameData {
        Texture2D texture{};
        Rectangle rec{};
        std::atomic<bool> in_progress = false;
    };

    static inline FrameData frame_data[tests_count]{};
    static inline VM vms[tests_count]{};

    static void step(VM& vm, FrameData& fm) {
        for (int i = 0; i < CYCLES_COUNT; ++i){
            vm.ppu_tick();
            vm.bus.tick();

            vm.ppu_tick();
            vm.bus.tick();
#if APU_TICK_ON
            vm.apu_tick();
#endif
            vm.cpu_tick();
            vm.ppu_tick();
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
        const auto tests_paths = []{
            switch (tm) {
                case CPUm : return cpu_tests;
                case PPUm : return ppu_tests;
                case APUm : return apu_tests;
            }
        }();
        bool test_active[tm == CPUm ? std::size(cpu_tests) : std::size(ppu_tests)];
        std::fill(std::begin(test_active), std::end(test_active), true);

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

        std::string task_sz_str = "Tasks sz:";
        while(!WindowShouldClose()) {
            move();
            for (int i = 0, e = tests_count; i < e; ++i){
                if (test_active[i] && !frame_data[i].in_progress.exchange(true, std::memory_order_acquire)) {wp.add(&step, vms[i], frame_data[i]);}
                if (test_active[i] && vms[i].ppu.update_display.exchange(false, std::memory_order_acquire))
                    UpdateTexture(frame_data[i].texture, vms[i].ppu.get_active_display().data());
                if (CheckCollisionPointRec(GetMousePosition(), frame_data[i].rec) && IsKeyPressed(KEY_M))
                    test_active[i] = !test_active[i];
                if (CheckCollisionPointRec(GetMousePosition(), frame_data[i].rec) && IsMouseButtonPressed(0))
                    SetClipboardText(tests_paths[i].data());
            }
            BeginDrawing();
                ClearBackground(WHITE);
                for (int i = 0, e = tests_count; i < e; ++i) {
                    DrawRectangleLinesEx(Rectangle{ frame_data[i].rec.x,  frame_data[i].rec.y,
                                                        frame_data[i].rec.width + 5,  frame_data[i].rec.height + 5}, 1.f, test_active[i] ? GREEN : RED);
                    DrawTexturePro(frame_data[i].texture, {0, 0, (float)frame_data[i].texture.width, (float) frame_data[i].texture.height},
                                     {frame_data[i].rec}, {0, 0}, 0, WHITE);
                }

                DrawFPS(0,0);
                auto task_sz_res = task_sz_str + std::to_string(wp.task_queue_sz());
                DrawText(task_sz_res.c_str(), WIDTH/2, 0, 12.f, RED);
            EndDrawing();
        }
        CloseWindow();
    }
};
#endif //NES_MULTITEST_HPP

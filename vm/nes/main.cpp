#include <iostream>
#include <iterator>
#include <SDL.h>
#include "raylib.h"
#include "APU.hpp"
#include "PPU.hpp"
#include "CPU.hpp"
#include "Cartridge.hpp"
#include "Shaders.hpp"
#include "Debug.hpp"
#include "MultiTest.hpp"
#include <initializer_list>

struct NESVM {
    APU apu{};
    Uint32 audio_dev;
    Bus bus{};
    CPU cpu{};
    PPU ppu{};
    Cartridge cartridge;
    std::string_view rom_path;
#if DEBUG_MODE
    Disassembler<30,520,200,400> dis;
    RegsInfo<30,30,180,425> reg_info{};
    SnapStorage snap_storage;
#endif

    NESVM()
#if DEBUG_MODE
    : dis(cpu, bus)
#endif
    {};

    void controller_tick() {
        if (cpu.cycles_count & 1)
            bus.controller_write_tick();
        bus.controller_read_tick();
    }

    void apu_tick(){
#if APU_TICK_ON
        apu.tick(bus);
#endif
#if APU_ON
        // TODO: debug
        if (apu.sample_count == 256){
            SDL_QueueAudio(audio_dev, apu.buffer + std::exchange(apu.read_i, (apu.read_i + 256) % std::size(apu.buffer)) , 512);
            apu.sample_count = 0;
        }
#endif
    }
    void cpu_tick(){
        cpu.tick_step(bus);
        ++cpu.cycles_count;
    }
    void ppu_tick(){
        ppu.tick(bus);
        ++ppu.current_cycle;
    }

    NESVM(std::string_view rom_path): rom_path(rom_path)
#if DEBUG_MODE
    ,dis(cpu, bus)
#endif
    {
#if GAME_GENIE_MODE
        cartridge.read("/Users/prazd/Desktop/books/cpp/cpp/vm/nes/rom/GameGenie.nes");
        read_cartridge_data();
#else
        cartridge.read(rom_path);
        read_cartridge_data();
#endif
    }

    void read_cartridge_data(){
        bus.mapper.mtype = static_cast<Mapper::MapperType>(cartridge.mapper_num);

        // for test
        size_t tiles_index_start = 16;
        if (cartridge.trainer_present)
            tiles_index_start += 512;

        if (cartridge.prg_rom_sz == 1){
            // TODO: update
            bus.mapper.prg.resize(16384 * 2);
            std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + tiles_index_start + 16384, bus.mapper.prg.begin());
            std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + tiles_index_start + 16384, bus.mapper.prg.begin() + 16384);
        } else {
            bus.mapper.prg.resize(16384 * cartridge.prg_rom_sz);
            std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + tiles_index_start + (16384 * cartridge.prg_rom_sz), bus.mapper.prg.begin());
        }

        tiles_index_start += 16384 * cartridge.prg_rom_sz;
        size_t chr_sz = cartridge.chr_rom_sz == 0 ?
                        8192 : // CHR_RAM
                        8192 * cartridge.chr_rom_sz;
        bus.mapper.chr.resize(chr_sz);
        std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.end(), bus.mapper.chr.begin());
        bus.mapper.set_screen_mirroring(cartridge.is_horizontal_arrangement, cartridge.is_alternative_nametable);
        bus.mapper.is_chr_ram = cartridge.chr_rom_sz == 0;
    }

    void ppu_init(){
        for (int i = 0; i < 4; ++i)
            ppu.tick(bus);
    }

    void cpu_init(){
        cpu.interrupt_handle(bus);
    }

    void reset(std::string_view path){
        bus = Bus{};
        bus.reset();
        cpu = CPU{};
        ppu.reset();
        cartridge.read(path);
        read_cartridge_data();
        rom_path = path;
        apu = APU{};
#if DEBUG_MODE
        snap_storage.reset();
#endif
        cpu_init();
        ppu_init();
    }

#if DEBUG_MODE
    void cpu_debug_exec() {
        size_t current_tick = 0;
        if (dis.next){
            cpu.debug_run = true;
            dis.next = false;
        }
        do {
            bool snap_update = cpu.debug_run;
            if (snap_update)
                snap_storage.update_state(bus, cpu, ppu);
            while (cpu.debug_run) {
                ppu_tick();
                bus.tick();

                ppu_tick();
                bus.tick();

                apu_tick(); cpu_tick();
                controller_tick();
                ppu_tick();
                bus.tick();
                ++current_tick;
            }
            if (snap_update)
                snap_storage.push_state(bus);
            if (dis.selected_move_active) {
                dis.selected_move_active = cpu.debug_run = (cpu.pc != dis.selected_line_pc);
            } else if (dis.trigger_search) {
                dis.trigger_search = cpu.debug_run = !(bus.cpu_addr_triggered_on_write || bus.cpu_addr_triggered_on_read);
            } else if (reg_info.is_cpu_watching()){
                cpu.debug_run = true;
            }
        } while((dis.selected_move_active || dis.trigger_search || reg_info.is_cpu_watching()) && current_tick < constants::CPU_FRAME_CYCLE);
        if (!dis.selected_move_active || !dis.trigger_search || !reg_info.is_cpu_watching())
            dis.handle(cpu);
    }

    void ppu_debug_exec(){
        size_t current_tick = 0;
        static size_t cpu_tick_count = 0;
        while ((dis.is_frame_jump_active || dis.is_tile_line_jump_active || dis.is_line_jump_active ||
                dis.is_one_tick_ppu_active || dis.trigger_search || ppu.run ||
                dis.selected_scanline_active || reg_info.is_ppu_watching()) && current_tick++ < constants::PPU_FRAME_CYCLE) {
            if (!cpu.debug_run) snap_storage.update_state(bus, cpu, ppu); // todo: check
            cpu.debug_run = true;
            if (cpu_tick_count++ % 3 == 2) {
                apu_tick();
                cpu_tick();
                controller_tick();
                if (dis.is_line_jump_active && ppu.line_start) {
                    dis.is_line_jump_active = false;
                }
                if (!cpu.debug_run) snap_storage.push_state(bus);
            }
            ppu_tick();
            if (dis.is_frame_jump_active && ppu.frame_start){
                dis.is_frame_jump_active = false;
            } else if (dis.is_tile_line_jump_active && ppu.tile_start) {
                dis.is_tile_line_jump_active = false;
                ppu.tile_start = false;
            } else if (dis.is_one_tick_ppu_active) {
                dis.is_one_tick_ppu_active = false;
            } else if (dis.trigger_search) {
                dis.trigger_search = !(bus.ppu_addr_triggered_on_read || bus.ppu_addr_triggered_on_write);
            } else if (dis.selected_scanline_active) {
                dis.selected_scanline_active = (ppu.current_y != dis.selected_scanline);
            }
            bus.tick();
        }
        if (!dis.selected_move_active &&
            !dis.is_frame_jump_active &&
            !dis.is_tile_line_jump_active &&
            !dis.is_line_jump_active &&
            !dis.is_one_tick_ppu_active &&
            !dis.trigger_search &&
            !dis.selected_scanline_active &&
            !reg_info.is_ppu_watching() &&
            !ppu.run) dis.handle(cpu);
    }
#endif

    void frame_execute(){
#if DEBUG_MODE
            if (dis.should_sync) {
                if (cpu.debug_run) cpu_debug_exec(); // todo: check
                dis.should_sync = !cpu.debug_run;
            } else {
                if (dis.run_mode == decltype(dis)::RunMode::CPU_M) {
                    cpu_debug_exec();
                } else {
                    ppu_debug_exec();
                }
            }
#else
        if (cpu.stop_requested) return;
        static double current_tick = 0.0;
        current_tick += (double)(constants::CPU_SPEED_f * std::min(GetFrameTime(), 0.016f));
        while(current_tick>=1.0){
             ppu_tick();
             bus.tick();

             ppu_tick();
             bus.tick();

             apu_tick(); cpu_tick();
             controller_tick();
             ppu_tick();
             bus.tick();
             current_tick-=1.0;
        }
#endif
    }

    void run(){
#if DEBUG_MODE
        InitWindow(constants::DEBUG_WIDTH, constants::DEBUG_HEIGHT, "NES");
#else
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        SetConfigFlags(FLAG_VSYNC_HINT);
        InitWindow(constants::WIDTH, constants::HEIGHT, "NES");
#endif

#if DEBUG_MODE
        PatternTableView<1110,0,480,432,128,128> ptv{128, 128};
        NameTableNumView<1110,0,480,432> ntnv{};
        PaletteViewer<1110,0,300,300> pv{};

        NameTableView<1110,432,480,432> ntv{};
        ColorsViewer<1110,432,480,432> cv{};
        SpriteTilesViewer<1110,432,480,432> stv{};
        MemoryViewer<1110,462,450,432> mem_reader{};
        InstructionFinder<1110,462,450,432> instr_finder{};
        Snapshot<1110,462,450,432> ds{};
        APUView<1110,432,480,432> apuv{};
#if GAME_GENIE_MODE
        CustomGameGenie<1110,462,450,432> cgg{};
#endif

        GeneralInfo<600, 700, 200,300> general_info{};

        CustomController<400, 700, 100,30> cc{};

        SpriteInfo<800, 700, 200,200> sprite_info{};

        DummyTilesGrid<320, 0> dtg;
        bool grid_view_enable = false;

        enum DownRightView {
            NameTableV,
            ColorsV,
            SpriteTilesV,
            MemoryReaderV,
            InstructionFinder,
            DiffSnapshotV,
            APUView,
#if GAME_GENIE_MODE
            CustomGameGenieV,
#endif
            DownRightViewCount
        } current_down_left_view = NameTableV;

        auto handle_down_left_view = [&]{
            switch (current_down_left_view) {
                case NameTableV:        {ntv.handle(ppu, bus);                 break;}
                case ColorsV:           {cv.handle(bus, ppu);                  break;}
                case SpriteTilesV:      {stv.handle(bus, ppu);                 break;}
                case MemoryReaderV:     {mem_reader.handle(bus, cpu, ppu);  break;}
                case InstructionFinder: {instr_finder.handle(bus, cpu);        break;}
                case DiffSnapshotV:     {ds.handle(bus, cpu, ppu);          break;}
                case APUView:           {apuv.handle(bus, apu);                break;}
#if GAME_GENIE_MODE
                case CustomGameGenieV:   {cgg.handle(bus); break;}
#endif
            }
        };
        auto is_collision_down_left = [&] {
            switch (current_down_left_view) {
                case NameTableV: return CheckCollisionPointRec(GetMousePosition(), ntv.rec);
                case ColorsV: return CheckCollisionPointRec(GetMousePosition(), cv.rec);
                case SpriteTilesV: return CheckCollisionPointRec(GetMousePosition(), stv.rec);
                case MemoryReaderV: return CheckCollisionPointRec(GetMousePosition(), mem_reader.rec);
                case InstructionFinder: return CheckCollisionPointRec(GetMousePosition(), instr_finder.rec);
                case DiffSnapshotV: return CheckCollisionPointRec(GetMousePosition(), ds.rec);
                case APUView: return CheckCollisionPointRec(GetMousePosition(), apuv.rec);
#if GAME_GENIE_MODE
                case CustomGameGenieV: return CheckCollisionPointRec(GetMousePosition(), cgg.rec);
#endif
            }
        };


        enum UpRightView {
            PatternTableV,
            NameTableNumV,
            PaletteViewer,
            UpRightViewCount,
        } current_up_right_view = PatternTableV;

        auto handle_up_right_view = [&]{
            switch(current_up_right_view) {
                case PatternTableV: {ptv.handle(ppu,bus); break;}
                case PaletteViewer: {pv.handle(bus, ppu); break;}
                default: {ntnv.handle(bus); break;}
            }
        };
        auto is_collision_up_right = [&]{
            switch (current_up_right_view) {
                case PatternTableV: return CheckCollisionPointRec(GetMousePosition(), ptv.rec);
                case PaletteViewer: return CheckCollisionPointRec(GetMousePosition(), pv.rec);
                case NameTableNumV: return CheckCollisionPointRec(GetMousePosition(), ntnv.rec);
            }
        };

#endif

        auto img = GenImageColor(constants::DISPLAY_PIXELS_WIDTH, constants::DISPLAY_PIXELS_HEIGHT, WHITE);
        auto display_texture = LoadTextureFromImage(img);
        Rectangle display_rec = {320, 0,(float)display_texture.width * 2.8f, (float)display_texture.height * 2.8f};
        UnloadImage(img);
#if APU_ON
        SDL_Init(SDL_INIT_AUDIO);
        SDL_AudioSpec want{};
        want.freq = 44100;
        want.format = AUDIO_S16LSB;
        want.channels = 1;
        want.samples = 256;

        audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
        if (!audio_dev){
            std::cout << "SDL_OpenAudioDevice error\n";
            std::abort();
        }
        SDL_PauseAudioDevice(audio_dev, 0);
#endif
        ppu.displays[0].fill(BLACK);
        ppu.displays[1].fill(BLACK);
        ppu_init();
        cpu_init();
#if DEBUG_MODE
        SetTargetFPS(60);
#endif
#if !DEBUG_MODE
        cartridge.update_game_state<false>((char*)(bus.cpu_map + constants::CARTRIDGE_RAM_BEGIN));
#endif

#if !DEBUG_MODE
        Shaders shaders = Shaders(std::initializer_list<const char*>{
            "/Users/prazd/Desktop/books/cpp/cpp/vm/nes/shaders/crt.fs",
            "/Users/prazd/Desktop/books/cpp/cpp/vm/nes/shaders/lcd.fs"
        });
        Vector2 resolution = {(float)constants::DISPLAY_PIXELS_WIDTH, (float)constants::DISPLAY_PIXELS_HEIGHT};
        bool shaders_mode = false;
#endif

        while(!WindowShouldClose()){
            frame_execute();
            UpdateTexture(display_texture, ppu.get_active_display().data());
#if !DEBUG_MODE
            auto shader_data = shaders.get();
            SetShaderValue(shader_data.s, shader_data.resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
            auto time = (float)GetTime();
            SetShaderValue(shader_data.s, shader_data.timeLoc, &time, SHADER_UNIFORM_FLOAT);
#endif

#if GAME_GENIE_MODE
            if (bus.gg.game_mode && !bus.gg.started){
                cpu.reset();
                ppu.reset();
                bus.reset();
#if DEBUG_MODE
                dis.reset();
#endif
                cartridge.read(rom_path);
                read_cartridge_data();
                bus.gg.started = true;
            }
#endif
            BeginDrawing();
            ClearBackground(WHITE);
#if DEBUG_MODE
            dis.draw(cpu, ppu, bus, apu, cartridge);
            dis.snap_handle(cpu, ppu, bus, snap_storage);
#endif

#if DEBUG_MODE
            DrawTexturePro(display_texture, {0, 0, (float)display_texture.width, (float)display_texture.height}, display_rec, {0, 0},0, WHITE);
            DrawRectangleLinesEx(display_rec, 1.f, BLACK);

            if (!dis.stress_speed){
                reg_info.handle(cpu, ppu, bus);
                handle_up_right_view();
                handle_down_left_view();
                general_info.handle(bus, cartridge, ppu, apu);
                cc.handle(bus.c);
                sprite_info.handle(bus, ppu);

                if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT)){
                    current_up_right_view = static_cast<UpRightView>((is_collision_up_right() ?
                            wrap((int)current_up_right_view, -IsKeyPressed(KEY_LEFT) + IsKeyPressed(KEY_RIGHT),  0, UpRightViewCount-1)
                             : current_up_right_view));

                    current_down_left_view = static_cast<DownRightView>((is_collision_down_left() ?
                            wrap((int)current_down_left_view, -IsKeyPressed(KEY_LEFT) + IsKeyPressed(KEY_RIGHT),  0, DownRightViewCount-1):
                            current_down_left_view));
                }

                if (CheckCollisionPointRec(GetMousePosition(),display_rec)){
                    if (IsKeyPressed(KEY_S))
                        cpu.stop_requested = !cpu.stop_requested;
                    if (IsKeyPressed(KEY_X))
                        grid_view_enable = !grid_view_enable;

                    if (IsKeyPressed(KEY_ONE) ||
                        IsKeyPressed(KEY_TWO) ||
                        IsKeyPressed(KEY_THREE) ||
                        IsKeyPressed(KEY_FOUR) ||
                        IsKeyPressed(KEY_FIVE) ||
                        IsKeyPressed(KEY_SIX)
                            ){
                        ppu.pixel_mixing_debug_view.m_switch(static_cast<PPU::PixelMixingDebugView::Mode>(GetKeyPressed() - KEY_ONE));
                    }
                    if (IsKeyPressed(KEY_C)){
                        ppu.displays[0].fill(WHITE);
                        ppu.displays[1].fill(WHITE);
                    }
                    if (IsKeyPressed(KEY_R)) reset(rom_path);
                }

                if (grid_view_enable) dtg.draw(22.4f,22.4f);
            }
#else
#if !DEBUG_MODE
            if (IsKeyPressed(KEY_Z)) shaders_mode = !shaders_mode;
            shaders.handle();
            if (shaders_mode) {
                BeginShaderMode(shader_data.s);
                DrawTexturePro(display_texture, {0, 0, (float)display_texture.width, (float)display_texture.height},  {0, 0,(float)GetScreenWidth(), (float)GetScreenHeight()}, {0, 0},0, WHITE);
                EndShaderMode();
            } else
#endif
            DrawTexturePro(display_texture, {0, 0, (float)display_texture.width, (float)display_texture.height},  {0, 0,(float)GetScreenWidth(), (float)GetScreenHeight()}, {0, 0},0, WHITE);

#endif
            DrawFPS(0,0);
            EndDrawing();
        }
#if APU_ON
        SDL_CloseAudioDevice(audio_dev);
        SDL_Quit();
#endif
        CloseWindow();
    }

    ~NESVM(){
#if !DEBUG_MODE && !MULTI_TEST_MODE
        if (bus.mapper.has_battery()) cartridge.update_game_state<true>((char*)(bus.cpu_map + constants::CARTRIDGE_RAM_BEGIN));
#endif
    }
};

// PPU tests:
// /Users/prazd/Desktop/dev/nes-test-roms/240pee/240pee.nes

// VBL:
// /Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/5.nmi_suppression.nes // todo: (e152) debug

int main(){
    // /Users/prazd/Desktop/dev/BattleCity.nes - TODO: https://www.nesdev.org/wiki/Vs._System
    // /Users/prazd/Desktop/dev/DonkeyKong.nes
    // /Users/prazd/Desktop/dev/DonkeyKongJU.nes (NROM)
    // /Users/prazd/Desktop/dev/KirbysAdventure.nes
    // /Users/prazd/Desktop/dev/SuperMarioBros.nes // debug: GameGenie
    // palette_ram.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/mmc3_irq_tests/1.Clocking.nes
    // /Users/prazd/Desktop/dev/GameGenie.nes

// TODO: debug
    // /Users/prazd/Desktop/dev/nes-test-roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/cpu_exec_space/test_cpu_exec_space_ppuio.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/dmc_dma_during_read4/dma_2007_write.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/dmc_dma_during_read4/dma_4016_read.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/dmc_dma_during_read4/dma_2007_read.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/instr_timing/instr_timing.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/scrolltest/scroll.nes - TODO: C20f
    // /Users/prazd/Desktop/dev/AccuracyCoin/AccuracyCoin.nes

#if MULTI_TEST_MODE
    MultiTest<NESVM, APUm>{}.run();
#else
//    /Users/prazd/Desktop/dev/nes-test-roms/vbl_nmi_timing/3.even_odd_frames.nes
//    /Users/prazd/Desktop/dev/nes-test-roms/test_apu_2/test_5.nes
//    /Users/prazd/Desktop/dev/nes-test-roms/test_apu_2/test_6.nes
// TODO: AccuracyCoin (APU reg activation debug)
    NESVM{"/Users/prazd/Desktop/dev/SuperMarioBros.nes"}.run(); //
#endif
}

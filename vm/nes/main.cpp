#include <iostream>
#include <iterator>
#include "raylib.h"
#include "PPU.hpp"
#include "CPU.hpp"
#include "Cartridge.h"
#include "Debug.h"

struct NESVM {
    Bus bus{};
    CPU cpu{};
    PPU ppu{};
    Cartridge cartridge;
    std::string_view rom_path;
#if DEBUG_MODE
    Disassembler<30,520,200,400> dis;
    RegsInfo<30,30,180,425> reg_info{};
#endif
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
                        8000 : // CHR_RAM
                        8192 * cartridge.chr_rom_sz;
        bus.mapper.chr.resize(chr_sz);
        std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.end(), bus.mapper.chr.begin());
        bus.mapper.update_screen_mirroring(cartridge.is_horizontal_arrangement, cartridge.is_alternative_nametable);
    }

    // todo: check
    void ppu_init(){
        for (int i = 0; i < 4; ++i)
            ppu.tick(bus);
    }

    void reset(std::string_view path){
        *this = NESVM(path);
        ppu_init();
        // TODO: debug
        // dis.reset();
        // cpu.reset();
        // ppu.reset();
        // bus.reset();
    }

#if DEBUG_MODE
    void cpu_debug_exec() {
        size_t current_tick = 0;
        if (dis.next){
            cpu.debug_run = true;
            dis.next = false;
        }
        do {
            while (cpu.debug_run) {
                for (int j = 0; j < 3; ++j)
                    ppu.tick(bus);
                cpu.tick(bus);
                bus.tick();
                ++current_tick;
            }
            if (dis.selected_move_active) {
                dis.selected_move_active = cpu.debug_run = (cpu.pc != dis.selected_line_pc);
            } else if (dis.trigger_search) {
                dis.trigger_search = cpu.debug_run = !(cpu.mem.triggered_on_write || cpu.mem.triggered_on_read);
            } else if (reg_info.is_cpu_watching()){
                cpu.debug_run = true;
            }
        } while((dis.selected_move_active || dis.trigger_search || reg_info.is_cpu_watching()) && current_tick < constants::CPU_FRAME_CYCLES);
        if (!dis.selected_move_active || !dis.trigger_search || !reg_info.is_cpu_watching()) {
            dis.handle(cpu);
            ppu.fill_white_background = true;
        }
    }

    void ppu_debug_exec(){
        size_t current_tick = 0;
        static size_t cpu_tick_count = 0;
        while ((dis.is_frame_jump_active || dis.is_tile_line_jump_active
                || dis.is_line_jump_active || dis.is_one_tick_ppu_active || dis.trigger_search || ppu.run || reg_info.is_ppu_watching()) && current_tick++ < constants::PPU_FRAME_CYCLES) {
            cpu.debug_run = true;
            ppu.tick(bus);
            if (dis.is_frame_jump_active && ppu.frame_start){
                dis.is_frame_jump_active = false;
            } else if (dis.is_tile_line_jump_active && ppu.tile_start) {
                dis.is_tile_line_jump_active = false; ppu.tile_start = false;
            } else if (dis.is_line_jump_active && ppu.line_start){
                dis.is_line_jump_active = false;
            } else if (dis.is_one_tick_ppu_active) {
                dis.is_one_tick_ppu_active = false;
            } else if (dis.trigger_search) {
                dis.trigger_search = !(ppu.mem.triggered_on_read || ppu.mem.triggered_on_write);
            }
            if (cpu_tick_count++ % 3 == 2) {cpu.tick(bus); bus.tick();}
        }
        if (!dis.selected_move_active &&
            !dis.is_frame_jump_active &&
            !dis.is_tile_line_jump_active &&
            !dis.is_line_jump_active &&
            !dis.is_one_tick_ppu_active &&
            !dis.trigger_search &&
            !reg_info.is_ppu_watching() &&
            !ppu.run) dis.handle(cpu);
    }
#endif

    void frame_execute(){
#if DEBUG_MODE
            if (dis.should_sync) {
                if (cpu.execute_in_progress) cpu_debug_exec();
                dis.should_sync = cpu.execute_in_progress;
            } else {
                if (dis.run_mode == decltype(dis)::RunMode::CPU_M) {
                    cpu_debug_exec();
                } else {
                    ppu_debug_exec();
                }
            }
#else
        if (cpu.stop_requested) return;
        for (int i = 0; i < constants::CPU_FRAME_CYCLES; ++i){
             cpu.tick(bus);
             bus.open_bus_decay();
             for (int j = 0; j < 3; ++j) ppu.tick(bus);
        }
#endif
    }

    void run(){
#if DEBUG_MODE
        InitWindow(constants::DEBUG_WIDTH, constants::DEBUG_HEIGHT, "NES");
#else
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
#if GAME_GENIE_MODE
                case CustomGameGenieV:   {cgg.handle(bus); break;}
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
                default: {ntnv.handle(ppu); break;}
            }
        };

#endif

        SetTargetFPS(60);
        auto img = GenImageColor(constants::DISPLAY_PIXELS_WIDTH, constants::DISPLAY_PIXELS_HEIGHT, WHITE);
        auto display_texture = LoadTextureFromImage(img);
        Rectangle display_rec = {320, 0,(float)display_texture.width * 2.8f, (float)display_texture.height * 2.8f};
        UnloadImage(img);

        ppu_init();
        while(!WindowShouldClose()){
            frame_execute();
#if DEBUG_MODE
            UpdateTexture(display_texture, ppu.internal_display.data());
#else
            UpdateTexture(display_texture, ppu.display.data());
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
            dis.draw(cpu, ppu, bus);
#endif

#if DEBUG_MODE
            DrawTexturePro(display_texture, {0, 0, (float)display_texture.width, (float)display_texture.height}, display_rec, {0, 0},0, WHITE);
            DrawRectangleLinesEx(display_rec, 1.f, BLACK);

            reg_info.handle(cpu, ppu, bus);
            handle_up_right_view();
            handle_down_left_view();
            general_info.handle(bus, cartridge, ppu);
            cc.handle(bus.c);
            sprite_info.handle(bus, ppu);

            if (CheckCollisionPointRec(GetMousePosition(), ptv.rec) && IsKeyPressed(KEY_M))
                current_up_right_view = static_cast<UpRightView>((current_up_right_view + 1) % UpRightViewCount);
            if (CheckCollisionPointRec(GetMousePosition(), ntv.rec) && IsKeyPressed(KEY_M))
                current_down_left_view = static_cast<DownRightView>((current_down_left_view + 1) % DownRightViewCount);

            if (CheckCollisionPointRec(GetMousePosition(),display_rec)){
                if (IsKeyPressed(KEY_S))
                    cpu.stop_requested = !cpu.stop_requested;
                if (IsKeyPressed(KEY_X))
                    grid_view_enable = !grid_view_enable;
                if (IsKeyPressed(KEY_ONE))
                    ppu.pixel_mixing_debug_view.m_switch(PPU::PixelMixingDebugView::Mode::Bg);
                if (IsKeyPressed(KEY_TWO))
                    ppu.pixel_mixing_debug_view.m_switch(PPU::PixelMixingDebugView::Mode::Sp);
                if (IsKeyPressed(KEY_THREE))
                    ppu.pixel_mixing_debug_view.m_switch(PPU::PixelMixingDebugView::Mode::BgSp);
                if (IsKeyPressed(KEY_FOUR))
                    ppu.pixel_mixing_debug_view.m_switch(PPU::PixelMixingDebugView::Mode::NBgSp);
            }

            if (grid_view_enable) dtg.draw(22.4f,22.4f);
            if (IsKeyPressed(KEY_R)) reset(rom_path);
#else
            DrawTexturePro(display_texture, {0, 0, (float)display_texture.width, (float)display_texture.height},  {0, 0,constants::WIDTH, constants::HEIGHT}, {0, 0},0, WHITE);
#endif
            DrawFPS(0,0);
            EndDrawing();
        }
        CloseWindow();
    }
};

// /Users/prazd/Desktop/dev/DonkeyKongJU.nes (NROM)

// PPU tests:
// - /Users/prazd/Desktop/dev/nes-test-roms/ppu_read_buffer/test_ppu_read_buffer.nes - todo (E224)
// /Users/prazd/Desktop/dev/nes-test-roms/240pee/240pee.nes

// Done:
// CPU:
// - /Users/prazd/Desktop/dev/nes-test-roms/blargg_nes_cpu_test5/cpu.nes
// - /Users/prazd/Desktop/dev/nes-test-roms/oam_read/oam_read.nes
// - /Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes

// PPU:
// - /Users/prazd/Desktop/dev/nes-test-roms/ppu_open_bus/ppu_open_bus.nes

int main(){
    // /Users/prazd/Desktop/dev/BattleCity.nes - TODO: https://www.nesdev.org/wiki/Vs._System
    // /Users/prazd/Desktop/dev/DonkeyKong.nes
    // /Users/prazd/Desktop/dev/SuperMarioBros.nes // debug: 0x8082/0x80e4/0x80c3/0x8eb6
    // palette_ram.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/mmc3_irq_tests/1.Clocking.nes
    // /Users/prazd/Desktop/dev/GameGenie.nes
    // /Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes
    auto nes = NESVM("/Users/prazd/Desktop/dev/nes-test-roms/blargg_nes_cpu_test5/cpu.nes");
    nes.run();
}

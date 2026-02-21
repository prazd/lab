#include <iostream>
#include <fstream>
#include "raylib.h"
#include "PPU.hpp"
#include "CPU.hpp"
#include "Debug.h"
#include <iterator>

struct Cartridge {
    std::vector<unsigned char> buffer{};

    enum TimingMode: unsigned char{
        RP2C02, // NTSC NES
        RP2C07, // Licensed PAL NES
        MultipleRegion, //
        UA6538 // Dendy
    } timing_mode = RP2C02;

    enum Format: unsigned char {
        iNES, NES2_0
    } current_format = iNES;

    enum Flags: unsigned char {
        //  PRG-ROM size LSB
        Flag4 = 4,
        //  CHR-ROM size LSB
        Flag5 = 5,
        // Mapper, mirroring, battery, trainer
        Flag6 = 6,
        // Mapper, VS/Playchoice, NES 2.0
        Flag7 = 7,
        //  PRG-RAM size (rarely used extension)
        Flag8 = 8,
        // TV system (rarely used extension)
        Flag9 = 9,
        // TV system, PRG-RAM presence (unofficial, rarely used extension)
        Flag10 = 10,
        // CHR-RAM size
        Flag11 = 11,
        // CPU/PPU Timing
        Flag12 = 12,

        // TOOD: check
        Flag13 = 13,
        Flag14 = 14,
        Flag15 = 15
    };

    bool nes_eof_begin = false;
    bool is_battery = false;
    bool is_horizontal_arrangement = false;
    bool trainer_present = false;
    bool is_alternative_nametable = false;
    unsigned short mapper_num = 0;
    unsigned short prg_rom_sz = 0;
    unsigned short chr_rom_sz = 0;
    unsigned char prg_ram_sz = 0;
    bool is_prg_nvram = false;
    unsigned prg_nvram_sz = 0;
    unsigned char chr_ram_sz = 0;
    bool is_chr_nvram = false;
    unsigned chr_nvram_sz = 0;

    void parse_header() {
        unsigned char nes_id = (buffer[Flag7] >> 2) & 0x3;
        nes_eof_begin = ((buffer[0] == 'N') && (buffer[1] == 'E') && (buffer[2] == 'S') && (buffer[3] == 0x1A));

        if (nes_id == 0b10 && nes_eof_begin)
            current_format = NES2_0;
        else if (nes_eof_begin)
            current_format = iNES;

        if (current_format == NES2_0 || current_format == iNES) {
            is_horizontal_arrangement = (buffer[Flag6]) & 1;
            is_battery = (buffer[Flag6] >> 1) & 1;
            trainer_present = (buffer[Flag6] >> 2) & 1;
            is_alternative_nametable = (buffer[Flag6] >> 3) & 1;
        }

        switch (current_format) {
            case NES2_0: {
                mapper_num = ((buffer[Flag8] & 0xf) << 8) | (((buffer[Flag7] >> 4) & 0xf) << 4) | ((buffer[Flag6] >> 4) & 0xf);
                prg_rom_sz = (((buffer[Flag9]) & 0xf) << 8) | (buffer[Flag4]);
                chr_rom_sz = (((buffer[Flag9]) >> 4) & 0xf << 8) | (buffer[Flag5]);
                prg_ram_sz = (buffer[Flag10]) & 0xf;
                is_prg_nvram = ((buffer[Flag10]) >> 4) & 0xf;
                if (is_prg_nvram) prg_nvram_sz = (64 << (((buffer[Flag10]) >> 4) & 0xf));
                chr_ram_sz = (buffer[Flag11]) & 0xf;
                is_chr_nvram = ((buffer[Flag11]) >> 4) & 0xf;
                if (is_chr_nvram) chr_nvram_sz = 64 << (((buffer[Flag11]) >> 4) & 0xf);
                timing_mode = static_cast<TimingMode>(buffer[Flag12] & 0x3);
                break;
            }
            case iNES: {
                chr_rom_sz = buffer[Flag5];
                prg_rom_sz =  buffer[Flag4];
                mapper_num = (((buffer[Flag7] >> 4) & 0xf) << 4) | ((buffer[Flag6] >> 4) & 0xf);
                break;
            }
        }
    }

    Cartridge(std::string_view rom_path) {
        auto sz = std::filesystem::file_size(rom_path);
        buffer.resize(sz);
        auto fs = std::ifstream(rom_path, std::ios::binary);
        if (!fs.is_open()) std::abort();
        fs.read(reinterpret_cast<char*>(buffer.data()),sz);
        parse_header();
    }
};

struct NESVM {
    Bus bus{};
    CPU cpu{};
    PPU ppu{};
    Cartridge cartridge;
#if UI_DEBUGGER
    Disassembler<30,520,200,400> dis;
#endif

    NESVM(std::string_view rom_path): cartridge(rom_path)
#if UI_DEBUGGER
    ,dis(cpu, bus)
#endif

    {
        std::cout << "MapperNum:" << cartridge.mapper_num << '\n';
        std::cout << "CHR_SZ:" << cartridge.chr_rom_sz << '\n';
        std::cout << "PRG_SZ:" << cartridge.prg_rom_sz << '\n';
        std::cout << "Trainer presents:" <<  cartridge.trainer_present << '\n';

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
                        16384 * cartridge.chr_rom_sz;
        bus.mapper.chr.resize(chr_sz);
        std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + tiles_index_start + chr_sz, bus.mapper.chr.begin());
        bus.mapper.update_screen_mirroring(cartridge.is_horizontal_arrangement, cartridge.is_alternative_nametable);
    }

#if UI_DEBUGGER
    void cpu_debug_exec() {
        size_t current_tick = 0;
        if (dis.next){
            cpu.debug_run = true;
            dis.next = false;
        }
        do {
            if (cpu.reset_requested) {
                cpu.reset();
                ppu.reset();
                cpu.debug_run = true;
                cpu.reset_requested = false;
            }
            while (cpu.debug_run) {
                cpu.tick(bus);
                bus.open_bus_decay();
                for (int j = 0; j < 3; ++j)
                    ppu.tick(bus);
                ++current_tick;
            }

            if (dis.selected_move_active) {
                dis.selected_move_active = cpu.debug_run = (cpu.pc != dis.selected_line_pc);
            }
        } while((dis.selected_move_active) && current_tick < constants::CPU_FRAME_CYCLES);
        if (!dis.selected_move_active) dis.handle(cpu);
    }

    void ppu_debug_exec(){
        size_t current_tick = 0;
        static size_t cpu_tick_count = 0;
        while ((dis.is_frame_jump_active || dis.is_tile_line_jump_active
                || dis.is_line_jump_active || dis.is_one_tick_ppu_active || ppu.run) && current_tick++ < constants::PPU_FRAME_CYCLES) {
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
            }
            if (cpu_tick_count++ % 3 == 2) cpu.tick(bus);
        }
        if (!dis.selected_move_active &&
            !dis.is_frame_jump_active &&
            !dis.is_tile_line_jump_active &&
            !dis.is_line_jump_active &&
            !dis.is_one_tick_ppu_active &&
            !ppu.run) dis.handle(cpu);
    }
#endif

    void frame_execute(){
#if UI_DEBUGGER
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
        auto gen_tile = [&] (int offset){
            std::array<unsigned char, 8> f{};
            for (int i = 0; i < 8; ++i)
                f[i] = *(ppu.mem.map + i + 16 * offset);
            std::array<unsigned char, 8> s{};
            for (int i = 8; i < 16; ++i)
                s[i-8] = *(ppu.mem.map + i + 16 * offset);
            return ppu.pattern_table(f, s);
        };
        int current_offset = 0;

#if DEBUG_MODE
        InitWindow(constants::DEBUG_WIDTH, constants::DEBUG_HEIGHT, "NES");
#else
        InitWindow(constants::WIDTH, constants::HEIGHT, "NES");
#endif

#if DEBUG_MODE
        PatternTableView<1110,0,480,432,128,128> ptv(128, 128);
        NameTableNumView<1110,0,480,432> ntnv{};
        PaletteViewer<1110,0,300,300> pv{};

        NameTableView<1110,432,480,432> ntv{};
        ColorsViewer<1110,432,480,432> cv{};
        SpriteTilesViewer<1110,432,480,432> stv{};

        PPUInfo<600, 700, 200,300> ppu_info;
        SpriteInfo<800, 700, 200,200> sprite_info{};

        RegsInfo<30,30,180,425> reg_info{};

        DummyTilesGrid<320, 0> dtg;
        bool grid_view_enable = false;

        enum DownLeftView {
            NameTableV,
            ColorsV,
            SpriteTilesV,
            DownLeftViewCount
        } current_down_left_view = NameTableV;

        auto handle_down_left_view = [&]{
            switch (current_down_left_view) {
                case NameTableV: {ntv.handle(ppu, bus); break;}
                case ColorsV:    {cv.handle(bus, ppu);  break;}
                case SpriteTilesV:    {stv.handle(bus, ppu); break;}
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

        auto img = GenImageColor(constants::DISPLAY_PIXELS_WIDTH, constants::DISPLAY_PIXELS_HEIGHT, WHITE);
        auto display_texture = LoadTextureFromImage(img);
        Rectangle display_rec = {320, 0,(float)display_texture.width * 2.8f, (float)display_texture.height * 2.8f};
        UnloadImage(img);

        SetTargetFPS(60);
        while(!WindowShouldClose()){
            frame_execute();
#if DEBUG_MODE && UI_DEBUGGER
            dis.draw(cpu, ppu, bus);
#endif

#if UI_DEBUGGER
            UpdateTexture(display_texture, ppu.internal_display.data());
#else
            UpdateTexture(display_texture, ppu.display.data());
#endif


            BeginDrawing();
            ClearBackground(WHITE);
#if DEBUG_MODE
            DrawTexturePro(display_texture, {0, 0, (float)display_texture.width, (float)display_texture.height}, display_rec, {0, 0},0, WHITE);
            DrawRectangleLinesEx(display_rec, 1.f, BLACK);
            reg_info.handle(cpu, ppu, bus);

            handle_up_right_view();
            handle_down_left_view();
            ppu_info.handle(ppu);
            sprite_info.handle(bus, ppu);

            if (CheckCollisionPointRec(GetMousePosition(), ptv.rec) && IsKeyPressed(KEY_M))
                current_up_right_view = static_cast<UpRightView>((current_up_right_view + 1) % UpRightViewCount);
            if (CheckCollisionPointRec(GetMousePosition(), ntv.rec) && IsKeyPressed(KEY_M))
                current_down_left_view = static_cast<DownLeftView>((current_down_left_view + 1) % DownLeftViewCount);

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
#endif
            cpu.reset_requested = (IsKeyPressed(KEY_R));



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
    // todo: continue debug (palette)
    auto nes = NESVM("/Users/prazd/Desktop/dev/nes-test-roms/240pee/240pee.nes");
    nes.run();
}

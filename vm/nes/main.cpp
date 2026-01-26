#include <iostream>
#include <fstream>
#include "raylib.h"
#include "PPU.hpp"
#include "CPU.hpp"
#include "Debug.h"

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
    Disassembler<30,432,200,400> dis;
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

        bus.mapper.rom.reserve(16384 * cartridge.prg_rom_sz);
        std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + (16384 * cartridge.prg_rom_sz), bus.mapper.rom.begin());

        tiles_index_start += 16384 * cartridge.prg_rom_sz;
        // todo: update
        std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + tiles_index_start + 8192, ppu.mem.map);
    }


    // Debug: - 81E9,  81EC, c4c3, c4a0 (on c44d - 19 iter) (c4e2- JMP 0x200 failed)
    // target - C44f +
    void frame_execute(){
#if UI_DEBUGGER
            if (dis.next){
                cpu.debug_run = true;
                dis.next = false;
            }
            int current_tick = 0;
            do {
                if (cpu.reset_requested) {
                    cpu.reset();
                    cpu.debug_run = true;
                    cpu.reset_requested = false;
                }
                while (cpu.debug_run) {
                    cpu.tick(bus);
                    ++current_tick;
                }

                if (dis.selected_move_active){
                    dis.selected_move_active = cpu.debug_run = (cpu.pc != dis.selected_line_pc);
                }
            } while(dis.selected_move_active && current_tick < constants::CPU_FRAME_CYCLES);
            if (!dis.selected_move_active) dis.handle(cpu);
#else
        for (int i = 0; i < constants::CPU_FRAME_CYCLES; ++i){
            cpu.tick(bus);
            // for (int j = 0; j < 3; ++j) ppu.tick(bus);
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
        TilesView<1110,0,480,432,128,128> tw(128,128);
        RegsInfo<0,0,300,300> reg_info{};
#endif

        auto img = GenImageColor(constants::DISPLAY_PIXELS_WIDTH, constants::DISPLAY_PIXELS_HEIGHT, WHITE);
        auto display_texture = LoadTextureFromImage(img);
        UnloadImage(img);


        bool paused = false;
        SetTargetFPS(60);
        while(!WindowShouldClose()){
            frame_execute();
#if DEBUG_MODE && UI_DEBUGGER
            dis.draw(cpu, bus);
#endif
            if (IsKeyPressed(KEY_P)) paused = !paused;
            BeginDrawing();
            ClearBackground(WHITE);
#if DEBUG_MODE
            reg_info.handle(cpu, ppu);
            tw.handle(ppu);
//            da.handle(cpu, bus);
//            DrawTexturePro(display_texture, {0, 0, (float)display_texture.width, (float)display_texture.height}, {0, 0,(float)display_texture.width * 2.8f, (float)display_texture.height * 2.8f}, {0, 0},0, WHITE);
//            DrawRectangleLinesEx({0, 0,(float)display_texture.width * 2.8f, (float)display_texture.height * 2.8f}, 1.f, BLACK);
#endif
            cpu.reset_requested = (IsKeyPressed(KEY_R));

            EndDrawing();
        }
        CloseWindow();
    }
};

int main(){
    auto nes = NESVM("/Users/prazd/Desktop/dev/nes-test-roms/blargg_nes_cpu_test5/cpu.nes");
    nes.run();
}

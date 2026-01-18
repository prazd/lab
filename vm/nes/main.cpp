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

    NESVM(std::string_view rom_path): cartridge(rom_path){
        std::cout << "MapperNum:" << cartridge.mapper_num << '\n';
        std::cout << "CHR_SZ:" << cartridge.chr_rom_sz << '\n';
        std::cout << "PRG_SZ:" << cartridge.prg_rom_sz << '\n';
        std::cout << "Trainer presents:" <<  cartridge.trainer_present << '\n';

        // for test
        size_t tiles_index_start = 16;
        if (cartridge.trainer_present) tiles_index_start += 512;
        // TODO: update
        std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + tiles_index_start + 32768, cpu.mem.map + 0x8000);
        tiles_index_start += 16384 * cartridge.prg_rom_sz;
        std::copy(cartridge.buffer.begin() + tiles_index_start, cartridge.buffer.begin() + tiles_index_start + 8192, ppu.mem.map);
    }

    void frame_execute(){
        for (int i = 0; i < constants::CPU_FRAME_CYCLES; ++i){
            cpu.tick(bus);
//            for (int j = 0; j < 3; ++j) ppu.tick();
        }
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
        TilesView<400,500,480,432,128,128> tw(128,128);
        Disassembler<100,100,200,200> da{cpu, bus};
#endif

        bool paused = false;

        size_t wait_count = 0;
        SetTargetFPS(60);
        while(!WindowShouldClose()){
            frame_execute();
#if DEBUG_MODE
//        d.move();
#endif


            if (IsKeyPressed(KEY_P)) paused = !paused;
            BeginDrawing();
#if DEBUG_MODE

//
//        float x_begin = 0;
//        float y_begin = 0;
//        auto pt_val = gen_tile(current_offset);
//        if (!paused) ++wait_count;
//        if (wait_count == 200) {++current_offset; wait_count = 0;}
//        for (auto&& row: pt_val){
//            for (auto el: row){
//                DrawRectangle(x_begin, y_begin, 10,10, color_by_id(el));
//                x_begin+=10;
//            }
//            x_begin = 0;
//            y_begin+=10;
//        }

            tw.handle(ppu);
            da.handle(cpu, bus);
//            d.draw_win();
//            d2.handle();
#endif
            ClearBackground(WHITE);
            EndDrawing();
        }
        CloseWindow();
    }
};

int main(){
    auto nes = NESVM("/Users/prazd/Desktop/dev/nes-test-roms/cpu_dummy_reads/cpu_dummy_reads.nes");
    nes.run();
}

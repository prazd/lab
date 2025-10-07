#include <iostream>
#include "raylib.h"
#include "PPU.h"
#include <queue>
#include <array>
#include "Memory.h"
#include "CPU.h"
#include "Joypad.h"
#include <filesystem>
#include <sstream>
#include <fstream>
#include "Debug.h"

struct GameBoyVM{
    CPU cpu;
    Memory mem;
    PPU ppu;
    Joypad joypad;
    std::string_view ram_path;

    Memory::MBC get_mbc_type(const auto& data) {
        switch (data[0x0147]) {
            case 0x01: // MBC1
            case 0x02: // MBC1 + Memory
            case 0x03: // MBC1 + Memory + Battery
                {return Memory::MBC1;}

            case 0x5:  // MBC2
            case 0x6:  // MBC2+BATTERY
                {return Memory::MBC2;}

            case 0x0f: // MBC3+TIMER+BATTERY
            case 0x10: // MBC3+TIMER+RAM+BATTERY
            case 0x11: // MBC3
            case 0x12: // MBC3+RAM
            case 0x13: // MBC3+RAM+BATTERY
                {return Memory::MBC3;}

            case 0x19: // MBC5
            case 0x1A: // MBC5+RAM
            case 0x1B: // MBC5+RAM+BATTERY
            case 0x1C: // MBC5+RUMBLE
            case 0x1D: // MBC5+RUMBLE+RAM
            case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
                {return Memory::MBC5;}

            default:{
                {return Memory::MBC0;}
            }
        }
    }

    bool is_cartridge_with_save(){
        constexpr static int mbc_with_battery_types[] = {
                0x03, 0x6, 0x0f, 0x10,0x13, 0x1B, 0x1E
        };
        return std::any_of(std::begin(mbc_with_battery_types),
                           std::end(mbc_with_battery_types),
                           [cartridge_type= mem.rom[0x147]](auto& el){return el == cartridge_type;});
    }
    void try_to_use_save() {
        if (!is_cartridge_with_save()) return;
        std::cout << "\nMBC1 with battery save\n";
        std::string spath(ram_path.substr(0, ram_path.size()-3));
        spath += ".sav";
        std::ifstream ifile(spath, std::ios::binary | std::ios::in);
        if (!ifile.is_open()){ std::cout << "Can't open save file:" << spath << '\n'; return;}
        ifile.read((char*)mem.external_ram.data(), mem.external_ram.size()); // todo: check
        ifile.close();
    }

    void main_loop_async(auto& cpu_r) {
        int current_t_cycle = 0;
        bool prev_lcdc_on = mem.map[constants::LCDC_ADDRESS] & 0x80;
        int lcdc_on_count = 0;
        bool lcdc_init_finished = true;
        while(current_t_cycle++ < 70'224) {
                joypad.update_state(mem);
                cpu.timer.tick(mem);

#if 0
                std::cout << cpu.instruction_str[cpu.decode(mem.read(cpu.pc))] << " REG[A]:" << (int)(cpu.regs[CPU::A]) << '\n';
                std::cout << "TIMA:" << (int)mem.read(constants::TIMA_REGISTER) << '\n';
#endif

                if (mem.map[constants::LCDC_ADDRESS] & 0x80) {
                    mem.map[constants::STAT_ADDRESS] |= 0x80;
                    if (!prev_lcdc_on) {
                        lcdc_init_finished = false;
                        lcdc_on_count = 12;
                        prev_lcdc_on = true;
                        ppu.lyc_ly_compare(mem);
                    } else {
                       if (!lcdc_init_finished && lcdc_on_count-- == 0) {
                           lcdc_init_finished = true;
                           ppu.reset(mem);
                           ppu.set_start_mode();
                           mem.ppu_oam_usage = true;
                       }
                       if (lcdc_init_finished){
                             ppu.tick(mem);
                       }
                    }
                }
                cpu_r.resume();
                if (!(mem.map[constants::LCDC_ADDRESS] & 0x80) && prev_lcdc_on) {
                    prev_lcdc_on = false;
                    ppu.reset(mem);
                }

                if (!cpu.halted && mem.dma_active) mem.dma_tick();
        }
    }

    int get_rom_size(const auto& data) {
        switch(data[0x0148]) {
            case 0x1: return 4;   // 64KB
            case 0x2: return 8;   // 128KB
            case 0x3: return 16;  // 256KB
            case 0x4: return 32;  // 512KB
            case 0x5: return 64;  // 1MB
            case 0x6: return 128; // 2MB
            case 0x7: return 256; // 4MB
            case 0x8: return 512; // 8MB
            case 0x52: return 72; // 1.1MB
            case 0x53: return 80; // 1.2MB
            case 0x54: return 96; // 1.5MB
        }
        return 2; // 32KB
    }

    int get_ram_size(const auto& data) {
        switch(data[0x0149]) {
            case 0x2: return 1; // 1 bank (8KiB)
            case 0x3: return 4; // 4 banks of 8KiB (32KiB)
            case 0x4: return 16; // 16 banks of 8KiB (128KiB)
            case 0x5: return 8; // 8 banks of 8KiB (64KiB)
        }
        // 0x01 - Unused
        // Listed in various unofficial docs as 2 KiB. However, a 2 KiB Memory chip was never used in a cartridge.
        // The source of this value is unknown.
        return 1; // No mem
    }

    void init_raw_rom(const std::string_view& path) {
        std::ifstream f{path, std::ios::binary | std::ios::in};
        if (!f.is_open()) {
            std::cout << "Cannot open file";
            std::abort();
        }
        auto sz = std::filesystem::file_size(path);
        std::vector<char> res(sz);
        f.seekg(0, std::ios::beg);
        f.read(res.data(), sz);
        f.close();
        mem.rom = std::move(res);
        std::cout << mem.rom.size();
    }

    // https://gbdev.io/pandocs/Power_Up_Sequence.html#register-initialization
    void boot() {
        cpu.regs[CPU::A] = 0x01;
        cpu.regs[CPU::F] = 0xB0;
        cpu.regs[CPU::B] = 0x00;
        cpu.regs[CPU::C] = 0x13;
        cpu.regs[CPU::D] = 0x00;
        cpu.regs[CPU::E] = 0xD8;
        cpu.regs[CPU::H] = 0x01;
        cpu.regs[CPU::L] = 0x4D;
        cpu.pc = 0x0100;
        cpu.sp = 0xFFFE;

        mem.map[constants::JOYPAD] = 0xcf;
        mem.map[constants::SC_ADDRESS] = 0x7e;
        mem.map[constants::DIV_REGISTER] = 0xAB; // --
        cpu.timer.div_internal = (0xAB << 8);
        mem.map[constants::TAC_REGISTER] = 0xF8;
        mem.map[constants::IF_ADDRESS] = 0xE1;

        mem.map[constants::NR10_ADDRESS] = 0x80;
        mem.map[constants::NR11_ADDRESS] = 0xBF;
        mem.map[constants::NR12_ADDRESS] = 0xF3;
        mem.map[constants::NR13_ADDRESS] = 0xFF;
        mem.map[constants::NR14_ADDRESS] = 0xBF;

        mem.map[constants::NR21_ADDRESS] = 0x3F;
        mem.map[constants::NR22_ADDRESS] = 0x00;
        mem.map[constants::NR23_ADDRESS] = 0xFF;
        mem.map[constants::NR24_ADDRESS] = 0xBF;

        mem.map[constants::NR30_ADDRESS] = 0x7F;
        mem.map[constants::NR31_ADDRESS] = 0xFF;
        mem.map[constants::NR32_ADDRESS] = 0x9F;
        mem.map[constants::NR33_ADDRESS] = 0xFF;
        mem.map[constants::NR34_ADDRESS] = 0xBF;

        mem.map[constants::NR41_ADDRESS] = 0xFF;
        mem.map[constants::NR42_ADDRESS] = 0x00;
        mem.map[constants::NR43_ADDRESS] = 0x00;
        mem.map[constants::NR44_ADDRESS] = 0xBF;

        mem.map[constants::NR50_ADDRESS] = 0x77;
        mem.map[constants::NR51_ADDRESS] = 0xF3;
        mem.map[constants::NR52_ADDRESS] = 0xF1;


        mem.map[constants::LCDC_ADDRESS] = 0x91;
        mem.map[constants::STAT_ADDRESS] = 0x85; // --
        mem.map[constants::DMA] = 0xFF;
        mem.map[constants::BGP] = 0xFC;
    }

    explicit GameBoyVM(const std::string_view& path): ram_path(path) {
        init_raw_rom(path);
        // init rom
        if ((unsigned char)mem.rom[0x143] == 0xc0) {
            std::cout << "\nGameBoyColor unsupported\n";
            std::abort();
        }

        mem.rom_sz = get_rom_size(mem.rom);
        mem.max_rom_bank_value = mem.rom_sz - 1;
        mem.current_mbc = get_mbc_type(mem.rom);
        mem.ram_sz = get_ram_size(mem.rom);
        boot();
        mem.max_ram_bank_value = mem.ram_sz - 1;
        if (mem.current_mbc == Memory::MBC2)
            mem.external_ram.resize(512);
        else
            mem.external_ram.resize(get_ram_size(mem.rom) * constants::EXTERNAL_RAM_SZ);

        if (mem.current_mbc == Memory::MBC3)
            mem.external_ram_enabled = true;
        mem.current_rom_bank_max = get_rom_size(mem.rom);
        try_to_use_save();
    }

    ~GameBoyVM(){
        if (!is_cartridge_with_save()) return;
        std::string save_path(ram_path.substr(0, ram_path.size()-3));
        save_path += ".sav";
        std::ofstream save_file(save_path, std::ios::binary | std::ios::out);
        if (save_file.is_open()) {

            save_file.write((char*)mem.external_ram.data(), mem.external_ram.size());
            std::cout << "\nSaved succesfully\n";
        } else {
            std::cout << "!!!!!!!!!!! Save error !!!!!!!!!!!" << '\n';
        }
        save_file.close();
    }
    
    void run() {
        InitWindow(constants::WIDTH, constants::HEIGHT, "GameBoy");
        SetTargetFPS(60); // 59.7275 FPS
        SetExitKey(0);

        // texture init
        Image img = GenImageColor(constants::DISPLAY_WIDTH, constants::DISPLAY_HEIGHT, WHITE);
        Texture2D target = LoadTextureFromImage(img);
        UnloadImage(img);
        std::array<std::array<Color, constants::DISPLAY_WIDTH>, constants::DISPLAY_HEIGHT> display_buffer{};

        // init display
        for (size_t i = 0; i < constants::DISPLAY_HEIGHT; ++i) {
            for (size_t j = 0 ; j < constants::DISPLAY_WIDTH; ++j) {
                display_buffer[i][j] = BLACK;
            }
        }
        UpdateTexture(target, display_buffer.data());

#if DEBUG_MODE
        TileWindow<160,144,500,500,300> tw(810,0, 0x8000);
        SpriteWindow<80, 32, 300, 200> sw(820, 517);
        RegWindow<0,0,300,400> rw(1150, 517);
        SpriteWindowText<0,0,600,200> swt(10, 750);
        JoypadWindow<0,0,300,250> jw(820, 730);
        bool print_pixels = false;
        bool print_tiles = false;
#endif

        auto cpu_r = cpu.instruction_async_loop(mem);
        while(!WindowShouldClose()){
            main_loop_async(cpu_r);
            UpdateTexture(target, ppu.read_buffer());
            BeginDrawing();
               ClearBackground(WHITE);
               DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{0, 0, constants::LCD_WIDTH, constants::LCD_HEIGHT}, {0, 0}, 0.0f, WHITE);
#if DEBUG_MODE

                tw.handle(mem);
                sw.handle(mem);
                rw.handle(cpu);
                swt.handle(sw);
                jw.handle(mem, joypad);
#endif
// PRINT PIXELS
#if 1
              if (IsKeyPressed(KEY_T))
                  print_tiles = !print_tiles;
              if (IsKeyPressed(KEY_P))
                  print_pixels = !print_pixels;

              if (print_pixels) DebugTools::print_rects<constants::DISPLAY_HEIGHT, constants::DISPLAY_WIDTH>(GREEN);
              if (print_tiles)  DebugTools::print_rects<18, 20>(RED);
#endif
                DrawFPS(700, 10);
            EndDrawing();
        }
    }
};

// Done:
// /Users/prazd/Desktop/books/cpp/cpp/vm/gameboy/roms/cpu_instrs.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/basic.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/reg_read.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/halt_ime0_ei.gb

// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/manual-only/sprite_priority.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/madness/mgb_oam_dma_halt_sprites.gb (halt bug) - check
// /Users/prazd/Desktop/dev/gb-test-roms/instr_timing/instr_timing.gb
// /Users/prazd/Desktop/dev/gb-test-roms/mem_timing/mem_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/interrupts/ie_push.gb

// MBC-1
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_bank1.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_bank2.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_ramg.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_mode.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/ram_256kb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/ram_64kb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_16Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_1Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_2Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_4Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_512kb.gb

// Failed:
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_oam_ok_timing.gb

// Timer:
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/div_write.gb

// PPU
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_1_2_timing-GS.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_0_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode0_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode3_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/stat_lyc_onoff.gb
// /Users/prazd/Desktop/dev/dmg-acid2/build/dmg-acid2.gb

// /Users/prazd/Desktop/dev/scribbltests/lycscx/lycscx.gb
// /Users/prazd/Desktop/dev/scribbltests/fairylake/fairylake.gb +-
// /Users/prazd/Desktop/dev/scribbltests/scxly/scxly.gb

// Failed:
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/vblank_stat_intr-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/stat_irq_blocking.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/lcdon_write_timing-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/hblank_ly_scx_timing-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode0_timing_sprites.gb - failed

// /Users/prazd/Desktop/dev/gb-test-roms/oam_bug/oam_bug.gb ?
// /Users/prazd/Desktop/dev/gb-test-roms/halt_bug.gb
// /Users/prazd/Desktop/dev/mealybug-tearoom-tests/m3_lcdc_bg_en_change.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/misc/ppu/vblank_stat_intr-C.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/sources-GS.gb

// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/div_timing.gb - failed

// Games:
// Tetris:
//     Source - https://github.com/alexsteb/tetris_disassembly/blob/master/main.asm
//              https://github.com/vinheim3/tetris-gb-disasm
//     ROM -    /Users/prazd/Desktop/dev/tetris.gb
// /Users/prazd/Desktop/dev/PokemonBlueVersion.gb - MBC3


// Next bug - /Users/prazd/Desktop/dev/ZeldaEn.gb (crocodile)

int main() {
        GameBoyVM vm("/Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim00.gb");
        std::cout << "\nRAM sz:" << vm.get_ram_size(vm.mem.rom) << '\n';
        std::cout << "\nROM sz:" << vm.get_rom_size(vm.mem.rom) << '\n';
        vm.run();
}
#include <iostream>
#include "raylib.h"
#include "PPU.h"
#include <queue>
#include <array>
#include "RAM.h"
#include "CPU.h"
#include "Timer.h"
#include <filesystem>
#include <sstream>
#include <fstream>

constexpr size_t WIDTH = 800;
constexpr size_t HEIGHT = 720;
constexpr size_t DISPLAY_HEIGHT = 144;
constexpr size_t DISPLAY_WIDTH = 160;
constexpr double CPU_FREQUENCY = 4194304.0;
constexpr double CPU_CYCLE_TIME = 1/CPU_FREQUENCY;

struct GameBoyVM{
    CPU cpu;
    RAM ram;
    PPU ppu;

    void print_segment(unsigned short address, int n) {
        std::cout << "\n=============================\n";
        for (int i = 0; i < n; ++i){
            std::cout << std::hex << address + i <<  ":" << (unsigned)ram.mem[address + i] << '\n';
        }
        std::cout << "\n=============================\n";
        std::cout << std::dec;
    }

    /*todo finish other components*/
    // Timer
    // Interrupts
    // Input
    // Sound

    RAM::MBC get_mbc_type(const auto& data) {
        switch (data[0x0147]) {
            case 0x01:
            case 0x02: // MBC1 + RAM
            case 0x03: // MBC1 + RAM + Battery
                {return RAM::MBC1;}
            // todo: finish other mbc types
            default:{
                return RAM::MBC0;
            }
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
        // Listed in various unofficial docs as 2 KiB. However, a 2 KiB RAM chip was never used in a cartridge.
        // The source of this value is unknown.
        return 0; // No ram
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
        ram.rom = std::move(res);
        std::cout << ram.rom.size();
    }

    explicit GameBoyVM(const std::string_view& path) {
        init_raw_rom(path);
        // init rom
        if ((unsigned char)ram.rom[0x143] == 0xc0) {
            std::cout << "\nGameBoyColor unsupported\n";
            std::abort();
        }
        cpu.pc = 0x0100;
        cpu.sp = 0xFFFE;
        ram.cart_ram.resize(get_ram_size(ram.rom) * constants::EXTERNAL_RAM_SZ);
        ram.current_mbc = get_mbc_type(ram.rom);
        ram.current_rom_bank_max = get_rom_size(ram.rom);
    }
    
    void run() {
        // Gamboy Doctor
#if 0
        cpu.regs[CPU::A] = 0x01;
        cpu.regs[CPU::F] = 0xB0;
        cpu.regs[CPU::B] = 0x00;
        cpu.regs[CPU::C] = 0x13;
        cpu.regs[CPU::D] = 0x00;
        cpu.regs[CPU::E] = 0xD8;
        cpu.regs[CPU::H] = 0x01;
        cpu.regs[CPU::L] = 0x4D;
        cpu.sp = 0xfffe;
        cpu.pc = 0x0100;
#endif


        InitWindow(WIDTH, HEIGHT, "GameBoy");
        SetTargetFPS(60); // 59.7275 FPS

        // texture init
        Image img = GenImageColor(DISPLAY_WIDTH, DISPLAY_HEIGHT, WHITE);
        Texture2D target = LoadTextureFromImage(img);
        UnloadImage(img);
        std::array<std::array<Color, DISPLAY_WIDTH>, DISPLAY_HEIGHT> display_buffer{};

        // init display
        for (size_t i = 0; i < DISPLAY_HEIGHT; ++i) {
            for (size_t j = 0 ; j < DISPLAY_WIDTH; ++j) {
                display_buffer[i][j] = BLACK;
            }
        }
        UpdateTexture(target, display_buffer.data());

        auto draw_tiles = [&] {
            static int counter = 0;
            static std::array<unsigned char, 16> arr{};
            std::cout << "Counter:" << counter << '\n';

            std::cout << ppu.pixel_fetcher.tile_address << '\n';
            for (int i = 0; i < 16; ++i){
                arr[i] = ram.read((0x8000 + (counter)) + i);
            }
            std::vector<std::vector<int>> colors;
            for (size_t i = 0; i < 16; i+=2){
                auto l = arr[i];
                auto h = arr[i+1];
                std::vector<int> tmp{};
                for (int j = 0; j < 8; ++j) {
                    auto lc = (l >> (7 - j)) & 1;
                    auto hc = (h >> (7 - j)) & 1;
                    tmp.push_back((hc << 1 | lc));
                }
                colors.push_back(tmp);
            }

            float xstep = 300.f/8.f;
            float ystep = 300.f/8.f;
            float x = 0.0f;
            float y = 0.0f;
            for (auto&& row: colors){
                for (auto el: row) {
                    DrawRectangle(x, y, xstep, ystep, Pixel::get_pixel_color(el));
                    x+=xstep;
                }
                y += ystep;
                x = 0.0f;
            }
            counter+=16;
        };

        auto print_rects = []<size_t height, size_t width>(Color c){
            std::array<Rectangle, height * width> res{};
            float x_pos = 0.0f;
            float y_pos = 0.0f;

            float step_x = WIDTH/width;
            float step_y = HEIGHT/height;
            int z = 0;
            for (int i = 0; i < height; ++i){
                for (int j = 0; j < width; ++j) {
                    res[z++] = Rectangle{x_pos, y_pos, step_x, step_y};
                    x_pos += step_x;
                }
                x_pos = 0;
                y_pos += step_y;
            }
            for (const auto& el: res){
                DrawRectangleLinesEx({el.x, el.y, el.width, el.height}, 1, c);
            }
        };

        int current_t_cycle = 0;
        double cpu_accumulator = 0.0;

        auto print_sp = [&](size_t n) {
            if (cpu.sp == 0xfffe) return;
            std::cout << "\n==============================\n";
            for (int i = 0; i < n; ++i){
                std::cout << "SP[" << i << "]" << std::hex << (unsigned)ram.read(cpu.sp + i) << '\n';
            }
            std::cout << "\n==============================\n";
        };

        size_t t_cycles_count = 0;
        while(!WindowShouldClose()){
            cpu_accumulator += GetFrameTime();
            while(current_t_cycle < 70'224 && cpu_accumulator >= CPU_CYCLE_TIME) {
                auto next_ins = cpu.fetch(ram);
                auto decoded_ins = cpu.decode(next_ins);
#if 0
                auto print_hex = [&](auto val){
                    auto num_bytes = sizeof(val);
                    std::stringstream ss("");
                    ss << std::hex << std::setw(num_bytes * 2) << std::setfill('0') << (unsigned)val;
                    return ss;
                };
                std::cout  << cpu.instruction_str[decoded_ins] << " ";
                if (!cpu.halted) {
                    std::cout <<

                              "A:" << print_hex(cpu.regs[CPU::A]).str() << " " <<
                              "F:" <<  print_hex(cpu.regs[CPU::F]).str() << " " <<
                              "B:" <<  print_hex(cpu.regs[CPU::B]).str() << " " <<
                              "C:" <<  print_hex(cpu.regs[CPU::C]).str() << " " <<
                              "D:" <<  print_hex(cpu.regs[CPU::D]).str() << " " <<
                              "E:" <<  print_hex(cpu.regs[CPU::E]).str() << " " <<
                              "H:" <<  print_hex(cpu.regs[CPU::H]).str() << " " <<
                              "L:" <<  print_hex(cpu.regs[CPU::L]).str() << " " <<
                              "SP:" <<  print_hex(cpu.sp).str() << " " <<
                              "PC:" <<  print_hex(cpu.pc).str() << " " <<
                              "PCMEM:" <<  print_hex(ram.read(cpu.pc)).str() << "," <<
                              print_hex(ram.read(cpu.pc + 1)).str() << "," <<
                              print_hex(ram.read(cpu.pc + 2)).str() << "," <<
                              print_hex(ram.read(cpu.pc + 3)).str() << '\n';
                    std::cout << std::dec;
                }


//                if (cpu.ime) {

#endif
                int t_cycle = 0;
                if (cpu.halted) {
                    auto if_value = ram.read(constants::IF_ADDRESS);
                    auto ie_value = ram.read(constants::IE_ADDRESS);
                    if (cpu.ime) {
                        if ((if_value & ie_value & 0x1f) != 0) {
                            cpu.handle_interrupt(ram, next_ins, decoded_ins);
                            t_cycle = cpu.execute(next_ins, decoded_ins, ram);
                            cpu.pc += cpu.pc_inc_by_ins(decoded_ins);
                        } else {
                            t_cycle = 4; // wait
                        }
                    } else if (!cpu.ime) {
                        if ((if_value & ie_value & 0x1f) != 0) {
                            if (cpu.skip_interrupt) {
                                cpu.halted = false;
                                cpu.skip_interrupt = false;
                                t_cycle = cpu.execute(next_ins, decoded_ins, ram);
                                cpu.pc += cpu.pc_inc_by_ins(decoded_ins);
                            } else {
                                cpu.halt_bug = true;
                                t_cycle = cpu.execute(next_ins, decoded_ins, ram);
                                cpu.pc += cpu.pc_inc_by_ins(decoded_ins) - 1;
                                cpu.halt_bug = false;
                                cpu.halted = false;
                            }
                        } else {
                            cpu.skip_interrupt = true;
                            t_cycle = 4;
                        }
                    }
                } else {
                    cpu.handle_interrupt(ram, next_ins, decoded_ins);
                    t_cycle = cpu.execute(next_ins, decoded_ins, ram);
                    cpu.pc += cpu.pc_inc_by_ins(decoded_ins);
                }
                t_cycles_count += t_cycle;
                for (int i = 0; i < t_cycle; ++i) {
                    cpu.timer.tick(ram);
                    ppu.tick(ram);
                    if (!cpu.halted)
                        ram.dma_tick();
                }

                current_t_cycle+=t_cycle;
                cpu_accumulator-=CPU_CYCLE_TIME;
//                std::cout << "IF_VALUE:" << std::hex << (unsigned)ram.read(constants::IF_ADDRESS) << '\n';
            }

//            print_segment(constants::OAM_BEGIN, 30);
            current_t_cycle = 0;
            std::cout << std::dec;
            UpdateTexture(target, ppu.read_buffer());

            BeginDrawing();
               ClearBackground(BLACK);
               DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{0, 0, WIDTH, HEIGHT}, {0, 0}, 0.0f, WHITE);


// PRINT PIXELS
#if 0
//              print_rects.template operator()<DISPLAY_HEIGHT, DISPLAY_WIDTH>(GREEN);
              print_rects.template operator()<18, 20>(RED);
#endif

                DrawFPS(700, 10);
            EndDrawing();
        }
    }
};

// Done:
// /Users/prazd/Desktop/books/cpp/cpp/vm/gameboy/roms/cpu_instrs.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/basic.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/halt_ime0_ei.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/div_write.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/manual-only/sprite_priority.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/madness/mgb_oam_dma_halt_sprites.gb (halt bug) - check
// /Users/prazd/Desktop/dev/gb-test-roms/instr_timing/instr_timing.gb


// MBC-1
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_bank1.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_bank2.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_ramg.gb


// Failed:
// /Users/prazd/Desktop/dev/gb-test-roms/oam_bug/oam_bug.gb ?
// /Users/prazd/Desktop/dev/gb-test-roms/halt_bug.gb
// /Users/prazd/Desktop/dev/dmg-acid2/build/dmg-acid2.gb

int main() {
        GameBoyVM vm("/Users/prazd/Desktop/dev/dmg-acid2/build/dmg-acid2.gb"); // todo
        std::cout << "\nRAM sz:" << vm.get_ram_size(vm.ram.rom) << '\n';
        std::cout << "\nROM sz:" << vm.get_rom_size(vm.ram.rom) << '\n';
        vm.run();
}
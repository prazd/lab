#include <iostream>
#include "raylib.h"
#include "PPU.h"
#include <queue>
#include <array>
#include "RAM.h"
#include "CPU.h"
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

    std::vector<char> row_rom{};
    /*todo finish other components*/
    // Timer
    // Interrupts
    // Input
    // Sound

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
        row_rom = std::move(res);
        std::cout << row_rom.size();
    }

    GameBoyVM(const std::string_view& path) {
        init_raw_rom(path);
        // init rom
        std::memcpy(ram.mem + constants::ROM_BANK_00_BEGIN, row_rom.data(), 16384);
        std::memcpy(ram.mem + constants::ROM_BANK_01_BEGIN, row_rom.data() + 16384, 16384);
        cpu.pc = 0x0100;
        cpu.sp = 0xFFFE;
        switch (get_rom_size(row_rom)) {
            case 4: {ram.current_mbc = RAM::MBC1; break;}
            default:
                {ram.current_mbc = RAM::MBC0; break;}
        }
    }
    
    void run() {
        // Gamboy Doctor
#if 1
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
                arr[i] = ram.read((0x8000 + 0x63 * 16) + i);
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
//            counter+=16;
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
//                std::cout  << cpu.instruction_str[decoded_ins] << " ";
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
#endif


                // todo: check
                int t_cycle = 0;
                if (cpu.halted && !cpu.ime) {
                    if (ram.read(constants::IF_ADDRESS) != 0) {cpu.halted = false;}
                    t_cycle = 4;
                } else {
                    t_cycle = cpu.execute(next_ins, decoded_ins, ram);
                }

                t_cycles_count+=t_cycle;

                ppu.handle_t_cycle(t_cycle, ram);
                cpu.handle_timer(t_cycle, ram);

                current_t_cycle+=t_cycle;
                cpu_accumulator-=CPU_CYCLE_TIME;
            }

            current_t_cycle = 0;
            std::cout << std::dec;
            // Render
            UpdateTexture(target, ppu.read_buffer());

            BeginDrawing();
                ClearBackground(BLACK);
//                draw_tiles();
                // todo: check
                DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{0, 0, WIDTH, HEIGHT}, {0, 0}, 0.0f, WHITE);
                DrawFPS(700, 10);
            EndDrawing();
        }
    }
};

void draw_tiles(auto& arr) {
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
    for (auto&& row: colors){
        for (auto el: row)
            std::cout << el;
        std::cout << '\n';
    }

    InitWindow(300, 300, "DrawTileTest");
    SetTargetFPS(60);
    auto get_color = [](int i){  
        switch(i){
            case 0: return WHITE;
            case 1: return LIGHTGRAY;
            case 2: return DARKGRAY;
            default:
                return BLACK;
        }
    };
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);
        float xstep = 300.f/8.f;
        float ystep = 300.f/8.f;
        float x = 0.0f;
        float y = 0.0f;
        for (auto&& row: colors){
            for (auto el: row) {
                DrawRectangle(x, y, xstep, ystep, get_color(el));
                x+=xstep;
            }
            y += ystep;
            x = 0.0f;
        }
        EndDrawing();
    }
    CloseWindow();
}

int main() {
        GameBoyVM vm("/Users/prazd/Desktop/books/cpp/cpp/vm/gameboy/roms/cpu_instrs.gb");
        std::cout << "\nRAM sz:" << vm.get_ram_size(vm.row_rom) << '\n';
        std::cout << "\nROM sz:" << vm.get_rom_size(vm.row_rom) << '\n';
        vm.run();
}

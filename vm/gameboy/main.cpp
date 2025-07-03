#include <iostream>
#include "raylib.h"
#include "PPU.h"
#include <queue>
#include <array>
#include "RAM.h"
#include "CPU.h"
#include <filesystem>
#include <fstream>
#include <iomanip>

constexpr size_t WIDTH = 1200;
constexpr size_t HEIGHT = 600;
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
    }

    void render(){}
    
    void run() {
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

        auto draw = [&]() {
                auto scy = ram.read(constants::SCY_ADDRESS);
                auto scx = ram.read(constants::SCX_ADDRESS);
                auto& display = ppu.read_lcd();
                for (int i = 0; i < DISPLAY_HEIGHT; ++i)
                    for (int j = 0; j < DISPLAY_WIDTH; ++j) {
                        display_buffer[i][j] = display[(scy + i)%DISPLAY_HEIGHT][(scx + j)%DISPLAY_WIDTH];
                    }
                UpdateTexture(target, display_buffer.data());
        };
    
        int current_t_cycle = 0;
        double cpu_accumulator = 0.0;
//        print_segment(0x0430, 50);
        while(!WindowShouldClose()){
            cpu_accumulator += GetFrameTime();
            while(current_t_cycle < 70'224 && cpu_accumulator >= CPU_CYCLE_TIME) {
                auto next_ins = cpu.fetch(ram); 
                auto decoded_ins = cpu.decode(next_ins);
                auto t_cycle = cpu.execute(next_ins, decoded_ins, ram);
                ppu.handle_t_cycle(t_cycle, ram);
                current_t_cycle+=t_cycle;
                cpu_accumulator-=CPU_CYCLE_TIME;
            }
//            if (ppu.get_current_mode() == PPU::VBlank) render();
//            std::cout << "Data in segments by HL (10 byte):\n";
//            print_segment(constants::OAM_BEGIN, 30);
//            std::cout << "\n===========================\n";
            std::cout << "LCDC_ADDRESS value:" << (unsigned)ram.read(constants::LCDC_ADDRESS) << '\n';
//            std::cout << "SCY address:" << (unsigned)ram.read(constants::SCY_ADDRESS) << '\n';
//            std::cout << "WY address:" << (unsigned)ram.read(constants::WY_ADDRESS) << '\n';
//            std::cout << "SCX address:" << (unsigned)ram.read(constants::SCX_ADDRESS) << '\n';
//            std::cout << "LY address:" <<  (unsigned)ram.read(constants::LY_ADDRESS) << '\n';
//            std::cout << "Window visible:" << (((ram.read(constants::LCDC_ADDRESS)) >> 5) & 1) << '\n';
//            std::cout << "\n===========================\n";
            current_t_cycle = 0;
            draw();
//            ppu.print_lcd();
            BeginDrawing();
                ClearBackground(BLACK);
                // todo: check
                DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{0, 0, DISPLAY_WIDTH * WIDTH, DISPLAY_HEIGHT * (HEIGHT)}, {0, 0}, 0.0f, WHITE);
                DrawFPS(1000, 10);
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
    // test tiles draw
//    unsigned char data[16]{0xFF, 0x00, 0x7E, 0xFF, 0x85, 0x81, 0x89, 0x83, 0x93, 0x85, 0xA5, 0x8B, 0xC9, 0x97, 0x7E, 0xFF};
//    draw_tiles(data);
    GameBoyVM vm("/Users/prazd/Desktop/dev/cpu_instrs/cpu_instrs.gb");
    std::cout << "\nRAM sz:" << vm.get_ram_size(vm.row_rom) << '\n';
    std::cout << "\nROM sz:" << vm.get_rom_size(vm.row_rom) << '\n';
    vm.run();
}

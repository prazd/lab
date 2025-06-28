#include <iostream>
#include "raylib.h"
#include "PPU.h"
#include <queue>
#include <array>
#include "RAM.h"
#include "CPU.h"

constexpr size_t DISPLAY_WIDTH = 1200;
constexpr size_t DISPLAY_HEIGHT = 600;
constexpr double CPU_FREQUENCY = 4194304.0;
constexpr double CPU_CYCLE_TIME = 1/CPU_FREQUENCY;

struct GameBoyVM{
    CPU cpu;
    RAM ram;
    PPU ppu;
    /*todo finish other components*/
    // Timer
    // Interrupts
    // Input
    // Sound

    void render(){/*todo*/}
    
    void run() {
        InitWindow(DISPLAY_WIDTH, DISPLAY_HEIGHT, "GameBoy");
        SetTargetFPS(60); // 59.7275 FPS

        // texture init
        Image img = GenImageColor(DISPLAY_WIDTH, DISPLAY_HEIGHT, BLACK);
        Texture2D target = LoadTextureFromImage(img);
        UnloadImage(img);
        std::array<std::array<Color, DISPLAY_WIDTH>, DISPLAY_HEIGHT> display_buffer;

        // init display
        for (size_t i = 0, ei = DISPLAY_HEIGHT; i < ei; ++i) {
            for (size_t j = 0, ej = DISPLAY_WIDTH; j < ej; ++j) {
                display_buffer[i][j] = BLACK;
            }
        }
        UpdateTexture(target, display_buffer.data());

        auto draw = [&]() {
             BeginDrawing();
                ClearBackground(RAYWHITE);
                // DrawFPS(1000, 10);
            EndDrawing();
        };
    
        int current_t_cycle = 0;
        double cpu_accumulator = 0.0;
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

            if (ppu.get_current_mode() == PPU::VBlank) render();
            current_t_cycle = 0;

            draw();
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

int main() {
    // test tiles draw
    unsigned char data[16]{0xFF, 0x00, 0x7E, 0xFF, 0x85, 0x81, 0x89, 0x83, 0x93, 0x85, 0xA5, 0x8B, 0xC9, 0x97, 0x7E, 0xFF};
    draw_tiles(data);
}

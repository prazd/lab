#include "raylib.h"
#include <CPU.hpp>
#include <Cartridge.hpp>
#include <iostream>
#include <string_view>
#include <bitset>
constexpr size_t HEIGHT = 800;
constexpr size_t WIDTH = 1000;

struct SMD {
    Cartridge cartridge;
    M68K m68k{};
    size_t offset{};
    //  1D92:
    SMD(std::string_view sv): cartridge(sv), offset(0x1d92){}
    void print_instruction(){
        // TODO: debug
        auto instruction = M68K::read_mem(M68K::WORD, (unsigned char*)(cartridge.buffer.data() + offset));
        auto di = M68K::decode(instruction);
        std::cout << std::hex << std::setfill('0') << std::setw(4) <<  instruction << '\n';
        std::cout << std::bitset<16>(instruction).to_string() << '\n';
//        std::cout <<std::dec<< (int)di.i_sz<< std::hex << '\n';
        auto res = M68K::disassemble(di, (unsigned char*)(cartridge.buffer.data() + offset));
        std::cout << std::hex << std::uppercase << offset << ':' << std::uppercase << res.to_sv() << '\n';
        offset += di.i_sz;
    }
};




int main(){
#if 0
    InitWindow(WIDTH, HEIGHT, "SegaMegaDrive");
    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WHITE);
        EndDrawing();
    }
    CloseWindow();
#endif

    // TODO: check tst
    SMD smd("/Users/prazd/Desktop/dev/Sonic.gen");
//    size_t x = 0;
//    while (std::cin >> x)
for (size_t i = 0; i < 2; ++i)
        smd.print_instruction();
}

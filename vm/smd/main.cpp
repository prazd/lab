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
    SMD(std::string_view sv): cartridge(sv), offset(0x37a){}
    void print_instruction(){
        // TODO: debug
        auto instruction = M68K::read_mem(M68K::WORD, (unsigned char*)(cartridge.buffer.data() + offset));
        auto di = M68K::decode(instruction);
        std::cout << std::hex << std::setfill('0') << std::setw(4) <<  instruction << '\n';
        std::cout << std::bitset<16>(instruction).to_string() << '\n';
        auto res = M68K::disassemble(di, (unsigned char*)(cartridge.buffer.data() + offset));
        std::cout <<std::hex << std::setfill('0') << std::setw(4) << offset << ':' << res.to_sv() << '\n';
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
    SMD smd("/Users/prazd/Desktop/dev/Sonic.gen");
    smd.print_instruction();
}

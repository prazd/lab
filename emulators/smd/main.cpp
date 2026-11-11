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
    size_t pc{};
//    1D92:
// 0x10312
//    debug
    SMD(std::string_view sv): cartridge(sv), pc(0x6d8){} // TODO: debug
    void print_instruction(){
        // TODO: debug
        auto instruction = M68K::read_mem(M68K::WORD, (unsigned char*)(cartridge.buffer.data() + pc));
        auto di = M68K::decode(instruction);
//        std::cout << std::hex << std::setfill('0') << std::setw(4) <<  instruction << '\n';
//        std::cout << std::bitset<16>(instruction).to_string() << '\n';
//        std::cout <<std::dec<< (int)di.i_sz<< std::hex << '\n';
        if (instruction && di.unknown){
            pc += 2;
            while(!M68K::read_mem(M68K::WORD, (unsigned char*)(cartridge.buffer.data() + pc)))
                pc += 2;
            return;
        }
        auto res = M68K::disassemble(di, pc, (unsigned char*)(cartridge.buffer.data()));
        std::cout << std::hex << std::uppercase << pc << ':' << std::uppercase << res.to_sv() << '\n';
        pc += di.i_sz;
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
    SMD smd("/Users/prazd/Desktop/dev/m68k_inst_speed/test_inst_speed.bin");
//    size_t x = 0;
//    while (std::cin >> x)
for (size_t i = 0; i < 2; ++i)
        smd.print_instruction();
}

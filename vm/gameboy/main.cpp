#include <iostream>
#include "raylib.h"


struct GameBoyVM{
    enum Registers {A, F, B, C, D, E, H, L};
    unsigned char res[8]{};

    unsigned char mem[8192]{}; // 8kib

    unsigned char ir{};
    unsigned char ie{};
    unsigned short pc{};
    unsigned short sp{};

    enum Instruction8 {
        LD_0b01xxxyyy, // 
        LD_0b01xxx110,
    };

    enum Instruction16{
        LD_0b00xxx110,
    };

    auto read3_5(Instruction8 i) {
        return (0x7 & (i >> 3));
    }

    auto read0_2(Instruction8 i) {
        return (0x7 & i);
    }


    // https://gekkio.fi/files/gb-docs/gbctr.pdf
    void exec8(unsigned char i, Instruction8 itype) {
        switch(itype) {
            case LD_0b01xxxyyy: { regs[read3_5(i)] = regs[read0_2(i)]; break;} // 20 page
            case LD_0b01xxx110: { regs[read3_5(i)] = mem[((regs[H] | regs[L] << 8)];  break;} // 22 page

        }
    }

    void exec16(unsigned short i, Instruction16 itype) {
        switch(itype){
            case LD_0b00xxx110: { regs[read3_5(i)] = (i >> 8); break;} // 21 page
       }
    }

};

int main() {

}

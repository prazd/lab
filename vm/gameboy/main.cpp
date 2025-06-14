#include <iostream>
#include "raylib.h"


struct GameBoyVM{
    enum Register {A, F, B, C, D, E, H, L};
    unsigned char regs[8]{};

    unsigned char mem[8192]{}; // 8kib

    unsigned char ir{};
    unsigned char ie{};
    unsigned short pc{};
    unsigned short sp{};

    enum Instruction {
        LD_0b01xxxyyy,
        LD_0b00xxx110,
        LD_0b01xxx110,
        LD_0b01110xxx,
        LD_0b00110110,
        LD_0b00001010,
        LD_0b00011010,
        LD_0b00000010,
        LD_0b00010010,
        LD_0b11111010,
        LD_0b11101010,
        LDH_0b11110010,
        LDH_0b11100010,
        LDH_0b11110000,
        LDH_0b11100000,
        LD_0b00111010,
        LD_0b00110010,
        LD_0b00101010,
        LD_0b00100010,
        LD_0b00xx0001,
        LD_0b00001000,
        LD_0b11111001,
        PUSH_0b11xx0101,
        POP_0b11xx0001,
        LD_0b11111000
    };

    Register greg(unsigned char i) {
        switch(i){
            case 0b000: {return B;}
            case 0b001: {return C;}
            case 0b010: {return D;}
            case 0b011: {return E;}
            case 0b100: {return H;}
            case 0b101: {return L;}
            case 0b111: {return A;}
        }
        throw std::invalid_argument("Unknown register");
    }

    auto read3_5(unsigned char i) {
        return (0x7 & (i >> 3));
    }

    auto read0_2(unsigned char i) {
        return (0x7 & i);
    }


    enum Flag {Zf, Nf, Hf, Cf};
    
    int flags_pos[4] = {7,6,5,4};
    
    void set_flag(Flag f, bool val) {
        if (val) {
            regs[F] |= (1 << flags_pos[f]);
        } else {
            regs[F] &= ~(1 << flags_pos[f]);
        }
    }

    void decrement_pair(Register l, Register h) {
        unsigned short pair = ((regs[h] << 8) | regs[l]);
        --pair;
        regs[h] = ((pair >> 8) & 0xff);
        regs[l] = pair & 0xff;
    }

    void increment_pair(Register l, Register h) {
        unsigned short pair = ((regs[h] << 8) | regs[l]);
        ++pair;
        regs[h] = ((pair >> 8) & 0xff);
        regs[l] = pair & 0xff;
    }

    void update_pair_ld(unsigned char l,unsigned char h, unsigned char pair) {
        switch(pair) {
            case 0b00: {regs[C] = l; regs[B] = h; break;}
            case 0b01: {regs[E] = l; regs[D] = h; break;}
            case 0b10: {regs[L] = l; regs[H] = h; break;}
            case 0b11: {sp = ((h << 8) | l); break;}
        }

    }
    void update_pair_pop(unsigned char l,unsigned char h, unsigned char pair) {
        switch(pair) {
            case 0b00: {regs[C] = l; regs[B] = h; break;}
            case 0b01: {regs[E] = l; regs[D] = h; break;}
            case 0b10: {regs[L] = l; regs[H] = h; break;}
            case 0b11: {regs[F] = l & 0xf0; regs[A] = h; break;} // todo: check
        }
    }
    unsigned short getpair(unsigned char i) {
        switch(i) {
            case 0b00: {return  (regs[C] | (regs[B] << 8)); break;}
            case 0b01: {return  (regs[E] | (regs[D] << 8)); break;}
            case 0b10: {return  (regs[L] | (regs[H] << 8)); break;}
            case 0b11: {return sp; break;}
        }
        throw std::invalid_argument("Unknown register");
    }

    auto read45(unsigned char i) {
        return ((0b00110000 & i) >> 4);
    }

    void exec(unsigned char i, Instruction itype) {
        switch(itype) {
            case LD_0b01xxxyyy: { regs[greg(read3_5(i))] = regs[greg(read0_2(i))]; ++pc; break;}
            case LD_0b01xxx110: { regs[greg(read3_5(i))] = mem[((regs[H] << 8) | regs[L])]; ++pc;  break;}
            case LD_0b00xxx110: { regs[greg(read3_5(i))] = mem[pc+1]; pc+=2; break;} 
            case LD_0b01110xxx: { mem[((regs[H] << 8)| regs[L])] = regs[greg(read0_2(i))]; ++pc; break;}
            case LD_0b00110110: { mem[((regs[H] << 8)| regs[L])] = mem[pc+1]; pc+=2; break;};
            case LD_0b00001010: { regs[A] = mem[((regs[B] << 8)| regs[C])]; ++pc; break;}
            case LD_0b00011010: { regs[A] = mem[((regs[D] << 8)| regs[E])]; ++pc; break;}
            case LD_0b00000010: { mem[((regs[B] << 8)| regs[C])] = regs[A]; ++pc; break;}
            case LD_0b00010010: { mem[((regs[D] << 8)| regs[E])] = regs[A]; ++pc; break;}
            case LD_0b11111010: { regs[A] = mem[mem[pc + 1] | (mem[pc + 2] << 8)]; pc+=3; break;}
            case LD_0b11101010: { mem[mem[pc + 1] | (mem[pc + 2] << 8)] = regs[A]; pc+=3; break;}
            case LDH_0b11110010: {regs[A] = mem[0xff00 | regs[C]]; ++pc; break;}
            case LDH_0b11100010: {mem[0xff00 | regs[C]] = regs[A]; ++pc; break;}
            case LDH_0b11110000: {regs[A] = mem[0xff00 | mem[pc+1]]; pc+=2;  break;}
            case LDH_0b11100000: {mem[0xff00 | mem[pc+1]] = regs[A]; pc+=2;  break;}
            case LD_0b00111010:  {regs[A] = mem[(regs[H] << 8) | regs[L]]; decrement_pair(L, H);  ++pc;  break;}
            case LD_0b00110010:  {mem[(regs[H] << 8) | regs[L]] = regs[A]; decrement_pair(L, H); ++pc; break;}
            case LD_0b00101010:  {regs[A] = mem[(regs[H] << 8) | regs[L]]; increment_pair(L, H);  ++pc;  break;}
            case LD_0b00100010: {mem[(regs[H] << 8) | regs[L]] = regs[A]; increment_pair(L, H); ++pc; break;}
            case LD_0b00xx0001: {update_pair_ld(mem[pc + 1], mem[pc+2], read45(i)); pc+=3; break;}
            case LD_0b00001000: {
                auto addess = (mem[pc + 1] | (mem[pc + 2] << 8));
                mem[addess] = (sp & 0xff);
                mem[addess+1] = ((sp>>8) & 0xff);
                pc+=3;
                break;    
            }
            case LD_0b11111001: {sp = (regs[L] | (regs[H] << 8)); ++pc; break;} // SP = HL
            case PUSH_0b11xx0101: {
                auto val = getpair(read45(i)); 
                sp-=2;  
                mem[sp] = ((val>>8) & 0xff);
                mem[sp+1] = (val & 0xff); 
                ++pc; 
                break;} // stack push xx
            case POP_0b11xx0001: {
                update_pair_pop(mem[sp+1], mem[sp], read45(i));
                sp+=2;
                ++pc;
                break;
            };
            case LD_0b11111000: {
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                auto e = static_cast<char>(mem[pc+1]);
                unsigned int result = e + sp;
                set_flag(Cf, ((sp & 0xff) + (e & 0xff) > 0xff));
                set_flag(Hf, ((sp & 0xf) + (e & 0xf) > 0xf));
                regs[H] = ((result >> 8) & 0xff);
                regs[L] = (result & 0xff);
                pc+=2;
                break;
            }
    }
}

};

int main() {

}

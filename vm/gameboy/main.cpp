#include <iostream>
#include "raylib.h"


struct GameBoyVM{
    enum Register {A, F, B, C, D, E, H, L};
    unsigned char regs[8]{};

    unsigned char mem[8192]{}; // 8kib
    unsigned char rst_vec[8]{0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38};

    unsigned char ir{};
    unsigned char ie{};
    unsigned short pc{};
    unsigned short sp{};
    
    bool ime = false;

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
        LD_0b11111000,
        ADD_0b10000xxx,
        ADD_0b10000110,
        ADD_0b11000110,
        ADC_0b10001xxx,
        ADC_0b10001110,
        ADC_0b11001110,
        SUB_0b10010xxx,
        SUB_0b10010110,
        SUB_0b11010110,
        SBC_0b10011xxx,
        SBC_0b10011110,
        SBC_0b11011110,
        CP_0b10111xxx,
        CP_0b10011110,
        CP_0b11111110,
        INC_0b00xxx100,
        INC_0b00110100,
        DEC_0b00xxx101,
        DEC_0b00110101,
        AND_0b10100xxx,
        AND_0b10100110,
        AND_0b11100110,
        OR_0b10110xxx,
        OR_0b10110110,
        OR_0b11110110,
        XOR_0b10101xxx,
        XOR_0b10101110,
        XOR_0b11101110,
        CCF_0b00111111,
        SCF_0b00110111,
        DAA_0b00100111,
        CPL_0b00101111,
        INC_0b00xx0011,
        DEC_0b00xx1011,
        ADD_0b00xx1001,
        ADD_0b11101000,

        RLCA_0b00000111,
        RRCA_0b00001111,
        RLA_0b00010111,
        RRA_0b00011111,
        RLC_0b00000xxx,
        RLC_0b00000110,
        RRC_0b00001xxx,
        RRC_0b00001110,
        RL_0b00010xxx,
        RL_0b00010110,
        RR_0b00011xxx,
        RR_0b00011110,
        SLA_0b00100xxx,
        SLA_0b00100110,
        SRA_0b00101xxx,
        SRA_0b00101110,
        SWAP_0b00110xxx,
        SWAP_0b00110110,
        SRL_0b00111xxx,
        SRL_0b00111110,
        BIT_0b01xxxxxx,
        BIT_0b01xxx110,
        RES_0b10xxxxxx,
        RES_0b10xxx110,
        SET_0b11xxxxxx,
        SET_0b11xxx110,
        
        // Control flow
        JP_0b11000011,
        JP_0b11101001,
        JP_0b110xx010,
        JR_0b00011000,
        JR_0b001xx000,
        CALL_0b11001101,
        CALL_0b110xx100,
        RET_0b11001001,
        RET_0b110xx000,
        RETI_0b11011001,
        RST_0b11xxx111,

        // Miscellaneous instructions
        HALT,
        STOP,
        DI_0b11110011,
        EI_0b11111011,
        NOP_0b00000000,

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

    unsigned char get_flag(Flag f) {
        return (regs[F] >> flags_pos[f]) & 1;
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
    void update_pair_pop(unsigned char l, unsigned char h, unsigned char pair) {
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

    auto read34(unsigned char i) {
        return ((0b0001'1000 & i) >> 3);
    }

    void add_reg_A(unsigned char val) {
        auto res = regs[A] + val;
        auto res8 = res & 0xff;
        set_flag(Nf, 0);
        set_flag(Zf, (res8 == 0));
        set_flag(Cf, (res > 0xff));
        set_flag(Hf, ((regs[A] & 0xf) + (val & 0xf) > 0xf));
        regs[A] = res8;
    }

    void add_reg_HL(unsigned short val) {
        auto& hl = reg_ref(HL);
        unsigned int result = hl + val;
        set_flag(Nf, 0);
        set_flag(Cf, (result > 0xffff));
        set_flag(Hf, ((val & 0x0fff) + (hl & 0x0fff) > 0x0fff));
        hl = (unsigned short)result;
    }

    void sub_reg_A(unsigned char val) {
        auto res = (unsigned short)regs[A] - (unsigned short)val;
        auto res8 = res & 0xff;
        set_flag(Nf, 1);
        set_flag(Zf, (res8 == 0));
        set_flag(Cf, (val > regs[A]));
        set_flag(Hf, ((val & 0xf) > (regs[A] & 0xf)));
        regs[A] = res8;
    }

    void and_reg_A(unsigned char val) {
        regs[A] &= val;
        set_flag(Nf, 0);
        set_flag(Zf, (regs[A] == 0));
        set_flag(Cf, 0);
        set_flag(Hf, 1);
    }

     void or_reg_A(unsigned char val) {
        regs[A] |= val;
        set_flag(Nf, 0);
        set_flag(Zf, (regs[A] == 0));
        set_flag(Cf, 0);
        set_flag(Hf, 0);
    }

     void xor_reg_A(unsigned char val) {
        regs[A] ^= val;
        set_flag(Nf, 0);
        set_flag(Zf, (regs[A] == 0));
        set_flag(Cf, 0);
        set_flag(Hf, 0);
    }

    void sub_reg_A_flags_only(unsigned char val) {
        auto res = (unsigned short)regs[A] - (unsigned short)val;
        auto res8 = res & 0xff;
        set_flag(Nf, 1);
        set_flag(Zf, (res8 == 0));
        set_flag(Cf, (val > regs[A]));
        set_flag(Hf, ((val & 0xf) > (regs[A] & 0xf)));
    }

    void inc_reg(unsigned char i) {
        auto& reg = reg_ref(i);
        auto cp = reg;
        reg+=1;
        set_flag(Nf, 0);
        set_flag(Zf, reg == 0);
        set_flag(Hf, (cp & 0xf) == 0xf);
    }

    void dec_reg(unsigned char i) {
        auto& reg = reg_ref(i);
        auto cp = reg;
        reg-=1;
        set_flag(Nf, 1);
        set_flag(Zf, reg == 0);
        set_flag(Hf, (cp & 0xf) == 0);
    }

    static constexpr unsigned char HL = 0b110;

    unsigned char& reg_ref(unsigned char i) {
        switch(i){
            case 0b000: {return regs[B];}
            case 0b001: {return regs[C];}
            case 0b010: {return regs[D];}
            case 0b011: {return regs[E];}
            case 0b100: {return regs[H];}
            case 0b101: {return regs[L];}
            case 0b110: {return mem[(regs[H] << 8) | regs[L]];}
            case 0b111: {return regs[A];}
        }
        throw std::invalid_argument("Unknown register");
    }

    void exec(unsigned char i, Instruction itype) {
        switch(itype) {
            case LD_0b01xxxyyy: { reg_ref(read3_5(i)) = reg_ref(read0_2(i)); ++pc; break;}
            case LD_0b01xxx110: { reg_ref(read3_5(i)) = reg_ref(HL); ++pc;  break;}
            case LD_0b00xxx110: { reg_ref(read3_5(i))  = mem[pc+1]; pc+=2; break;} 
            case LD_0b01110xxx: { reg_ref(HL) = reg_ref(read0_2(i)); ++pc; break;}
            case LD_0b00110110: { reg_ref(HL) = mem[pc+1]; pc+=2; break;};
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
            case LD_0b00111010:  {regs[A] = reg_ref(HL); decrement_pair(L, H);  ++pc;  break;}
            case LD_0b00110010:  {reg_ref(HL) = regs[A]; decrement_pair(L, H); ++pc; break;}
            case LD_0b00101010:  {regs[A] = reg_ref(HL); increment_pair(L, H);  ++pc;  break;}
            case LD_0b00100010:  {reg_ref(HL) = regs[A]; increment_pair(L, H); ++pc; break;}
            case LD_0b00xx0001:  {update_pair_ld(mem[pc + 1], mem[pc+2], read45(i)); pc+=3; break;}
            case LD_0b00001000:  {
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
                int result = e + sp;
                set_flag(Cf, ((sp & 0xff) +  (unsigned char)(e) > 0xff));
                set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
                regs[H] = ((result >> 8) & 0xff);
                regs[L] = (result & 0xff);
                pc+=2;
                break;
            }

            case ADD_0b10000xxx: { add_reg_A(reg_ref(read0_2(i))); ++pc; break;}
            case ADD_0b10000110: { add_reg_A(reg_ref(HL)); ++pc; break;}
            case ADD_0b11000110: { add_reg_A(mem[pc+1]); pc+=2; break;}
            case ADC_0b10001xxx: { add_reg_A(reg_ref(read0_2(i)) + get_flag(Cf));++pc;break;}
            case ADC_0b10001110: { add_reg_A(reg_ref(HL) + get_flag(Cf)); ++pc; break;}
            case ADC_0b11001110: { add_reg_A(mem[pc+1] + get_flag(Cf)); pc+=2; break;}
            case SUB_0b10010xxx: { sub_reg_A(reg_ref(read0_2(i))); ++pc; break;}
            case SUB_0b10010110: { sub_reg_A(reg_ref(HL)); ++pc; break;}
            case SUB_0b11010110: { sub_reg_A(mem[pc+1]); pc+=2; break;}
            case SBC_0b10011xxx: { sub_reg_A(reg_ref(read0_2(i)) + get_flag(Cf)); ++pc; break;} // (reg[A] - r - reg[C]) == reg[A] - (r + reg[C])
            case SBC_0b10011110: { sub_reg_A(reg_ref(HL) + get_flag(Cf)); ++pc; break;}
            case SBC_0b11011110: { sub_reg_A(mem[pc+1] + get_flag(Cf)); pc+=2; break;}
            case CP_0b10111xxx:  { sub_reg_A_flags_only(reg_ref(read0_2(i))); ++pc; break;}
            case CP_0b10011110:  { sub_reg_A_flags_only(reg_ref(HL)); ++pc; break;}
            case CP_0b11111110:  { sub_reg_A_flags_only(mem[pc+1]); pc+=2; break;}
            case INC_0b00xxx100: { inc_reg(read3_5(i)); ++pc; break;}
            case INC_0b00110100: { inc_reg(HL); ++pc; break;}
            case DEC_0b00xxx101: { dec_reg(read3_5(i)); ++pc; break;}
            case DEC_0b00110101: { dec_reg(HL); ++pc; break;}
            case AND_0b10100xxx: { and_reg_A(reg_ref(read0_2(i))); ++pc; break;}
            case AND_0b10100110: { and_reg_A(reg_ref(HL)); ++pc; break;}
            case AND_0b11100110: { and_reg_A(mem[pc+1]); pc+=2; break;}
            case OR_0b10110xxx:  { or_reg_A(reg_ref(read0_2(i))); ++pc; break;}
            case OR_0b10110110:  { or_reg_A(reg_ref(HL)); ++pc; break;}
            case OR_0b11110110:  { or_reg_A(mem[pc+1]); pc+=2; break;}
            case XOR_0b10101xxx: { xor_reg_A(reg_ref(read0_2(i))); ++pc; break;}
            case XOR_0b10101110: { xor_reg_A(reg_ref(HL)); ++pc; break;}
            case XOR_0b11101110: { xor_reg_A(mem[pc+1]); pc+=2; break;}
            case CCF_0b00111111: { set_flag(Cf, !get_flag(Cf)); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; break;}
            case SCF_0b00110111: { set_flag(Cf, 1); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; break;}
            case DAA_0b00100111: { /*todo*/ ++pc; break;}
            case CPL_0b00101111: {regs[A] = ~regs[A]; set_flag(Nf, 1); set_flag(Hf, 1); ++pc; break;}
            case INC_0b00xx0011: { 
                auto rr = read45(i);  
                unsigned short pair = getpair(rr); 
                ++pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                ++pc;
                break;
            }
            case DEC_0b00xx1011: {
                auto rr = read45(i);  
                unsigned short pair = getpair(rr); 
                --pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                ++pc;
                break;
            }
            case ADD_0b00xx1001: { add_reg_HL(getpair(read45(i))); ++pc; break;}
            case ADD_0b11101000: {
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                auto e = static_cast<char>(mem[pc+1]);
                int result = e + sp;
                set_flag(Cf, ((sp & 0xff) +  (unsigned char)(e) > 0xff));
                set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
                sp = (unsigned short)(result);
                pc+=2;
                break;
            }
            case RLCA_0b00000111: {/*todo*/ ++pc;  break;}
            case RRCA_0b00001111: {/*todo*/ ++pc;  break;}
            case RLA_0b00010111:  {/*todo*/ ++pc;  break;}
            case RRA_0b00011111:  {/*todo*/ ++pc;  break;}
            case RLC_0b00000xxx:  {/*todo*/ pc+=2; break;}
            case RLC_0b00000110:  {/*todo*/ pc+=2; break;}
            case RRC_0b00001xxx:  {/*todo*/ pc+=2; break;}
            case RRC_0b00001110:  {/*todo*/ pc+=2; break;}
            case RL_0b00010xxx:   {/*todo*/ pc+=2; break;}
            case RL_0b00010110:   {/*todo*/ pc+=2; break;}
            case RR_0b00011xxx:   {/*todo*/ pc+=2; break;}
            case RR_0b00011110:   {/*todo*/ pc+=2; break;}
            case SLA_0b00100xxx:  {/*todo*/ pc+=2; break;}
            case SLA_0b00100110:  {/*todo*/ pc+=2; break;}
            case SRA_0b00101xxx:  {/*todo*/ pc+=2; break;}
            case SRA_0b00101110:  {/*todo*/ pc+=2; break;}
            case SWAP_0b00110xxx: {/*todo*/ pc+=2; break;}
            case SWAP_0b00110110: {/*todo*/ pc+=2; break;}
            case SRL_0b00111xxx:  {/*todo*/ pc+=2; break;}
            case SRL_0b00111110:  {/*todo*/ pc+=2; break;}
            case BIT_0b01xxxxxx:  {/*todo*/ pc+=2; break;}
            case BIT_0b01xxx110:  {/*todo*/ pc+=2; break;}
            case RES_0b10xxxxxx:  {/*todo*/ pc+=2; break;}
            case RES_0b10xxx110:  {/*todo*/ pc+=2; break;}
            case SET_0b11xxxxxx:  {/*todo*/ pc+=2; break;}
            case SET_0b11xxx110:  {/*todo*/ pc+=2; break;}
            // Control flow

            case JP_0b11000011: { pc = ((mem[pc+2] << 8) | mem[pc + 1]); break;}
            case JP_0b11101001: { pc = reg_ref(HL); break;}
            case JP_0b110xx010: { 
                if(!get_flag(Zf))
                    pc = ((mem[pc+2] << 8) | mem[pc + 1]);
                else
                    pc += 3;
                break;
            }
            case JR_0b00011000: {pc += 2 + (char)(mem[pc+1]); break;}
            case JR_0b001xx000: {
                unsigned char val = read34(i);
                auto jmp = [&]{pc += 2 + (char)(mem[pc+1]);};
                if (val == 0b00) {
                    if(!get_flag(Zf)) {jmp();}else{pc+= 2;}
                } else if (val == 0b01) {
                    if(get_flag(Zf)) {jmp();}else{pc+= 2;}
                } else if (val == 0b10) {
                    if(!get_flag(Cf)) {jmp();}else{pc+= 2;}
                } else if (val == 0b11) {
                    if(get_flag(Cf)) {jmp();}else{pc+= 2;}
                }
                break;
            }
            case CALL_0b11001101: {
                unsigned short nn = ((mem[pc + 2] << 8) | mem[pc + 1]);
                sp-=2;
                mem[sp] = (pc + 3) & 0xff;
                mem[sp+1] = ((pc + 3) >> 8);
                pc = nn;
                break;
            }
            case CALL_0b110xx100: {
                auto val = read34(i);
                unsigned short nn = ((mem[pc + 2] << 8) | mem[pc + 1]);
                auto call = [&]{
                    sp-=2;
                    mem[sp] = (pc + 3) & 0xff;
                    mem[sp+1] = ((pc + 3) >> 8);
                    pc = nn;
                };
                if (val == 0b00) {
                    if(!get_flag(Zf)) {call();}else{pc+= 3;} 
                } else if (val == 0b01) {
                    if(get_flag(Zf)) {call();}else{pc+=3;}
                } else if (val == 0b10) {
                    if(!get_flag(Cf)) {call();}else{pc+=3;}
                } else if (val == 0b11) {
                    if(get_flag(Cf)) {call();}else{pc+=3;}
                }
                break;
            }
            case RET_0b11001001: { pc = ((mem[sp + 1] << 8) | mem[sp]); sp+=2; break;}
            case RET_0b110xx000: {
                auto val = read34(i);
                auto ret = [&]{
                    pc = ((mem[sp + 1] << 8) | mem[sp]); sp+=2;
                };
                if (val == 0b00) {
                    if(!get_flag(Zf)) {ret();}else{++pc;} 
                } else if (val == 0b01) {
                    if(get_flag(Zf)) {ret();}else{++pc;}
                } else if (val == 0b10) {
                    if(!get_flag(Cf)) {ret();}else{++pc;}
                } else if (val == 0b11) {
                    if(get_flag(Cf)) {ret();}else{++pc;}
                }
                break;
            }
            case RETI_0b11011001: {pc = ((mem[sp + 1] << 8) | mem[sp]); sp+=2; ime = true; break;}
            case RST_0b11xxx111: {
                unsigned short nn = ((0x0 << 8) | rst_vec(read3_5(i)));
                sp-=2;
                mem[sp] = (pc + 1) & 0xff;
                mem[sp+1] = ((pc + 1) >> 8);
                pc = nn;
                break;
            }


            case HALT: {/*todo*/ break;}
            case STOP: {/*todo*/ break;}
            case DI_0b11110011: {ime = false; ++pc; break;}
            case EI_0b11111011: {ime = true; ++pc; break;} // todo: check
            case NOP_0b00000000: {++pc; break;}
    }
}

};

int main() {

}

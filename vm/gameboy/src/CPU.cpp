#include "CPU.h"
#include <chrono>

#define DEBUG 0

unsigned char CPU::read345(unsigned char i)  {
    return (0x7 & (i >> 3));
}
unsigned char CPU::read012(unsigned char i)  {
    return (0x7 & i);
}

unsigned char CPU::read45(unsigned char i)  {
    return ((0b0011'0000 & i) >> 4);
}

unsigned char CPU::read34(unsigned char i)  {
    return ((0b0001'1000 & i) >> 3);
}

void CPU::add_reg_A(unsigned char val)  {
    auto res = regs[A] + val;
    auto res8 = res & 0xff;
    set_flag(Nf, 0);
    set_flag(Zf, (res8 == 0));
    set_flag(Cf, (res > 0xff));
    set_flag(Hf, ((regs[A] & 0xf) + (val & 0xf) > 0xf));
    regs[A] = res8;
}

void CPU::add_reg_HL(unsigned short val)  {
    unsigned short hl = (regs[H] << 8) | regs[L];
    unsigned int result = hl + val;
    set_flag(Nf, 0);
    set_flag(Cf, (result > 0xffff));
    set_flag(Hf, ((val & 0x0fff) + (hl & 0x0fff) > 0x0fff));
    regs[H] = (result >> 8) & 0xff;
    regs[L] = result & 0xff;
}

void CPU::sub_reg_A(unsigned char val) {
    auto res = (int)regs[A] - (int)val;
    auto res8 = res & 0xff;
    set_flag(Nf, 1);
    set_flag(Zf, (res8 == 0));
    set_flag(Cf, (val > regs[A]));
    set_flag(Hf, ((val & 0xf) > (regs[A] & 0xf)));
    regs[A] = res8;
}

void CPU::and_reg_A(unsigned char val)  {
    regs[A] &= val;
    set_flag(Nf, 0);
    set_flag(Zf, (regs[A] == 0));
    set_flag(Cf, 0);
    set_flag(Hf, 1);
}

void CPU::or_reg_A(unsigned char val)  {
    regs[A] |= val;
    set_flag(Nf, 0);
    set_flag(Zf, (regs[A] == 0));
    set_flag(Cf, 0);
    set_flag(Hf, 0);
}

void CPU::xor_reg_A(unsigned char val)  {
    regs[A] ^= val;
    set_flag(Nf, 0);
    set_flag(Zf, (regs[A] == 0));
    set_flag(Cf, 0);
    set_flag(Hf, 0);
}

void CPU::sub_reg_A_flags_only(unsigned char val)  {
    auto res = (int)regs[A] - (int)val;
    auto res8 = (unsigned char)(res & 0xff);
    set_flag(Nf, 1);
    set_flag(Zf, (res8 == 0));
    set_flag(Cf, (val > regs[A]));
    set_flag(Hf, ((regs[A] & 0xf) - (val & 0xf) < 0)); // todo: check((val & 0xf) > (regs[A] & 0xf)));
}

void CPU::inc_reg(unsigned char i, RAM &ram)  {
    auto reg = read_regs(i, ram);
    auto cp = reg;
    unsigned char result = reg + 1;
    set_flag(Nf, 0);
    set_flag(Zf, result == 0);
    set_flag(Hf, (cp & 0xf) == 0xf);
    update_regs(i, ram, result);
}

void CPU::dec_reg(unsigned char i, RAM &ram)  {
    auto reg = read_regs(i, ram);
    auto cp = reg;
    unsigned char result = reg - 1;
    set_flag(Nf, 1);
    set_flag(Zf, result == 0);
    set_flag(Hf, (cp & 0xf) == 0);
    update_regs(i, ram, result);
}

unsigned char CPU::read_regs(unsigned char i, RAM &ram)  {
    switch(i){
        case 0b000: {return regs[B];}
        case 0b001: {return regs[C];}
        case 0b010: {return regs[D];}
        case 0b011: {return regs[E];}
        case 0b100: {return regs[H];}
        case 0b101: {return regs[L];}
        case 0b110: {return ram.read(HL_data());}
        case 0b111: {return regs[A];}
    }
    throw std::invalid_argument("Unknown register");
}

void CPU::update_regs(unsigned char i, RAM &ram, unsigned char value)  {
    switch(i){
        case 0b000: {regs[B] = value; return;}
        case 0b001: {regs[C] = value; return;}
        case 0b010: {regs[D] = value; return;}
        case 0b011: {regs[E] = value; return;}
        case 0b100: {regs[H] = value; return;}
        case 0b101: {regs[L] = value; return;}
        case 0b110: {ram.write(HL_data(), value); return;}
        case 0b111: {regs[A] = value; return;}
    }
    throw std::invalid_argument("Unknown register");
}


bool CPU::get_cc_flag(unsigned char val)  {
    return (val == 0b00 && !get_flag(Zf) ||
            val == 0b01 &&  get_flag(Zf) ||
            val == 0b10 && !get_flag(Cf) ||
            val == 0b11 && get_flag(Cf));
}

unsigned char CPU::fetch(const RAM &ram) const  {
    return ram.read(pc);
}

CPU::Instruction CPU::decode(unsigned char ins)  {
    switch (ins) {
        case 0b00110110: return LD_0b00110110;
        case 0b00001010: return LD_0b00001010;
        case 0b00011010: return LD_0b00011010;
        case 0b00000010: return LD_0b00000010;
        case 0b00010010: return LD_0b00010010;
        case 0b11111010: return LD_0b11111010;
        case 0b11101010: return LD_0b11101010;
        case 0b11110010: return LDH_0b11110010;
        case 0b11100010: return LDH_0b11100010;
        case 0b11110000: return LDH_0b11110000;
        case 0b11100000: return LDH_0b11100000;
        case 0b00111010: return LD_0b00111010;
        case 0b00110010: return LD_0b00110010;
        case 0b00101010: return LD_0b00101010;
        case 0b00100010: return LD_0b00100010;
        case 0b00001000: return LD_0b00001000;
        case 0b11111001: return LD_0b11111001;
        case 0b11111000: return LD_0b11111000;
        case 0b10000110: return ADD_0b10000110;
        case 0b11000110: return ADD_0b11000110;
        case 0b10001110: return ADC_0b10001110;
        case 0b11001110: return ADC_0b11001110;
        case 0b10010110: return SUB_0b10010110;
        case 0b11010110: return SUB_0b11010110;
        case 0b10011110: return SBC_0b10011110;
        case 0b11011110: return SBC_0b11011110;
        case 0b10111110: return CP_0b10111110;
        case 0b11111110: return CP_0b11111110;
        case 0b00110100: return INC_0b00110100;
        case 0b00110101: return DEC_0b00110101;
        case 0b10100110: return AND_0b10100110;
        case 0b11100110: return AND_0b11100110;
        case 0b10110110: return OR_0b10110110;
        case 0b11110110: return OR_0b11110110;
        case 0b10101110: return XOR_0b10101110;
        case 0b11101110: return XOR_0b11101110;
        case 0b00111111: return CCF_0b00111111;
        case 0b00110111: return SCF_0b00110111;
        case 0b00100111: return DAA_0b00100111;
        case 0b00101111: return CPL_0b00101111;
        case 0b11101000: return ADD_0b11101000;
        case 0b00000111: return RLCA_0b00000111;
        case 0b00001111: return RRCA_0b00001111;
        case 0b00010111: return RLA_0b00010111;
        case 0b00011111: return RRA_0b00011111;

        case 0b11000011: return JP_0b11000011;
        case 0b11101001: return JP_0b11101001;
        case 0b00011000: return JR_0b00011000;
        case 0b11001101: return CALL_0b11001101;
        case 0b11001001: return RET_0b11001001;
        case 0b11011001: return RETI_0b11011001;
        case 0b11110011: return DI_0b11110011;
        case 0b11111011: return EI_0b11111011;
        case 0b00000000: return NOP_0b00000000;

        case 0xCB: return _0xCB;
        case 0x76: return HALT;
        case 0x10: return STOP;
    }

    if (((ins >> 5) & 0x7) == 0b110) {
        if ((ins & 0x7) == 0b010) return JP_0b110xx010;
        if ((ins & 0x7) == 0b100) return CALL_0b110xx100;
        if ((ins & 0x7) == 0b000) return RET_0b110xx000;

    }

    if (((ins >> 5) & 0x7) == 0b001) {
        if ((ins & 0x7) == 0b000) return JR_0b001xx000;
    }

    switch ((ins >> 3) & 0x1f) {
        case 0b10000: return ADD_0b10000xxx;
        case 0b10001: return ADC_0b10001xxx;
        case 0b10010: return SUB_0b10010xxx;
        case 0b10011: return SBC_0b10011xxx;
        case 0b10111: return CP_0b10111xxx;
        case 0b10100: return AND_0b10100xxx;
        case 0b10110: return OR_0b10110xxx;
        case 0b10101: return XOR_0b10101xxx;
    }

    if ((ins >> 6 & 0x3) == 0b11) {
        if ((ins & 0xf) == 0b0101) return PUSH_0b11xx0101;
        if ((ins & 0xf) == 0b0001) return POP_0b11xx0001;
        if ((ins & 0x7) == 0b111) return RST_0b11xxx111;
    }

    if ((ins >> 6 & 0x3) == 0b01) {
        if ((ins & 0x7) == 0b110) return LD_0b01xxx110;
        if ((ins >> 3 & 0x7) == 0b110) return LD_0b01110xxx;
        return LD_0b01xxxyyy;
    }

    if ((ins >> 6 & 0x3) == 0b00) {

        if ((ins & 0x7) == 0b101) return DEC_0b00xxx101;
        if ((ins & 0x7) == 0b100) return INC_0b00xxx100;
        if ((ins & 0x7) == 0b110) return LD_0b00xxx110;


        if ((ins & 0xf) == 0b0001) return LD_0b00xx0001;
        if ((ins & 0xf) == 0b0011) return INC_0b00xx0011;
        if ((ins & 0xf) == 0b1011) return DEC_0b00xx1011;
        if ((ins & 0xf) == 0b1001) return ADD_0b00xx1001;
    }
#if DEBUG
    std::cout << "Unknown opcode: " << ins << '\n';
#endif
    return NOP_0b00000000;
}

CPU::Instruction0xCB CPU::decode_0xCB(unsigned char ins) {
    switch ((ins >> 6) & 0x3) {
        case 0b01: return BIT_0xCB_0b01xxxxxx;
        case 0b10: return RES_0xCB_0b10xxxxxx;
        case 0b11: return SET_0xCB_0b11xxxxxx;
    }
    switch ((ins >> 3) & 0x1f) {
        case 0b00000: return RLC_0xCB_0b00000xxx;
        case 0b00001: return RRC_0xCB_0b00001xxx;
        case 0b00010: return RL_0xCB_0b00010xxx;
        case 0b00011: return RR_0xCB_0b00011xxx;
        case 0b00100: return SLA_0xCB_0b00100xxx;
        case 0b00101: return SRA_0xCB_0b00101xxx;
        case 0b00110: return SWAP_0xCB_0b00110xxx;
        case 0b00111: return SRL_0xCB_0b00111xxx;
    }
#if DEBUG
    std::cout << "Unknown opcode: " << ins << '\n';
#endif
    return NOP_0xCB_0b00000000;
}

unsigned short CPU::HL_data() const {
    return ((regs[H] << 8) | regs[L]);
}

void CPU::set_flag(CPU::Flag f, bool val)  {
    if (val) {
        regs[F] |= (1 << flags_pos[f]);
    } else {
        regs[F] &= ~(1 << flags_pos[f]);
    }
}

unsigned char CPU::get_flag(CPU::Flag f) const {
    unsigned char value = (regs[F] >> flags_pos[f]) & 1;
    return value;
}

void CPU::decrement_pair(CPU::Register l, CPU::Register h)  {
    unsigned short pair = ((regs[h] << 8) | regs[l]);
    --pair;
    regs[h] = ((pair >> 8) & 0xff);
    regs[l] = pair & 0xff;
}

void CPU::increment_pair(CPU::Register l, CPU::Register h)  {
    unsigned short pair = ((regs[h] << 8) | regs[l]);
    ++pair;
    regs[h] = ((pair >> 8) & 0xff);
    regs[l] = pair & 0xff;
}

void CPU::update_pair_ld(unsigned char l, unsigned char h, unsigned char pair)  {
    switch(pair) {
        case 0b00: {regs[C] = l; regs[B] = h; break;}
        case 0b01: {regs[E] = l; regs[D] = h; break;}
        case 0b10: {regs[L] = l; regs[H] = h; break;}
        case 0b11: {sp = ((h << 8) | l); break;}
    }
}

void CPU::update_pair_pop(unsigned char l, unsigned char h, unsigned char pair)  {
    switch(pair) {
        case 0b00: {regs[C] = l; regs[B] = h; break;}
        case 0b01: {regs[E] = l; regs[D] = h; break;}
        case 0b10: {regs[L] = l; regs[H] = h; break;}
        case 0b11: {regs[F] = (l & 0xf0); regs[A] = h; break;} // todo: check
    }
}

unsigned short CPU::getpair(unsigned char i)  {
    switch(i) {
        case 0b00: {return  (regs[C] | (regs[B] << 8));}
        case 0b01: {return  (regs[E] | (regs[D] << 8));}
        case 0b10: {return  (regs[L] | (regs[H] << 8));}
//        case 0b11: {return sp;} // todo: check
        case 0b11: {return (regs[F] | regs[A] << 8);}
    }
    throw std::invalid_argument("Unknown register");
}

int CPU::t_cycle_by_ins_0xCB(CPU::Instruction0xCB ins, unsigned char value) {
    switch (ins) {
        case RLC_0xCB_0b00000xxx: {return value == 0x06 ? 16 : 8;} // RLC (HL): Rotate left circular (indirect HL) - 4 (0x06)
        case RRC_0xCB_0b00001xxx: {return value == 0x0E ? 16 : 8;} // RRC (HL): Rotate right circular (indirect HL) - 4 (0x0E)
        case RL_0xCB_0b00010xxx:  {return value == 0x16 ? 16 : 8;} // RL (HL): Rotate left (indirect HL) - 4 (0x16)
        case RR_0xCB_0b00011xxx:  {return value == 0x1E ? 16 : 8;} // RR (HL): Rotate right (indirect HL) - 4 (0x1E)
        case SLA_0xCB_0b00100xxx: {return value == 0x26 ? 16 : 8;} // SLA (HL): Shift left arithmetic (indirect HL) - 4 (0x26)
        case SRA_0xCB_0b00101xxx: {return value == 0x2E ? 16 : 8;} // SRA (HL): Shift right arithmetic (indirect HL) - 4 (0x2E)
        case SWAP_0xCB_0b00110xxx:{return value == 0x36 ? 16 : 8;} // SWAP (HL): Swap nibbles (indirect HL) - 4 (0x36)
        case SRL_0xCB_0b00111xxx: {return value == 0x3E ? 16 : 8;} // SRL (HL): Shift right logical (indirect HL) - 4 (0x3E)
        case BIT_0xCB_0b01xxxxxx: {return (value & 0x07) == 6 ? 12 : 8;} // BIT b, (HL): Test bit (indirect HL) - 3(0b01xxx110)
        case RES_0xCB_0b10xxxxxx: {return (value & 0x07) == 6 ? 16 : 8;} // RES b, (HL): Reset bit (indirect HL) - 4(0b10xxx110)
        case SET_0xCB_0b11xxxxxx: {return (value & 0x07) == 6 ? 16 : 8;} // SET b, (HL): Set bit (indirect HL) - 4 (0b11xxx110)
        case NOP_0xCB_0b00000000: {return 1;}
    }
}

int CPU::t_cycle_by_ins(CPU::Instruction ins, unsigned char value)  { // value - 0xCB pc + 1 / cc_flag
    switch(ins) {
        case LD_0b01xxxyyy:       {return 4;}
        case LD_0b00xxx110:       {return 8;}
        case LD_0b01xxx110:       {return 8;}
        case LD_0b01110xxx:       {return 8;}
        case LD_0b00110110:       {return 12;}
        case LD_0b00001010:       {return 8;}
        case LD_0b00011010:       {return 8;}
        case LD_0b00000010:       {return 8;}
        case LD_0b00010010:       {return 8;}
        case LD_0b11111010:       {return 16;}
        case LD_0b11101010:       {return 16;}
        case LDH_0b11110010:      {return 8;}
        case LDH_0b11100010:      {return 8;}
        case LDH_0b11110000:      {return 12;}
        case LDH_0b11100000:      {return 12;}
        case LD_0b00111010:       {return 8;}
        case LD_0b00110010:       {return 8;}
        case LD_0b00101010:       {return 8;}
        case LD_0b00100010:       {return 8;}
        case LD_0b00xx0001:       {return 12;}
        case LD_0b00001000:       {return 20;}
        case LD_0b11111001:       {return 8;}
        case PUSH_0b11xx0101:     {return 16;}
        case POP_0b11xx0001:      {return 12;}
        case LD_0b11111000:       {return 12;}
        case ADD_0b10000xxx:      {return 4;}
        case ADD_0b10000110:      {return 8;}
        case ADD_0b11000110:      {return 8;}
        case ADC_0b10001xxx:      {return 4;}
        case ADC_0b10001110:      {return 8;}
        case ADC_0b11001110:      {return 8;}
        case SUB_0b10010xxx:      {return 4;}
        case SUB_0b10010110:      {return 8;}
        case SUB_0b11010110:      {return 8;}
        case SBC_0b10011xxx:      {return 4;}
        case SBC_0b10011110:      {return 8;}
        case SBC_0b11011110:      {return 8;}
        case CP_0b10111xxx:       {return 4;}
        case CP_0b10111110:       {return 8;}
        case CP_0b11111110:       {return 8;}
        case INC_0b00xxx100:      {return 4;}
        case INC_0b00110100:      {return 12;}
        case DEC_0b00xxx101:      {return 4;}
        case DEC_0b00110101:      {return 12;}
        case AND_0b10100xxx:      {return 4;}
        case AND_0b10100110:      {return 8;}
        case AND_0b11100110:      {return 8;}
        case OR_0b10110xxx:       {return 4;}
        case OR_0b10110110:       {return 8;}
        case OR_0b11110110:       {return 8;}
        case XOR_0b10101xxx:      {return 4;}
        case XOR_0b10101110:      {return 8;}
        case XOR_0b11101110:      {return 8;}
        case CCF_0b00111111:      {return 4;}
        case SCF_0b00110111:      {return 4;}
        case DAA_0b00100111:      {return 4;}
        case CPL_0b00101111:      {return 4;}
        case INC_0b00xx0011:      {return 8;}
        case DEC_0b00xx1011:      {return 8;}
        case ADD_0b00xx1001:      {return 8;}
        case ADD_0b11101000:      {return 16;}
        case RLCA_0b00000111:     {return 4;}
        case RRCA_0b00001111:     {return 4;}
        case RLA_0b00010111:      {return 4;}
        case RRA_0b00011111:      {return 4;}
        case _0xCB:               {return 0;} // use 0xCB version
        case JP_0b11000011:       {return 16;}
        case JP_0b11101001:       {return 4;}
        case JP_0b110xx010:       {return value ? 16 : 12;} // n 4 machine cycles (cc=true) / 3 machine cycles (cc=false)
        case JR_0b00011000:       {return 12;}
        case JR_0b001xx000:       {return value ? 12 : 8;} // 3 machine cycles (cc=true)/ 2 machine cycles (cc=false)
        case CALL_0b11001101:     {return 24;}
        case CALL_0b110xx100:     {return value ? 24 : 12;} // 6 machine cycles (cc=true) / 3 machine cycles (cc=false)
        case RET_0b11001001:      {return 16;}
        case RET_0b110xx000:      {return value ? 20 : 8;} // n 5 machine cycles (cc=true)/2 machine cycles (cc=false)
        case RETI_0b11011001:     {return 16;}
        case RST_0b11xxx111:      {return 16;}

        case HALT:                {return 0;}
        case STOP:                {return 16;}
        case DI_0b11110011:       {return 4;}
        case EI_0b11111011:       {return 4;}
        case NOP_0b00000000:      {return 4;}
    }
}

int CPU::execute(unsigned char i, CPU::Instruction itype, RAM &ram)  {
#if DEBUG
//    std::cout << "HL value:" << HL_data() << '\n';
//    std::cout << "RAM value by HL_address:" << (unsigned)ram.read(HL_data()) << '\n';
//    auto print_range = [&](int n){
//        while(n--) std::cout << "mem[" << pc + n << "] :" << (unsigned)ram.read(pc + n) << '\n';
//    };

//    std::cout << "\n==============================\n";

//    std::cout << std::hex << "REG[A]" << (unsigned)regs[A] << '\n';
//    std::cout <<"REG[B]: " << (unsigned)regs[B] << '\n';
//    std::cout <<"REG[C]: " << (unsigned)regs[C] << '\n';
//    std::cout <<"REG[D]: " << (unsigned)regs[D] << '\n';
//    std::cout <<"REG[E]: " << (unsigned)regs[E] << '\n';
//    std::cout <<"REG[F]: " << (unsigned)regs[F] << '\n';
//    std::cout << "Zero:" << (unsigned)get_flag(Zf) << '\n';
//    std::cout << "Negative: " << (unsigned)get_flag(Nf) << '\n';
//    std::cout << "HalfCarry: " << (unsigned)get_flag(Hf) << '\n';
//    std::cout << "Carry: " << (unsigned)get_flag(Cf) << '\n';
//    std::cout << "PC: " << pc << '\n';
//    std::cout << "SP: " << sp << '\n';
//    std::cout << "MEM:\n";
//    print_range(3);
//    std::cout << std::dec;
//    std::cout << "\n==============================\n";

#endif
//    std::cout << instruction_str[itype] << " ";
    switch(itype) {
        case LD_0b01xxxyyy:  { update_regs(read345(i), ram, read_regs(read012(i), ram)); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b01xxx110:  { update_regs(read345(i), ram, read_regs(HL, ram)); ++pc;  return t_cycle_by_ins(itype);}
        case LD_0b00xxx110:  { update_regs(read345(i), ram, ram.read(pc + 1)); pc+=2; return t_cycle_by_ins(itype);}
        case LD_0b01110xxx:  { update_regs(HL, ram, read_regs(read012(i), ram)); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b00110110:  { update_regs(HL, ram, ram.read(pc + 1));  pc+=2; return t_cycle_by_ins(itype);};
        case LD_0b00001010:  { regs[A] = ram.read(((regs[B] << 8)| regs[C])); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b00011010:  { regs[A] = ram.read(((regs[D] << 8)| regs[E])); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b00000010:  { ram.write(((regs[B] << 8)| regs[C]), regs[A]); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b00010010:  { ram.write(((regs[D] << 8)| regs[E]), regs[A]); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b11111010:  { regs[A] = ram.read(ram.read(pc + 1) | (ram.read(pc + 2) << 8)); pc+=3; return t_cycle_by_ins(itype);}
        case LD_0b11101010:  { ram.write((ram.read(pc + 1) | (ram.read(pc + 2) << 8)), regs[A]); pc+=3; return t_cycle_by_ins(itype);}
        case LDH_0b11110010: { regs[A] = ram.read(0xff00 | regs[C]); ++pc; return t_cycle_by_ins(itype);}
        case LDH_0b11100010: { ram.write((0xff00 | regs[C]), regs[A]); ++pc; return t_cycle_by_ins(itype);}
        case LDH_0b11110000: {
            unsigned short address = (0xff00 | ram.read(pc+1));
            regs[A] = ram.read(address);
            pc+=2;
            return t_cycle_by_ins(itype);
        }
        case LDH_0b11100000: { ram.write((0xff00 | ram.read(pc+1)), regs[A]); pc+=2;  return t_cycle_by_ins(itype);}
        case LD_0b00111010:  { regs[A] = read_regs(HL, ram); decrement_pair(L, H);  ++pc;  return t_cycle_by_ins(itype);}
        case LD_0b00110010:  { update_regs(HL, ram, regs[A]); decrement_pair(L, H); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b00101010:  { regs[A] = read_regs(HL, ram); increment_pair(L, H);  ++pc;  return t_cycle_by_ins(itype);}
        case LD_0b00100010:  { update_regs(HL, ram, regs[A]); increment_pair(L, H); ++pc; return t_cycle_by_ins(itype);}
        case LD_0b00xx0001:  { update_pair_ld(ram.read(pc + 1), ram.read(pc+2), read45(i)); pc+=3; return t_cycle_by_ins(itype);}
        case LD_0b00001000:  {
            unsigned short address = (ram.read(pc + 1) | (ram.read(pc + 2) << 8));
            ram.write(address, sp & 0xff);
            ram.write(address + 1, ((sp>>8) & 0xff));
            pc+=3;
            return t_cycle_by_ins(itype);
        }
        case LD_0b11111001: {sp = (regs[L] | (regs[H] << 8)); ++pc; return t_cycle_by_ins(itype);} // SP = HL
        case PUSH_0b11xx0101: {
            auto val = getpair(read45(i));
            sp-=2;
            ram.write(sp, (val & 0xff));
            ram.write(sp + 1, ((val>>8) & 0xff));
            ++pc;
            return t_cycle_by_ins(itype);} // stack push xx
        case POP_0b11xx0001: {
            update_pair_pop(ram.read(sp) , ram.read(sp+1), read45(i));
            sp+=2;
            ++pc;
            return t_cycle_by_ins(itype);
        };
        case LD_0b11111000: {
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Nf, 0);
            auto e = static_cast<char>(ram.read(pc+1));
            unsigned int result = e + sp;
            set_flag(Cf, ((sp & 0xff) + (unsigned char)(e) > 0xff));
            set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
            regs[H] = ((result >> 8) & 0xff);
            regs[L] = (result & 0xff);
            pc+=2;
            return t_cycle_by_ins(itype);
        }

        case ADD_0b10000xxx: { add_reg_A(read_regs(read012(i), ram)); ++pc; return t_cycle_by_ins(itype);}
        case ADD_0b10000110: { add_reg_A(read_regs(HL, ram)); ++pc;  return t_cycle_by_ins(itype);}
        case ADD_0b11000110: { add_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
        case ADC_0b10001xxx: { add_reg_A(read_regs(read012(i), ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);}
        case ADC_0b10001110: { add_reg_A(read_regs(HL, ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);}
        case ADC_0b11001110: { add_reg_A(ram.read(pc+1) + get_flag(Cf)); pc+=2; return t_cycle_by_ins(itype);}
        case SUB_0b10010xxx: { sub_reg_A(read_regs(read012(i), ram)); ++pc; return t_cycle_by_ins(itype);}
        case SUB_0b10010110: { sub_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
        case SUB_0b11010110: { sub_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
        case SBC_0b10011xxx: { sub_reg_A(read_regs(read012(i), ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);} // (reg[A] - r - reg[C]) == reg[A] - (r + reg[C])
        case SBC_0b10011110: { sub_reg_A(read_regs(HL, ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);}
        case SBC_0b11011110: { sub_reg_A(ram.read(pc+1) + get_flag(Cf)); pc+=2; return t_cycle_by_ins(itype);}
        case CP_0b10111xxx:  { sub_reg_A_flags_only(read_regs(read012(i), ram)); ++pc; return t_cycle_by_ins(itype);}
        case CP_0b10111110:  { sub_reg_A_flags_only(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
        case CP_0b11111110:  {
            sub_reg_A_flags_only(ram.read(pc+1));
#if DEBUG
            std::cout << "CP 0x90, A: " << (unsigned)regs[A] << " Zf: " << (unsigned)get_flag(Zf) << '\n';
#endif
            pc+=2;
            return t_cycle_by_ins(itype);}
        case INC_0b00xxx100: {
            inc_reg(read345(i), ram);
            ++pc;
            return t_cycle_by_ins(itype);}
        case INC_0b00110100: { inc_reg(HL, ram); ++pc; return t_cycle_by_ins(itype);}
        case DEC_0b00xxx101: { dec_reg(read345(i), ram); ++pc; return t_cycle_by_ins(itype);}
        case DEC_0b00110101: { dec_reg(HL, ram); ++pc; return t_cycle_by_ins(itype);}
        case AND_0b10100xxx: { and_reg_A(read_regs(read012(i), ram)); ++pc; return t_cycle_by_ins(itype);}
        case AND_0b10100110: { and_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
        case AND_0b11100110: { and_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
        case OR_0b10110xxx:  {
            or_reg_A(read_regs(read012(i), ram));
#if DEBUG
            std::cout << "OR C, Zf: " << (unsigned)get_flag(Zf) << " BC: " << (short)getpair(0) << '\n';
#endif
            ++pc;
            return t_cycle_by_ins(itype);}
        case OR_0b10110110:  { or_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
        case OR_0b11110110:  { or_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
        case XOR_0b10101xxx: { xor_reg_A(read_regs(read012(i), ram)); ++pc; return t_cycle_by_ins(itype);}
        case XOR_0b10101110: { xor_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
        case XOR_0b11101110: { xor_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
        case CCF_0b00111111: { set_flag(Cf, !get_flag(Cf)); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; return t_cycle_by_ins(itype);}
        case SCF_0b00110111: { set_flag(Cf, 1); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; return t_cycle_by_ins(itype);}
        case DAA_0b00100111: {
            if (!get_flag(Nf)) {
                if (get_flag(Cf) || regs[A] > 0x99) {
                    regs[A] += 0x60;
                    set_flag(Cf, 1);
                }
                if (get_flag(Hf) || (regs[A] & 0x0F) > 0x09) {
                    regs[A] += 0x06;
                }
            }else {
                if (get_flag(Cf)) regs[A]-=0x60;
                if (get_flag(Hf)) regs[A]-=0x06;
            }
            set_flag(Zf, regs[A] == 0);
            set_flag(Hf, 0);
            ++pc; return t_cycle_by_ins(itype);}
        case CPL_0b00101111: {regs[A] = ~regs[A]; set_flag(Nf, 1); set_flag(Hf, 1); ++pc; return t_cycle_by_ins(itype);}
        case INC_0b00xx0011: {
            auto rr = read45(i);
            unsigned short pair = getpair(rr);
            ++pair;
            update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
            ++pc;
            return t_cycle_by_ins(itype);
        }
        case DEC_0b00xx1011: {
            auto rr = read45(i);
            unsigned short pair = getpair(rr);
            --pair;
            update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
            ++pc;
            return t_cycle_by_ins(itype);
        }
        case ADD_0b00xx1001: { add_reg_HL(getpair(read45(i))); ++pc; return t_cycle_by_ins(itype);}
        case ADD_0b11101000: {
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            auto e = static_cast<char>(ram.read(pc+1));
            int result = e + sp;
            set_flag(Cf, ((sp & 0xff) +  (unsigned char)(e) > 0xff));
            set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
            sp = (unsigned short)(result);
            pc+=2;
            return t_cycle_by_ins(itype);
        }
        case RLCA_0b00000111: {
            auto res = std::rotl(regs[A], 1);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            set_flag(Cf, regs[A] & 0x80);
            regs[A] = res;
            ++pc;
            return t_cycle_by_ins(itype);}
        case RRCA_0b00001111: {
            auto res = std::rotr(regs[A], 1);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            set_flag(Cf, regs[A] & 1);
            regs[A] = res;
            ++pc;
            return t_cycle_by_ins(itype);}
        case RLA_0b00010111:  {
            auto carry = get_flag(Cf);
            auto res = ((regs[A] << 1) | carry);
            set_flag(Cf, regs[A] & 0x80);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            regs[A] = res;
            ++pc; return t_cycle_by_ins(itype);}
        case RRA_0b00011111:  {
            auto carry = get_flag(Cf);
            auto res = ((regs[A] >> 1) | carry << 7);
            set_flag(Cf, regs[A] & 1);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            regs[A] = res;
            ++pc;  return t_cycle_by_ins(itype);}
        case _0xCB: {
            auto pc_next = ram.read(pc+1);
            auto ins = decode_0xCB(pc_next);
            switch (ins) {
                case NOP_0xCB_0b00000000: { ++pc; return t_cycle_by_ins(itype);}
                case RLC_0xCB_0b00000xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto res = std::rotl(reg, 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    set_flag(Cf, reg & 0x80);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RRC_0xCB_0b00001xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto res = std::rotr(reg, 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    set_flag(Cf, reg & 1);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RL_0xCB_0b00010xxx:   {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto carry = get_flag(Cf);
                    auto res = ((reg << 1) | carry);
                    set_flag(Cf, reg & 0x80);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RR_0xCB_0b00011xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto carry = get_flag(Cf);
                    auto res = ((reg >> 1) | carry << 7);
                    set_flag(Cf, reg & 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SLA_0xCB_0b00100xxx:  {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto res = reg << 1;
                    set_flag(Cf, reg & 0x80);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SRA_0xCB_0b00101xxx:  {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto sign_bit = reg & 0x80;
                    auto res = ((reg >> 1) | sign_bit);
                    set_flag(Cf, reg & 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SWAP_0xCB_0b00110xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto l = reg & 0x0f;
                    auto h = reg & 0xf0;
                    auto res = (l << 4 | h >> 4);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    set_flag(Cf, 0);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SRL_0xCB_0b00111xxx:  {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto res = reg >> 1;
                    set_flag(Cf, reg & 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case BIT_0xCB_0b01xxxxxx:  {
                    auto bit_n = read345(pc_next);
                    auto reg = read_regs(read012(pc_next), ram);
                    set_flag(Zf, !((reg >> bit_n) & 1));
                    set_flag(Nf, 0);
                    set_flag(Hf, 1);
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RES_0xCB_0b10xxxxxx:  {
                    auto bit_n = read345(pc_next);
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    update_regs(rid, ram, reg & ~(1 << bit_n));
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SET_0xCB_0b11xxxxxx:  {
                    auto bit_n = read345(pc_next);
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    update_regs(rid, ram, reg | (1 << bit_n));
                    pc+=2; return t_cycle_by_ins_0xCB(ins, pc_next);}
                }
        };
            // Control flow
        case JP_0b11000011: { pc = ((ram.read(pc+2) << 8) | ram.read(pc + 1)); return t_cycle_by_ins(itype);}
        case JP_0b11101001: { pc = ((regs[H] << 8) | regs[L]); return t_cycle_by_ins(itype);}
        case JP_0b110xx010: {
            auto val = read34(i);
            auto jmp = [&]{pc = ((ram.read(pc+2) << 8) | ram.read(pc + 1));};
            bool cc_flag = get_cc_flag(val);
            if (cc_flag)
                jmp();
            else
                pc+=3;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case JR_0b00011000: {pc += 2 + (char)(ram.read(pc+1)); return t_cycle_by_ins(itype);}
        case JR_0b001xx000: {
            unsigned char val = read34(i);
            auto jmp = [&]{pc += 2 + (char)(ram.read(pc+1));};
            bool cc_flag = get_cc_flag(val);
            if (cc_flag)
                jmp();
            else
                pc+=2;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case CALL_0b11001101: {
            unsigned short nn = ((ram.read(pc + 2) << 8) | ram.read(pc + 1));
            sp-=2;
            ram.write(sp, (pc + 3) & 0xff);
            ram.write(sp + 1, ((pc + 3) >> 8));
            pc = nn;
            return t_cycle_by_ins(itype);
        }
        case CALL_0b110xx100: {
            auto val = read34(i);
            unsigned short nn = ((ram.read(pc + 2) << 8) | ram.read(pc + 1));
            auto call = [&]{
                sp-=2;
                ram.write(sp, (pc + 3) & 0xff);
                ram.write(sp+1, ((pc + 3) >> 8) & 0xff);
                pc = nn;
            };
            bool cc_flag = get_cc_flag(val);
            if (cc_flag)
                call();
            else
                pc+=3;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case RET_0b11001001: { pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2; return t_cycle_by_ins(itype);}
        case RET_0b110xx000: {
            auto val = read34(i);
            auto ret = [&]{
                pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2;
            };
            auto cc_flag = get_cc_flag(val);
            if (cc_flag)
                ret();
            else
                ++pc;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case RETI_0b11011001: {pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2; ime = true; return t_cycle_by_ins(itype);}
        case RST_0b11xxx111: {
            unsigned short nn = ((0x0 << 8) | rst_vec[read345(i)]);
            sp-=2;
            ram.write(sp, (pc + 1) & 0xff);
            ram.write(sp + 1, ((pc + 1) >> 8));
            pc = nn;
            return t_cycle_by_ins(itype);
        }
        case HALT: { /*todo*/ return t_cycle_by_ins(itype);}
        case STOP: {
            if (ram.read(constants::LY_ADDRESS) == 144) ++pc;
            return 4;}
        case DI_0b11110011: {ime = false; ++pc; return t_cycle_by_ins(itype);}
        case EI_0b11111011: {ime = true; ++pc; return t_cycle_by_ins(itype);} // todo: check
        case NOP_0b00000000: {++pc; return t_cycle_by_ins(itype);}
    }
    return 0;
}

void CPU::print_regs() const {
    std::cout << "RegA:" << std::hex << (unsigned)regs[CPU::A] << '\n';
    std::cout << "RegB:" << std::hex << (unsigned)regs[CPU::B] << '\n';
    std::cout << "RegC:" << std::hex << (unsigned)regs[CPU::C] << '\n';
    std::cout << "RegD:" << std::hex << (unsigned)regs[CPU::D] << '\n';
    std::cout << "RegE:" << std::hex << (unsigned )regs[CPU::E] << '\n';
    std::cout << "RegF:" << std::hex << (unsigned)regs[CPU::F] << '\n';
    std::cout << "Zero:" << (unsigned)get_flag(Zf) << std::endl;
    std::cout << "Negative:" << (unsigned)get_flag(Nf) << std::endl;
    std::cout << "Carry" << (unsigned)get_flag(Cf) << std::endl;
    std::cout << "HalfCarry" << (unsigned)get_flag(Hf) << std::endl;
    std::cout << "PC:" << pc << std::endl;
    std::cout << "SP:" << sp << '\n';
}



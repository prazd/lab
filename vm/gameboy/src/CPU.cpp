#include "CPU.h"
#include <chrono>
#include <iomanip>
#include <sstream>
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
    unsigned res = regs[A] + val;
    auto res8 = res & 0xff;
    set_flag(Nf, 0);
    set_flag(Zf, (res8 == 0));
    set_flag(Cf, (res > 0xff));
    set_flag(Hf, ((regs[A] & 0xf) + (val & 0xf) > 0xf));
    regs[A] = res8;
}

void CPU::add_reg_A_plus(unsigned char val, unsigned char f) {
    unsigned res = regs[A] + val + f;
    auto res8 = res & 0xff;
    set_flag(Nf, 0);
    set_flag(Zf, (res8 == 0));
    set_flag(Cf, (res > 0xff));
    set_flag(Hf, ((regs[A] & 0xf) + (val & 0xf) + f > 0xf));
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
    set_flag(Cf, (res < 0));
    set_flag(Hf, ((val & 0xf) > (regs[A] & 0xf)));
    regs[A] = res8;
}

void CPU::sub_reg_A_plus(unsigned char val, unsigned char f) {
    int res = (int)regs[A] - ((int)val + f);
    unsigned char res8 = res & 0xff;
    set_flag(Nf, 1);
    set_flag(Zf, (res8 == 0));
    set_flag(Cf, (res < 0));
    set_flag(Hf, ((val & 0xf) + f > (regs[A] & 0xf)));
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
    set_flag(Nf, true);
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
        case 0b110: {return tread(ram, HL_data());}
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
        case 0b110: {
            twrite(ram, HL_data(), value); return;
        }
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

unsigned char CPU::fetch(RAM &ram)   {
    return tread(ram,pc);
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
    throw std::exception();
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
    switch(pair & 0b11) {
        case 0b00: {regs[C] = l; regs[B] = h; break;}
        case 0b01: {regs[E] = l; regs[D] = h; break;}
        case 0b10: {regs[L] = l; regs[H] = h; break;}
        case 0b11: {sp = ((h << 8) | l); break;}
    }
}

void CPU::update_pair_pop(unsigned char l, unsigned char h, unsigned char pair)  {
    switch(pair) {
        case 0b00: {regs[C] = l; regs[B] = h; break;}
        case 0b01: {
            regs[E] = l;
            regs[D] = h;
            break;
        }
        case 0b10: {regs[L] = l; regs[H] = h; break;}
        case 0b11: {regs[F] = (l & 0xf0); regs[A] = h; break;} // todo: check
    }
}

// todo: check for pop and inc
unsigned short CPU::stack_getpair(unsigned char i)  {
    switch(i) {
        case 0b00: {return  (regs[C] | (regs[B] << 8));}
        case 0b01: {return  (regs[E] | (regs[D] << 8));}
        case 0b10: {return  (regs[L] | (regs[H] << 8));}
        case 0b11: {return (regs[F] | regs[A] << 8);}
    }
    throw std::invalid_argument("Unknown register");
}

unsigned short CPU::inc_getpair(unsigned char i) {
    switch(i) {
        case 0b00: {return  (regs[C] | (regs[B] << 8));}
        case 0b01: {return  (regs[E] | (regs[D] << 8));}
        case 0b10: {return  (regs[L] | (regs[H] << 8));}
        case 0b11: {return sp;}
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
    }
}

int CPU::t_cycle_by_ins(CPU::Instruction ins, unsigned char value)  { // value - 0xCB pc + 1 / cc_flag
    switch(ins) {
        case LD_0b01xxxyyy:       {return read345(value) == HL ? 8 : 4;}
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
        case LD_0b11111000:       {return 8;} // todo: check 12 ??
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

        case HALT:                {return 4;}
        case STOP:                {return 16;}
        case DI_0b11110011:       {return 4;}
        case EI_0b11111011:       {return 4;}
        case NOP_0b00000000:      {return 4;}
    }
}

void CPU::off_IF_value(CPU::InterruptFlag flag, RAM& ram) {
    auto if_value = ram.read( constants::IF_ADDRESS);
//    std::cout << "OFF IF value" << '\n';
    switch (flag) {
        case VBlank_I:   {ram.write(constants::IF_ADDRESS, if_value & 0xfe); break;}
        case LCD_I:      {ram.write(constants::IF_ADDRESS, if_value & 0xfd); break;}
        case Timer_I:    {ram.write(constants::IF_ADDRESS, if_value & 0xfb); break;}
        case Serial_I:   {ram.write(constants::IF_ADDRESS, if_value & 0xf7); break;}
        case Joypad_I:   {ram.write(constants::IF_ADDRESS, if_value & 0xef); break;}
    }
//    std::cout << "OFF IF value:" << (unsigned)ram.read(constants::IF_ADDRESS) << '\n';
}

int CPU::execute(unsigned char i, CPU::Instruction itype, RAM &ram)  {
    prev_pc = pc;

    unsigned char arg0 = tread(ram, pc);
    unsigned char arg1 = tread(ram,pc + 1);
    unsigned char arg2 = tread(ram,pc + 2);
    if (halt_bug) {
        auto tmp = arg1;
        arg1 = arg0;
        arg2 = tmp;
    }

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
    // maybe use std::scope (c++23)
    struct defer {
        CPU& cpuref;
        ~defer(){
            if (cpuref.ime_requested && cpuref.ime_req_count++ == 1) {
                cpuref.ime_req_count = 0;
                cpuref.ime_requested = false;
                cpuref.ime = true;
            }
        }
    } d{.cpuref = *this};

    switch(itype) {
        case LD_0b01xxxyyy:  { update_regs(read345(i), ram, read_regs(read012(i), ram)); return t_cycle_by_ins(itype);}
        case LD_0b01xxx110:  { update_regs(read345(i), ram, read_regs(HL, ram));  return t_cycle_by_ins(itype);}
        case LD_0b00xxx110:  { update_regs(read345(i), ram, arg1); return t_cycle_by_ins(itype);}
        case LD_0b01110xxx:  { update_regs(HL, ram, read_regs(read012(i), ram)); return t_cycle_by_ins(itype);}
        case LD_0b00110110:  { update_regs(HL, ram, arg1);  return t_cycle_by_ins(itype);};
        case LD_0b00001010:  { regs[A] = tread(ram, ((regs[B] << 8)| regs[C])); return t_cycle_by_ins(itype);}
        case LD_0b00011010:  { regs[A] = tread(ram,((regs[D] << 8)| regs[E])); return t_cycle_by_ins(itype);}
        case LD_0b00000010:  { twrite(ram,((regs[B] << 8)| regs[C]), regs[A]); return t_cycle_by_ins(itype);}
        case LD_0b00010010:  { twrite(ram,((regs[D] << 8)| regs[E]), regs[A]); return t_cycle_by_ins(itype);}
        case LD_0b11111010:  { regs[A] = tread(ram,arg1 | (arg2 << 8)); return t_cycle_by_ins(itype);}
        case LD_0b11101010:  {
            twrite(ram,(arg1 | (arg2 << 8)), regs[A]);
            return t_cycle_by_ins(itype);}
        case LDH_0b11110010: { regs[A] = tread(ram,0xff00 | regs[C]); return t_cycle_by_ins(itype);}
        case LDH_0b11100010: { twrite(ram,(0xff00 | regs[C]), regs[A]); return t_cycle_by_ins(itype);}
        case LDH_0b11110000: {
            unsigned short address = (0xff00 | arg1);
            regs[A] = tread(ram,address);
            return t_cycle_by_ins(itype);
        }
        case LDH_0b11100000: {
            twrite(ram,(0xff00 | arg1), regs[A]);
            return t_cycle_by_ins(itype);
        }
        case LD_0b00111010:  { regs[A] = read_regs(HL, ram); decrement_pair(L, H); return t_cycle_by_ins(itype);}
        case LD_0b00110010:  {
            update_regs(HL, ram, regs[A]);
            decrement_pair(L, H);
            return t_cycle_by_ins(itype);}
        case LD_0b00101010:  { regs[A] = read_regs(HL, ram); increment_pair(L, H); return t_cycle_by_ins(itype);}
        case LD_0b00100010:  { update_regs(HL, ram, regs[A]); increment_pair(L, H); return t_cycle_by_ins(itype);}
        case LD_0b00xx0001:  {
            update_pair_ld(arg1, arg2, read45(i));
            return t_cycle_by_ins(itype);}
        case LD_0b00001000:  {
            unsigned short address = (arg1 | (arg2 << 8));
            twrite(ram,address, sp & 0xff);
            twrite(ram,address + 1, ((sp>>8) & 0xff));
            return t_cycle_by_ins(itype);
        }
        case LD_0b11111001: {sp = (regs[L] | (regs[H] << 8)); return t_cycle_by_ins(itype);} // SP = HL
        case PUSH_0b11xx0101: {
            auto val = stack_getpair(read45(i));
            sp-=2;
            twrite(ram,sp, (val & 0xff));
            twrite(ram,sp + 1, ((val>>8) & 0xff));
            return t_cycle_by_ins(itype);} // stack push xx
        case POP_0b11xx0001: {
            update_pair_pop(tread(ram,sp) , tread(ram,sp+1), read45(i));
            sp+=2;
            return t_cycle_by_ins(itype);
        };
        case LD_0b11111000: {
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            auto e = static_cast<char>(arg1);
            unsigned int result = e + sp;
            set_flag(Cf, ((sp & 0xff) + (unsigned char)(e) > 0xff));
            set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
            regs[H] = ((result >> 8) & 0xff);
            regs[L] = (result & 0xff);
            return t_cycle_by_ins(itype);
        }
        case ADD_0b10000xxx: { add_reg_A(read_regs(read012(i), ram)); return t_cycle_by_ins(itype);}
        case ADD_0b10000110: { add_reg_A(read_regs(HL, ram)); return t_cycle_by_ins(itype);}
        case ADD_0b11000110: { add_reg_A(arg1); return t_cycle_by_ins(itype);}
        case ADC_0b10001xxx: { add_reg_A_plus(read_regs(read012(i), ram) , get_flag(Cf)); return t_cycle_by_ins(itype);}
        case ADC_0b10001110: { add_reg_A_plus(read_regs(HL, ram), get_flag(Cf)); return t_cycle_by_ins(itype);}
        case ADC_0b11001110: { add_reg_A_plus(arg1, get_flag(Cf)); return t_cycle_by_ins(itype);}
        case SUB_0b10010xxx: { sub_reg_A(read_regs(read012(i), ram)); return t_cycle_by_ins(itype);}
        case SUB_0b10010110: { sub_reg_A(read_regs(HL, ram)); return t_cycle_by_ins(itype);}
        case SUB_0b11010110: { sub_reg_A(arg1); return t_cycle_by_ins(itype);}
        case SBC_0b10011xxx: { sub_reg_A_plus(read_regs(read012(i), ram), get_flag(Cf)); return t_cycle_by_ins(itype);} // (reg[A] - r - reg[C]) == reg[A] - (r + reg[C])
        case SBC_0b10011110: { sub_reg_A_plus(read_regs(HL, ram), get_flag(Cf)); return t_cycle_by_ins(itype);}
        case SBC_0b11011110: { sub_reg_A_plus(arg1, get_flag(Cf)); return t_cycle_by_ins(itype);}
        case CP_0b10111xxx:  { sub_reg_A_flags_only(read_regs(read012(i), ram)); return t_cycle_by_ins(itype);}
        case CP_0b10111110:  { sub_reg_A_flags_only(read_regs(HL, ram)); return t_cycle_by_ins(itype);}
        case CP_0b11111110:  {
            sub_reg_A_flags_only(arg1);
            return t_cycle_by_ins(itype);}
        case INC_0b00xxx100: {
            inc_reg(read345(i), ram);
            return t_cycle_by_ins(itype);}
        case INC_0b00110100: { inc_reg(HL, ram); return t_cycle_by_ins(itype);}
        case DEC_0b00xxx101: {
            dec_reg(read345(i), ram);
            return t_cycle_by_ins(itype);}
        case DEC_0b00110101: { dec_reg(HL, ram); return t_cycle_by_ins(itype);}
        case AND_0b10100xxx: { and_reg_A(read_regs(read012(i), ram)); return t_cycle_by_ins(itype);}
        case AND_0b10100110: { and_reg_A(read_regs(HL, ram)); return t_cycle_by_ins(itype);}
        case AND_0b11100110: { and_reg_A(arg1); return t_cycle_by_ins(itype);}
        case OR_0b10110xxx:  {
            or_reg_A(read_regs(read012(i), ram));
#if DEBUG
            std::cout << "OR C, Zf: " << (unsigned)get_flag(Zf) << " BC: " << (short)getpair(0) << '\n';
#endif
            return t_cycle_by_ins(itype);}
        case OR_0b10110110:  { or_reg_A(read_regs(HL, ram)); return t_cycle_by_ins(itype);}
        case OR_0b11110110:  { or_reg_A(arg1); return t_cycle_by_ins(itype);}
        case XOR_0b10101xxx: { xor_reg_A(read_regs(read012(i), ram)); return t_cycle_by_ins(itype);}
        case XOR_0b10101110: { xor_reg_A(read_regs(HL, ram)); return t_cycle_by_ins(itype);}
        case XOR_0b11101110: { xor_reg_A(arg1); return t_cycle_by_ins(itype);}
        case CCF_0b00111111: { set_flag(Cf, !get_flag(Cf)); set_flag(Nf, 0); set_flag(Hf, 0); return t_cycle_by_ins(itype);}
        case SCF_0b00110111: { set_flag(Cf, 1); set_flag(Nf, 0); set_flag(Hf, 0); return t_cycle_by_ins(itype);}
        case DAA_0b00100111: {
            if (!get_flag(Nf)) {
                if (get_flag(Cf) || regs[A] > 0x99) {
                    regs[A] += 0x60;
                    set_flag(Cf, 1);
                }
                if (get_flag(Hf) || (regs[A] & 0x0F) > 0x09) {
                    regs[A] += 0x06;
                }
            } else {
                if (get_flag(Cf)) regs[A]-=0x60;
                if (get_flag(Hf)) regs[A]-=0x06;
            }
            set_flag(Zf, regs[A] == 0);
            set_flag(Hf, 0);
            return t_cycle_by_ins(itype);}
        case CPL_0b00101111: {regs[A] = ~regs[A]; set_flag(Nf, 1); set_flag(Hf, 1); return t_cycle_by_ins(itype);}
        case INC_0b00xx0011: {
            auto rr = read45(i);
            unsigned short pair = inc_getpair(rr);
            ++pair;
            update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
            return t_cycle_by_ins(itype);
        }
        case DEC_0b00xx1011: {
            auto rr = read45(i);
            unsigned short pair = inc_getpair(rr);
            --pair;
            update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
            return t_cycle_by_ins(itype);
        }
        case ADD_0b00xx1001: { add_reg_HL(inc_getpair(read45(i))); return t_cycle_by_ins(itype);}
        case ADD_0b11101000: {
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            auto e = static_cast<char>(arg1);
            int result = e + sp;
            set_flag(Cf, ((sp & 0xff) +  (unsigned char)(e) > 0xff));
            set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
            sp = (unsigned short)(result);
            return t_cycle_by_ins(itype);
        }
        case RLCA_0b00000111: {
            auto res = std::rotl(regs[A], 1);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            set_flag(Cf, regs[A] & 0x80);
            regs[A] = res;
            return t_cycle_by_ins(itype);}
        case RRCA_0b00001111: {
            auto res = std::rotr(regs[A], 1);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            set_flag(Cf, regs[A] & 1);
            regs[A] = res;
            return t_cycle_by_ins(itype);}
        case RLA_0b00010111:  {
            auto carry = get_flag(Cf);
            auto res = ((regs[A] << 1) | carry);
            set_flag(Cf, regs[A] & 0x80);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            regs[A] = res;
            return t_cycle_by_ins(itype);}
        case RRA_0b00011111:  {
            auto carry = get_flag(Cf);
            auto res = ((regs[A] >> 1) | carry << 7);
            set_flag(Cf, regs[A] & 1);
            set_flag(Zf, 0);
            set_flag(Nf, 0);
            set_flag(Hf, 0);
            regs[A] = res;
            return t_cycle_by_ins(itype);}
        case _0xCB: {
            auto pc_next = arg1;
            auto ins = decode_0xCB(pc_next);
            switch (ins) {
                case RLC_0xCB_0b00000xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto res = std::rotl(reg, 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    set_flag(Cf, reg & 0x80);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RRC_0xCB_0b00001xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto res = std::rotr(reg, 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    set_flag(Cf, reg & 1);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RL_0xCB_0b00010xxx:   {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto carry = get_flag(Cf);
                    unsigned char res = ((reg << 1) | carry);
                    set_flag(Cf, reg & 0x80);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RR_0xCB_0b00011xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto carry = get_flag(Cf);
                    unsigned char res = ((reg >> 1) | carry << 7);
                    set_flag(Cf, reg & 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SLA_0xCB_0b00100xxx:  {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    unsigned char res = reg << 1;
                    set_flag(Cf, reg & 0x80);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SRA_0xCB_0b00101xxx:  {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto sign_bit = reg & 0x80;
                    unsigned char res = ((reg >> 1) | sign_bit);
                    set_flag(Cf, reg & 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SWAP_0xCB_0b00110xxx: {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    auto l = reg & 0x0f;
                    auto h = reg & 0xf0;
                    unsigned char res = (l << 4 | h >> 4);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    set_flag(Cf, 0);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SRL_0xCB_0b00111xxx:  {
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    unsigned char res = reg >> 1;
                    set_flag(Cf, reg & 1);
                    set_flag(Zf, res == 0);
                    set_flag(Nf, 0);
                    set_flag(Hf, 0);
                    update_regs(rid, ram, res);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case BIT_0xCB_0b01xxxxxx:  {
                    auto bit_n = read345(pc_next);
                    auto reg = read_regs(read012(pc_next), ram);
                    set_flag(Zf, !((reg >> bit_n) & 1));
                    set_flag(Nf, 0);
                    set_flag(Hf, 1);
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case RES_0xCB_0b10xxxxxx:  {
                    auto bit_n = read345(pc_next);
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    update_regs(rid, ram, reg & ~(1 << bit_n));
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                case SET_0xCB_0b11xxxxxx:  {
                    auto bit_n = read345(pc_next);
                    auto rid = read012(pc_next);
                    auto reg = read_regs(rid, ram);
                    update_regs(rid, ram, reg | (1 << bit_n));
                    return t_cycle_by_ins_0xCB(ins, pc_next);}
                }
        };
            // Control flow
        case JP_0b11000011: { pc = ((arg2 << 8) | arg1); return t_cycle_by_ins(itype);}
        case JP_0b11101001: { pc = ((regs[H] << 8) | regs[L]); return t_cycle_by_ins(itype);}
        case JP_0b110xx010: {
            auto val = read34(i);
            auto jmp = [&]{pc = ((arg2 << 8) | arg1);};
            bool cc_flag = get_cc_flag(val);
            if (cc_flag)
                jmp();
            else
                pc+=3;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case JR_0b00011000: {pc += 2 + (char)(arg1); return t_cycle_by_ins(itype);}
        case JR_0b001xx000: {
            unsigned char val = read34(i);
            auto jmp = [&]{pc += 2 + (char)(arg1);};
            bool cc_flag = get_cc_flag(val);
            if (cc_flag)
                jmp();
            else
                pc+=2;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case CALL_0b11001101: {
            unsigned short nn = ((arg2 << 8) | arg1);
            sp-=2;
            twrite(ram,sp, (pc + 3) & 0xff);
            twrite(ram,sp + 1, ((pc + 3) >> 8));
            pc = nn;
            return t_cycle_by_ins(itype);
        }
        case CALL_0b110xx100: {
            auto val = read34(i);
            unsigned short nn = ((arg2 << 8) | arg1);
            auto call = [&]{
                sp-=2;
                twrite(ram,sp, (pc + 3) & 0xff);
                twrite(ram,sp+1, ((pc + 3) >> 8) & 0xff);
                pc = nn;
            };
            bool cc_flag = get_cc_flag(val);
            if (cc_flag)
                call();
            else
                pc+=3;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case RET_0b11001001: { pc = ((tread(ram,sp + 1) << 8) | tread(ram,sp)); sp+=2; return t_cycle_by_ins(itype);}
        case RET_0b110xx000: {
            auto val = read34(i);
            auto ret = [&]{
                pc = ((tread(ram,sp + 1) << 8) | tread(ram,sp)); sp+=2;
            };
            auto cc_flag = get_cc_flag(val);
            if (cc_flag)
                ret();
            else
                ++pc;
            return t_cycle_by_ins(itype, cc_flag);
        }
        case RETI_0b11011001: {pc = ((tread(ram,sp + 1) << 8) | tread(ram,sp)); sp+=2; ime = true; return t_cycle_by_ins(itype);}
        case RST_0b11xxx111: {
            unsigned short nn = ((0x0 << 8) | rst_vec[read345(i)]);
            sp-=2;
            twrite(ram,sp, (pc + 1) & 0xff);
            twrite(ram,sp + 1, ((pc + 1) >> 8));
            pc = nn;
            return t_cycle_by_ins(itype);
        }
        case HALT: {
            if(!halted) {halted = true; ++pc;}
            return t_cycle_by_ins(itype);}
        case STOP: {
//            if (tread(ram, constants::LY_ADDRESS) == 144)
//                ++pc;
            return 4;
        }
        case DI_0b11110011: {
            ime_requested = false;
            ime_req_count = 0; ime = false;
            return t_cycle_by_ins(itype);}
        case EI_0b11111011: {
            ime_requested = true;
            ime_req_count = 0;  /*std::cout << "IF:" << (unsigned)tread(ram,constants::IF_ADDRESS) << '\n'*/;  return t_cycle_by_ins(itype);} // todo: check
        case NOP_0b00000000: {return t_cycle_by_ins(itype);}
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

void CPU::fetch_IF_value(RAM& ram) {
    auto if_value = ram.read(constants::IF_ADDRESS);
    if_status[0] = vblank_interrupt_req = (if_value & 1);
    if_status[1] = lcd_interrupt_req =    ((if_value >> 1) & 1);
    if_status[2] = timer_interrupt_req =  ((if_value >> 2) & 1);
    if_status[3] = serial_interrupt_req = ((if_value >> 3) & 1);
    if_status[4] = joypad_interrupt_req = ((if_value >> 4) & 1);
}

void CPU::fetch_IE_value(RAM& ram) {
    auto if_value =    ram.read(constants::IE_ADDRESS);
    ie_status[0] = vblank_interrupt_enabled =  (if_value & 1);
    ie_status[1] = lcd_interrupt_enabled    =  ((if_value >> 1) & 1);
    ie_status[2] = timer_interrupt_enabled  =  ((if_value >> 2) & 1);
    ie_status[3] = serial_interrupt_enabled =  ((if_value >> 3) & 1);
    ie_status[4] = joypad_interrupt_enabled =  ((if_value >> 4) & 1);
}

void CPU::push_stack(unsigned char l, unsigned char h, RAM& ram) {
    sp-=2;
    twrite(ram, sp, l);
    twrite(ram,sp + 1, h);
}

unsigned char CPU::tread(RAM &ram, unsigned short index) {
    return ram.read(index);
}

void CPU::twrite(RAM &ram, unsigned short index, unsigned char value) {
    ram.write(index, value);
}

int CPU::pc_inc_by_ins(CPU::Instruction ins, unsigned char value) {
    switch(ins) {
        case LD_0b01xxxyyy:       {return 1;}
        case LD_0b00xxx110:       {return 2;}
        case LD_0b01xxx110:       {return 1;}
        case LD_0b01110xxx:       {return 1;}
        case LD_0b00110110:       {return 2;}
        case LD_0b00001010:       {return 1;}
        case LD_0b00011010:       {return 1;}
        case LD_0b00000010:       {return 1;}
        case LD_0b00010010:       {return 1;}
        case LD_0b11111010:       {return 3;}
        case LD_0b11101010:       {return 3;}
        case LDH_0b11110010:      {return 1;}
        case LDH_0b11100010:      {return 1;}
        case LDH_0b11110000:      {return 2;}
        case LDH_0b11100000:      {return 2;}
        case LD_0b00111010:       {return 1;}
        case LD_0b00110010:       {return 1;}
        case LD_0b00101010:       {return 1;}
        case LD_0b00100010:       {return 1;}
        case LD_0b00xx0001:       {return 3;}
        case LD_0b00001000:       {return 3;}
        case LD_0b11111001:       {return 1;}
        case PUSH_0b11xx0101:     {return 1;}
        case POP_0b11xx0001:      {return 1;}
        case LD_0b11111000:       {return 2;}
        case ADD_0b10000xxx:      {return 1;}
        case ADD_0b10000110:      {return 1;}
        case ADD_0b11000110:      {return 2;}
        case ADC_0b10001xxx:      {return 1;}
        case ADC_0b10001110:      {return 1;}
        case ADC_0b11001110:      {return 2;}
        case SUB_0b10010xxx:      {return 1;}
        case SUB_0b10010110:      {return 1;}
        case SUB_0b11010110:      {return 2;}
        case SBC_0b10011xxx:      {return 1;}
        case SBC_0b10011110:      {return 1;}
        case SBC_0b11011110:      {return 2;}
        case CP_0b10111xxx:       {return 1;}
        case CP_0b10111110:       {return 1;}
        case CP_0b11111110:       {return 2;}
        case INC_0b00xxx100:      {return 1;}
        case INC_0b00110100:      {return 1;}
        case DEC_0b00xxx101:      {return 1;}
        case DEC_0b00110101:      {return 1;}
        case AND_0b10100xxx:      {return 1;}
        case AND_0b10100110:      {return 1;}
        case AND_0b11100110:      {return 2;}
        case OR_0b10110xxx:       {return 1;}
        case OR_0b10110110:       {return 1;}
        case OR_0b11110110:       {return 2;}
        case XOR_0b10101xxx:      {return 1;}
        case XOR_0b10101110:      {return 1;}
        case XOR_0b11101110:      {return 2;}
        case CCF_0b00111111:      {return 1;}
        case SCF_0b00110111:      {return 1;}
        case DAA_0b00100111:      {return 1;}
        case CPL_0b00101111:      {return 1;}
        case INC_0b00xx0011:      {return 1;}
        case DEC_0b00xx1011:      {return 1;}
        case ADD_0b00xx1001:      {return 1;}
        case ADD_0b11101000:      {return 2;}
        case RLCA_0b00000111:     {return 1;}
        case RRCA_0b00001111:     {return 1;}
        case RLA_0b00010111:      {return 1;}
        case RRA_0b00011111:      {return 1;}
        case _0xCB:               {return 2;}

        // Control flow + HALT - 0 (update pc in instruction logic)
        case JP_0b11000011:       {return 0;}
        case JP_0b11101001:       {return 0;}
        case JP_0b110xx010:       {return 0;}
        case JR_0b00011000:       {return 0;}
        case JR_0b001xx000:       {return 0;}
        case CALL_0b11001101:     {return 0;}
        case CALL_0b110xx100:     {return 0;}
        case RET_0b11001001:      {return 0;}
        case RET_0b110xx000:      {return 0;}
        case RETI_0b11011001:     {return 0;}
        case RST_0b11xxx111:      {return 0;}
        case HALT:                {return 0;}

        case STOP:                {return 2;}
        case DI_0b11110011:       {return 1;}
        case EI_0b11111011:       {return 1;}
        case NOP_0b00000000:      {return 1;}
    }
}

void CPU::handle_interrupt(RAM& ram, unsigned char& next_ins, Instruction& decoded_ins) {
    if (ime) { // Interrupt
        auto if_value = ram.read(constants::IF_ADDRESS);
        auto ie_value = ram.read(constants::IE_ADDRESS);
        if ((if_value & ie_value & 0x1f) != 0) {
            unsigned char pc_l = pc & 0xff;
            unsigned char pc_h = (pc >> 8) & 0xff;

            fetch_IF_value(ram);
            fetch_IE_value(ram);

            if (vblank_interrupt_req && vblank_interrupt_enabled) {
                if (halted) halted = false;
                ime = false;
                push_stack(pc_l, pc_h, ram);
                vblank_interrupt_req = false;
                off_IF_value(CPU::VBlank_I, ram);
                pc = constants::VBLANK_INTERRUPT;
            } else if (lcd_interrupt_req && lcd_interrupt_enabled) {
                if (halted) halted = false;
                ime = false;
                push_stack(pc_l, pc_h, ram);
                lcd_interrupt_req = false;
                off_IF_value(CPU::LCD_I, ram);
                pc = constants::STAT_INTERRUPT;
            } else if (timer_interrupt_req && timer_interrupt_enabled) {
                if (halted) halted = false;
                ime = false;
                push_stack(pc_l, pc_h, ram);
                timer_interrupt_req = false;
                off_IF_value(CPU::Timer_I, ram);
                pc = constants::TIMER_INTERRUPT;
            } else if (serial_interrupt_req && serial_interrupt_enabled) {
                if (halted) halted = false;
                ime = false;
                push_stack(pc_l, pc_h, ram);
                serial_interrupt_req = false;
                off_IF_value(CPU::Serial_I, ram);
                pc = constants::SERIAL_INTERRUPT;
            } else if (joypad_interrupt_req && joypad_interrupt_enabled) {
                if (halted) halted = false;
                ime = false;
                push_stack(pc_l, pc_h, ram);
                joypad_interrupt_req = false;
                off_IF_value(CPU::Joypad_I, ram);
                pc = constants::JOYPAD_INTERRUPT;
            }
            next_ins = fetch(ram);
            decoded_ins = decode(next_ins);
        }
    }
}

Resumable CPU::instruction_async_loop(RAM &ram) {
#if 1
    auto print_hex = [&](auto val){
        auto num_bytes = sizeof(val);
        std::stringstream ss("");
        ss << std::hex << std::setw(num_bytes * 2) << std::setfill('0') << (unsigned)val;
        return ss;
    };
//                std::cout  << instruction_str[itype] << " ";
    if (!halted) {
        std::cout <<

                  "A:" << print_hex(regs[CPU::A]).str() << " " <<
                  "F:" <<  print_hex(regs[CPU::F]).str() << " " <<
                  "B:" <<  print_hex(regs[CPU::B]).str() << " " <<
                  "C:" <<  print_hex(regs[CPU::C]).str() << " " <<
                  "D:" <<  print_hex(regs[CPU::D]).str() << " " <<
                  "E:" <<  print_hex(regs[CPU::E]).str() << " " <<
                  "H:" <<  print_hex(regs[CPU::H]).str() << " " <<
                  "L:" <<  print_hex(regs[CPU::L]).str() << " " <<
                  "SP:" <<  print_hex(sp).str() << " " <<
                  "PC:" <<  print_hex(pc).str() << " " <<
                  "PCMEM:" <<  print_hex(ram.read(pc)).str() << "," <<
                  print_hex(ram.read(pc + 1)).str() << "," <<
                  print_hex(ram.read(pc + 2)).str() << "," <<
                  print_hex(ram.read(pc + 3)).str() << '\n';
        std::cout << std::dec;
    }
#endif

    #define WAIT(n) for (int i = 0; i < n; ++i) co_await std::suspend_always();
    while (true) {
        if (halted) {
            auto if_value = ram.read(constants::IF_ADDRESS);
            auto ie_value = ram.read(constants::IE_ADDRESS);
            if (ime) {
                if ((if_value & ie_value & 0x1f) != 0) {
                    // todo: check
//                    handle_interrupt(ram, i, itype);
//                    t_cycle = cpu.execute(next_ins, decoded_ins, ram);
//                    cpu.pc += cpu.pc_inc_by_ins(decoded_ins);
                } else {
                    WAIT(4)
                    continue;
                }
            } else if (!ime) {
                if ((if_value & ie_value & 0x1f) != 0) {
                    if (skip_interrupt) {
                        halted = false;
                        skip_interrupt = false;
                    } else {
                        // todo: check halt bug
                        halt_bug = true;
//                        t_cycle = cpu.execute(next_ins, decoded_ins, ram);
//                        pc += cpu.pc_inc_by_ins(decoded_ins) - 1;
                        halted = false;
                    }
                } else {
                    skip_interrupt = true;
                    WAIT(4)
                    continue;
                }
            }
        }
        prev_pc = pc;
        unsigned char arg0 = tread(ram, pc);
        unsigned char arg1 = tread(ram, pc + 1);
        unsigned char arg2 = tread(ram, pc + 2);
        if (halt_bug) {
            auto tmp = arg1;
            arg1 = arg0;
            arg2 = tmp;
        }
        if (ime) { // Interrupt
            auto if_value = ram.read(constants::IF_ADDRESS);
            auto ie_value = ram.read(constants::IE_ADDRESS);
            if ((if_value & ie_value & 0x1f) != 0) {
                unsigned char pc_l = pc & 0xff;
                unsigned char pc_h = (pc >> 8) & 0xff;
                WAIT(4)
                update_interrupts_status(ram);
                for (int i = 0; i < 5; ++i) {
                    if (ie_status[i] && if_status[i]) {
                        if (halted) halted = false;
                        ime = false;
                        sp-=1;
                        WAIT(4)
                        twrite(ram, sp, pc_h);
                        update_interrupts_status(ram);
                        if (!ie_status[i] || !if_status[i]) {
                            bool updated = false;
                            for(int j = i; j < 5; ++j){
                                if (ie_status[j] && if_status[j]) {
                                    i = j;
                                    updated = true;
                                    break;
                                }
                            }
                            if (!updated){
                                pc = 0x0000;
                                break;
                            }
                        }
                        sp-=1;
                        WAIT(4)
                        twrite(ram, sp, pc_l);
                        if_status[i] = false;
                        WAIT(4)
                        off_IF_value( static_cast<InterruptFlag>(i), ram);
                        pc = InterruptJumpAddress[i];
                        break;
                    }
                }
                WAIT(4)
                continue;
            }
        }
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
        // maybe use std::scope (c++23)
        struct defer {
            CPU &cpuref;
            ~defer() {
                if (cpuref.ime_requested && cpuref.ime_req_count++ == 1) {
                    cpuref.ime_req_count = 0;
                    cpuref.ime_requested = false;
                    cpuref.ime = true;
                }
            }
        } d{.cpuref = *this};

        WAIT(3) // fetch/decode
        auto i = fetch(ram);
        auto itype = decode(i);

        switch (itype) {
            case LD_0b01xxxyyy: { // 4/8 t-cycle
                // todo: check
//                WAIT(4)
                update_regs(read345(i), ram, read_regs(read012(i), ram));
//                unsigned char value {};
//                if (read345(i) != HL)
//                    value = read_regs(read345(i), ram);
//                else {
//                    WAIT(4)
//                    value = tread(ram, HL_data());
//                }
//                update_regs(read345(i), ram, value);
                break;
            }
            case LD_0b01xxx110: { // 8 t-cycles
                WAIT(4)
                auto regs_val = read_regs(HL, ram);
                update_regs(read345(i), ram, regs_val);
                break;
            }
            case LD_0b00xxx110: {
                WAIT(4)
                update_regs(read345(i), ram, arg1);
                break;
            }
            case LD_0b01110xxx: {
                WAIT(4)
                update_regs(HL, ram, read_regs(read012(i), ram));
                break;
            }
            case LD_0b00110110: {
                WAIT(8)
                update_regs(HL, ram, arg1);
                break;
            };
            case LD_0b00001010: {
                WAIT(4)
                regs[A] = tread(ram, ((regs[B] << 8) | regs[C]));
                break;
            }
            case LD_0b00011010: {
                WAIT(4)
                regs[A] = tread(ram, ((regs[D] << 8) | regs[E]));
                break;
            }
            case LD_0b00000010: {
                WAIT(4)
                twrite(ram, ((regs[B] << 8) | regs[C]), regs[A]);
                break;
            }
            case LD_0b00010010: {
                WAIT(4)
                twrite(ram, ((regs[D] << 8) | regs[E]), regs[A]);
                break;
            }
            case LD_0b11111010: {
                WAIT(12)
                regs[A] = tread(ram, arg1 | (arg2 << 8));
                break;
            }
            case LD_0b11101010: {
                WAIT(12)
                twrite(ram, (arg1 | (arg2 << 8)), regs[A]);
                break;
            }
            case LDH_0b11110010: {
                WAIT(4)
                regs[A] = tread(ram, 0xff00 | regs[C]);
                break;
            }
            case LDH_0b11100010: {
                WAIT(4)
                twrite(ram, (0xff00 | regs[C]), regs[A]);
                break;
            }
            case LDH_0b11110000: {
                WAIT(4)
                unsigned short address = (0xff00 | arg1);
                WAIT(4)
                regs[A] = tread(ram, address);
                break;
            }
            case LDH_0b11100000: {
                WAIT(8)
                twrite(ram, (0xff00 | arg1), regs[A]);
                break;
            }
            case LD_0b00111010: {
                WAIT(4)
                regs[A] = read_regs(HL, ram);
                decrement_pair(L, H);
                break;
            }
            case LD_0b00110010: {
                WAIT(4)
                update_regs(HL, ram, regs[A]);
                decrement_pair(L, H);
                break;
            }
            case LD_0b00101010: {
                WAIT(4)
                regs[A] = read_regs(HL, ram);
                increment_pair(L, H);
                break;
            }
            case LD_0b00100010: {
                WAIT(4)
                update_regs(HL, ram, regs[A]);
                increment_pair(L, H);
                break;
            }
            case LD_0b00xx0001: {
                WAIT(8)
                update_pair_ld(arg1, arg2, read45(i));
                break;
            }
            case LD_0b00001000: {
                WAIT(8)
                unsigned short address = (arg1 | (arg2 << 8));
                WAIT(4)
                twrite(ram, address, sp & 0xff);
                WAIT(4)
                twrite(ram, address + 1, ((sp >> 8) & 0xff));
                break;
            }
            case LD_0b11111001: {
                WAIT(4)
                sp = (regs[L] | (regs[H] << 8));
                break;
            } // SP = HL
            case PUSH_0b11xx0101: {
                WAIT(4)
                auto val = stack_getpair(read45(i));
                sp -= 2;
                WAIT(4)
                twrite(ram, sp, (val & 0xff));
                WAIT(4)
                twrite(ram, sp + 1, ((val >> 8) & 0xff));
                break;
            } // stack push xx
            case POP_0b11xx0001: {
                WAIT(4)
                auto a1 = tread(ram, sp);
                WAIT(4)
                auto a2 = tread(ram, sp + 1);
                update_pair_pop(a1, a2, read45(i));
                sp += 2;
                break;
            };
            case LD_0b11111000: {
                WAIT(4)
                auto e = static_cast<char>(arg1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                unsigned int result = e + sp;
                set_flag(Cf, ((sp & 0xff) + (unsigned char) (e) > 0xff));
                set_flag(Hf, ((sp & 0xf) + ((unsigned char) (e) & 0xf) > 0xf));
                regs[H] = ((result >> 8) & 0xff);
                regs[L] = (result & 0xff);
                WAIT(4)
                break;
            }
            case ADD_0b10000xxx: {
                add_reg_A(read_regs(read012(i), ram));
                break;
            }
            case ADD_0b10000110: {
                WAIT(4)
                add_reg_A(read_regs(HL, ram));
                break;
            }
            case ADD_0b11000110: {
                WAIT(4)
                add_reg_A(arg1);
                break;
            }
            case ADC_0b10001xxx: {
                add_reg_A_plus(read_regs(read012(i), ram), get_flag(Cf));
                break;
            }
            case ADC_0b10001110: {
                WAIT(4)
                add_reg_A_plus(read_regs(HL, ram), get_flag(Cf));
                break;
            }
            case ADC_0b11001110: {
                WAIT(4)
                add_reg_A_plus(arg1, get_flag(Cf));
                break;
            }
            case SUB_0b10010xxx: {
                sub_reg_A(read_regs(read012(i), ram));
                break;
            }
            case SUB_0b10010110: {
                WAIT(4)
                sub_reg_A(read_regs(HL, ram));
                break;
            }
            case SUB_0b11010110: {
                WAIT(4)
                sub_reg_A(arg1);
                break;
            }
            case SBC_0b10011xxx: {
                sub_reg_A_plus(read_regs(read012(i), ram), get_flag(Cf));
                break;
            } // (reg[A] - r - reg[C]) == reg[A] - (r + reg[C])
            case SBC_0b10011110: {
                WAIT(4)
                sub_reg_A_plus(read_regs(HL, ram), get_flag(Cf));
                break;
            }
            case SBC_0b11011110: {
                WAIT(4)
                sub_reg_A_plus(arg1, get_flag(Cf));
                break;
            }
            case CP_0b10111xxx: {
                sub_reg_A_flags_only(read_regs(read012(i), ram));
                break;
            }
            case CP_0b10111110: {
                WAIT(4)
                sub_reg_A_flags_only(read_regs(HL, ram));
                break;
            }
            case CP_0b11111110: {
                WAIT(4)
                sub_reg_A_flags_only(arg1);
                break;
            }
            case INC_0b00xxx100: {
                inc_reg(read345(i), ram);
                break;
            }
            case INC_0b00110100: {
                WAIT(4) // todo: check
                auto reg = read_regs(HL, ram);
                auto cp = reg;
                unsigned char result = reg + 1;
                set_flag(Nf, 0);
                set_flag(Zf, result == 0);
                set_flag(Hf, (cp & 0xf) == 0xf);
                WAIT(4)
                update_regs(HL, ram, result);
                break;
            }
            case DEC_0b00xxx101: {
                dec_reg(read345(i), ram);
                break;
            }
            case DEC_0b00110101: {
                WAIT(4)
                auto reg = read_regs(HL, ram);
                auto cp = reg;
                unsigned char result = reg - 1;
                set_flag(Nf, 1);
                set_flag(Zf, result == 0);
                set_flag(Hf, (cp & 0xf) == 0);
                WAIT(4)
                update_regs(HL, ram, result);
                break;
            }
            case AND_0b10100xxx: {
                and_reg_A(read_regs(read012(i), ram));
                break;
            }
            case AND_0b10100110: {
                WAIT(4)
                and_reg_A(read_regs(HL, ram));
                break;
            }
            case AND_0b11100110: {
                WAIT(4)
                and_reg_A(arg1);
                break;
            }
            case OR_0b10110xxx: {
                or_reg_A(read_regs(read012(i), ram));
#if DEBUG
                std::cout << "OR C, Zf: " << (unsigned)get_flag(Zf) << " BC: " << (short)getpair(0) << '\n';
#endif
                break;
            }
            case OR_0b10110110: {
                WAIT(4)
                or_reg_A(read_regs(HL, ram));
                break;
            }
            case OR_0b11110110: {
                WAIT(4)
                or_reg_A(arg1);
                break;
            }
            case XOR_0b10101xxx: {
                xor_reg_A(read_regs(read012(i), ram));
                break;
            }
            case XOR_0b10101110: {
                WAIT(4)
                xor_reg_A(read_regs(HL, ram));
                break;
            }
            case XOR_0b11101110: {
                WAIT(4)
                xor_reg_A(arg1);
                break;
            }
            case CCF_0b00111111: {
                set_flag(Cf, !get_flag(Cf));
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                break;
            }
            case SCF_0b00110111: {
                set_flag(Cf, 1);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                break;
            }
            case DAA_0b00100111: {
                if (!get_flag(Nf)) {
                    if (get_flag(Cf) || regs[A] > 0x99) {
                        regs[A] += 0x60;
                        set_flag(Cf, 1);
                    }
                    if (get_flag(Hf) || (regs[A] & 0x0F) > 0x09) {
                        regs[A] += 0x06;
                    }
                } else {
                    if (get_flag(Cf)) regs[A] -= 0x60;
                    if (get_flag(Hf)) regs[A] -= 0x06;
                }
                set_flag(Zf, regs[A] == 0);
                set_flag(Hf, 0);
                break;
            }
            case CPL_0b00101111: {
                regs[A] = ~regs[A];
                set_flag(Nf, 1);
                set_flag(Hf, 1);
                break;
            }
            case INC_0b00xx0011: {
                WAIT(4)
                auto rr = read45(i);
                unsigned short pair = inc_getpair(rr);
                ++pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                break;
            }
            case DEC_0b00xx1011: {
                WAIT(4)
                auto rr = read45(i);
                unsigned short pair = inc_getpair(rr);
                --pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                break;
            }
            case ADD_0b00xx1001: {
                WAIT(4)
                add_reg_HL(inc_getpair(read45(i)));
                break;
            }
            case ADD_0b11101000: {
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                WAIT(4)
                auto e = static_cast<char>(arg1);
                WAIT(4)
                int result = e + sp;
                set_flag(Cf, ((sp & 0xff) + (unsigned char) (e) > 0xff));
                set_flag(Hf, ((sp & 0xf) + ((unsigned char) (e) & 0xf) > 0xf));
                sp = (unsigned short) (result);
                WAIT(4)
                break;
            }
            case RLCA_0b00000111: {
                auto res = std::rotl(regs[A], 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, regs[A] & 0x80);
                regs[A] = res;
                break;
            }
            case RRCA_0b00001111: {
                auto res = std::rotr(regs[A], 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, regs[A] & 1);
                regs[A] = res;
                break;
            }
            case RLA_0b00010111: {
                auto carry = get_flag(Cf);
                auto res = ((regs[A] << 1) | carry);
                set_flag(Cf, regs[A] & 0x80);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                regs[A] = res;
                break;
            }
            case RRA_0b00011111: {
                auto carry = get_flag(Cf);
                auto res = ((regs[A] >> 1) | carry << 7);
                set_flag(Cf, regs[A] & 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                regs[A] = res;
                break;
            }
            case _0xCB: {
                auto pc_next = arg1;
                auto ins = decode_0xCB(pc_next);
                switch (ins) {
                    case RLC_0xCB_0b00000xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        auto res = std::rotl(reg, 1);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        set_flag(Cf, reg & 0x80);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case RRC_0xCB_0b00001xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        auto res = std::rotr(reg, 1);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        set_flag(Cf, reg & 1);
                        if (rid == HL){
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case RL_0xCB_0b00010xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        auto carry = get_flag(Cf);
                        unsigned char res = ((reg << 1) | carry);
                        set_flag(Cf, reg & 0x80);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case RR_0xCB_0b00011xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        auto carry = get_flag(Cf);
                        unsigned char res = ((reg >> 1) | carry << 7);
                        set_flag(Cf, reg & 1);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case SLA_0xCB_0b00100xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        unsigned char res = reg << 1;
                        set_flag(Cf, reg & 0x80);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case SRA_0xCB_0b00101xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        auto sign_bit = reg & 0x80;
                        unsigned char res = ((reg >> 1) | sign_bit);
                        set_flag(Cf, reg & 1);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case SWAP_0xCB_0b00110xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        auto l = reg & 0x0f;
                        auto h = reg & 0xf0;
                        unsigned char res = (l << 4 | h >> 4);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        set_flag(Cf, 0);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case SRL_0xCB_0b00111xxx: {
                        WAIT(4)
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        unsigned char res = reg >> 1;
                        set_flag(Cf, reg & 1);
                        set_flag(Zf, res == 0);
                        set_flag(Nf, 0);
                        set_flag(Hf, 0);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, res);
                        break;
                    }
                    case BIT_0xCB_0b01xxxxxx: {
                        WAIT(4)
                        auto bit_n = read345(pc_next);
                        if (read012(pc_next) == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(read012(pc_next), ram);
                        set_flag(Zf, !((reg >> bit_n) & 1));
                        set_flag(Nf, 0);
                        set_flag(Hf, 1);
                        break;
                    }
                    case RES_0xCB_0b10xxxxxx: {
                        WAIT(4)
                        auto bit_n = read345(pc_next);
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, reg & ~(1 << bit_n));
                        break;
                    }
                    case SET_0xCB_0b11xxxxxx: {
                        WAIT(4)
                        auto bit_n = read345(pc_next);
                        auto rid = read012(pc_next);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        auto reg = read_regs(rid, ram);
                        if (rid == HL) {
                            WAIT(4)
                        }
                        update_regs(rid, ram, reg | (1 << bit_n));
                        break;
                    }
                }
                break;
            }
                // Control flow
            case JP_0b11000011: {
                WAIT(12)
                pc = ((arg2 << 8) | arg1);
                break;
            }
            case JP_0b11101001: {
                pc = ((regs[H] << 8) | regs[L]);
                break;
            }
            case JP_0b110xx010: {
                auto val = read34(i);
                auto jmp = [&] { pc = ((arg2 << 8) | arg1); };
                bool cc_flag = get_cc_flag(val);
                if (cc_flag){
                    WAIT(12)
                    jmp();
                } else {
                    WAIT(8)
                    pc += 3;
                }
                break;
            }
            case JR_0b00011000: {
                WAIT(8)
                pc += 2 + (char) (arg1);
                break;
            }
            case JR_0b001xx000: {
                unsigned char val = read34(i);
                auto jmp = [&] { pc += 2 + (char) (arg1); };
                bool cc_flag = get_cc_flag(val);
                if (cc_flag) {
                    WAIT(8)
                    jmp();
                }else {
                    WAIT(4)
                    pc += 2;
                }
                break;
            }
            case CALL_0b11001101: {
                WAIT(8)
                unsigned short nn = ((arg2 << 8) | arg1);
                sp -= 2;
                WAIT(4)
                twrite(ram, sp, (pc + 3) & 0xff);
                WAIT(4)
                twrite(ram, sp + 1, ((pc + 3) >> 8));
                WAIT(4)
                pc = nn;
                break;
            }
            case CALL_0b110xx100: {
                WAIT(8)
                auto val = read34(i);
                unsigned short nn = ((arg2 << 8) | arg1);
                bool cc_flag = get_cc_flag(val);
                if (cc_flag) {
                    sp -= 2;
                    WAIT(4)
                    twrite(ram, sp, (pc + 3) & 0xff);
                    WAIT(4)
                    twrite(ram, sp + 1, ((pc + 3) >> 8) & 0xff);
                    WAIT(4)
                    pc = nn;
                } else {
                    pc += 3;
                }
                break;
            }
            case RET_0b11001001: {
                WAIT(4)
                unsigned short a1 = (tread(ram, sp + 1) << 8);
                WAIT(4)
                unsigned short a2 = tread(ram, sp);
                pc = (a1 | a2);
                WAIT(4)
                sp += 2;
                break;
            }
            case RET_0b110xx000: {
                WAIT(4)
                auto val = read34(i);
                auto cc_flag = get_cc_flag(val);
                if (cc_flag) {
                    WAIT(4)
                    unsigned short a1 = (tread(ram, sp + 1) << 8);
                    WAIT(4)
                    unsigned short a2 = tread(ram, sp);
                    WAIT(4)
                    pc = (a1 | a2);
                    sp += 2;
                } else {
                    ++pc;
                }
                break;
            }
            case RETI_0b11011001: {
                WAIT(4)
                unsigned short a1 = (tread(ram, sp + 1) << 8);
                WAIT(4)
                unsigned short a2 = tread(ram, sp);
                WAIT(4)
                pc = (a1 | a2);
                sp += 2;
                ime = true;
                break;
            }
            case RST_0b11xxx111: {
                WAIT(4)
                unsigned short nn = ((0x0 << 8) | rst_vec[read345(i)]);
                sp -= 2;
                WAIT(4)
                twrite(ram, sp, (pc + 1) & 0xff);
                WAIT(4)
                twrite(ram, sp + 1, ((pc + 1) >> 8));
                pc = nn;
                break;
            }
            case HALT: {
                if (!halted) {
                    halted = true;
                    ++pc;
                }
                break;
            }
            case STOP: {
//            if (tread(ram, constants::LY_ADDRESS) == 144)
//                ++pc;
                break;
            }
            case DI_0b11110011: {
                ime_requested = false;
                ime_req_count = 0;
                ime = false;
                break;
            }
            case EI_0b11111011: {
                ime_requested = true;
                ime_req_count = 0;  /*std::cout << "IF:" << (unsigned)tread(ram,constants::IF_ADDRESS) << '\n'*/;
                break;
            } // todo: check
            case NOP_0b00000000: {
                break;
            }
        }
        pc += pc_inc_by_ins(itype);
        if (halt_bug) {
            --pc;
            halt_bug = false;
        }
#if 0

                std::cout  << instruction_str[itype] << " ";
                if (!halted) {
                    std::cout <<

                              "A:" << print_hex(regs[CPU::A]).str() << " " <<
                              "F:" <<  print_hex(regs[CPU::F]).str() << " " <<
                              "B:" <<  print_hex(regs[CPU::B]).str() << " " <<
                              "C:" <<  print_hex(regs[CPU::C]).str() << " " <<
                              "D:" <<  print_hex(regs[CPU::D]).str() << " " <<
                              "E:" <<  print_hex(regs[CPU::E]).str() << " " <<
                              "H:" <<  print_hex(regs[CPU::H]).str() << " " <<
                              "L:" <<  print_hex(regs[CPU::L]).str() << " " <<
                              "SP:" <<  print_hex(sp).str() << " " <<
                              "PC:" <<  print_hex(pc).str() << " " <<
                              "PCMEM:" <<  print_hex(ram.read(pc)).str() << "," <<
                              print_hex(ram.read(pc + 1)).str() << "," <<
                              print_hex(ram.read(pc + 2)).str() << "," <<
                              print_hex(ram.read(pc + 3)).str() << '\n';
                    std::cout << std::dec;
                }

//                if (cpu.ime) {
#endif

        WAIT(1)
    }
}

void CPU::update_interrupts_status(RAM& ram) {
    fetch_IE_value(ram);
    fetch_IF_value(ram);
}

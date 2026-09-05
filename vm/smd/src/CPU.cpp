#include "CPU.hpp"
#include "BUS.hpp"

unsigned MC68K::sign_extend_to_l(unsigned short v) {
    return v & 0x8000 ? (0xffff0000 | v) : v;
}

void MC68K::fill_read_req(unsigned addr, BusReadDest brd, OperandSize sz) {
    iops_q.push_back({.address = addr, .brd = brd, .iop = BusReadReq, .req_count = 1});
    iops_q.push_back({.iop = BusReadWait});
    if (sz != LONG) return;
    iops_q.push_back({.address = addr, .brd = BusReadDest((int)brd + 1), .iop = BusReadReq, .req_count = 2});
    iops_q.push_back({.iop = BusReadWait});
}

void MC68K::ea_fill(unsigned char val){
    auto am = to_adress_mode(val);
    ea_reg_n = val & 0x7;
    switch(am) {
        case DataRegisterDirectMode:    ea_reg_type = Dn; break;
        case AddressRegisterDirectMode: ea_reg_type = An; break;
        case AddressRegisterIndirectMode: {
            fill_read_req(aregs[ea_reg_n], BRDOperandA1, opsz);
            break; 
        }
        case AddressRegisterIndirectPreDecMode: {
            aregs[ea_reg_n] -= opsz;
            // TODO: check
            iops_q.push_back({.iop = Nop});
            iops_q.push_back({.iop = Nop});
            fill_read_req(aregs[ea_reg_n], BRDOperandA1, opsz);
            break;
        }
        case AddressRegisterIndirectPostIncMode: {
            // TODO: check
            fill_read_req(aregs[ea_reg_n], BRDOperandA1, opsz);
            aregs[ea_reg_n] += opsz;
            break;
        }
        case AddressRegisterIndirectDisplacementMode: {
            fill_read_req(pc + 2, BRDSignExtend, WORD);
            break;
        }
    }
}

void MC68K::tick(BUS& bus){
    switch(iops_q.front().iop){
        case FetchDec: {
            auto opcode = prefetch_q.get();
            decoded_info = decode(opcode);
            iops_q.pop_front();
            break;
        }
        case BusReadReq: {
            // TODO: check bus_mode
            bus.mc68k_r(bus_address, bus_mode);
            iops_q.pop_front();
            break;
        }
        case BusReadWait: {
            if (!bus.mc68k_r_in_progress) {
                switch(iops_q.front().brd) {
                    case BRDOperandA1: operand_a_w1 = bus.mc68k_data; break;
                    case BRDOperandA2: operand_a_w2 = bus.mc68k_data; operands_a_to_long(); break;
                    case BRDOperandB1: operand_b_w1 = bus.mc68k_data; break;
                    case BRDOperandB2: operand_b_w2 = bus.mc68k_data; operands_b_to_long(); break;
            
                    case BRDSignExtend: {
                        fill_read_req(aregs[ea_reg_n] + sign_extend_to_l(bus.mc68k_data), BRDOperandA1, opsz);
                        break;
                    }
                }
                iops_q.pop_front();
            }
            break;
        }
        case BusWriteReq: {
            bus.mc68k_w(iops_q.front().address, bus_data, bus_mode);
            iops_q.pop_front();
            break;
        }
        case BusWriteWait: {
            if (!bus.mc68k_w_in_progress) {iops_q.pop_front();}
            break;
        }
        case Nop: {iops_q.pop_front(); break;}
        case ExecAdd: {
            // TODO:
            break;
        }
    }
  
    // TODO: 
}

MC68K::AddressMode MC68K::to_adress_mode(unsigned char val){
    unsigned char mode_field = (val >> 3) & 0x7;
    unsigned char reg_field = val & 0x7;
    if (mode_field == 0b000) return MC68K::DataRegisterDirectMode;
    if (mode_field == 0b001) return MC68K::AddressRegisterDirectMode;
    if (mode_field == 0b010) return MC68K::AddressRegisterIndirectMode;
    if (mode_field == 0b011) return MC68K::AddressRegisterIndirectPostIncMode;
    if (mode_field == 0b100) return MC68K::AddressRegisterIndirectPreDecMode;
    if (mode_field == 0b101) return MC68K::AddressRegisterIndirectDisplacementMode;
    if (mode_field == 0b110) return MC68K::AddressRegisterIndirectIndexMode;
    if (mode_field == 0b111 && reg_field == 0b000) return MC68K::AbsoluteShort;
    if (mode_field == 0b111 && reg_field == 0b001) return MC68K::AbsoluteLong;
    if (mode_field == 0b111 && reg_field == 0b010) return MC68K::ProgramCounterInderectDisplacementMode;
    if (mode_field == 0b111 && reg_field == 0b011) return MC68K::ProgramCounterInderectIndexMode;
    if (mode_field == 0b111 && reg_field == 0b100) return MC68K::Immediate;
    return MC68K::Unknown;
}

unsigned& MC68K::read_operand(unsigned char effective_address, BUS& bus){
    // auto am = to_adress_mode(effective_address);
    // TODO:
    // switch(am.type){
    //     case MC68K::DataRegisterDirectMode:             return dregs[effective_address & 0x7];
    //     case MC68K::AddressRegisterDirectMode:          return aregs[effective_address & 0x7];
    //     case MC68K::AddressRegisterIndirectMode:        return bus.mc68kmem[aregs[effective_address & 0x7]];
    //     case MC68K::AddressRegisterIndirectPostIncMode: {
    //         auto r = return bus.mc68kmem[aregs[effective_address & 0x7]]; 
    //         aregs[effective_address & 0x7] += opsz;
    //         return r;
    //     }
    //     case MC68K::AddressRegisterIndirectPreDecMode:       return bus.mc68kmem[aregs[effective_address & 0x7] -= operand_sz];
    // }
    return dregs[effective_address & 0x7];
}

void MC68K::exec(DecodedInfo di, BUS& bus) {
    switch(di.i){
        case MC68K::ADD:{
            unsigned& operand = read_operand(di.effective_address, bus);
            unsigned res = (dregs[di.rx] & di.mask) + (operand & di.mask);
            set_sreg(Extend, res > di.mask);
            set_sreg(Carry,  res > di.mask);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Overflow, ((dregs[di.rx] & di.sign_bit) == (operand & di.sign_bit)) && ((res & di.sign_bit) != (operand & di.sign_bit)));
            unsigned& v = (di.d1 == 0b000 || di.d1 == 0b001 || di.d1 == 0b010) ? dregs[di.rx] : operand;
            v &= ~di.mask;
            v |= res & di.mask;
            break;
        }
        case MC68K::ADDA: {
            auto res = dregs[di.rx] + read_operand(di.effective_address, bus);
            dregs[di.rx] = res;
            break;
        }
    }
}

MC68K::DecodedInfo MC68K::decode(unsigned short val) {
    if ((((val >> 12) & 0xf) == 0b1100) && (((val >> 4) & 0x1f) == 0b10000)) {
        return {MC68K::ABCD, bool(val & 0x8), (unsigned char)((val >> 9) & 0x7), (unsigned char)(val & 0x7)};
    }
    if ((((val >> 12) & 0xf) == 0b1101)) {
        unsigned char opmode = (val >> 6) & 0x7;
        unsigned char rx = (val >> 9) & 0x7;
        unsigned mask{};
        unsigned sign_bit{};
        
        switch(opmode){
            case 0b000:
            case 0b100: {opsz = MC68K::BYTE; sign_bit = 0x80; mask = 0x000000ff; break;}
            case 0b001:
            case 0b011:
            case 0b101: {opsz = MC68K::WORD; sign_bit = 0x8000; mask = 0x0000ffff; break;}
            default:    {opsz = MC68K::LONG; sign_bit = 0x80000000; mask = 0xffffffff; break;}
        }
        return {
            .i = (opmode == 0b011 || opmode == 0b111) ? MC68K::ADDA : MC68K::ADD,
            .rx = rx, 
            .d1 = opmode,
            .mask = mask,
            .sign_bit = sign_bit,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }
    if (((val >> 8) & 0xff) == 0b0000'0110) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::ADDI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

     if ((val >> 12) == 0b0101 && !(val & 0x100)) {
        unsigned char data = (val >> 9) & 0x3;
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::ADDQ,
            .d1 = data,
            .d2 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0b1101 && (val & 0x100) && !(val & 0x30)){
        unsigned char rx = (val >> 9) & 0x3;
        unsigned char ry = (val) & 0x3;
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::ADDX,
            .rx = rx,
            .ry = ry,
            .address_reg = (bool)(val & 0x8),
            .d1 = sz,
        };
    }

     if ((val >> 12) == 0b1100 && (val & 0x100) && !(val & 0x30)){
        unsigned char opmode = (val >> 6) & 0x3;
        unsigned char rx = (val >> 9) & 0x3;
        return {
            .i = MC68K::AND,
            .rx = rx,
            .d1 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

     if ((val >> 8) == 0b0000'0010){
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::ANDI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xe && !(val & 0x18)){
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char i_r = (val >> 5) & 1;
        return {
            .i = (val & 0x100) ? MC68K::ASL : MC68K::ASR,
            .d1 = count_reg,
            .d2 = sz,
            .d3 = i_r,
        };
    }

    if (val >> 12 == 0x6) {
        unsigned char cond = (val >> 8) & 0xf;
        unsigned char displacement = (val & 0xff);
        return {
            .i = MC68K::BCC,
            .d1 = cond,
            .d2 = displacement,
        };
    }

    if (!(val >> 12)) {
        unsigned char reg = (val >> 9) & 0x7;
        // MC68K::BCHG - 0b101
        // MC68K::BCLR - 0b110
        return {
            .i = ((val >> 6) & 0x7) == 0x5 ? MC68K::BCHG : MC68K::BCLR,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


    if ((val >> 8) == 0b0110'0000) {
        unsigned char displacement = val & 0xff;
        return {
            .i = MC68K::BRA,
            .d1 = displacement,
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::BSET,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0b0110'0001) {
        unsigned char displacement = (val >> 8) & 0xff;
        return {
            .i = MC68K::BSR,
            .d1 = displacement,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x4) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::BTST,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x4 && !(val & 0x40)) {
        unsigned char sz = (val >> 7) & 0x3;
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::CHK,
            .d1 = reg,
            .d2 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


      if ((val >> 8) == 0x42) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::CLR,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


    if ((val >> 12) == 0xb) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        return {
            .i = (opmode == 0b11 || opmode == 0b111) ? MC68K::CMPA : MC68K::CMP,
            .d1 = reg,
            .d2 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0xc) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::CMPI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


    if ((val >> 12) == 0xb && ((val >> 8) & 0x1) && ((val >> 3) & 0x7) == 0x1) {
        unsigned char rx = (val >> 9) & 0x7;
        unsigned char ry = (val & 0x7);
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::CMPM,
            .rx = rx,
            .ry = ry,
            .d1 = sz,
        };
    }

    if ((val >> 12) == 0xa && ((val >> 3) & 0x1f) == 0x19) {
        unsigned char cond = (val >> 8) & 0xf;
        unsigned char reg = (val & 0x7);
        return {
            .i = MC68K::DBcc,
            .d1 = cond,
            .d2 = reg,
        };
    }


    if ((val >> 12) == 0x8 && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = ((val >> 9) & 0x7);
        return {
            .i = MC68K::DIVS,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    // TODO: check
    if ((val >> 6) == 0x131) {
        return {
            .i = MC68K::DIVSL,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x8 && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::DIVU,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    // TODO: check
    if ((val >> 6) == 0x131) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::DIVUL,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xb) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
           return {
            .i = MC68K::EOR,
            .d1 = reg,
            .d2 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0xa) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::EORI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (val == 0xa3c) {
           return {
            .i = MC68K::EORItoCCR,
        };
    }

    if ((val >> 12) == 0xc && (val & 0x100)) {
        unsigned char rx = (val >> 9) & 0x7;
        unsigned char ry = (val & 0x7);
        unsigned char opmode = (val >> 3) & 0x1f;
        return {
            .i = MC68K::EXG,
            .rx = rx,
            .ry = ry,
            .d1 = opmode,
        };
    }

    if ((val >> 9) == 0x24 && !((val >> 3) & 0x7)) {
        unsigned char reg = (val & 0x7);
        unsigned char opmode = (val >> 3) & 0x1f;
        return {
            .i = MC68K::EXT,
            .d1 = reg,
            .d2 = opmode,
        };
    }

    if (val == 0x4afc) {
        return {
            .i = MC68K::ILLEGAL,
        };
    }

    if (val == 0x13b) {
        return {
            .i = MC68K::JMP,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (val == 0x13a) {
        return {
            .i = MC68K::JSR,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }
    
    if ((val >> 12) == 0x4 && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::LEA,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 3) == 0x9ca) {
        unsigned char reg = (val) & 0x7;
        return {
            .i = MC68K::LINKw,
            .d1 = reg,
        };
    }

    if ((val >> 3) == 0x901) {
        unsigned char reg = (val) & 0x7;
        return {
            .i = MC68K::LINKl,
            .d1 = reg,
        };
    }

    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x1) {
        unsigned char reg = (val) & 0x7;
        unsigned char i_r = (val >> 5) & 0x1;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char count_reg = (val >> 9) & 0x7;
        return {
            .i = (val & 0x100) ? MC68K::LSL : MC68K::LSR,
            .d1 = reg,
            .d2 = i_r,
            .d3 = sz,
            .d4 = count_reg
        };
    }

    if (!(val >> 14)) {
        unsigned char sz = (val >> 12) & 0x3;
        return {
            .i = MC68K::MOVE,
            .d1 = sz,
            .d2 = (unsigned char)((val >> 6) & 0x3f),
            .d3 = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 14) && ((val >> 6) & 0x7) == 0x1) {
        unsigned char sz = (val >> 12) & 0x3;
        unsigned char dst = (val >> 9) & 0x7;
        return {
            .i = MC68K::MOVEA,
            .d1 = sz,
            .d2 = dst,
            .d3 = (unsigned char)(val & 0x3f),
        };
    }


    if ((val >> 6) == 0x113) {
        return {
            .i = MC68K::MOVEtoCCR,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


    if ((val >> 11) == 0x9 && ((val >> 7) & 0x7) == 0x1) {
        unsigned char sz = (val >> 6) & 0x1;
        unsigned char dr = (val >> 10) & 0x1;
        return {
            .i = MC68K::MOVEM,
            .d1 = sz,
            .d2 = dr,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 11) == 0x9 && ((val >> 7) & 0x7) == 0x1) {
        unsigned char sz = (val >> 6) & 0x1;
        unsigned char dr = (val >> 10) & 0x1;
        return {
            .i = MC68K::MOVEM,
            .d1 = sz,
            .d2 = dr,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 12)  && ((val >> 3) & 0x7) == 0x1) {
        unsigned char data_reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        unsigned char address_reg = val & 0x7;
        return {
            .i = MC68K::MOVEP,
            .d1 = data_reg,
            .d2 = opmode,
            .d3 = address_reg,
        };
    }

    if ((val >> 12) == 0x7 && !(val & 0x100)) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char data = val & 0xff;
        return {
            .i = MC68K::MOVEP,
            .d1 = reg,
            .d2 = data,
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::MULS,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::MULU,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MC68K::MULU,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 6) == 0x120) {
        return {
            .i = MC68K::NBCD,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x44) {
        return {
            .i = MC68K::NEG,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x44) {
        return {
            .i = MC68K::NEG,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x40) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::NEGX,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (val == 0x4e71) {
        return {
            .i = MC68K::NOP,
        };
    }

    if ((val >> 8) == 0x46) {
        return {
            .i = MC68K::NOT,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }
    
    if ((val >> 12) == 0x8) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        return {
            .i = MC68K::OR,
            .d1 = reg,
            .d2 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 8)) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::ORI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 8)) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::ORItoCCR,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (val == 0x3c) {
        return {
            .i = MC68K::ORItoCCR,
        };
    }

    if ((val >> 6) == 0x121) {
        return {
            .i = MC68K::PEA,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 6) == 0x121) {
        return {
            .i = MC68K::PEA,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x3) {
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char i_r = (val >> 5) & 0x1;
        return {
            .i = (val & 0x100) ?  MC68K::ROR : MC68K::ROL,
            .d1 = count_reg,
            .d2 = sz,
            .d3 = i_r,
        };
    }

    // TODO: check
    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x3) {
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char i_r = (val >> 5) & 0x1;
        return {
            .i = (val & 0x100) ?  MC68K::ROR : MC68K::ROL,
            .d1 = count_reg,
            .d2 = sz,
            .d3 = i_r,
        };
    }

    // TODO: check
    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x2) {
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char i_r = (val >> 5) & 0x1;
        return {
            .i = (val & 0x100) ?  MC68K::ROXR : MC68K::ROXL,
            .d1 = count_reg,
            .d2 = sz,
            .d3 = i_r,
        };
    }

    if ((val >> 4) == 0x6c) {
        unsigned char da = (val >> 3) & 0x1;
        unsigned char reg = val & 0x7;
        return {
            .i = MC68K::RTM,
            .d1 = da,
            .d2 = reg,
        };
    }

    if (val == 0x4e77) {
        return {
            .i = MC68K::RTR,
        };
    }

    if (val == 0x4e75) {
        return {
            .i = MC68K::RTS,
        };
    }

    if ((val >> 12) == 0x8 && ((val >> 4) & 0x1f) == 0x10) {
        unsigned char rx = val & 0x7;
        unsigned char ry = (val >> 9) & 0x7;
        unsigned char rm = (val >> 3) & 0x1;
        return {
            .i = MC68K::SBCD,
            .rx = rx,
            .ry = ry,
            .d1 = rm,
        };
    }

    if ((val >> 12) == 0x5 && ((val >> 6) & 0x3) == 0x3) {
        unsigned char cond = (val >> 8) & 0xf;
        return {
            .i = MC68K::Scc,
            .d1 = cond,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x9) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        return {
            .i = (opmode == 0b11 || opmode == 0b111) ?  MC68K::SUBA : MC68K::SUB,
            .d1 = reg,
            .d2 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x4) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::SUBI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x5 && ((val >> 8) & 0x1)) {
        unsigned char data = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::SUBQ,
            .d1 = data,
            .d2 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x9 && ((val >> 8) & 0x1) && !((val >> 4) & 0x3)) {
        unsigned char rx = val & 0x7;
        unsigned char ry = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char rm = (val >> 3) & 0x1;
        return {
            .i = MC68K::SUBX,
            .rx = rx,
            .ry = ry,
            .d1 = sz,
            .d2 = rm,
        };
    }

    if ((val >> 3) == 0x908) {
        unsigned char reg = val & 0x7;
        return {
            .i = MC68K::SWAP,
            .d1 = reg,
        };
    }

    if ((val >> 6) == 0x12b) {
        unsigned char reg = val & 0x7;
        return {
            .i = MC68K::TAS,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 4) == 0x4e4) {
        unsigned char vec = val & 0xf;
        return {
            .i = MC68K::TRAP,
            .d1 = vec,
        };
    }

    if (val == 0x4e76) {
        return {
            .i = MC68K::TRAPV,
        };
    }


    if ((val >> 8) == 0x4a) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = MC68K::TST,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 3) == 0x9cb) {
        unsigned char reg = val & 0x7;
        return {
            .i = MC68K::UNLK,
            .d1 = reg,
        };
    }

    return {};
}
#include "CPU.hpp"
#include "BUS.hpp"

void MC68K::req(unsigned short a, unsigned short b, ALUOp op){
    arg1 = a;
    arg2 = b;
    alu_op = op;
    count = 4; // TODO: check
}

void MC68K::set_ea_dst() {
    switch (ea_mode) {
        case DataRegisterDirectMode:    dst = DSTDn; dst_reg_n = ea_reg_n;  break;
        case AddressRegisterDirectMode: dst = DSTAn; dst_reg_n = ea_reg_n;  break;
        default:
            dst = DSTMem; dst_addr = ea_address; break;
    }
}

void MC68K::write_dst() {
    switch (dst) {
        case DSTDn:  dregs[dst_reg_n] &= ~mask; dregs[dst_reg_n] |= dst_value & mask; break;
        case DSTAn:  aregs[dst_reg_n] &= ~mask; aregs[dst_reg_n] |= dst_value & mask; break;
        case DSTMem: fill_bus_write_req(dst_addr, dst_value, opsz); break;
    }
}

unsigned char MC68K::ea_fetch_cycles(MC68K::AddressMode ea, MC68K::OperandSize sz) {
    switch (am) {
        case AddressRegisterDirectMode:
        case DataRegisterDirectMode: {
            return 0;
        }
        case AddressRegisterIndirectMode:
        case AddressRegisterIndirectPostIncMode:{
            return sz == LONG ? 8 : 4;
        }
        case AddressRegisterIndirectPreDecMode: {
            return sz == LONG ? 10 : 6;
        }
        case ProgramCounterInderectDisplacementMode:
        case AddressRegisterIndirectDisplacementMode: {
            return  sz == LONG ? 12 : 8;
        }
        case ProgramCounterInderectIndexMode:
        case AddressRegisterIndirectIndexMode: {
            return  sz == LONG ? 14 : 10;
        }
        case AbsoluteShort: {
            return 4;
        }
        case AbsoluteLong: {
            return 8;
        }
        case Immediate: {
            return sz == LONG ? 8 : 4;
        }
        default: return 0;
    }
}

void MC68K::fill_bus_write_req(unsigned address, unsigned value, OperandSize sz) {
    auto even_addr = address & 0xfffffffe;
    if (sz == BYTE){
        auto val = (unsigned short)(value & 0xff);
        val = (val << 8) | val; // duplicate
        iops_q.push_back({.address = even_addr, .iop = BusWriteReq, .bus_data = val,.bus_mode = (address & 1) ? BUS::LDS : BUS::UDS});
        iops_q.push_back({.iop = BusWriteWait});
    } else if (sz == WORD) {
        iops_q.push_back({.address = even_addr, .iop = BusWriteReq, .bus_data = (unsigned short)(value & 0xffff), .bus_mode = BUS::UDS_LDS});
        iops_q.push_back({.iop = BusWriteWait});
    } else if (sz == LONG){
        iops_q.push_back({.address = even_addr, .iop = BusWriteReq, .bus_data = (unsigned short)((value >> 16) & 0xffff), .bus_mode = BUS::UDS_LDS});
        iops_q.push_back({.iop = BusWriteWait});
        iops_q.push_back({.address = even_addr + 2, .iop = BusWriteReq, .bus_data = (unsigned short)(value & 0xffff), .bus_mode = BUS::UDS_LDS});
        iops_q.push_back({.iop = BusWriteWait});
    }
}

void MC68K::fill_bus_read_req(unsigned addr, BusReadDest brd, OperandSize sz) {
    auto even_addr = addr & 0xfffffffe;
    switch (sz) {
        case BYTE: {
            iops_q.push_back({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = ((addr & 1) ? BUS::LDS : BUS::UDS)});
            iops_q.push_back({.iop = BusReadWait, .brd = brd});
            break;
        }
        case WORD: {
            iops_q.push_back({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = BUS::UDS_LDS});
            iops_q.push_back({.iop = BusReadWait, .brd = brd});
            break;
        }
        case LONG: {
            iops_q.push_back({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = BUS::UDS_LDS});
            iops_q.push_back({.iop = BusReadWait, .brd = brd});
            iops_q.push_back({.address = even_addr + 2, .iop = BusReadReq, .brd = BusReadDest((int)brd + 1), .bus_mode = BUS::UDS_LDS});
            iops_q.push_back({.iop = BusReadWait, .brd = brd});
            break;
        }
    }
}

//https://github.com/larsbrinkhoff/m68k-microcode/blob/master/doc/Yacht.txt
unsigned char MC68K::exec_cycles(const DecodedInfo& di) {
    auto ea = to_adress_mode(di.effective_address);
    switch (di.i) {
        //        #<data>,CCR     check
        case EORI:
        case ORI:
        case ANDI:
        case SUBI:
        case ADDI: {
            if (ea == DataRegisterDirectMode) return di.opsz == LONG ? 16 : 8;
            return di.opsz == LONG ? 20 : 16;
        }
        case MOVEtoCCR: return 12;
        case NBCD: return (ea == DataRegisterDirectMode ? 6 : 8);
        case Scc : return (di.opmode ? (ea == DataRegisterDirectMode ? 4 : 8) : (ea == DataRegisterDirectMode ? 6 : 8));
        case CMPI: return (di.opsz == LONG ? (ea == DataRegisterDirectMode ? 14 : 12) : 8);
        case BCHG:
        case BSET: {
            if (di.opsz == BYTE) return di.opmode ? 12 : 8;
            // TODO: immediate use
            return di.opmode ? (di.immediate > 15 ? 12 : 10) : (dregs[di.d1] > 15 ? 8 : 6);
        }
        case BCLR: {
            if (di.opsz == BYTE) return di.opmode ? 12 : 8;
            // TODO: immediate use
            return di.opmode ? (di.immediate > 15 ? 14 : 12) : (dregs[di.d1] > 15 ? 10 : 8);
        }
        case BTST: {
            if (di.opmode) return di.opsz == BYTE ? 8 : 10;
            return (di.opsz == LONG || ea == Immediate) ? 6 : 4;
        }
        case MOVEA: {
            if (di.opsz == WORD) {
                switch (ea) {
                    case DataRegisterDirectMode:
                    case AddressRegisterDirectMode: return 4;
                    case AddressRegisterIndirectMode:
                    case AddressRegisterIndirectPostIncMode: return 8;
                    case AddressRegisterIndirectPreDecMode: return 10;
                    case AddressRegisterIndirectDisplacementMode: return 12;
                    case AddressRegisterIndirectIndexMode: return 14;
                    case AbsoluteShort: return 12;
                    case AbsoluteLong: return 16;
                    case Immediate: return 8;
                }
            }
            switch (ea) {
                case DataRegisterDirectMode:
                case AddressRegisterDirectMode: return 4;
                case AddressRegisterIndirectMode:
                case AddressRegisterIndirectPostIncMode: return 12;
                case AddressRegisterIndirectPreDecMode: return 14;
                case AddressRegisterIndirectDisplacementMode: return 16;
                case AddressRegisterIndirectIndexMode: return 18;
                case AbsoluteShort: return 16;
                case AbsoluteLong: return 20;
                case Immediate: return 12;
            }
        }
        case CLR:
        case NEGX:
        case NEG:
        case NOT: {
            if (di.opsz == BYTE)
                return ea == DataRegisterDirectMode ? 4 : 8;
            return ea == DataRegisterDirectMode ? 6 : 12;
        }
        case ADDA:
        case SUBA:{
            if (di.opsz == LONG)
                return (ea == DataRegisterDirectMode || ea == AddressRegisterDirectMode || ea == Immediate) ?  8 : 6;
            return 8;
        }
        case ADDX:
        case SUBX: {
            return opmode ? (di.opsz == LONG ? 30 : 18) : (di.opsz == LONG ? 8 : 4);
        }
        case CMP: {
            return di.opsz == LONG ? 6 : 4;
        }
        case CMPA: return 6;
        case CMPM: return di.opsz == LONG ? 20 : 12;
        case EOR:  return di.opsz == LONG ? (ea == DataRegisterDirectMode ? 8 : 12) : (ea == DataRegisterDirectMode ? 4 : 8);
        // TODO: continue
    }
}

void MC68K::ea_fill(unsigned char val){
    ea_mode = to_adress_mode(val);
    ea_reg_n = val & 0x7;
    switch(ea_mode) {
        case DataRegisterDirectMode:    ea_reg_type = Dn; operand_a_l = dregs[ea_reg_n]; break;
        case AddressRegisterDirectMode: ea_reg_type = An; operand_a_l = aregs[ea_reg_n]; break;
        case AddressRegisterIndirectMode: {
            ea_address = aregs[ea_reg_n];
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            break; 
        }
        case AddressRegisterIndirectPostIncMode: {
            ea_address = aregs[ea_reg_n];
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            aregs[ea_reg_n] += opsz;
            break;
        }
        case AddressRegisterIndirectPreDecMode: {
            aregs[ea_reg_n] -= opsz;
            // TODO: check
            iops_q.push_back({.iop = Nop});
            iops_q.push_back({.iop = Nop});
            ea_address = aregs[ea_reg_n];
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            break;
        }
        case AddressRegisterIndirectDisplacementMode: {
            auto pv = read_prefetch_q();
            ea_address = aregs[ea_reg_n] + sign_extend_to_l(pv);
            fill_prefetch_req();
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            break;
        }
        case AddressRegisterIndirectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            ea_address = aregs[ea_reg_n] +
                sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;     
    
            fill_prefetch_req();
            iops_q.push_back({.iop = Nop});
            iops_q.push_back({.iop = Nop}); // calc time
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            break;
        }
        case ProgramCounterInderectDisplacementMode: {
            auto pv = read_prefetch_q();
            ea_address = pc + sign_extend_to_l(pv);
            fill_prefetch_req();
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            break;
        }
        case ProgramCounterInderectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            ea_address = pc +
                sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;
            fill_prefetch_req();
            iops_q.push_back({.iop = Nop});
            iops_q.push_back({.iop = Nop}); // calc time
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            break;
        }
        case AbsoluteShort: {
            operand_a_w1 = read_prefetch_q();
            // TODO: check
            ea_address = pc + 2;
            fill_prefetch_req();
            break;
        }
        case AbsoluteLong: {
            operand_a_w1 = read_prefetch_q();
            operand_a_w2 = read_prefetch_q();
            // TODO: check
            ea_address = pc + 2;
            operands_a_to_long();
            fill_prefetch_req();
            fill_prefetch_req();
            break;
        }
         case Immediate: {
            operand_a_w1 = read_prefetch_q();
            fill_prefetch_req();
            if (opsz != LONG) break;
            operand_a_w2 = read_prefetch_q();
            operands_a_to_long();
            fill_prefetch_req();
            break;
        }
    }
}

void MC68K::tick(BUS& bus){
    auto iopv = iops_q.front();
    switch(iopv.iop){
        case FetchDec: {
            // TODO: update
            auto opcode = read_prefetch_q();
            fill_prefetch_req();
            di = decode(opcode);
            opsz = di.opsz;
            update_mask_n_sign_bit();
            opmode = di.opmode;
            mask = di.mask;
            if (di.is_ea) ea_fill(di.effective_address);
            iops_q.pop_front();
            break;
        }
        case BusReadReq: {
            // TODO: check bus_mode
            bus.mc68k_r(iopv.address, bus_mode);
            iops_q.pop_front();
            break;
        }
        case BusReadWait: {
            if (!bus.mc68k_r_in_progress) {
                switch(iopv.brd) {
                    case BRDOperandA1: operand_a_w1 = bus.mc68k_get(); break;
                    case BRDOperandA2: operand_a_w2 = bus.mc68k_get(); operands_a_to_long(); break;
                    case BRDOperandB1: operand_b_w1 = bus.mc68k_get(); break;
                    case BRDOperandB2: operand_b_w2 = bus.mc68k_get(); operands_b_to_long(); break;
                    case BRDPrefetchQ: {prefetch_q.push_back(bus.mc68k_data); break;}
                }
                iops_q.pop_front();
            }
            break;
        }
        case BusWriteReq: {
            bus.mc68k_w(iopv.address, iopv.bus_data, iopv.bus_mode);
            iops_q.pop_front();
            break;
        }
        case BusWriteWait: {
            if (!bus.mc68k_w_in_progress) {iops_q.pop_front();}
            break;
        }
        case Nop: {iops_q.pop_front(); break;}
    }
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


void MC68K::fill_exec(MC68K::Instruction i) {}

void MC68K::exec(DecodedInfo di, BUS& bus) {
    switch(di.i){
        case MC68K::ADD:{
            operands_a_to_long();
            unsigned res = (dregs[rx] & mask) + (operand_a_l & mask);
            // TODO: move to function
            set_sreg(Extend, res > mask);
            set_sreg(Carry,  res > mask);
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Overflow, ((dregs[rx] & sign_bit) == (operand_a_l & sign_bit)) && ((res & sign_bit) != (operand_a_l & sign_bit)));
            // TODO: check
            dst_value = res & mask;
            if (opmode == 0b000 || opmode == 0b001 || opmode == 0b010) {
                dst = DSTDn;
                dst_reg_n = di.rx;
            } else {
                set_ea_dst();
            }
            break;
        }
        case MC68K::ADDA: {
//            auto res = dregs[di.rx] + read_operand(di.effective_address, bus);
//            dregs[di.rx] = res;
            break;
        }
    }
}

MC68K::DecodedInfo MC68K::decode(unsigned short val) {
    if ((((val >> 12) & 0xf) == 0b1100) && (((val >> 4) & 0x1f) == 0b10000)) {
        return {
            .i = MC68K::ABCD,
            .opmode = (unsigned char)((val >> 3) & 0x1), // r/m
            .address_reg = bool(val & 0x8),
            .rx = (unsigned char)((val >> 9) & 0x7),
            .ry = (unsigned char)(val & 0x7)

        };
    }
    if ((((val >> 12) & 0xf) == 0b1101)) {
        unsigned char opmode = (val >> 6) & 0x7;
        unsigned char rx = (val >> 9) & 0x7;
        OperandSize sz;
        switch(opmode){
            case 0b000:
            case 0b100: {sz = MC68K::BYTE;break;}
            case 0b001:
            case 0b011:
            case 0b101: {sz = MC68K::WORD;break;}
            default:    {sz = MC68K::LONG;break;}
        }
        return {
            .i = (opmode == 0b011 || opmode == 0b111) ? MC68K::ADDA : MC68K::ADD,
            .rx = rx,
            .operand_sz = sz,
            .effective_address = (unsigned char)(val & 0x3f),
            .opmode = opmode,
            .is_ea = true,
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
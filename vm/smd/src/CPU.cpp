#include "CPU.hpp"
#include "BUS.hpp"

void M68K::req(unsigned short a, unsigned short b, ALUOp op){
    arg1 = a;
    arg2 = b;
    alu_op = op;
    count = 4; // TODO: check
}

void M68K::set_ea_dst() {
    switch (ea_mode) {
        case DataRegisterDirectMode:    dst = DSTDn; dst_reg_n = ea_reg_n;  break;
        case AddressRegisterDirectMode: dst = DSTAn; dst_reg_n = ea_reg_n;  break;
        default:
            dst = DSTMem; dst_addr = ea_address; break;
    }
}

void M68K::write_dst() {
    switch (dst) {
        case DSTDn:  dregs[dst_reg_n] &= ~mask; dregs[dst_reg_n] |= dst_value & mask; break;
        case DSTAn:  aregs[dst_reg_n] &= ~mask; aregs[dst_reg_n] |= dst_value & mask; break;
        case DSTMem: fill_bus_write_req(dst_addr, dst_value, opsz); break;
    }
}

unsigned char M68K::ea_fetch_cycles(M68K::AddressMode ea, M68K::OperandSize sz) {
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

void M68K::fill_bus_write_req(unsigned address, unsigned value, OperandSize sz) {
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

void M68K::fill_bus_read_req(unsigned addr, BusReadDest brd, OperandSize sz) {
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

//https://github.com/larsbrinkhoff/M68K-microcode/blob/master/doc/Yacht.txt
unsigned char M68K::exec_cycles(const DecodedInfo& di) {
    auto ea = to_adress_mode(di.effective_address, true);
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

void M68K::ea_write(unsigned char ea_val, unsigned val) {
    // TODO: check
    auto mode = to_adress_mode(ea_val, false);
    auto reg_n = (ea_val >> 3) & 0x7;
    switch(mode) {
        case DataRegisterDirectMode:    dregs[reg_n] &= ~mask; dregs[reg_n] |= (val & mask); break;
        case AddressRegisterDirectMode: aregs[reg_n] &= ~mask; aregs[reg_n] |= (val & mask); break;
        case AddressRegisterIndirectMode: {
            fill_bus_write_req(aregs[reg_n], val, opsz);
            break;
        }
        case AddressRegisterIndirectPostIncMode: {
            fill_bus_write_req(aregs[ea_reg_n], val, opsz);
            aregs[ea_reg_n] += opsz;
            break;
        }
        case AddressRegisterIndirectPreDecMode: {
            aregs[ea_reg_n] -= opsz;
            // TODO: check
            iops_q.push_back({.iop = Nop});
            iops_q.push_back({.iop = Nop});
            fill_bus_write_req(aregs[ea_reg_n], val, opsz);
            break;
        }
        case AddressRegisterIndirectDisplacementMode: {
            auto pv = read_prefetch_q();
            fill_prefetch_req();
            fill_bus_write_req(aregs[ea_reg_n] + sign_extend_to_l(pv), val, opsz);
            break;
        }
        case AddressRegisterIndirectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            auto addr = aregs[ea_reg_n] +
                         sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;

            fill_prefetch_req();
            iops_q.push_back({.iop = Nop});
            iops_q.push_back({.iop = Nop}); // calc time
            fill_bus_write_req(addr, val, opsz);
            break;
        }
        case ProgramCounterInderectDisplacementMode: {
            auto pv = read_prefetch_q();
            auto addr = pc + sign_extend_to_l(pv);
            fill_prefetch_req();
            fill_bus_write_req(addr, val, opsz);
            break;
        }
        case ProgramCounterInderectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            auto addr = pc +
                         sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;
            fill_prefetch_req();
            iops_q.push_back({.iop = Nop});
            iops_q.push_back({.iop = Nop}); // calc time
            fill_bus_write_req(addr, val, opsz);
            break;
        }
    }
}

void M68K::ea_fill(unsigned char val){
    ea_mode = to_adress_mode(val, true);
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

void M68K::tick(BUS& bus){
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
            bus.m68k_r_req(iopv.address, bus_mode);
            iops_q.pop_front();
            break;
        }
        case BusReadWait: {
            if (!bus.r_in_progress) {
                switch(iopv.brd) {
                    case BRDOperandA1: operand_a_w1 = bus.m68k_get(); break;
                    case BRDOperandA2: operand_a_w2 = bus.m68k_get(); operands_a_to_long(); break;
                    case BRDOperandB1: operand_b_w1 = bus.m68k_get(); break;
                    case BRDOperandB2: operand_b_w2 = bus.m68k_get(); operands_b_to_long(); break;
                    case BRDPrefetchQ: {prefetch_q.push_back(bus.m68k_data); break;}
                }
                iops_q.pop_front();
            }
            break;
        }
        case BusWriteReq: {
            bus.m68k_w_req(iopv.address, iopv.bus_data, iopv.bus_mode);
            iops_q.pop_front();
            break;
        }
        case BusWriteWait: {
            if (!bus.w_in_progress) {iops_q.pop_front();}
            break;
        }
        case Nop: {iops_q.pop_front(); break;}
    }
}

M68K::AddressMode M68K::to_adress_mode(unsigned char val, bool mr){
    unsigned char mode_field = mr ? (val >> 3) & 0x7 : val & 0x7;
    unsigned char reg_field = mr ? val & 0x7 : (val >> 3) & 0x7;
    if (mode_field == 0b000) return DataRegisterDirectMode;
    if (mode_field == 0b001) return AddressRegisterDirectMode;
    if (mode_field == 0b010) return AddressRegisterIndirectMode;
    if (mode_field == 0b011) return AddressRegisterIndirectPostIncMode;
    if (mode_field == 0b100) return AddressRegisterIndirectPreDecMode;
    if (mode_field == 0b101) return AddressRegisterIndirectDisplacementMode;
    if (mode_field == 0b110) return AddressRegisterIndirectIndexMode;
    if (mode_field == 0b111 && reg_field == 0b000) return AbsoluteShort;
    if (mode_field == 0b111 && reg_field == 0b001) return AbsoluteLong;
    if (mode_field == 0b111 && reg_field == 0b010) return ProgramCounterInderectDisplacementMode;
    if (mode_field == 0b111 && reg_field == 0b011) return ProgramCounterInderectIndexMode;
    if (mode_field == 0b111 && reg_field == 0b100) return Immediate;
    return Unknown;
}


bool M68K::is_cond(M68K::ConditionCode cc) {
    switch (cc) {
        case CC_CC: return !get_sreg(Carry);
        case CC_CS: return get_sreg(Carry);
        case CC_VS: return get_sreg(Overflow);
        case CC_VC: return !get_sreg(Overflow);
        case CC_EQ: return get_sreg(Zero);
        case CC_NE: return !get_sreg(Zero);
        case CC_GE: return get_sreg(Negative) == get_sreg(Overflow);
        case CC_GT: return !get_sreg(Zero) && get_sreg(Negative) == get_sreg(Overflow);
        case CC_LE: return get_sreg(Zero) || get_sreg(Negative) != get_sreg(Overflow);
        case CC_LT: return get_sreg(Negative) != get_sreg(Overflow);
        case CC_HI: return !get_sreg(Carry) && !get_sreg(Zero);
        case CC_LS: return get_sreg(Carry) || get_sreg(Zero);
        case CC_MI: return get_sreg(Negative);
        case CC_PL: return !get_sreg(Negative);
    }
}


void M68K::fill_exec(M68K::Instruction i) {}

void M68K::exec(DecodedInfo di, BUS& bus) {
    switch(di.i){
        case ADD:{
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
        case ADDA: {
            operands_a_to_long();
            auto res = ((aregs[rx] & mask) + (operand_a_l & mask)) & mask;
            aregs[rx] &= ~mask;
            aregs[rx] |= res;
            break;
        }
        case ADDI: {
            auto res = (operand_a_l & mask) + (operand_b_l & mask);
            set_sreg(Extend, res > mask);
            set_sreg(Carry,  res > mask);
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Overflow, ((operand_a_l & sign_bit) == (operand_b_l & sign_bit)) && ((res & sign_bit) != (operand_a_l & sign_bit)));
            dst_value = res & mask;
            set_ea_dst();
            break;
        }
        case ADDQ: {
            auto res = (operand_a_l & mask) + (d1);
            if (ea_mode != AddressRegisterDirectMode){
                set_sreg(Extend, res > mask);
                set_sreg(Carry,  res > mask);
                set_sreg(Negative, res & sign_bit);
                set_sreg(Zero,      (res & mask) == 0);
                set_sreg(Overflow, ((operand_a_l & sign_bit) == (d1 & sign_bit)) && ((res & sign_bit) != (operand_a_l & sign_bit)));
                break;
            }
            dst_value = res & mask;
            set_ea_dst();
            break;
        }
        case ADDX: {
            // TODO:
            break;
        }
        case AND: {
            unsigned res = (dregs[rx] & mask) & (operand_a_l & mask);
            set_sreg(Carry,  0);
            set_sreg(Overflow, 0);
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            dst_value = res & mask;
            if (opmode == 0b000 || opmode == 0b001 || opmode == 0b010) {
                dst = DSTDn;
                dst_reg_n = di.rx;
            } else {
                set_ea_dst();
            }
            break;
        }
        case ANDItoCCR: {
            unsigned char res = sreg & (operand_a_w1 & 0xff);
            sreg &= 0xff00;
            sreg |= res;
            break;
        }
        case BCC: {
            pc += is_cond((ConditionCode)d1) ? (di.dl & mask) : 0;
            prefetch_q.clear();
            // TODO: check
            fill_prefetch_req();
            fill_prefetch_req();
            break;
        }
        case BCHG: {
            auto n = opmode ? dregs[dst_reg_n] & 0x1f : (d1 & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            dst_value = operand_a_l ^ (1 << n);
            set_ea_dst();
            break;
        }
        case BCLR: {
            auto n = opmode ? dregs[dst_reg_n] & 0x1f : (d1 & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            dst_value = operand_a_l & (~(1 << n));
            set_ea_dst();
            break;
        }
        case BRA: {
            if (di.opsz == BYTE) pc += sign_extend_to_l((unsigned char)(di.dl));
            else pc += sign_extend_to_l((unsigned short)(di.dl));
            break;
        }
        case BSET: {
            auto n = opmode ? dregs[dst_reg_n] & 0x1f : (d1 & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            dst_value = operand_a_l | ((1 << n));
            set_ea_dst();
            break;
        }
        case BSR: {
            usp-=4;
            // TODO: PC to next instruction
            fill_bus_write_req(usp, pc, LONG);
            if (di.opsz == BYTE) pc += sign_extend_to_l((unsigned char)(di.dl));
            else pc += sign_extend_to_l((unsigned short)(di.dl));
            break;
        }
        case BTST: {
            auto n = opmode ? dregs[dst_reg_n] & 0x1f : (d1 & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            break;
        }
        case CHK: {
            auto val = sign_extend_to_l((unsigned short)(dregs[di.d1] & 0xffff));
            if (val < 0) {
                // TODO: exception
            } else if (val > operand_a_l){
                // TODO: exception
            }
            break;
        }
        case CLR: {
            // TODO: check
            dst_value = 0;
            set_ea_dst();
            break;
        }
        case CMP: {
            auto res = (dregs[di.d1] & mask) - (operand_a_l & mask);
            set_sreg(Carry,  (operand_a_l & mask) > (dregs[di.d1] & mask));
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Overflow, ((dregs[di.d1] & sign_bit) != (operand_a_l & sign_bit)) && ((res & sign_bit) != (dregs[di.d1] & sign_bit)));
            break;
        }

        case CMPA: {
            auto res = di.opsz ? ((aregs[di.d1] & mask) - (operand_a_l & mask)) :
                       sign_extend_to_l((unsigned short)(aregs[di.d1] & mask)) - sign_extend_to_l((unsigned short)(operand_a_l & mask));
            set_sreg(Carry,  (operand_a_l & mask) > (aregs[di.d1] & mask));
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Overflow, ((aregs[di.d1] & sign_bit) != (operand_a_l & sign_bit)) && ((res & sign_bit) != (aregs[di.d1] & sign_bit)));
            break;
        }
        case CMPI: {
            auto res = (operand_a_l & mask) - (operand_b_l & mask);
            set_sreg(Carry,  (operand_b_l & mask) > (operand_a_l & mask));
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Overflow, ((operand_b_l & sign_bit) != (operand_a_l & sign_bit)) && ((res & sign_bit) != (operand_a_l & sign_bit)));
            break;
        }

        case CMPM: {
            auto res = (aregs[di.rx] & mask) - (aregs[di.ry] & mask);
            set_sreg(Carry,   (aregs[di.ry] & mask) > (aregs[di.rx] & mask));
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Overflow, ((aregs[di.rx] & sign_bit) != (aregs[di.ry] & sign_bit)) && ((res & sign_bit) != (aregs[di.ry] & sign_bit)));
            aregs[di.rx] += di.opsz;
            aregs[di.ry] += di.opsz;
            break;
        }
        case DBcc: {
            // TODO: check
            if (!is_cond((ConditionCode)d1)){
                --dregs[di.d2];
                pc += (sign_extend_to_l((unsigned short)dregs[di.d2]) != -1) ?
                        (sign_extend_to_l((unsigned short)di.dl)) : 0;
            }
            prefetch_q.clear();
            fill_prefetch_req();
            fill_prefetch_req();
            break;
        }

        case DIVS: {
            // TODO: check
            if (!operand_a_l)
                // TODO: Trap EV_ZeroDivide
                break;
            auto res1 = ((int)dregs[di.d1] / (short)operand_a_l);
            auto res2 = ((int)dregs[di.d1] % (short)operand_a_l);
            set_sreg(Carry,   0);
            set_sreg(Negative, res1 & sign_bit);
            set_sreg(Zero,      (res1 & mask) == 0);
            set_sreg(Overflow,  res1 > std::numeric_limits<short>::max() || res1 < std::numeric_limits<short>::min());
            dregs[di.d1] = get_sreg(Overflow) ? dregs[di.d1] : (res2 << 16) | res1;
            break;
        }

        case DIVU: {
            // TODO: check
            if (!operand_a_l)
                // TODO: Trap EV_ZeroDivide
                break;
            auto res1 = (dregs[di.d1] / (unsigned short)operand_a_l);
            auto res2 = (dregs[di.d1] % (unsigned short)operand_a_l);
            set_sreg(Carry,   0);
            set_sreg(Negative, res1 & sign_bit);
            set_sreg(Zero,      (res1 & mask) == 0);
            set_sreg(Overflow,  res1 > std::numeric_limits<unsigned short>::max());
            dregs[di.d1] = get_sreg(Overflow) ? dregs[di.d1] : (res2 << 16) | res1;
            break;
        }

        case EOR: {
            auto res = ((dregs[di.d1] & mask) ^ (operand_a_l & mask));
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            dst_value = res & mask;
            set_ea_dst();
            break;
        }

        case EORI: {
            auto res = ((dregs[di.d1] & mask) ^ (di.dl & mask));
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            dst_value = res & mask;
            set_ea_dst();
            break;
        }

        case EORItoCCR: {
            // TODO: check (5bit)
            unsigned char res = (di.d1 ^ (sreg & 0xff));
            sreg &= 0xff00;
            sreg |= res;
            break;
        }
        case EXG: {
            if (di.opmode == 0x8)      std::swap(dregs[di.rx], dregs[di.ry]);
            else if (di.opmode == 0x9) std::swap(aregs[di.rx], aregs[di.ry]);
            else std::swap(dregs[di.rx], aregs[di.ry]);
            break;
        }
        case EXT: {
            // TODO: check
            unsigned res{};
            if (di.opmode == 0x2){
                res = (sign_extend_to_l((unsigned char)dregs[di.d1]) & 0xffff);
            } else if (di.opmode == 0x3){
                res = sign_extend_to_l((unsigned short)dregs[di.d1]);
            } else {
                res = sign_extend_to_l((unsigned char)dregs[di.d1]);
            }
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            dregs[di.d1] = res;
            break;
        }

        case ILLEGAL: {
            // TODO: EV_IllegalInstruction
            break;
        }
        case JMP: {
            // TODO: check
            pc = aregs[(di.d1 & 0x7)];
            break;
        }
        case JSR: {
            usp-=4;
            fill_bus_write_req(usp, pc, LONG);
            pc = aregs[(di.d1 & 0x7)];
            break;
        }
        case LEA: {
            aregs[di.d1] = aregs[di.d2 & 0x7];
            break;
        }
        case LINK: {
            usp-=4;
            fill_bus_write_req(usp,  aregs[di.d1], LONG);
            aregs[di.d1] = usp;
            usp += sign_extend_to_l((unsigned short)di.dl);
            break;
        }
        case LSR: {
            auto n = (di.d2 ? (dregs[di.d3 & 0x7] & 0x3f) : (di.d3 & 0x7));
            if (n > opsz * 8) {
                set_sreg(Extend,   0);
                set_sreg(Negative, 0);
                set_sreg(Zero,      1);
                set_sreg(Carry,   0);
                set_sreg(Overflow,  0);
                dregs[di.d1] &= ~mask;
                break;
            }
            if (!di.d2 && !n) n = 8;
            auto res = (dregs[di.d1] & mask)  >> n;
            set_sreg(Extend,   n != 0 ? ((dregs[di.d1] & mask) >> (n - 1)) & 1 : get_sreg(Extend));
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Carry,   n != 0 ? ((dregs[di.d1] & mask) >> (n - 1)) & 1 : 0);
            set_sreg(Overflow,  0);
            dregs[di.d1] &= ~mask;
            dregs[di.d1] |= res;
            break;
        }
        case LSL: {
            auto n = (di.d2 ? (dregs[di.d3 & 0x7] & 0x3f) : (di.d3 & 0x7));
            if (n > opsz * 8) {
                set_sreg(Extend,   0);
                set_sreg(Negative, 0);
                set_sreg(Zero,      1);
                set_sreg(Carry,   0);
                set_sreg(Overflow,  0);
                dregs[di.d1] &= ~mask;
                break;
            }
            if (!di.d2 && !n) n = 8;
            auto res = (dregs[di.d1] & mask) << n;
            auto bound = (opsz * 8) - n;
            set_sreg(Extend,   n != 0 ? ((dregs[di.d1] & mask) >> bound) & 1 : get_sreg(Extend));
            set_sreg(Negative, res & sign_bit);
            set_sreg(Zero,      (res & mask) == 0);
            set_sreg(Carry,   n != 0 ? ((dregs[di.d1] & mask) >> bound) & 1 : 0);
            set_sreg(Overflow,  0);
            dregs[di.d1] &= ~mask;
            dregs[di.d1] |= res;
            break;
        }

        case MOVE: {
            set_sreg(Negative, operand_a_l & sign_bit);
            set_sreg(Zero,      (operand_a_l & mask) == 0);
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            ea_write(di.d1, operand_a_l);
            break;
        }

        case MOVEA: {
            aregs[di.d1] = di.opsz == WORD ? sign_extend_to_l((unsigned short)(operand_a_l & mask)) : operand_a_l;
            break;
        }

        case MOVEtoCCR: {
            sreg &= 0xff00;
            sreg |= (operand_a_l & 0xff);
            break;
        }

        case MOVEfromSR: {
            ea_write(di.d1, sreg);
            break;
        }

    }
}

M68K::DecodedInfo M68K::decode(unsigned short val) {
    if ((((val >> 12) & 0xf) == 0b1100) && (((val >> 4) & 0x1f) == 0b10000)) {
        return {
            .i = ABCD,
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
            case 0b100: {sz = BYTE;break;}
            case 0b001:
            case 0b011:
            case 0b101: {sz = WORD;break;}
            default:    {sz = LONG;break;}
        }
        return {
            .i = (opmode == 0b011 || opmode == 0b111) ? ADDA : ADD,
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
            .i = ADDI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

     if ((val >> 12) == 0b0101 && !(val & 0x100)) {
        unsigned char data = (val >> 9) & 0x3;
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = ADDQ,
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
            .i = ADDX,
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
            .i = AND,
            .rx = rx,
            .opmode = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

     if ((val >> 8) == 0b0000'0010){
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = ANDI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xe && !(val & 0x18)){
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char i_r = (val >> 5) & 1;
        return {
            .i = (val & 0x100) ? ASL : ASR,
            .d1 = count_reg,
            .d2 = sz,
            .d3 = i_r,
        };
    }

    if (val == 0x23c) {
        return {.i = ANDItoCCR};
    }

    if (val >> 12 == 0x6) {
        unsigned char cond = (val >> 8) & 0xf;
        unsigned char d8 = val & 0xff;
        // TODO: check
        unsigned displacement = d8 ? d8 : read_prefetch_q();
        fill_prefetch_req();
        return {
            .i = BCC,
            .d1 = cond,
            .dl = displacement,
            .opsz = (val & 0xff) ? BYTE : WORD,
        };
    }

    if ((val >> 6) == 0x21){
        auto num = read_prefetch_q();
        fill_prefetch_req();
        return {
            .i = BCHG,
            .d1 = (unsigned char)(num & 0xff),
            .effective_address = (unsigned char)(val & 0x3f),
            .opmode = 1,
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x5) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i =  BCHG,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
            .opmode = 0,
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x6) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i =  BCLR,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
            .opmode = 0,
        };
    }

    if (val == 0x22){
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i =  BCLR,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
            .opmode = 1,
        };
    }

    if ((val >> 8) == 0b0110'0000) {
        unsigned displacement = !(val & 0xff) ? (val >> 16) & 0xffff : val & 0xff;
        OperandSize opsz = !(val & 0xff) ? WORD : BYTE;
        return {
            .i = BRA,
            .opsz = opsz,
            .dl = displacement,
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = BSET,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0b0110'0001) {
        unsigned displacement = 0;
        OperandSize opsz;
        if (val & 0xff){
            displacement = val & 0xff;
            opsz = BYTE;
        } else {
            displacement = read_prefetch_q();
            fill_prefetch_req();
            opsz = WORD;
        }
        return {
            .i = BSR,
            .dl = displacement,
            .opsz = opsz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x4) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = BTST,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x4 && !(val & 0x40)) {
        unsigned char sz = (val >> 7) & 0x3;
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = CHK,
            .d1 = reg,
            .d2 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


      if ((val >> 8) == 0x42) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = CLR,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


    if ((val >> 12) == 0xb) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        return {
            .i = (opmode == 0b11 || opmode == 0b111) ? CMPA : CMP,
            .d1 = reg,
            .d2 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0xc) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = CMPI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }


    if ((val >> 12) == 0xb && ((val >> 8) & 0x1) && ((val >> 3) & 0x7) == 0x1) {
        unsigned char rx = (val >> 9) & 0x7;
        unsigned char ry = (val & 0x7);
        unsigned char sz = (val >> 6) & 0x3;
        OperandSize opsz;
        if (sz == 0x00) opsz = BYTE;
        else if (sz == 0x1) opsz = WORD;
        else opsz = LONG;
        return {
            .i = CMPM,
            .rx = rx,
            .ry = ry,
            .opsz = opsz,
        };
    }

    if ((val >> 12) == 0xa && ((val >> 3) & 0x1f) == 0x19) {
        unsigned char cond = (val >> 8) & 0xf;
        unsigned char reg = (val & 0x7);
        unsigned displacement = read_prefetch_q();
        fill_prefetch_req();
        return {
            .i = DBcc,
            .d1 = cond,
            .d2 = reg,
            .dl = displacement,
        };
    }


    if ((val >> 12) == 0x8 && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = ((val >> 9) & 0x7);
        return {
            .i = DIVS,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    // TODO: check
//    if ((val >> 6) == 0x131) {
//        return {
//            .i = DIVSL,
//            .effective_address = (unsigned char)(val & 0x3f),
//        };
//    }

    if ((val >> 12) == 0x8 && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = DIVU,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    // TODO: check
//    if ((val >> 6) == 0x131) {
//        unsigned char reg = (val >> 9) & 0x7;
//        return {
//            .i = DIVUL,
//            .d1 = reg,
//            .effective_address = (unsigned char)(val & 0x3f),
//        };
//    }

    if ((val >> 12) == 0xb) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
           return {
            .i = EOR,
            .d1 = reg,
            .opmode = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0xa) {
        // TODO: check
        unsigned char sz = (val >> 6) & 0x3;
        unsigned displacement = read_prefetch_q();
        fill_prefetch_req();
        OperandSize opsz;
        if (sz == 0x0){
            displacement &= 0xff;
            opsz = BYTE;
        } else if (sz == 0x1){
            opsz = WORD;
        } else {
            auto displacement_h = read_prefetch_q();
            fill_prefetch_req();
            displacement |= (displacement_h << 16);
            opsz = LONG;
        }
        return {
            .i = EORI,
            .d1 = sz,
            .opsz = opsz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (val == 0xa3c) {
        unsigned displacement = read_prefetch_q();
        fill_prefetch_req();
        return {
            .i = EORItoCCR,
            .d1 = (unsigned char)(displacement & 0xff),
            .opsz = BYTE,
        };
    }

    if ((val >> 12) == 0xc && (val & 0x100)) {
        unsigned char rx = (val >> 9) & 0x7;
        unsigned char ry = (val & 0x7);
        unsigned char opmode = (val >> 3) & 0x1f;
        return {
            .i = EXG,
            .rx = rx,
            .ry = ry,
            .opmode = opmode,
        };
    }

    if ((val >> 9) == 0x24 && !((val >> 3) & 0x7)) {
        unsigned char reg = (val & 0x7);
        unsigned char opmode = (val >> 3) & 0x1f;
        return {
            .i = EXT,
            .d1 = reg,
            .opmode = opmode,
        };
    }

    if (val == 0x4afc) {
        return {
            .i = ILLEGAL,
        };
    }

    if (val == 0x13b) {
        return {
            .i = JMP,
            .d1 = (unsigned char)(val & 0x3f), // EA
        };
    }

    if (val == 0x13a) {
        return {
            .i = JSR,
            .d1 = (unsigned char)(val & 0x3f), // EA
        };
    }
    
    if ((val >> 12) == 0x4 && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = LEA,
            .d1 = reg,
            .d2 = (unsigned char)(val & 0x3f), // EA
        };
    }

    if ((val >> 3) == 0x9ca) {
        unsigned char reg = (val) & 0x7;
        auto displacement = read_prefetch_q();
        fill_prefetch_req();
        return {
            .i = LINK,
            .d1 = reg,
            .dl = displacement,
        };
    }

    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x1) {
        unsigned char reg = (val) & 0x7;
        unsigned char i_r = (val >> 5) & 0x1;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char count_reg = (val >> 9) & 0x7;
        OperandSize opsz;
        if (sz == 0x0){
            opsz = BYTE;
        } else if (sz == 0x1){
            opsz = WORD;
        } else {
            opsz = LONG;
        }

        return {
            .i = (val & 0x100) ? LSL : LSR,
            .d1 = reg,
            .d2 = i_r,
            .d3 = count_reg,
            .opsz = opsz,
        };
    }

    if (!(val >> 14)) {
        unsigned char sz = (val >> 12) & 0x3;
        OperandSize opsz{};
        if (sz == 0x0){
            opsz = BYTE;
        } else if (sz == 0x3) {
            opsz = WORD;
        } else {
            opsz = LONG;
        }
        return {
            .i = MOVE,
            .d1 = (unsigned char)((val >> 6) & 0x3f),
            .effective_address = (unsigned char)(val & 0x3f),
            .opsz = opsz,
        };
    }

    if (!(val >> 14) && ((val >> 6) & 0x7) == 0x1) {
        unsigned char sz = (val >> 12) & 0x3;
        unsigned char dst = (val >> 9) & 0x7;
        OperandSize opsz{};
        if (sz == 0x3){
            opsz = WORD;
        } else {
            opsz = LONG;
        }
        return {
            .i = MOVEA,
            .d1 = dst,
            .effective_address = (unsigned char)(val & 0x3f),
            .opsz = opsz,
        };
    }


    if ((val >> 6) == 0x113) {
        return {
            .i = MOVEtoCCR,
            .effective_address = (unsigned char)(val & 0x3f),
            .opsz = WORD,
        };
    }

    if ((val >> 6) == 0x103) {
        return {
            .i = MOVEfromSR,
            .d1 = (unsigned char)(val & 0x3f),
            .opsz = WORD,
        };
    }



    if ((val >> 11) == 0x9 && ((val >> 7) & 0x7) == 0x1) {
        unsigned char sz = (val >> 6) & 0x1;
        unsigned char dr = (val >> 10) & 0x1;
        return {
            .i = MOVEM,
            .d1 = sz,
            .d2 = dr,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 11) == 0x9 && ((val >> 7) & 0x7) == 0x1) {
        unsigned char sz = (val >> 6) & 0x1;
        unsigned char dr = (val >> 10) & 0x1;
        return {
            .i = MOVEM,
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
            .i = MOVEP,
            .d1 = data_reg,
            .d2 = opmode,
            .d3 = address_reg,
        };
    }

    if ((val >> 12) == 0x7 && !(val & 0x100)) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char data = val & 0xff;
        return {
            .i = MOVEP,
            .d1 = reg,
            .d2 = data,
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MULS,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MULU,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MULU,
            .d1 = reg,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 6) == 0x120) {
        return {
            .i = NBCD,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x44) {
        return {
            .i = NEG,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x44) {
        return {
            .i = NEG,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x40) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = NEGX,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (val == 0x4e71) {
        return {
            .i = NOP,
        };
    }

    if ((val >> 8) == 0x46) {
        return {
            .i = NOT,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }
    
    if ((val >> 12) == 0x8) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        return {
            .i = OR,
            .d1 = reg,
            .d2 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 8)) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = ORI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (!(val >> 8)) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = ORItoCCR,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if (val == 0x3c) {
        return {
            .i = ORItoCCR,
        };
    }

    if ((val >> 6) == 0x121) {
        return {
            .i = PEA,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 6) == 0x121) {
        return {
            .i = PEA,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x3) {
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char i_r = (val >> 5) & 0x1;
        return {
            .i = (val & 0x100) ?  ROR : ROL,
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
            .i = (val & 0x100) ?  ROR : ROL,
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
            .i = (val & 0x100) ?  ROXR : ROXL,
            .d1 = count_reg,
            .d2 = sz,
            .d3 = i_r,
        };
    }

    if ((val >> 4) == 0x6c) {
        unsigned char da = (val >> 3) & 0x1;
        unsigned char reg = val & 0x7;
        return {
            .i = RTM,
            .d1 = da,
            .d2 = reg,
        };
    }

    if (val == 0x4e77) {
        return {
            .i = RTR,
        };
    }

    if (val == 0x4e75) {
        return {
            .i = RTS,
        };
    }

    if ((val >> 12) == 0x8 && ((val >> 4) & 0x1f) == 0x10) {
        unsigned char rx = val & 0x7;
        unsigned char ry = (val >> 9) & 0x7;
        unsigned char rm = (val >> 3) & 0x1;
        return {
            .i = SBCD,
            .rx = rx,
            .ry = ry,
            .d1 = rm,
        };
    }

    if ((val >> 12) == 0x5 && ((val >> 6) & 0x3) == 0x3) {
        unsigned char cond = (val >> 8) & 0xf;
        return {
            .i = Scc,
            .d1 = cond,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x9) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        return {
            .i = (opmode == 0b11 || opmode == 0b111) ?  SUBA : SUB,
            .d1 = reg,
            .d2 = opmode,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 8) == 0x4) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = SUBI,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 12) == 0x5 && ((val >> 8) & 0x1)) {
        unsigned char data = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = SUBQ,
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
            .i = SUBX,
            .rx = rx,
            .ry = ry,
            .d1 = sz,
            .d2 = rm,
        };
    }

    if ((val >> 3) == 0x908) {
        unsigned char reg = val & 0x7;
        return {
            .i = SWAP,
            .d1 = reg,
        };
    }

    if ((val >> 6) == 0x12b) {
        unsigned char reg = val & 0x7;
        return {
            .i = TAS,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 4) == 0x4e4) {
        unsigned char vec = val & 0xf;
        return {
            .i = TRAP,
            .d1 = vec,
        };
    }

    if (val == 0x4e76) {
        return {
            .i = TRAPV,
        };
    }


    if ((val >> 8) == 0x4a) {
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = TST,
            .d1 = sz,
            .effective_address = (unsigned char)(val & 0x3f),
        };
    }

    if ((val >> 3) == 0x9cb) {
        unsigned char reg = val & 0x7;
        return {
            .i = UNLK,
            .d1 = reg,
        };
    }

    return {};
}
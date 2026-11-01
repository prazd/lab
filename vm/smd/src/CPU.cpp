#include "CPU.hpp"
#include "BUS.hpp"
#include <algorithm>
#if DEBUG_MODE
#include <cassert>
#endif

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
        case DSTDn:  dregs[dst_reg_n] &= ~di.mask; dregs[dst_reg_n] |= dst_value & di.mask; break;
        case DSTAn:  aregs[dst_reg_n] &= ~di.mask; aregs[dst_reg_n] |= dst_value & di.mask; break;
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
        case ProgramCounterIndirectDisplacementMode:
            case AddressRegisterIndirectDisplacementMode: {
            return  sz == LONG ? 12 : 8;
        }
        case ProgramCounterIndirectIndexMode:
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

void M68K::fill_bus_read_req(unsigned addr, BusReadDest brd, OperandSize sz, unsigned char n) {
    auto even_addr = addr & 0xfffffffe;
    switch (sz) {
        case BYTE: {
            iops_q.push_back({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = ((addr & 1) ? BUS::LDS : BUS::UDS)});
            iops_q.push_back({.iop = BusReadWait, .brd = brd, .n = n});
            break;
        }
        case WORD: {
            iops_q.push_back({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = BUS::UDS_LDS});
            iops_q.push_back({.iop = BusReadWait, .brd = brd, .n = n});
            break;
        }
        case LONG: {
            iops_q.push_back({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = BUS::UDS_LDS});
            iops_q.push_back({.iop = BusReadWait, .brd = brd, .n = n});
            iops_q.push_back({.address = even_addr + 2, .iop = BusReadReq, .brd = BusReadDest((int)brd + 1), .bus_mode = BUS::UDS_LDS});
            iops_q.push_back({.iop = BusReadWait, .brd = brd, .n = n});
            break;
        }
    }
}

void M68K::fill_bus_read_req_front(unsigned addr, BusReadDest brd, OperandSize sz, unsigned char n) {
    auto even_addr = addr & 0xfffffffe;
    switch (sz) {
        case BYTE: {
            iops_q.push_front({.iop = BusReadWait, .brd = brd, .n = n});
            iops_q.push_front({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = ((addr & 1) ? BUS::LDS : BUS::UDS)});
            break;
        }
        case WORD: {
            iops_q.push_front({.iop = BusReadWait, .brd = brd, .n = n});
            iops_q.push_front({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = BUS::UDS_LDS});
            break;
        }
        case LONG: {
            iops_q.push_front({.iop = BusReadWait, .brd = brd, .n = n});
            iops_q.push_front({.address = even_addr + 2, .iop = BusReadReq, .brd = BusReadDest((int)brd + 1), .bus_mode = BUS::UDS_LDS});
            iops_q.push_front({.iop = BusReadWait, .brd = brd, .n = n});
            iops_q.push_front({.address = even_addr, .iop = BusReadReq, .brd = brd, .bus_mode = BUS::UDS_LDS});
            break;
        }
    }
}

//https://github.com/larsbrinkhoff/M68K-microcode/blob/master/doc/Yacht.txt
unsigned char M68K::calc_exec_cycles(const DecodedInfo& di) {
    auto ea = to_address_mode(di.ea_src, true);
    switch (di.i) {
        //        #<data>,CCR     check
        case ADD:
        case SUB: {
            // TODO: check
            if (di.opmode && di.opsz == LONG)
                return (ea == DataRegisterDirectMode || ea == AddressRegisterDirectMode || ea == Immediate) ? 4 : 2;
            return 0;
        }
        case ADDQ:
        case SUBQ: {
            return di.opsz == LONG ? 4 : 0;
        }
        case SBCD:
        case ABCD: {
            return 2;
        }

        case AND:
        case OR:{
            if (di.opsz != LONG) return 0;
            return (ea == DataRegisterDirectMode || ea == Immediate) ? 4 : 2;
        }


        // TODO: check
        case EORItoSR:
        case EORItoCCR:
        case ANDItoSR:
        case ANDItoCCR:
        case ORItoSR:
        case ORItoCCR:
            return 4;


        case ORI:
        case ANDI:
        case SUBI:
        case ADDI: {
            return 4;
        }

        case MOVEtoSR:
        case MOVEtoCCR: return 0;

        case NBCD: return 2;
        case Scc : return (di.opmode ? (ea == DataRegisterDirectMode ? 2 : 0) : 0);
        case CMPI: return (di.opsz == LONG ? 4 : 0);

        case BCLR:
        case BCHG:
        case BSET: {
            if (di.opsz == BYTE) return di.opmode ? 4 : 0;
            return di.opmode ? (di.d1_b > 15 ? 4 : 2) : (dregs[di.d1_b] > 15 ? 4 : 2);
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
            // TODO: check
            if (di.opsz == LONG)
                return ea == DataRegisterDirectMode ? 2 : 4;
            return 0;
        }

        case ADDA:
        case SUBA:{
            if (di.opsz == LONG)
                return (ea == DataRegisterDirectMode || ea == AddressRegisterDirectMode || ea == Immediate) ?  4 : 2; // 8 - (np) / 6 - (np)
            return 4;
        }
        case ADDX:
        case SUBX: {
            return di.opmode ? (di.opsz == LONG ? 26 : 14) : (di.opsz == LONG ? 4 : 0);
        }
        case CMP: {
            return di.opsz == LONG ? 2 : 0;
        }
        case CMPA: return 2;
        // TODO: check
        case CMPM:
        case EOR:
        case PEA:
        case MOVEM: return 0;


        case ASL:
        case ASR:
        case LSL:
        case LSR:
        case ROL:
        case ROR:
        case ROXL:
        case ROXR:{
            // TODO: finish
            return 0;
        }
    }
}

void M68K::ea_write(unsigned char ea_val, unsigned val) {
    // TODO: check
    auto mode = to_address_mode(ea_val, false);
    auto reg_n = (ea_val >> 3) & 0x7;
    switch(mode) {
        case DataRegisterDirectMode:    dregs[reg_n] &= ~di.mask; dregs[reg_n] |= (val & di.mask); break;
        case AddressRegisterDirectMode: aregs[reg_n] &= ~di.mask; aregs[reg_n] |= (val & di.mask); break;
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
            iops_q.push_back({.iop = INOP});
            iops_q.push_back({.iop = INOP});
            fill_bus_write_req(aregs[ea_reg_n], val, opsz);
            break;
        }
        case AddressRegisterIndirectDisplacementMode: {
            auto pv = read_prefetch_q();
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

            iops_q.push_back({.iop = INOP});
            iops_q.push_back({.iop = INOP}); // calc time
            fill_bus_write_req(addr, val, opsz);
            break;
        }
        case ProgramCounterIndirectDisplacementMode: {
            auto pv = read_prefetch_q();
            auto addr = pc + sign_extend_to_l(pv);
            fill_bus_write_req(addr, val, opsz);
            break;
        }
        case ProgramCounterIndirectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            auto addr = pc +
                         sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;
            iops_q.push_back({.iop = INOP});
            iops_q.push_back({.iop = INOP}); // calc time
            fill_bus_write_req(addr, val, opsz);
            break;
        }
    }
}

unsigned M68K::to_ea_address(unsigned char ea_val) {
    // TODO: check
    auto mode = to_address_mode(ea_val, false);
    auto reg_n = (ea_val >> 3) & 0x7;
    switch(mode) {
        case AddressRegisterIndirectMode: {
            return aregs[reg_n];
        }
        case AddressRegisterIndirectPostIncMode: {
            return std::exchange(aregs[ea_reg_n],aregs[ea_reg_n] + opsz);
        }
        case AddressRegisterIndirectPreDecMode: {
            aregs[ea_reg_n] -= opsz;
            return aregs[ea_reg_n];
        }
        case AddressRegisterIndirectDisplacementMode: {
            auto pv = read_prefetch_q();
            return aregs[ea_reg_n] + sign_extend_to_l(pv);
        }
        case AddressRegisterIndirectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            auto addr = aregs[ea_reg_n] +
                        sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;
            return addr;
        }
        case ProgramCounterIndirectDisplacementMode: {
            auto pv = read_prefetch_q();
            auto addr = pc + sign_extend_to_l(pv);
            return addr;
        }
        case ProgramCounterIndirectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            auto addr = pc +
                        sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;
            return addr;
        }
    }
    return 0;
}

void M68K::ea_address_calc(unsigned char val){
    ea_mode = to_address_mode(val, true);
    ea_reg_n = val & 0x7;
    switch(ea_mode) {
        case AddressRegisterIndirectMode: {
            ea_address = aregs[ea_reg_n];
            break;
        }
        case AddressRegisterIndirectPostIncMode: {
            ea_address = aregs[ea_reg_n];
            aregs[ea_reg_n] += opsz;
            break;
        }
        case AddressRegisterIndirectPreDecMode: {
            aregs[ea_reg_n] -= opsz;
            // TODO: check
            iops_q.push_back({.iop = INOP});
            iops_q.push_back({.iop = INOP});
            ea_address = aregs[ea_reg_n];
            break;
        }
        case AddressRegisterIndirectDisplacementMode: {
            auto pv = read_prefetch_q();
            ea_address = aregs[ea_reg_n] + sign_extend_to_l(pv);
            break;
        }
        case AddressRegisterIndirectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            ea_address = aregs[ea_reg_n] +
                         sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;

            // n np nr
            iops_q.push_back({.iop = INOP});
            iops_q.push_back({.iop = INOP}); // calc time
            break;
        }
        case ProgramCounterIndirectDisplacementMode: {
            // TODO: check
            auto pv = read_n_fill_prefetch_q();
            ea_address = pc + sign_extend_to_l(pv);
            break;
        }
        case ProgramCounterIndirectIndexMode: {
            auto pv = read_prefetch_q();
            auto regn = (pv >> 12) & 0x7;
            auto xn = pv & 0x8000 ? aregs[regn] : dregs[regn];
            xn = pv & 0x800 ? xn : sign_extend_to_l((unsigned short)(xn & 0xffff));
            ea_address = pc +
                         sign_extend_to_l((unsigned char)(pv & 0xff)) + xn;

            iops_q.push_back({.iop = INOP});
            iops_q.push_back({.iop = INOP}); // calc time
            break;
        }
        case AbsoluteShort: {
            ea_address = read_prefetch_q();
            break;
        }
        case AbsoluteLong: {
            ea_address = read_prefetch_q(); // high
            ea_address = (ea_address << 16) | read_prefetch_q(); // low
            break;
        }
    }
}

void M68K::ea_fill(unsigned char val){
    ea_mode = to_address_mode(val, true);
    ea_reg_n = val & 0x7;
    switch(ea_mode) {
        case DataRegisterDirectMode:    ea_reg_type = Dn; operand_a_l = dregs[ea_reg_n]; break;
        case AddressRegisterDirectMode: ea_reg_type = An; operand_a_l = aregs[ea_reg_n]; break;
        case Immediate: {
            operand_a_w1 = read_prefetch_q();
            fill_prefetch_req();
            if (opsz != LONG) break;
            operand_a_w2 = read_prefetch_q();
            operands_a_to_long();
            break;
        }
        default: {
            ea_address_calc(val);
            fill_prefetch_req();
            fill_bus_read_req(ea_address, BRDOperandA1, opsz);
            break;
        }
    }
}

void M68K::tick(BUS& bus){
    if (iops_q.empty()){
#if DEBUG_MODE
    assert(prefetch_q.size() == 2);
#endif
        auto opcode = read_prefetch_q();
        di = decode(opcode);
        opsz = di.opsz;
        update_mask_n_sign_bit();
        if (di.read_ea) {
            auto am = to_address_mode(di.ea_src, true);
            if (am == AbsoluteLong || (am == Immediate && di.opsz == LONG)) {
                fill_prefetch_req();
                iop_push_nops(3); // TODO: check
            }
            iops_q.push_back({.iop = ReadEA});
        } else if (di.calc_ea){
            ea_address_calc(di.ea_src);
        }
        // TODO: finish
        if (di.read_displacement) read_displacement();
        if (di.read_displacement_by_value) read_displacement_by_value();
        if (di.read_prefetch) di.prefetched = read_prefetch_q(); // TODO: check
        if (di.read_predec_args)  fill_predec_args();
        if (di.read_postinc_args) fill_post_inc_args();

        if (exec_cycles = calc_exec_cycles(di); exec_cycles){
            iop_push_nops(exec_cycles);
            iops_q.push_back({.iop = Execute});
        }
    }

    auto iopv = iops_q.front();
    switch(iopv.iop){
        case ReadEA: {
            // TODO: check
            ea_fill(di.ea_src);
            if (!exec_cycles) exec(di, bus);
            break;
        }
        case BusReadReq: {
            // TODO: check bus_mode
            bus.m68k_r_req(iopv.address, iopv.bus_mode);
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
                    case BRDDataRegisterL: {
                        switch (iopv.opsz) {
                            case BYTE: dregs[iopv.n] = sign_extend_to_l((unsigned char)(bus.m68k_get() & 0xff)); break;
                            case WORD: dregs[iopv.n] = sign_extend_to_l(bus.m68k_get()); break;
                            case LONG: dregs[iopv.n] &= 0xffff0000; dregs[iopv.n] |= bus.m68k_get(); break;
                        }
                        break;
                    }
                    case BRDDataRegisterH: {
                        dregs[iopv.n] &= 0xffff; dregs[iopv.n] |= (bus.m68k_get() << 16);
                        break;
                    }
                    case BRDAddressRegisterL: {
                        switch (iopv.opsz) {
                            case BYTE: aregs[iopv.n] = sign_extend_to_l((unsigned char)(bus.m68k_get() & 0xff)); break;
                            case WORD: aregs[iopv.n] = sign_extend_to_l(bus.m68k_get()); break;
                            case LONG: aregs[iopv.n] &= 0xffff0000; aregs[iopv.n] |= (bus.m68k_get() & 0xff); break;
                        }
                        break;
                    }
                    case BRDAddressRegisterH: {
                        aregs[iopv.n] &= 0xffff; aregs[iopv.n] |= (bus.m68k_get() << 16);
                        break;
                    }
                    case BRDDataRegisterByteL: {
                        aregs[iopv.n] &= 0xff; aregs[iopv.n] |= ((bus.m68k_get() >> 8) & 0xff); // TODO: check
                        break;
                    }
                    case BRDDataRegisterByteH: {
                        aregs[iopv.n] &= 0xff00; aregs[iopv.n] |= ((bus.m68k_get() >> 8) & 0xff); // TODO: check
                        break;
                    }
                    case BRD_PC_H:{
                        pc &= 0xffff;
                        pc |= (bus.m68k_get() << 16);
                        break;
                    }
                    case BRD_PC_L:{
                        pc &= 0xffff0000;
                        pc |= bus.m68k_get();
                        break;
                    }

                    case BRD_SSP_H:{
                        ssp &= 0xffff;
                        ssp |= (bus.m68k_get() << 16);
                        break;
                    }

                    case BRD_SSP_L:{
                        ssp &= 0xffff0000;
                        ssp |= bus.m68k_get();
                        break;
                    }

                    case BRD_SREG:{
                        sreg = bus.m68k_get();
                        break;
                    }
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
        case INOP: {iops_q.pop_front(); break;}
        case Execute:{exec(di, bus); break;}
    }

}

// read opcode (prefetch queue)
// read dst src
// prefetch if need
//


M68K::AddressMode M68K::to_address_mode(unsigned char val, bool mr){
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
    if (mode_field == 0b111 && reg_field == 0b010) return ProgramCounterIndirectDisplacementMode;
    if (mode_field == 0b111 && reg_field == 0b011) return ProgramCounterIndirectIndexMode;
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

void M68K::exec(const DecodedInfo& di, BUS& bus) {
    // TODO: finish
    switch(di.i){
        case ABCD: {
            unsigned char a = di.opmode ? (operand_a_l & 0xff) : dregs[di.r_dst] & 0xff;
            unsigned char b = di.opmode ? (operand_b_l & 0xff) : dregs[di.r_src] & 0xff;

            unsigned res = to_bcd(a) + to_bcd(b) + get_sreg(Extend);
            set_sreg(Carry,  res >= 100);
            set_sreg(Extend,  res >= 100);
            set_sreg(Zero,      res != 0 ? 0 : get_sreg(Zero));
            if (di.opmode)
                fill_bus_write_req(aregs[di.r_dst], from_bcd(res % 100) & 0xff, BYTE);
            else
                dregs[di.r_dst] = from_bcd(res % 100) & 0xff;
            break;
        }
        case ADD:{
            operands_a_to_long();
            unsigned res = (dregs[di.r_dst] & di.mask) + (operand_a_l & di.mask);
            // TODO: move to function
            set_sreg(Extend, res > di.mask);
            set_sreg(Carry,  res > di.mask);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            // TODO: check
            if (di.opmode)
                set_sreg(Overflow, ((dregs[di.r_dst] & di.sign_bit) == (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (dregs[di.r_dst] & di.sign_bit)));
            else
                set_sreg(Overflow, ((dregs[di.r_dst] & di.sign_bit) == (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
            // TODO: check
            dst_value = res & di.mask;
            if (di.opmode) {
                dst = DSTDn;
                dst_reg_n = di.r_dst;
            } else {
                set_ea_dst();
            }
            break;
        }
        case ADDA: {
            operands_a_to_long();
            auto res = ((aregs[di.r_dst] & di.mask) + (operand_a_l & di.mask)) & di.mask;
            aregs[di.r_dst] &= ~di.mask;
            aregs[di.r_dst] |= res;
            break;
        }
        case ADDI: {
            auto res = (operand_a_l & di.mask) + (operand_b_l & di.mask);
            set_sreg(Extend, res > di.mask);
            set_sreg(Carry,  res > di.mask);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Overflow, ((operand_a_l & di.sign_bit) == (operand_b_l & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
            dst_value = res & di.mask;
            fill_prefetch_req();
            set_ea_dst();
            break;
        }
        case ADDQ: {
            auto res = (operand_a_l & di.mask) + (di.d1_b);
            if (ea_mode != AddressRegisterDirectMode){
                set_sreg(Extend, res > di.mask);
                set_sreg(Carry,  res > di.mask);
                set_sreg(Negative, res & di.sign_bit);
                set_sreg(Zero,      (res & di.mask) == 0);
                set_sreg(Overflow, ((operand_a_l & di.sign_bit) == (di.d1_b & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
                break;
            }
            dst_value = res & di.mask;
            set_ea_dst();
            break;
        }
        case ADDX: {
            unsigned a = di.d1_b ? (operand_a_l & di.mask) : dregs[di.r_dst] & di.mask;
            unsigned b = di.d1_b ? (operand_b_l & di.mask) : dregs[di.r_src] & di.mask;
            unsigned res = a + b + get_sreg(Extend);
            set_sreg(Overflow, ((a & di.sign_bit) == ((b + get_sreg(Extend) & di.mask) & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
            set_sreg(Extend, res > di.mask);
            set_sreg(Carry,  res > di.mask);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      res ? 0 : get_sreg(Zero));
            if (di.d1_b){
                dst_value = res & di.mask;
                set_ea_dst();
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= (res & di.mask);
            }
            break;
        }
        case AND: {
            unsigned res = ((dregs[di.r_dst]) & (operand_a_l)) & di.mask;
            set_sreg(Carry,  0);
            set_sreg(Overflow, 0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dst_value = res & di.mask;
            if (di.opmode) {
                // TODO: check
                dst = DSTDn;
                dst_reg_n = di.r_dst;
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

        case ANDI:{
            auto res = ((operand_a_l) & (di.d_l)) & di.mask;
            set_sreg(Carry,  0);
            set_sreg(Overflow, 0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dst_value = res;

            fill_prefetch_req();
            set_ea_dst();
            break;
        }


        case ASR: {
            // TODO: check
            if (di.read_ea){
                unsigned long res = operand_a_l & di.mask;
                bool last_v = 0;
                bool is_signed = operand_a_l & di.sign_bit;
                last_v = res & 1;
                res >>= 1;
                res |= (is_signed << ((CHAR_BIT * di.opsz)-1));
                set_sreg(Extend,  last_v);
                set_sreg(Carry,   last_v);
                set_sreg(Negative, res & di.sign_bit);
                set_sreg(Zero,      (res & di.mask) == 0);
                dst_value = res & di.mask;
                set_ea_dst();
                break;
            }
            // TODO: check
            auto shift = di.opmode ? dregs[di.r_src] & 0x3f : (!di.r_src ? 8 : di.r_src);
            unsigned long res = dregs[di.r_dst] & di.mask;
            bool last_v = 0;
            bool is_signed = dregs[di.r_dst] & di.sign_bit;
            for (size_t i = 0; i < shift; ++i){
                last_v = res & 1;
                res >>= 1;
                res |= (is_signed << ((CHAR_BIT * di.opsz)-1));
            }
            set_sreg(Extend,  shift ? last_v : get_sreg(Extend));
            set_sreg(Carry,   shift ? last_v : 0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dregs[di.r_dst] &= ~di.mask;
            dregs[di.r_dst] |= res & di.mask;
            break;
        }

        case ASL:{
            // TODO: check
            if (di.read_ea){
                unsigned long res = operand_a_l & di.mask;
                bool is_signed = operand_a_l & di.sign_bit;
                bool last_v{};
                bool v = false;
                last_v = res & di.sign_bit;
                res <<= 1;
                if (((bool)(res & di.sign_bit)) != is_signed) v = true;
                set_sreg(Extend,   last_v);
                set_sreg(Carry,   last_v);
                set_sreg(Negative, res & di.sign_bit);
                set_sreg(Overflow, v);
                set_sreg(Zero,      (res & di.mask) == 0);
                dst_value = res & di.mask;
                set_ea_dst();
                break;
            }
            // TODO: check
            auto shift = di.opmode ? dregs[di.r_src] & 0x3f : (!di.r_src ? 8 : di.r_src);
            bool v = false;
            unsigned long res = dregs[di.r_dst] & di.mask;
            bool is_signed = dregs[di.r_dst] & di.sign_bit;
            bool last_v{};
            for (size_t i = 0; i < shift; ++i){
                last_v = res & di.sign_bit;
                res <<= 1;
                if (((bool)(res & di.sign_bit)) != is_signed) v = true;
            }
            set_sreg(Extend,  shift ? last_v : get_sreg(Extend));
            set_sreg(Carry,   shift ? last_v : 0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Overflow, v);
            set_sreg(Zero,      (res & di.mask) == 0);
            dregs[di.r_dst] &= ~di.mask;
            dregs[di.r_dst] |= res & di.mask;
            break;
        }

        case Bcc: {
            if (is_cond((ConditionCode)di.d1_b)){
                if (di.opsz == BYTE) pc += sign_extend_to_l((unsigned char)(di.d_l));
                else pc += sign_extend_to_l((unsigned short)(di.d_l));
                prefetch_q.clear();
                // TODO: check
                fill_prefetch_req();
                fill_prefetch_req();
            }
            break;
        }

        case BCHG: {
            auto n = di.opmode ? dregs[di.r_src] & 0x1f : (di.d_l & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            dst_value = operand_a_l ^ (1 << n);
            // TODO: check
            fill_prefetch_req();
            set_ea_dst();
            break;
        }

        case BCLR: {
            // TODO: check
            auto n = di.opmode ? dregs[di.r_src] & 0x1f : (di.d_l & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            dst_value = operand_a_l & (~(1 << n));
            set_ea_dst();
            break;
        }
        case BRA: {
            if (di.opsz == BYTE) pc += sign_extend_to_l((unsigned char)(di.d_l));
            else pc += sign_extend_to_l((unsigned short)(di.d_l));
            break;
        }
        case BSET: {
            auto n = di.opmode ? dregs[di.r_src] & 0x1f : (di.d_l & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            dst_value = operand_a_l | ((1 << n));

            fill_prefetch_req();
            set_ea_dst();
            break;
        }
        case BSR: {
            SP-=4;
            // TODO: PC to next instruction
            fill_bus_write_req(SP, pc, LONG);
            if (di.opsz == BYTE) pc += sign_extend_to_l((unsigned char)(di.d_l));
            else pc += sign_extend_to_l((unsigned short)(di.d_l));
            break;
        }
        case BTST: {
            auto n = di.opmode ? dregs[di.r_src] & 0x1f : (di.d1_b & 0x7);
            set_sreg(Zero,  !(operand_a_l & (1 << n)));
            break;
        }
        case CHK: {
            auto val = sign_extend_to_l((unsigned short)(dregs[di.r_src] & 0xffff));
            // TODO: check - https://github.com/larsbrinkhoff/m68k-microcode/blob/master/doc/Yacht.txt#L816
            if (val < 0 || val > operand_a_l)
                jump(EV_CheckInstruction);
            break;
        }
        case CLR: {
            // TODO: check
            dst_value = 0;
            fill_prefetch_req();
            set_ea_dst();
            break;
        }
        case CMP: {
            auto res = (dregs[di.r_src] & di.mask) - (operand_a_l & di.mask);
            set_sreg(Carry,  (operand_a_l & di.mask) > (dregs[di.r_src] & di.mask));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Overflow, ((dregs[di.r_src] & di.sign_bit) != (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (dregs[di.r_src] & di.sign_bit)));
            break;
        }

        case CMPA: {
            auto res = di.opsz ? ((aregs[di.r_src] & di.mask) - (operand_a_l & di.mask)) :
                       sign_extend_to_l((unsigned short)(aregs[di.r_src] & di.mask)) - sign_extend_to_l((unsigned short)(operand_a_l & di.mask));
            set_sreg(Carry,  (operand_a_l & di.mask) > (aregs[di.r_src] & di.mask));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Overflow, ((aregs[di.r_src] & di.sign_bit) != (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (aregs[di.r_src] & di.sign_bit)));
            break;
        }
        case CMPI: {
            auto res = (operand_a_l & di.mask) - (operand_b_l & di.mask);
            set_sreg(Carry,  (operand_b_l & di.mask) > (operand_a_l & di.mask));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Overflow, ((operand_b_l & di.sign_bit) != (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
            fill_prefetch_req();
            break;
        }

        case CMPM: {
            auto a = (operand_a_l & di.mask);
            auto b = (operand_b_l & di.mask);
            auto res = a - b;
            set_sreg(Carry,   b > a);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Overflow, ((a & di.sign_bit) != (b & di.sign_bit)) && ((res & di.sign_bit) != (b & di.sign_bit)));
            dregs[di.r_dst] = res & di.mask;
            break;
        }
        case DBcc: {
            // TODO: check
            if (!is_cond((ConditionCode)di.d1_b)){
                --dregs[di.r_src];
                pc += (sign_extend_to_l((unsigned short)dregs[di.r_src]) != -1) ?
                      (sign_extend_to_l((unsigned short)di.d_l)) : 0;
            }
            prefetch_q.clear();
            fill_prefetch_req();
            fill_prefetch_req();
            break;
        }

        case DIVS: {
            if (!operand_a_l){
                jump(EV_ZeroDivide);
                break;
            }
            auto res1 = ((int)dregs[di.r_dst] / (short)operand_a_l);
            auto res2 = ((int)dregs[di.r_dst] % (short)operand_a_l);
            set_sreg(Carry,   0);
            set_sreg(Negative, res1 & di.sign_bit);
            set_sreg(Zero,      (res1 & di.mask) == 0);
            set_sreg(Overflow,  res1 > std::numeric_limits<short>::max() || res1 < std::numeric_limits<short>::min());
            dregs[di.r_dst] = get_sreg(Overflow) ? dregs[di.r_dst] : (res2 << 16) | res1;
            break;
        }

        case DIVU: {
            if (!operand_a_l){
                jump(EV_ZeroDivide);
                break;
            }
            auto res1 = (dregs[di.r_dst] / (unsigned short)operand_a_l);
            auto res2 = (dregs[di.r_dst] % (unsigned short)operand_a_l);
            set_sreg(Carry,   0);
            set_sreg(Negative, res1 & di.sign_bit);
            set_sreg(Zero,      (res1 & di.mask) == 0);
            set_sreg(Overflow,  res1 > std::numeric_limits<unsigned short>::max());
            dregs[di.r_dst] = get_sreg(Overflow) ? dregs[di.r_dst] : (res2 << 16) | res1;
            break;
        }

        case EOR: {
            auto res = ((dregs[di.r_src] & di.mask) ^ (operand_a_l & di.mask));
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dst_value = res & di.mask;
            set_ea_dst();
            break;
        }

        case EORI: {
            auto res = ((dregs[di.d1_b] & di.mask) ^ (di.d_l & di.mask));
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dst_value = res & di.mask;
            fill_prefetch_req();
            set_ea_dst();
            break;
        }

        case EORItoCCR: {
            // TODO: check (5bit)
            unsigned char res = (di.d_l ^ (sreg & 0xff));
            sreg &= 0xff00;
            sreg |= res;
            break;
        }
        case EXG: {
            if (di.opmode == 0x8)      std::swap(dregs[di.r_dst], dregs[di.r_src]);
            else if (di.opmode == 0x9) std::swap(aregs[di.r_dst], aregs[di.r_src]);
            else std::swap(dregs[di.r_dst], aregs[di.r_src]);
            break;
        }
        case EXT: {
            // TODO: check
            unsigned res{};
            if (di.opmode == 0x2){
                res = (sign_extend_to_l((unsigned char)dregs[di.r_dst]) & 0xffff);
            } else if (di.opmode == 0x3){
                res = sign_extend_to_l((unsigned short)dregs[di.r_dst]);
            } else {
                res = sign_extend_to_l((unsigned char)dregs[di.r_dst]);
            }
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dregs[di.r_dst] = res;
            fill_prefetch_req();
            break;
        }

        case ILLEGAL: {
            jump(EV_IllegalInstruction);
            break;
        }
        case JMP: {
            jump(ea_address);
            break;
        }
        case JSR: {
            SP-=4;
            fill_bus_write_req(SP, pc, LONG);
            jump(ea_address);
            break;
        }
        case LEA: {
            aregs[di.r_dst] = aregs[di.d1_b & 0x7];
            fill_prefetch_req();
            break;
        }
        case LINK: {
            SP-=4;
            fill_bus_write_req(SP, aregs[di.r_dst], LONG);
            aregs[di.r_dst] = SP;
            SP += sign_extend_to_l((unsigned short)di.d_l);
            break;
        }
        // TODO: check
        case LSR: {
            auto n = (di.read_ea ? 1 : (di.opmode ? (dregs[di.r_src & 0x7] & 0x3f) : (di.r_src & 0x7)));
            unsigned arg = di.read_ea ? operand_a_l : dregs[di.r_dst];
            if (n > opsz * 8) {
                set_sreg(Extend,   0);
                set_sreg(Negative, 0);
                set_sreg(Zero,      1);
                set_sreg(Carry,   0);
                set_sreg(Overflow,  0);
                if (di.read_ea){
                    dst_value = 0;
                    set_ea_dst();
                } else {
                    dregs[di.r_dst] &= ~di.mask;
                }
                break;
            }
            if (!di.opmode && !n) n = 8;
            auto res = (arg & di.mask) >> n;
            set_sreg(Extend,   n != 0 ? ((arg & di.mask) >> (n - 1)) & 1 : get_sreg(Extend));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Carry,   n != 0 ? ((arg & di.mask) >> (n - 1)) & 1 : 0);
            set_sreg(Overflow,  0);
            if (di.read_ea){
                dst_value = res & di.mask;
                set_ea_dst();
            } else {
                dregs[di.d1_b] &= ~di.mask;
                dregs[di.d1_b] |= res;
            }
            break;
        }
        case LSL: {
            // TODO: check
            auto n = (di.read_ea ? 1 : (di.opmode ? (dregs[di.r_src & 0x7] & 0x3f) : (di.r_src & 0x7)));
            unsigned arg = di.read_ea ? operand_a_l : dregs[di.r_dst];
            if (n > opsz * 8) {
                set_sreg(Extend,   0);
                set_sreg(Negative, 0);
                set_sreg(Zero,      1);
                set_sreg(Carry,   0);
                set_sreg(Overflow,  0);
                if (di.read_ea){
                    dst_value = 0;
                    set_ea_dst();
                } else {
                    dregs[di.r_dst] &= ~di.mask;
                }
                break;
            }
            if (!di.d1_b && !n) n = 8;
            auto res = (arg & di.mask) << n;
            auto bound = (opsz * 8) - n;
            set_sreg(Extend,   n != 0 ? ((arg & di.mask) >> bound) & 1 : get_sreg(Extend));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            set_sreg(Carry,   n != 0 ? ((arg & di.mask) >> bound) & 1 : 0);
            set_sreg(Overflow,  0);
            if (di.read_ea){
                dst_value = res & di.mask;
                set_ea_dst();
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= res;
            }
            break;
        }

        case MOVE: {
            set_sreg(Negative, operand_a_l & di.sign_bit);
            set_sreg(Zero,      (operand_a_l & di.mask) == 0);
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            ea_write(di.d1_b, operand_a_l);
            break;
        }

        case MOVEA: {
            aregs[di.r_dst] = di.opsz == WORD ? sign_extend_to_l((unsigned short)(operand_a_l & di.mask)) : operand_a_l;
            fill_prefetch_req();
            break;
        }

        case MOVEtoCCR: {
            sreg &= 0xff00;
            sreg |= (operand_a_l & 0xff);
            reset_prefetch_q();
            break;
        }

        case MOVEfromSR: {
            fill_prefetch_req();
            ea_write(di.d1_b, sreg);
            break;
        }

        case MOVEM: {
            unsigned short r_mask = di.prefetched & 0xffff;
            auto m = to_address_mode(di.d1_b, true);
            auto addr = to_ea_address(di.d1_b); // TODO: check
            // TODO: maybe refactoring
            if (m != AddressRegisterIndirectPreDecMode){
                if (di.opmode){
                    for (int i = 0; i < 8; ++i){
                        if (r_mask & 1){
                            fill_bus_read_req(addr, BRDDataRegisterL,di.opsz, i);
                            addr += 2;
                            if (di.opsz == LONG){
                                fill_bus_read_req(addr, BRDDataRegisterH,di.opsz, i);
                                addr += 2;
                            }
                        }
                        r_mask >>= 1;
                    }

                    for (int i = 0; i < 8; ++i){
                        if (r_mask & 1){
                            fill_bus_read_req(addr, BRDAddressRegisterL,di.opsz, i);
                            addr += 2;
                            if (di.opsz == LONG){
                                fill_bus_read_req(addr, BRDAddressRegisterH,di.opsz, i);
                                addr += 2;
                            }
                        }
                        r_mask >>= 1;
                    }

                } else {
                    for (int i = 0; i < 8; ++i) {
                        if (r_mask & 1) {
                            fill_bus_write_req(addr,dregs[i],di.opsz);
                            addr += 2;
                        }
                        r_mask >>= 1;
                    }

                    for (int i = 0; i < 8; ++i) {
                        if (r_mask & 1) {
                            fill_bus_write_req(addr,aregs[i],di.opsz);
                            addr += 2;
                        }
                        r_mask >>= 1;
                    }
                }
            } else {
                if (di.opmode){
                    for (int i = 0; i < 8; ++i){
                        if (r_mask & 1){
                            addr -= di.opsz == LONG ? 4 : 2;
                            fill_bus_read_req(addr, BRDAddressRegisterL,di.opsz, 7-i);
                            if (di.opsz == LONG){
                                fill_bus_read_req(addr + 2, BRDAddressRegisterH,di.opsz, 7-i);
                            }
                        }
                        r_mask >>= 1;
                    }

                    for (int i = 0; i < 8; ++i){
                        if (r_mask & 1){
                            addr -= di.opsz == LONG ? 4 : 2;
                            fill_bus_read_req(addr, BRDDataRegisterL,di.opsz, 7-i);
                            if (di.opsz == LONG){
                                fill_bus_read_req(addr + 2, BRDDataRegisterH,di.opsz, 7-i);
                            }
                        }
                        r_mask >>= 1;
                    }

                } else {
                    for (int i = 0; i < 8; ++i) {
                        if (r_mask & 1) {
                            addr -= di.opsz == LONG ? 4 : 2;
                            fill_bus_write_req(addr,aregs[7-i],di.opsz);
                        }
                        r_mask >>= 1;
                    }

                    for (int i = 0; i < 8; ++i) {
                        if (r_mask & 1) {
                            addr -= di.opsz == LONG ? 4 : 2;
                            fill_bus_write_req(addr,dregs[7-i],di.opsz);
                        }
                        r_mask >>= 1;
                    }
                }
            }
            if (m == AddressRegisterIndirectPostIncMode || m == AddressRegisterIndirectPreDecMode)
                aregs[(di.d1_b >> 3) & 0x7] = addr;

            fill_prefetch_req();
            break;
        }
        case MOVEP: {
            // TODO: check
            auto addr = aregs[di.r_src] + sign_extend_to_l(read_prefetch_q());
            if (di.opmode == 0x4){
                // WORD mem->reg
                // TODO: check
                fill_bus_read_req(addr, BRDDataRegisterByteH, BYTE, di.r_dst);
                fill_bus_read_req(addr+2, BRDDataRegisterByteL, BYTE, di.r_dst);
            } else if (di.opmode == 0x5){
                // LONG mem->reg
                fill_bus_read_req(addr, BRDDataRegisterH, WORD, di.r_dst);
                fill_bus_read_req(addr+2, BRDDataRegisterL, WORD, di.r_dst);
            } else if (di.opmode == 0x6) {
                // WORD reg->MEM
                fill_bus_write_req(addr, (dregs[di.r_dst] >> 8) & 0xff, BYTE);
                fill_bus_write_req(addr + 2, dregs[di.r_dst] & 0xff, BYTE);
            } else {
                // LONG reg->MEM
                fill_bus_write_req(addr, (dregs[di.r_dst] >> 16) & 0xffff, WORD);
                fill_bus_write_req(addr + 2, dregs[di.r_dst] & 0xffff, WORD);
            }
            fill_prefetch_req();
            break;
        }
        case MOVEQ: {
            dregs[di.r_dst] = sign_extend_to_l((unsigned char)di.d_l);
            set_sreg(Carry,   0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, dregs[di.r_dst] & di.sign_bit);
            set_sreg(Zero, (dregs[di.r_dst] & di.mask) == 0);
            break;
        }
        case MULS: {
            int res = (short)(dregs[di.r_dst] & 0xffff) * (short)(operand_a_l & 0xffff);
            set_sreg(Carry,   0);
            set_sreg(Overflow,  res > std::numeric_limits<short>::max() || res < std::numeric_limits<short>::min());
            set_sreg(Negative, res & 0x8000'0000);
            set_sreg(Zero,      res == 0);
            dregs[di.r_dst] = res;
            break;
        }
        case MULU: {
            unsigned res = (unsigned short)(dregs[di.r_dst] & 0xffff) * (unsigned short)(operand_a_l & 0xffff);
            set_sreg(Carry,   0);
            set_sreg(Overflow,  res > std::numeric_limits<unsigned short>::max());
            set_sreg(Negative, res & 0x8000'0000);
            set_sreg(Zero,      res == 0);
            dregs[di.r_dst] = res;
            break;
        }
        case NBCD: {
            // TODO: check
            int res = 100 - to_bcd(operand_a_l) - get_sreg(Extend);
            bool c_e =  (to_bcd(operand_a_l) + get_sreg(Extend)) > 0;
            set_sreg(Carry,  c_e);
            set_sreg(Extend,  c_e);
            set_sreg(Zero,      res != 0 ? 0 : get_sreg(Zero));
            dst_value = from_bcd(res);
            fill_prefetch_req();
            set_ea_dst();
            break;
        }

        case NEG: {
            int res = 0 - operand_a_l;
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Carry,  operand_a_l != 0);
            set_sreg(Extend,  operand_a_l != 0);
            set_sreg(Zero,      operand_a_l == 0);
            set_sreg(Overflow, operand_a_l == di.sign_bit);
            dst_value = res & di.mask;
            fill_prefetch_req();
            set_ea_dst();
            break;
        }

        case NEGX: {
            int res = (0 - operand_a_l - get_sreg(Extend));
            bool c_e = operand_a_l + get_sreg(Extend) > 0;
            set_sreg(Overflow, (operand_a_l + get_sreg(Extend)) == di.sign_bit);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Carry,  c_e);
            set_sreg(Extend,  c_e);
            set_sreg(Zero,      res != 0 ? 0 : get_sreg(Zero));
            dst_value = res & di.mask;
            fill_prefetch_req();
            set_ea_dst();
            break;
        }

        case NOP: break;

        case NOT: {
            // TODO: check
            auto res = ~operand_a_l;
            set_sreg(Carry,  0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      res == 0);
            dst_value = res;
            fill_prefetch_req();
            set_ea_dst();
            break;
        }
        case OR: {
            // TODO: check
            auto res = (operand_a_l & di.mask) | (dregs[di.r_dst] & di.mask);
            set_sreg(Carry,  0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      res == 0);
            dst_value = res & di.mask;
            if (di.opmode){
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] = dst_value;
            } else {
                set_ea_dst();
            }
            break;
        }
        case ORI: {
            // TODO: check
            auto res = (operand_a_l & di.mask) | (di.d_l & di.mask);
            set_sreg(Carry,  0);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      res == 0);
            dst_value = res & di.mask;

            fill_prefetch_req();
            set_ea_dst();
            break;
        }
        case ORItoCCR: {
            // TODO: check
            sreg |= di.d_l & 0xff;
            break;
        }
        case PEA: {
            // TODO: check - https://github.com/larsbrinkhoff/m68k-microcode/blob/master/doc/Yacht.txt#L691
            SP-=4;
            fill_bus_write_req(SP, operand_a_l, LONG);
            break;
        }

        case ROL: {
            unsigned res{};
            unsigned char shift = (di.read_ea ? 1 : di.opmode ? dregs[di.r_src] % 64 : (!di.r_src ? 8 : di.r_src));
            unsigned arg = di.read_ea ? operand_a_l & di.mask : dregs[di.r_dst];
            if (di.opsz == BYTE){
                res = std::rotl((unsigned char)arg, shift);
            } else if (di.opsz == WORD) {
                res = std::rotl((unsigned short)arg, shift);
            } else {
                res = std::rotl((unsigned)arg, shift);
            }
            set_sreg(Carry,  res & 1);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            if (di.read_ea){
                dst_value = res & di.mask;
                set_ea_dst();
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= (res & di.mask);
            }
            break;
        }

        case ROR: {
            unsigned res{};
            unsigned char shift = (di.read_ea ? 1 : di.opmode ? dregs[di.r_src] % 64 : (!di.r_src ? 8 : di.r_src));
            unsigned arg = di.read_ea ? operand_a_l & di.mask : dregs[di.r_dst];
            if (di.opsz == BYTE){
                res = std::rotr((unsigned char)arg, shift);
            } else if (di.opsz == WORD) {
                res = std::rotr((unsigned short)arg, shift);
            } else {
                res = std::rotr((unsigned)arg, shift);
            }
            set_sreg(Carry,  res & di.sign_bit);
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            if (di.read_ea){
                dst_value = res & di.mask;
                set_ea_dst();
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= (res & di.mask);
            }
            break;
        }

        // TODO: check
        case ROXL: {
            unsigned char shift{};
            unsigned long arg{};
            if (di.read_ea){
                shift = 1;
                arg = operand_a_l & di.mask;
            } else {
                shift = di.opmode ? dregs[di.r_src] % 64 : (!di.r_src ? 8 : di.r_src);
                arg = dregs[di.r_dst];
            }

            unsigned sz = (di.opsz * CHAR_BIT);
            arg |= (((unsigned long)get_sreg(Extend)) << (di.opsz * CHAR_BIT));
            unsigned long res = rotl(arg, shift,  sz + 1);
            set_sreg(Extend, shift ? (res & 0x1) : get_sreg(Extend));
            set_sreg(Carry, shift ? (res & 0x1) : get_sreg(Extend));
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);

            if (di.read_ea){
                dst_value = res & di.mask;
                set_ea_dst();
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= (res & di.mask);
            }
            break;
        }

        // TODO: check
        case ROXR: {
            unsigned char shift{};
            unsigned long arg{};
            if (di.read_ea){
                shift = 1;
                arg = operand_a_l & di.mask;
            } else {
                shift = di.opmode ? dregs[di.r_src] % 64 : (!di.r_src ? 8 : di.r_src);
                arg = dregs[di.r_dst];
            }

            unsigned sz = (di.opsz * CHAR_BIT);
            arg |= (((unsigned long)get_sreg(Extend)) << (di.opsz * CHAR_BIT));
            unsigned long res = rotr(arg, shift,  sz + 1);

            set_sreg(Extend, shift ? (res & (di.sign_bit << 1)) : get_sreg(Extend));
            set_sreg(Carry,  shift ? (res & (di.sign_bit << 1)) : get_sreg(Extend));
            set_sreg(Overflow,  0);
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);

            if (di.read_ea){
                dst_value = res & di.mask;
                set_ea_dst();
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= (res & di.mask);
            }
            break;
        }

        case RTR: {
            fill_bus_read_req(SP, BRD_SREG, WORD);
            fill_bus_read_req(SP + 2, BRD_PC_H, WORD);
            fill_bus_read_req(SP + 4, BRD_PC_L, WORD);
            SP += 6;
            break;
        }

        case RTS: {
            fill_bus_read_req(SP, BRD_PC_H, WORD);
            fill_bus_read_req(SP + 2, BRD_PC_L, WORD);
            SP += 4;
            break;
        }

        case SBCD: {
            // TODO: check
            unsigned a = di.opmode ? to_bcd(operand_a_l & di.mask) : to_bcd(dregs[di.r_dst] & di.mask);
            unsigned b = di.opmode ? to_bcd(operand_b_l & di.mask) : to_bcd(dregs[di.r_src] & di.mask);
            bool carry = (b + get_sreg(Extend)) > (a & di.mask);
            unsigned res = mod<unsigned>(a - b - get_sreg(Extend), 100);
            set_sreg(Extend, carry);
            set_sreg(Carry,  carry);
            set_sreg(Zero,      res ? 0 : get_sreg(Zero));
            if (di.opmode){
                fill_bus_write_req(aregs[di.r_dst], from_bcd(res), di.opsz);
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= from_bcd(res);
            }
            break;
        }

        case Scc: {
            unsigned char res = is_cond((ConditionCode)di.d1_b) ? 0xff : 0x0;
            dst_value = res;
            set_ea_dst();
            break;
            // TODO: check
            //  A subsequent NEG.B instruction with the same effective address can be used to change the Scc result from TRUE or FALSE to the equivalent arithmetic value (TRUE = 1, FALSE = 0).
            //  In the MC68000 and MC68008, a memory destination is read before it is written.
        }
        case SUB: {
            // TODO: check
            unsigned long res{};
            if (di.opmode){
                res = (dregs[di.r_dst] & di.mask) - (operand_a_l & di.mask);
                set_sreg(Extend, (operand_a_l & di.mask) > (dregs[di.r_dst] & di.mask));
                set_sreg(Carry,  (operand_a_l & di.mask) > (dregs[di.r_dst] & di.mask));
                set_sreg(Overflow, ((dregs[di.r_dst] & di.sign_bit) == (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (dregs[di.d1_b] & di.sign_bit)));
            } else {
                res = (operand_a_l & di.mask) - (dregs[di.r_dst] & di.mask);
                set_sreg(Extend, (dregs[di.r_dst] & di.mask) > (operand_a_l & di.mask));
                set_sreg(Carry, (dregs[di.r_dst] & di.mask) > (operand_a_l & di.mask));
                set_sreg(Overflow, ((dregs[di.r_dst] & di.sign_bit) == (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
            }
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);

            res &= di.mask;
            if (di.opmode){
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= res;
            } else {
                dst_value = res;
                set_ea_dst();
            }
            break;
        }

        case SUBA: {
            aregs[di.r_dst] -= (opsz == WORD ? sign_extend_to_l((unsigned short)operand_a_l) : operand_a_l);
            break;
        }

        case SUBI: {
            auto res = (operand_a_l & di.mask) - (di.d_l & di.mask);
            set_sreg(Extend, (di.d_l & di.mask) > (operand_a_l & di.mask));
            set_sreg(Carry, (di.d_l & di.mask) > (operand_a_l & di.mask));
            set_sreg(Overflow, ((di.d_l & di.sign_bit) != (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dst_value = res & di.mask;

            fill_prefetch_req();
            set_ea_dst();
            break;
        }

        case SUBQ: {
            auto imm = di.d1_b + 1;
            auto res = (operand_a_l & di.mask) - imm;
            set_sreg(Extend, imm > (operand_a_l & di.mask));
            set_sreg(Carry,  imm > (operand_a_l & di.mask));
            set_sreg(Overflow, ((imm & di.sign_bit) != (operand_a_l & di.sign_bit)) && ((res & di.sign_bit) != (operand_a_l & di.sign_bit)));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      (res & di.mask) == 0);
            dst_value = res & di.mask;
            set_ea_dst();
            break;
        }

        case SUBX: {
            // TODO: check
            unsigned a = di.opmode ? operand_a_l & di.mask : dregs[di.r_dst] & di.mask;
            unsigned b = di.opmode ? operand_b_l & di.mask : dregs[di.r_src] & di.mask;
            unsigned res = a - b - get_sreg(Extend);
            set_sreg(Extend, ((b + get_sreg(Extend)) & di.mask) > (a & di.mask));
            set_sreg(Carry,  ((b + get_sreg(Extend)) & di.mask) > (a & di.mask));
            set_sreg(Overflow, (((b + get_sreg(Extend) & di.mask) & di.sign_bit) != (a & di.sign_bit)) &&
                                        ((res & di.sign_bit) != (a & di.sign_bit)));
            set_sreg(Negative, res & di.sign_bit);
            set_sreg(Zero,      res ? 0 : get_sreg(Zero));
            if (di.opmode){
                fill_bus_write_req(aregs[di.r_dst], res, di.opsz);
            } else {
                dregs[di.r_dst] &= ~di.mask;
                dregs[di.r_dst] |= res;
            }
            break;
        }
        case SWAP:{
            rotl(dregs[di.r_dst], 16);
            fill_prefetch_req();
            break;
        }
        case TAS: {
            set_sreg(Carry,  0);
            set_sreg(Overflow, 0);
            set_sreg(Negative, operand_a_l & di.sign_bit);
            set_sreg(Zero,      (operand_a_l & di.mask) == 0);
            operand_a_l |= 1 << 7;
            dst_value = operand_a_l & 0xff;
            set_ea_dst();
            fill_prefetch_req();
            break;
        }
        case TRAP:{
            set_sreg(SupervisorMode, 1);
            ssp-=4;
            fill_bus_write_req(ssp, pc, LONG);
            ssp-=2;
            fill_bus_write_req(ssp, sreg, WORD);
            jump(di.d1_b);
            break;
        }
        case TRAPV:{
            if (get_sreg(Overflow)) jump(EV_TRAPV);
            break;
        }

        case TST: {
            set_sreg(Carry,  0);
            set_sreg(Overflow, 0);
            set_sreg(Negative, operand_a_l & di.sign_bit);
            set_sreg(Zero,      (operand_a_l & di.mask) == 0);
            fill_prefetch_req();
            break;
        }

        case UNLK:{
            SP = aregs[di.r_dst];
            fill_bus_read_req(SP, BRDAddressRegisterH, WORD, di.r_dst);
            fill_bus_read_req(SP, BRDAddressRegisterL, WORD, di.r_dst);
            SP += 4;
            break;
        }

        case ANDItoSR: {
            if (!get_sreg(SupervisorMode)){
                jump(EV_PrivilegeViolation);
                break;
            }
            sreg &= read_prefetch_q();
            break;
        }

        case EORItoSR: {
            if (!get_sreg(SupervisorMode)){
                jump(EV_PrivilegeViolation);
                break;
            }
            sreg ^= read_prefetch_q();
            break;
        }

        case MOVEtoSR: {
            if (!get_sreg(SupervisorMode)) {
                jump(EV_PrivilegeViolation);
                break;
            }
            sreg = operand_a_l & di.mask;
            reset_prefetch_q();
            break;
        }

        case MOVEUSP: {
            if (!get_sreg(SupervisorMode)){
                jump(EV_PrivilegeViolation);
                break;
            }
            if (di.opmode)
                usp = aregs[di.r_dst];
             else
                aregs[di.r_dst] = usp;
            break;
        }

        case ORItoSR: {
            if (!get_sreg(SupervisorMode)){
                jump(EV_PrivilegeViolation);
                break;
            }
            sreg |= read_prefetch_q();
            break;
        }

        case RESET: {
            if (!get_sreg(SupervisorMode)){
                jump(EV_PrivilegeViolation);
                break;
            }
            fill_bus_read_req(EV_ResetInitialSSP, BRD_SSP_H, WORD);
            fill_bus_read_req(EV_ResetInitialSSP, BRD_SSP_L, WORD);
            fill_bus_read_req(EV_ResetInitialPC, BRD_PC_H, WORD);
            fill_bus_read_req(EV_ResetInitialPC, BRD_PC_L, WORD);
            break;
        }

        case RTE: {
            if (!get_sreg(SupervisorMode)){
                jump(EV_PrivilegeViolation);
                break;
            }
            fill_bus_read_req(ssp, BRD_SREG, WORD);
            fill_bus_read_req(ssp+2, BRD_PC_H, WORD);
            fill_bus_read_req(ssp+4, BRD_PC_L, WORD);
            ssp+=6;
            break;
        }

        case STOP:{
            if (!get_sreg(SupervisorMode)){
                jump(EV_PrivilegeViolation);
                break;
            }
            sreg = read_prefetch_q();
            // TODO: check
            break;
        }

    }
}

M68K::DecodedInfo M68K::decode(unsigned short val) {
    if ((((val >> 12) & 0xf) == 0b1100) && (((val >> 4) & 0x1f) == 0b10000)) {
        auto r =  (unsigned char)((val >> 3) & 0x1);
        return {
            .i = ABCD,
            .r_src = (unsigned char)(val & 0x7),
            .r_dst = (unsigned char)((val >> 9) & 0x7),
            .opmode = r, // r/m
            .opsz = BYTE,
            .read_predec_args = (bool)r,
            .i_sz = 2,
            .src_type = (r ? OT_EA : OT_DREG),
            .dst_type = (r ? OT_EA : OT_DREG)
        };
    }
    if ((((val >> 12) & 0xf) == 0b1101)) {
        unsigned char opmode = (val >> 6) & 0x7;
        unsigned char rx = (val >> 9) & 0x7;
        OperandSize sz;
        switch(opmode){
            case 0b000:
            case 0b100: {sz = BYTE ;break;}
            case 0b001:
            case 0b011:
            case 0b101: {sz = WORD; break;}
            default:    {sz = LONG; break;}
        }
        bool mode = (opmode == 0b000 || opmode == 0b001 || opmode == 0b010);
        return {
            .i = (opmode == 0b011 || opmode == 0b111) ? ADDA : ADD,
            .r_dst = rx,
            .opsz = sz,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = mode,
            .i_sz = (unsigned char)(2 + ea_i_sz( val & 0x3f, sz)),
            .src_type = (mode ? OT_EA : OT_DREG),
            .dst_type = (mode ? OT_DREG : OT_EA),
        };
    }

    if (((val >> 8) & 0xff) == 0b0000'0110) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = ADDI,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .read_displacement = true,
            // TODO: update imm + ea logic
            .i_sz = (unsigned char)(2 + ea_i_sz( val & 0x3f, opsz) + (1 << sz)),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

     if ((val >> 12) == 0b0101 && !(val & 0x100)) {
        unsigned char data = (val >> 9) & 0x3;
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = ADDQ,
            .d1_b = data,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = (unsigned char)(2 + ea_i_sz( val & 0x3f, opsz)),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if ((val >> 12) == 0b1101 && (val & 0x100) && !(val & 0x30)){
        unsigned char rx = (val >> 9) & 0x3;
        unsigned char ry = (val) & 0x3;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char rm = (val >> 3) & 0x1;
        return {
            .i = ADDX,
            .r_src = ry,
            .r_dst = rx,
            .d1_b = rm,
            .opsz = (OperandSize)(1 << sz),
            .read_predec_args = (bool)rm,
            .i_sz = 2,
            .src_type = (rm ? OT_EA : OT_DREG),
            .dst_type = (rm ? OT_EA : OT_DREG),
        };
    }

     if ((val >> 12) == 0b1100 && (val & 0x100) && !(val & 0x30)){
        unsigned char opmode = (val >> 6) & 0x3;
        unsigned char rx = (val >> 9) & 0x3;
        OperandSize opsz{};
        if (opmode == 0x0 || opmode == 0x4)
            opsz = BYTE;
        else if (opmode == 0x1 || opmode == 0x5)
            opsz = WORD;
        else
            opsz = LONG;
        bool mode = (opmode == 0b000 || opmode == 0b001 || opmode == 0b010);
        return {
            .i = AND,
            .r_dst = (unsigned char)(mode ? rx : 0),
            .r_src = (unsigned char)(mode ? 0 : rx),
            .ea_src = (unsigned char)(mode ? (val & 0x3f) : (0)),
            .ea_dst = (unsigned char)(mode ? (0) : (val & 0x3f)),
            .read_ea = true,
            .opsz = opsz,
            .opmode = mode,
            .i_sz = (unsigned char)(2 + ea_i_sz( val & 0x3f, opsz)),
            .src_type = (mode ? OT_EA : OT_DREG),
            .dst_type = (mode ? OT_DREG : OT_EA),
        };
    }

     if ((val >> 8) == 0x2){
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = ANDI,
            .d1_b = sz,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .read_displacement = true,
            .i_sz = (unsigned char)(2 + ea_i_sz( val & 0x3f, opsz) + ((opsz == LONG) ? opsz : 2)),
            .src_type = OT_IMM,
            .dst_type = OT_EA
        };
    }

    if (val == 0x23c) {
        return {
            .i = ANDItoCCR,
            .opsz = BYTE,
            .read_displacement = true,
            .i_sz = 4,
            .src_type = OT_IMM,
            .dst_type = OT_CCR,
        };
    }
    if ((val >> 12) == 0xe && !(val & 0x18)){
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char i_r = (val >> 5) & 1;
        unsigned char reg = (val & 0x7);
        return {
            .i = (val & 0x100) ? ASL : ASR,
            .r_src = count_reg,
            .r_dst = reg,
            .opmode = i_r,
            .opsz = (OperandSize)(1 << sz),
            .i_sz = 2,
            .src_type = (i_r ? OT_DREG : OT_IMM),
            .dst_type = OT_DREG,
        };
    }

    if ((val >> 9) == 0x70 && ((val >> 6) & 0x3) == 0x3){
        return {
            .i = (val & 0x100) ? ASL : ASR,
            .opsz = WORD,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, WORD)),
            .dst_type = OT_EA,
        };
    }


    if (val >> 12 == 0x6 && std::any_of(std::begin(cond_code_list), std::end(cond_code_list),
                                        [val](auto el){return el == ((val >> 8) & 0xf);})) {
        unsigned char cond = (val >> 8) & 0xf;
        unsigned char d8 = val & 0xff;
        return {
            .i = Bcc,
            .d1_b = cond,
            .d_l = d8,
            .opsz = (val & 0xff) ? BYTE : WORD,
            .read_displacement = !d8,
            .i_sz = (unsigned char)(2 + (!d8 ? 2 : 0)),
            .dst_type = OT_LABEL,
        };
    }

    if ((val >> 6) == 0x21){
        return {
            .i = BCHG,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = 0,
            .opsz = BYTE,
            .read_displacement = true,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, BYTE) + 2),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x5) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i =  BCHG,
            .r_src = reg,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = 1,
            .opsz = LONG, // TODO: check
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, LONG)),
            .src_type = OT_DREG,
            .dst_type = OT_EA,
        };
    }

    // TODO: check EA and imm pos
    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x6) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = BCLR,
            .r_src = reg,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = 1,
            .opsz = LONG,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, LONG)),
            .src_type = OT_DREG,
            .dst_type = OT_EA,
        };
    }

    if (val == 0x22){
        return {
            .i = BCLR,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = 0,
            .opsz = BYTE, // TODO: check
            .read_displacement = true,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, BYTE) + 2),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if ((val >> 8) == 0b0110'0000) {
        unsigned displacement = 0;
        OperandSize opsz{};
        if (auto v = (val & 0xff)){
            displacement = v;
            opsz = BYTE;
        } else {
            opsz = WORD;
        }
        return {
            .i = BRA,
            .d_l = displacement,
            .opsz = opsz,
            .read_displacement = !displacement,
            .i_sz = (unsigned char)(2 + (opsz == BYTE ? 0 : opsz)),
            .dst_type = OT_LABEL,
        };
    }

    // TODO: check instruction
    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = BSET,
            .r_src = reg,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = 1,
            .opsz = LONG,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, LONG)),
            .src_type = OT_DREG,
            .dst_type= OT_EA,
        };
    }

    // TODO: check
    if ((val >> 6) == 0x23) {
        return {
            .i = BSET,
            .ea_dst = (unsigned char)(val & 0x3f),
            .opsz = BYTE,
            .read_ea = true,
            .opmode = 0,
            .read_displacement = true,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, BYTE) + 2),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if ((val >> 8) == 0b0110'0001) {
        unsigned displacement = 0;
        OperandSize opsz{};
        if (auto v = (val & 0xff)){
            displacement = v;
            opsz = BYTE;
        } else {
            opsz = WORD;
        }
        return {
            .i = BSR,
            .d_l = displacement,
            .opsz = opsz,
            .read_displacement = opsz != BYTE,
            .i_sz = (unsigned char)(2 + (opsz == BYTE ? 0 : opsz)),
            .dst_type = OT_LABEL,
        };
    }

    if (!(val >> 12) && ((val >> 6) & 0x7) == 0x4) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = BTST,
            .r_src = reg,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = 1,
            .opsz = LONG,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, LONG)),
            .src_type = OT_DREG,
            .dst_type = OT_EA,
        };
    }

    if ((val >> 6) == 0x20) {
        return {
            .i = BTST,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .read_displacement = true,
            .opmode = 0,
            .opsz = BYTE,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, BYTE) + 2),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

        if ((val >> 12) == 0x4 && !(val & 0x40) && (((val >> 7) & 0x3) == 0x3)) { // sz:WORD
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = CHK,
            .r_src = reg,
            .opsz = WORD,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, WORD)),
            .src_type = OT_EA,
            .dst_type = OT_DREG,
        };
    }


      if ((val >> 8) == 0x42) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = CLR,
            .d1_b = sz,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, opsz)),
            .dst_type = OT_EA,
        };
    }


    if ((val >> 12) == 0xb) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        bool is_cmpa = (opmode == 0b11 || opmode == 0b111);
        OperandSize opsz{};
        if (is_cmpa){
            opsz = opmode == 0b111 ? LONG : WORD;
        } else {
            opsz = (OperandSize)(1 << opmode);
        }
        return {
            .i = is_cmpa ? CMPA : CMP,
            .r_dst = reg,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = opmode,
            .opsz = opsz,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, opsz)),
            .src_type = OT_EA,
            .dst_type = OT_AREG,
        };
    }

    if ((val >> 8) == 0xc) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = CMPI,
            .d1_b = sz,
            .opsz = opsz,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .read_displacement = true,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, opsz) + (opsz == LONG ? 4 : 2)),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }


    if ((val >> 12) == 0xb && ((val >> 8) & 0x1) && ((val >> 3) & 0x7) == 0x1) {
        unsigned char rx = (val >> 9) & 0x7;
        unsigned char ry = (val & 0x7);
        unsigned char sz = (val >> 6) & 0x3;
        return {
            .i = CMPM,
            .r_dst = rx,
            .r_src = ry,
            .opsz = (OperandSize)(1 << sz),
            .read_postinc_args = true,
            .i_sz = 2,
            // TODO: check
            .src_type = OT_EA,
            .dst_type = OT_EA,
        };
    }

    if ((val >> 12) == 0x5 && ((val >> 3) & 0x1f) == 0x19) {
        unsigned char cond = (val >> 8) & 0xf;
        unsigned char reg = (val & 0x7);
        return {
            .i = DBcc,
            .r_src = reg,
            .d1_b = cond,
            .opsz = WORD,
            .read_displacement = true,
            .i_sz = 4,
            .src_type = OT_DREG,
            .dst_type = OT_LABEL,
        };
    }


    if ((val >> 12) == 0x8 && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = ((val >> 9) & 0x7);
        return {
            .i = DIVS,
            .r_dst = reg,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .i_sz = 2, // TODO: check
            .src_type = OT_EA,
            .dst_type = OT_DREG, // TODO: check
        };
    }

    if ((val >> 12) == 0x8 && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = DIVU,
            .r_dst = reg,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .i_sz = 2, // TODO: check
            .src_type = OT_EA,
            .dst_type = OT_DREG,
        };
    }


    if ((val >> 12) == 0xb) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        OperandSize opsz{};
        if (opmode == 0x4) opsz = BYTE;
        else if (opmode == 0x5) opsz = WORD;
        else opmode = LONG;
           return {
            .i = EOR,
            .r_src = reg,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opmode = opmode,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .src_type = OT_DREG,
            .dst_type = OT_EA,

        };
    }

    // TODO: check
    if ((val >> 8) == 0xa) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = EORI,
            .d1_b = sz,
            .opsz = opsz,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .read_displacement = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz) + (opsz == LONG ? 4 : 2))),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if (val == 0xa3c) {
        return {
            .i = EORItoCCR,
            .opsz = BYTE,
            .read_displacement = true,
            .i_sz = 4,
            .src_type = OT_IMM,
            .dst_type = OT_CCR,

        };
    }

    if ((val >> 12) == 0xc && (val & 0x100)) {
        unsigned char rx = (val >> 9) & 0x7;
        unsigned char ry = (val & 0x7);
        unsigned char opmode = (val >> 3) & 0x1f;

        OperandType src{};
        OperandType dst{};
        if (opmode == 0x8){
            src = OT_DREG;
            dst = OT_DREG;
        } else if (opmode == 0x9){
            src = OT_AREG;
            dst = OT_AREG;
        } else {
            src = OT_DREG;
            dst = OT_AREG;
        }
        return {
            // TODO: check
            .i = EXG,
            .r_src = ry,
            .r_dst = rx,
            .opmode = opmode,
            .i_sz = 2,
            .src_type = src,
            .dst_type = dst,
        };
    }

    if ((val >> 9) == 0x24 && !((val >> 3) & 0x7)) {
        unsigned char reg = (val & 0x7);
        unsigned char opmode = (val >> 3) & 0x1f;
        return {
            .i = EXT,
            .r_dst = reg,
            .opmode = opmode,
            .i_sz = 2,
            .dst_type = OT_DREG, // TODO: check
        };
    }

    if (val == 0x4afc) {
        return {
            .i = ILLEGAL,
            .i_sz = 2,
        };
    }

    if ((val >> 6) == 0x13b) {
        return {
            .i = JMP,
            .ea_src = (unsigned char)(val & 0x3f), // EA
            .calc_ea = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, Unsized))),
            .src_type = OT_EA,
        };
    }

    if ((val >> 6) == 0x13a) {
        return {
            .i = JSR,
            .ea_src = (unsigned char)(val & 0x3f), // EA
            .calc_ea = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, Unsized))),
            .src_type = OT_EA,
        };
    }
    
    if ((val >> 12) == 0x4 && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        // TODO: check
        return {
            .i = LEA,
            .r_dst = reg,
            .d1_b = (unsigned char)(val & 0x3f), // EA
            .opsz = LONG,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, LONG))),
            .src_type = OT_EA,
            .dst_type = OT_AREG,
        };
    }

    if ((val >> 3) == 0x9ca) {
        unsigned char reg = (val) & 0x7;
        return {
            .i = LINK,
            .r_dst = reg,
            .read_displacement = true,
            .opsz = WORD,
            // TODO: check
            .i_sz = 4,
            .src_type = OT_IMM,
            .dst_type = OT_AREG,
        };
    }

    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x1) {
        unsigned char reg = (val) & 0x7;
        unsigned char i_r = (val >> 5) & 0x1;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char count_reg = (val >> 9) & 0x7;
        return {
            .i = (val & 0x100) ? LSL : LSR,
            .r_src = count_reg,
            .r_dst = reg,
            .opmode = i_r,
            .opsz = (OperandSize)(1 << sz), // TODO: check
            .i_sz = 2,
            .src_type = (i_r ? OT_DREG : OT_IMM),
            .dst_type = OT_DREG,
        };
    }

    if ((val >> 9) == 0x71 && ((val >> 6) & 0x3) == 0x3) {
        return {
            .i = (val & 0x100) ? LSL : LSR,
            .opsz = WORD,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, WORD))),
            .src_type = OT_EA,
        };
    }

    if (!(val >> 14) && ((val >> 12) & 0x3) != 0x0) {
        unsigned char sz = (val >> 12) & 0x3;
        OperandSize opsz{};
        if (sz == 0x1){
            opsz = BYTE;
        } else if (sz == 0x3) {
            opsz = WORD;
        } else {
            opsz = LONG;
        }
        return {
            .i = MOVE,
            .ea_src = (unsigned char)(val & 0x3f),
            .ea_dst = (unsigned char)((val >> 6) & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz) + ea_i_sz(((val >> 6) & 0x3f), opsz, false))),
            .src_type = OT_EA,
            .dst_type = OT_EA,
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
            .r_dst = dst,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .src_type = OT_EA,
            .dst_type = OT_AREG,
        };
    }


    if ((val >> 6) == 0x113) {
        return {
            .i = MOVEtoCCR,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = WORD,
            .i_sz = 2,
            .src_type = OT_EA,
            .dst_type = OT_CCR,
        };
    }

    if ((val >> 6) == 0x103) {
        return {
            .i = MOVEfromSR,
            .d1_b = (unsigned char)(val & 0x3f),
            .opsz = WORD,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, WORD))),
            .src_type = OT_CCR,
            .dst_type = OT_EA,
        };
    }

    // TODO: check
    if ((val >> 11) == 0x9 && ((val >> 7) & 0x7) == 0x1) {
        unsigned char sz = (val >> 6) & 0x1;
        unsigned char dr = (val >> 10) & 0x1;
        auto opsz = !sz ? WORD : LONG;
        return {
            .i = MOVEM,
            .opmode = dr,
            .ea_src =  (dr ? (unsigned char)(val & 0x3f) : (unsigned char)0),
            .ea_dst = (dr ? (unsigned char)0 : (unsigned char)(val & 0x3f)),
            .opsz = opsz,
            .read_ea = true,
            .read_prefetch = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz) + 2)),
            // TODO: check
            .src_type = (dr ? OT_EA : OT_LIST),
            .dst_type = (dr ? OT_LIST : OT_EA)
        };
    }

    if (!(val >> 12)  && ((val >> 3) & 0x7) == 0x1) {
        unsigned char data_reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        unsigned char address_reg = val & 0x7;
        return {
            .i = MOVEP,
            .r_src = address_reg,
            .r_dst = data_reg,
            .opmode = opmode,
            .i_sz = 4,
            // TODO: check
            .src_type = ((opmode == 0x4 || opmode == 0x5) ? OT_EA : OT_DREG),
            .dst_type = ((opmode == 0x4 || opmode == 0x5) ? OT_DREG : OT_EA),
        };
    }

    if ((val >> 12) == 0x7 && !(val & 0x100)) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char data = val & 0xff;
        return {
            .i = MOVEQ,
            .r_dst = reg,
            .d_l = data,
            .opsz = LONG,
            .i_sz = 2,
            .src_type = OT_IMM,
            .dst_type = OT_DREG,
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x7) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MULS,
            .r_dst = reg,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = WORD,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, WORD))),
            .src_type = OT_EA,
            .dst_type = OT_DREG,
        };
    }

    if ((val >> 12) == 0xc && ((val >> 6) & 0x7) == 0x3) {
        unsigned char reg = (val >> 9) & 0x7;
        return {
            .i = MULU,
            .r_dst = reg,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = WORD,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, WORD))),
            .src_type = OT_EA,
            .dst_type = OT_DREG,
        };
    }


    if ((val >> 6) == 0x120) {
        return {
            .i = NBCD,
            .d1_b = (unsigned char)(val & 0x3f),
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = BYTE,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, BYTE))),
            .dst_type = OT_EA,
        };
    }

    if ((val >> 8) == 0x44) {
        unsigned char sz = ((val >> 6) & 0x3);
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = NEG,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f,opsz))),
            .dst_type = OT_EA,
        };
    }

    if ((val >> 8) == 0x40) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = NEGX,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .dst_type = OT_EA,
        };
    }

    if (val == 0x4e71) {
        return {
            .i = NOP,
            .i_sz = 2,
        };
    }

    if ((val >> 8) == 0x46 && ((val >> 6) & 0x3) != 0x3) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = NOT,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .dst_type = OT_EA,
        };
    }
    
    if ((val >> 12) == 0x8) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        OperandSize opsz{};
        if (opmode == 0x0 || opmode == 0x4){
            opsz = BYTE;
        } else if (opmode == 0x1 || opmode == 0x5){
            opsz = WORD;
        } else {
            opsz = LONG;
        }
        auto mode = (unsigned char)((opmode == 0x0 || opmode == 0x1 || opmode == 0x2) ? 1 : 0);
        return {
            .i = OR,
            .r_dst = (unsigned char)(mode ? reg : 0),
            .r_src = (unsigned char)(mode ? 0 : reg),
            .opmode = mode,
            .opsz = opsz,
            .ea_src = (unsigned char)(mode ? (val & 0x3f) : 0),
            .ea_dst = (unsigned char)(mode ? 0 : (val & 0x3f)),
            .read_ea = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .src_type = (mode ? OT_EA : OT_DREG),
            .dst_type = (mode ? OT_DREG : OT_EA),
        };
    }

    if (!(val >> 8) && ((val >> 6) & 0x3) != 0x3) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = ORI,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .read_displacement = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz) + (opsz == LONG ? 4 : 2))),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if (!(val >> 8)) {
        return {
            .i = ORItoCCR,
            .read_displacement = true,
            .opsz = BYTE,
            .i_sz = 4,
            .src_type = OT_IMM,
            .dst_type = OT_CCR,
        };
    }


    if ((val >> 6) == 0x121) {
        return {
            .i = PEA,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = LONG,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, LONG))),
            .dst_type = OT_EA,
        };
    }

    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x3) {
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char i_r = (val >> 5) & 0x1;
        unsigned char d_r = (val >> 8) & 0x1;
        return {
            .i = (val & 0x100) ?  ROR : ROL,
            .r_src = count_reg,
            .r_dst = d_r,
            .opmode = i_r,
            .opsz = (OperandSize)(1 << sz),
            .i_sz = 2,
            .src_type = (i_r ? OT_DREG : OT_IMM),
            .dst_type = OT_DREG,
        };
    }

    if ((val >> 9) == 0x73 && ((val >> 6) & 0x3) == 0x3) {
        return {
            .i = (val & 0x100) ?  ROR : ROL,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = WORD,
            .i_sz = 2,
            .dst_type = OT_EA,
        };
    }

    // TODO: check
    if ((val >> 12) == 0xe && ((val >> 3) & 0x3) == 0x2) {
        unsigned char count_reg = (val >> 9) & 0x7;
        unsigned char reg = val & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char i_r = (val >> 5) & 0x1;
        return {
            .i = (val & 0x100) ? ROXR : ROXL,
            .r_src = count_reg,
            .r_dst = reg,
            .opmode = i_r,
            .opsz = (OperandSize)(1 << sz),
            .i_sz = 2,
            .src_type = (i_r ? OT_DREG : OT_IMM),
            .dst_type = OT_DREG,
        };
    }

    if ((val >> 9) == 0x72 && ((val >> 6) & 0x3) == 0x3) {
        return {
            .i = (val & 0x100) ? ROXR : ROXL,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = WORD,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, WORD)),
            .dst_type = OT_EA,
        };
    }

    if (val == 0x4e77) {
        return {
            .i = RTR,
            .i_sz = 2,
        };
    }

    if (val == 0x4e75) {
        return {
            .i = RTS,
            .i_sz = 2,
        };
    }

    if ((val >> 12) == 0x8 && ((val >> 4) & 0x1f) == 0x10) {
        unsigned char rx = val & 0x7;
        unsigned char ry = (val >> 9) & 0x7;
        unsigned char rm = (val >> 3) & 0x1;
        return {
            .i = SBCD,
            .r_dst = rx,
            .r_src = ry,
            .opmode = rm,
            .opsz = BYTE,
            .read_predec_args = true,
            .i_sz = 2,
            // TODO: check
            .src_type = (rm ? OT_EA : OT_DREG),
            .dst_type = (rm ? OT_EA :OT_DREG),
        };
    }

    if ((val >> 12) == 0x5 && ((val >> 6) & 0x3) == 0x3) {
        unsigned char cond = (val >> 8) & 0xf;
        return {
            .i = Scc,
            .d1_b = cond,
            .opsz = BYTE,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, BYTE))),
            .dst_type = OT_EA,
        };
    }

    if ((val >> 12) == 0x9 &&  !(((val >> 6) & 0x7) == 0b11 || ((val >> 6) & 0x7) == 0b111)) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        OperandSize opsz{};
        if (opmode == 0x0 || opmode == 0x4){
            opsz = BYTE;
        } else if (opmode == 0x1 || opmode == 0x5) {
            opsz = WORD;
        } else {
            opsz = LONG;
        }
        auto mode = (opmode == 0x0 || opmode == 0x1 || opmode == 0x2);
        return {
            .i =  SUB,
            .r_dst = (unsigned char)(mode ? 0 : reg),
            .r_src = (unsigned char)(mode ? reg : 0),
            .opmode = mode,
            .ea_src = (unsigned char)(mode ?  0 : (val & 0x3f)),
            .ea_dst = (unsigned char)(mode ? (val & 0x3f) : 0),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .src_type = (mode ? OT_DREG : OT_EA),
            .dst_type = (mode ? OT_EA : OT_DREG),
        };
    }

    if ((val >> 12) == 0x9 && (((val >> 6) & 0x7) == 0b11 || ((val >> 6) & 0x7) == 0b111)) {
        unsigned char reg = (val >> 9) & 0x7;
        unsigned char opmode = (val >> 6) & 0x7;
        OperandSize opsz{};
        if (opmode == 0b11){
            opsz = WORD;
        } else {
            opsz = LONG;
        }
        return {
            .i = SUBA,
            .r_dst = reg,
            .opmode = opmode, // TODO: check
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            // TODO: check
            .src_type = OT_EA,
            .dst_type = OT_AREG,
        };
    }


    if ((val >> 8) == 0x4 && ((val >> 6) & 0x3) != 0x3) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = SUBI,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = opsz,
            .read_displacement = true,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz) + (opsz == LONG ? 4 : 2))),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if ((val >> 12) == 0x5 && ((val >> 8) & 0x1)) {
        unsigned char data = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = SUBQ,
            .d1_b = data,
            .ea_dst = (unsigned char)(val & 0x3f),
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .src_type = OT_IMM,
            .dst_type = OT_EA,
        };
    }

    if ((val >> 12) == 0x9 && ((val >> 8) & 0x1) && !((val >> 4) & 0x3)) {
        unsigned char rx = val & 0x7;
        unsigned char ry = (val >> 9) & 0x7;
        unsigned char sz = (val >> 6) & 0x3;
        unsigned char rm = (val >> 3) & 0x1;
        return {
            .i = SUBX,
            .r_dst = rx,
            .r_src = ry,
            .d1_b = sz,
            .opmode = rm,
            .read_predec_args = !!rm,
            .opsz = (OperandSize)(1 << sz),
            .i_sz = 2,
            // TODO: check
            .src_type = (rm ? OT_EA : OT_DREG),
            .dst_type = (rm ? OT_EA : OT_DREG),
        };
    }

    if ((val >> 3) == 0x908) {
        unsigned char reg = val & 0x7;
        return {
            .i = SWAP,
            .r_dst = reg,
            .opsz = WORD,
            .i_sz = 2,
            .dst_type = OT_DREG,
        };
    }

    if ((val >> 6) == 0x12b) {
        unsigned char reg = val & 0x7;
        return {
            .i = TAS,
            .ea_dst = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .opsz = BYTE,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, BYTE))),
            .dst_type = OT_EA,
        };
    }

    if ((val >> 4) == 0x4e4) {
        unsigned char vec = val & 0xf;
        return {
            .i = TRAP,
            .d1_b = vec,
            .i_sz = 2,
            // TODO: check
            .dst_type = OT_IMM,
        };
    }

    if (val == 0x4e76) {
        return {
            .i = TRAPV,
            .i_sz = 2,
        };
    }


    if ((val >> 8) == 0x4a) {
        unsigned char sz = (val >> 6) & 0x3;
        auto opsz = (OperandSize)(1 << sz);
        return {
            .i = TST,
            .ea_dst = (unsigned char)(val & 0x3f),
            .opsz = opsz,
            .i_sz = ((unsigned char)(2 + ea_i_sz(val & 0x3f, opsz))),
            .dst_type = OT_EA,
        };
    }

    if ((val >> 3) == 0x9cb) {
        unsigned char reg = val & 0x7;
        return {
            .i = UNLK,
            .r_dst = reg,
            .i_sz = 2,
            .dst_type = OT_AREG,
        };
    }

    if (val == 0x27c){
        return {
            .i = ANDItoSR,
            .opsz = WORD,
            .read_displacement = true,
            .i_sz = 4,
            .src_type = OT_IMM,
            .dst_type = OT_SR,
        };
    }

    if (val == 0xa7c){
        return {
            .i = EORItoSR,
            .opsz = WORD,
            .read_displacement = true,
            .i_sz = 4,
            .src_type = OT_IMM,
            .dst_type = OT_SR,
        };
    }

    if ((val >> 6) == 0x11b){
        return {
            .i = MOVEtoSR,
            .opsz = WORD,
            .ea_src = (unsigned char)(val & 0x3f),
            .read_ea = true,
            .i_sz = (unsigned char)(2 + ea_i_sz(val & 0x3f, WORD)),
            .src_type = OT_EA,
            .dst_type = OT_SR,
        };
    }

    if ((val >> 4) == 0x4e6){
        unsigned char dr = (val >> 3) & 0x1;
        return {
            .i = MOVEUSP,
            .r_dst =  (dr ? (unsigned char)(val & 0x7) : (unsigned char)0),
            .r_src =  (dr ? (unsigned char)0 : (unsigned char)(val & 0x7)),
            .opmode = dr,
            .opsz = LONG,
            .i_sz = 2,
            .src_type = (dr ? OT_USP : OT_AREG),
            .dst_type = (dr ? OT_AREG : OT_USP),
        };
    }

    if (val == 0x7c)  return  {
        .i = ORItoSR,
        .opsz = WORD,
        .read_displacement = true,
        .i_sz = 4,
        .src_type = OT_IMM,
        .dst_type = OT_SR,
    };
    if (val == 0x4e70) return {.i = RESET, .i_sz = 2};
    if (val == 0x4e73) return {.i = RTE, .i_sz = 2};
    if (val == 0x4e72) return {
        .i = STOP,
        .i_sz = 4,
        .dst_type = OT_IMM,
    };
    return {.i_sz = 2};
}


unsigned M68K::read_mem(M68K::OperandSize sz, const unsigned char* mem) {
    switch (sz) {
        case BYTE: return (*mem) & 0xff;
        case WORD: return (((*(mem + 1))) | (((unsigned)(*mem)) << 8));
        default:
            return ((unsigned)(*mem) << 24) |
                   ((unsigned)(*(mem + 1))) << 16 |
                   ((unsigned)((*(mem + 2)) << 8)) |
                   *(mem + 3);
    }
}

std::pair<SmallString<128>, size_t> M68K::read_operand(const M68K::DecodedInfo& di, bool src, const unsigned char* mem) {
    OperandType ot = src ? di.src_type : di.dst_type;
    SmallString<128> res{};
    size_t step = 0;
    if (ot == OT_IMM) {
        auto imm_s = di.read_displacement ?
                    // TODO: check
                    (step += (di.opsz == LONG) ? 4 : 2, SmallString<128>(be_to_le(read_mem(di.opsz == LONG ? LONG : WORD, mem), di.opsz))) :
                    SmallString<128>(di.d_l);
        res += imm_s;
    } else if (ot == OT_EA) {
        // TODO: MOVE
        if (di.i == MOVE && !src) {
            auto [ea_str, s] = ea_to_str<false>(src ? di.ea_src : di.ea_dst, di.opsz, mem);
            res += ea_str;
            step = s;
        } else {
            auto [ea_str, s] = ea_to_str(src ? di.ea_src : di.ea_dst, di.opsz, mem);
            res += ea_str;
            step = s;
        }
    } else if (ot == OT_DREG || ot == OT_AREG){
        res.push(ot == OT_DREG ? 'D' : 'A');
        res.push('0' + (src ? (char)di.r_src : (char)di.r_dst));
    } else if (ot == OT_CCR){
        res.push("CCR");
    } else if (ot == OT_SR) { // TODO: check
        res.push("SR");
    } else if (ot == OT_LABEL) {
        // TODO: check
        if (di.i == BSR || di.i == BRA || di.i == Bcc) {
            res += (di.opsz == WORD) ? (step += 2, SmallString<128>(
                    sign_extend_to_l((unsigned short) read_mem(di.opsz, mem)))) :
                   SmallString<128>(sign_extend_to_l((unsigned char) di.d_l));
        } else if (di.i == DBcc) {
            // TODO: dec
            res += (step += 2, SmallString<128>(sign_extend_to_l((unsigned short) read_mem(di.opsz, mem))));
        }
    } else if (ot == OT_USP){
        res.push("USP");
    } else if (ot == OT_LIST){
        auto am = to_address_mode(src ? di.ea_dst : di.ea_src, true);
        unsigned short mask = read_mem(WORD, mem);
        step += 2;
        if (am == AddressRegisterIndirectPreDecMode)
            mask = reverse_bits(mask);
        // TODO: check
        unsigned char count = 0;
        for (size_t i = 0; i < 8; ++i){
            auto v = mask & (1 << i);
            if (v && !count){
                res.push('D');
                res.push('0' + i);
            } else if (v && count == 1) {
                res.push('-');
            } else if (!v && count){
                if (count == 1) res.push('/');
                else {res.push('D'); res.push('0' + i-1);}
                count = 0;
            }
            if (v) ++count;
        }
        if (count) {res.push("D7");}
        count = 0;

        res.push(',');

        for (size_t i = 8; i < 16; ++i){
            auto v = mask & (1 << i);
            if (v && !count){
                res.push('A');
                res.push('0' + (i-8));
            } else if (v && count == 1) {
                res.push('-');
            } else if (!v && count){
                if (count == 1) res.push('/');
                else {res.push('A'); res.push('0' + (i-8)-1);}
                count = 0;
            }
            if (v) ++count;
        }
        if (count) {res.push("A7");}
    }
    return {res,step};
}


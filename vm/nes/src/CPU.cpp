#include <iostream>
#include <algorithm>
#include <cassert>
#include "CPU.hpp"
#include "Constants.hpp"

bool CPU::StatusReg::get(Flag flag) {
    return (reg >> flag) & 1;
}

void CPU::StatusReg::set(Flag flag, bool value) {
    if (value)
        reg |= (1 << flag);
    else 
        reg &= ~(1 << flag);
}

unsigned char CPU::StatusReg::get_status() const {
    return reg | (3 << 4);
}

void CPU::StatusReg::set_status(unsigned char v) {
    reg = v;
    reg &= (~(1 << 4));
    reg |= (1 << 5);
}

void CPU::reset() {
    sp = 0xFD;
    sr.reg = 0b0000'0011;
    a = 0;
    x = 0;
    y = 0;
    interrupt_requests[CPU::RESET] = true;
    sr.set(StatusReg::I, false);
    stop_requested = false;
    pc = 0;
#if DEBUG_MODE
    cycles_count = 0;
#endif
}


// https://www.nesdev.org/wiki/Instruction_reference
// https://www.masswerk.at/nowgobang/2021/6502-illegal-opcodes
CPU::DecodedInstruction CPU::decode(unsigned char i) {
    switch(i){
        case 0x69: {return {Immediate, ADC,  2, 2};}
        case 0x65: {return {ZeroPage,  ADC,  2, 3};}
        case 0x75: {return {ZeroPageX, ADC,  2, 4};}
        case 0x6d: {return {Absolute,  ADC,  3, 4};}
        case 0x7d: {return {AbsoluteX, ADC,  3, 4, CrossPageM};} // 5 if page crossed
        case 0x79: {return {AbsoluteY, ADC,  3, 4, CrossPageM};} // 5 if page crossed
        case 0x61: {return {IndexedIndirectX, ADC, 2, 6};}
        case 0x71: {return {IndexedIndirectY, ADC, 2, 5, CrossPageM};} // 6 if page crossed

        case 0x0B:
        case 0x2B:
                    {return {Immediate, AAC, 2, 2};}

        case 0x29: {return {Immediate, AND, 2, 2};}
        case 0x25: {return {ZeroPage,  AND, 2, 3};}
        case 0x35: {return {ZeroPageX, AND, 2, 4};}
        case 0x2d: {return {Absolute,  AND, 3, 4};}
        case 0x3d: {return {AbsoluteX, AND, 3, 4, CrossPageM};} // 5 if page crossed
        case 0x39: {return {AbsoluteY, AND, 3, 4, CrossPageM};} // 5 if page crossed
        case 0x21: {return {IndexedIndirectX,  AND, 2, 6};}
        case 0x31: {return {IndexedIndirectY,  AND, 2, 5, CrossPageM};} // 6 if page crossed

        case 0x0A: {return {Accumulator, ASL, 1, 2};}
        case 0x06: {return {ZeroPage,    ASL, 2, 5};}
        case 0x16: {return {ZeroPageX,   ASL, 2, 6};}
        case 0x0E: {return {Absolute,    ASL, 3, 6};}
        case 0x1E: {return {AbsoluteX,   ASL, 3, 7, DummyReadM};}

        case 0x90: {return {Relative, BCC, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*
        case 0xB0: {return {Relative, BCS, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*
        case 0xF0: {return {Relative, BEQ, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*
        case 0x30: {return {Relative, BMI, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*
        case 0xD0: {return {Relative, BNE, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*
        case 0x10: {return {Relative, BPL, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*
        case 0x50: {return {Relative, BVC, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*
        case 0x70: {return {Relative, BVS, 2, 2, CrossPageM};} // (3 if branch taken, 4 if page crossed)*

        case 0xC9: {return {Immediate, CMP,        2, 2};}
        case 0xC5: {return {ZeroPage, CMP,         2, 3};}
        case 0xD5: {return {ZeroPageX, CMP,        2, 4};}
        case 0xCD: {return {Absolute, CMP,         3, 4};}
        case 0xDD: {return {AbsoluteX, CMP,        3, 4, CrossPageM};} // 5 if page crossed
        case 0xD9: {return {AbsoluteY, CMP,        3, 4, CrossPageM};} // 5 if page crossed
        case 0xC1: {return {IndexedIndirectX, CMP, 2, 6};}
        case 0xD1: {return {IndexedIndirectY, CMP, 2, 5, CrossPageM};} // 6 if page crossed

        case 0xE0: {return {Immediate, CPX, 2, 2};}
        case 0xE4: {return {ZeroPage,  CPX, 2, 3};}
        case 0xEC: {return {Absolute,  CPX, 3, 4};}

        case 0xC0: {return {Immediate, CPY, 2, 2};}
        case 0xC4: {return {ZeroPage,  CPY, 2, 3};}
        case 0xCC: {return {Absolute,  CPY, 3, 4};}

        case 0xC6: {return {ZeroPage,  DEC, 2, 5};}
        case 0xD6: {return {ZeroPageX,  DEC, 2, 6};}
        case 0xCE: {return {Absolute,  DEC, 3, 6};}
        case 0xDE: {return {AbsoluteX,  DEC, 3, 7, DummyReadM};}

        case 0x49: {return {Immediate,           EOR, 2, 2};}
        case 0x45: {return {ZeroPage,            EOR, 2, 3};}
        case 0x55: {return {ZeroPageX,           EOR, 2, 4};}
        case 0x4D: {return {Absolute,            EOR, 3, 4};}
        case 0x5D: {return {AbsoluteX,           EOR, 3, 4, CrossPageM};} // 5 if page crossed
        case 0x59: {return {AbsoluteY,           EOR, 3, 4, CrossPageM};} // 5 if page crossed
        case 0x41: {return {IndexedIndirectX,    EOR, 2, 6};}
        case 0x51: {return {IndexedIndirectY,    EOR, 2, 5, CrossPageM};} // 6 if page crossed

        case 0xE6: {return {ZeroPage,    INC, 2, 5};}
        case 0xF6: {return {ZeroPageX,   INC, 2, 6};}
        case 0xEE: {return {Absolute,    INC, 3, 6};}
        case 0xFE: {return {AbsoluteX,   INC, 3, 7, DummyReadM};}

        case 0xA9: {return {Immediate, LDA,         2, 2};}
        case 0xA5: {return {ZeroPage, LDA,          2, 3};}
        case 0xB5: {return {ZeroPageX, LDA,         2, 4};}
        case 0xAD: {return {Absolute, LDA,          3, 4};}
        case 0xBD: {return {AbsoluteX, LDA,         3, 4, CrossPageM};} // 5 if page crossed
        case 0xB9: {return {AbsoluteY, LDA,         3, 4, CrossPageM};} // 5 if page crossed
        case 0xA1: {return {IndexedIndirectX, LDA,  2, 6};} 
        case 0xB1: {return {IndexedIndirectY, LDA,  2, 5, CrossPageM};} // 6 if page crossed


        case 0xA2: {return {Immediate, LDX,  2, 2};}
        case 0xA6: {return {ZeroPage, LDX,   2, 3};}
        case 0xB6: {return {ZeroPageY, LDX,  2, 4};}
        case 0xAE: {return {Absolute, LDX,   3, 4};}
        case 0xBE: {return {AbsoluteY, LDX,  3, 4, CrossPageM};} // 5 if page crossed

        case 0xA0: {return {Immediate, LDY, 2, 2};}
        case 0xA4: {return {ZeroPage, LDY,  2, 3};}
        case 0xB4: {return {ZeroPageX, LDY, 2, 4};}
        case 0xAC: {return {Absolute, LDY,  3, 4};}
        case 0xBC: {return {AbsoluteX, LDY, 3, 4, CrossPageM};}  // 5 if page crossed


        case 0x6B: {return {Immediate, ARR, 2, 2};}

        case 0xAB: {return {Immediate, ATX, 2, 2};}
        case 0xCB: {return {Immediate, AXS, 2, 2};}

        case 0x4B: {return {Immediate, ASR, 2, 2};}
        case 0x4A: {return {Accumulator, LSR, 1, 2};}
        case 0x46: {return {ZeroPage, LSR,    2, 5};}
        case 0x56: {return {ZeroPageX, LSR,    2, 6};}
        case 0x4E: {return {Absolute, LSR,    3, 6};}
        case 0x5E: {return {AbsoluteX, LSR,   3, 7, DummyReadM};}

        case 0x09: {return {Immediate, ORA,         2, 2};}
        case 0x05: {return {ZeroPage,  ORA,         2, 3};}
        case 0x15: {return {ZeroPageX, ORA,         2, 4};}
        case 0x0D: {return {Absolute,  ORA,         3, 4};}
        case 0x1D: {return {AbsoluteX, ORA,         3, 4, CrossPageM};}  // 5 if page crossed
        case 0x19: {return {AbsoluteY, ORA,         3, 4, CrossPageM};}  // 5 if page crossed
        case 0x01: {return {IndexedIndirectX, ORA,  2, 6};} 
        case 0x11: {return {IndexedIndirectY, ORA,  2, 5, CrossPageM};}  // 6 if page crossed

        case 0x2A: {return {Accumulator, ROL, 1, 2};}
        case 0x26: {return {ZeroPage, ROL,    2, 5};}
        case 0x36: {return {ZeroPageX, ROL,   2, 6};}
        case 0x2E: {return {Absolute, ROL,    3, 6};}
        case 0x3E: {return {AbsoluteX, ROL,   3, 7, DummyReadM};}

        case 0x6A: {return {Accumulator, ROR,  1, 2};}
        case 0x66: {return {ZeroPage,  ROR,    2, 5};}
        case 0x76: {return {ZeroPageX, ROR,    2, 6};}
        case 0x6E: {return {Absolute,  ROR,    3, 6};}
        case 0x7E: {return {AbsoluteX, ROR,    3, 7, DummyReadM};}

        case 0xEB:
        case 0xE9: {return {Immediate, SBC,         2, 2};}
        case 0xE5: {return {ZeroPage,  SBC,         2, 3};}
        case 0xF5: {return {ZeroPageX, SBC,         2, 4};}
        case 0xED: {return {Absolute,  SBC,         3, 4};}
        case 0xFD: {return {AbsoluteX, SBC,         3, 4, CrossPageM};}  // 5 if page crossed
        case 0xF9: {return {AbsoluteY, SBC,         3, 4, CrossPageM};}  // 5 if page crossed
        case 0xE1: {return {IndexedIndirectX, SBC,  2, 6};} 
        case 0xF1: {return {IndexedIndirectY, SBC,  2, 5, CrossPageM};}  // 6 if page crossed

        case 0x85: {return {ZeroPage,  STA,    2, 3};}
        case 0x95: {return {ZeroPageX, STA,    2, 4};}
        case 0x8D: {return {Absolute,  STA,    3, 4};}
        case 0x9D: {return {AbsoluteX, STA,    3, 5, DummyReadM};}
        case 0x99: {return {AbsoluteY, STA,    3, 5, DummyReadM};}
        case 0x81: {return {IndexedIndirectX, STA,  2, 6, DummyReadM};}
        case 0x91: {return {IndexedIndirectY, STA,  2, 6, DummyReadM};}


        case 0x07: {return {ZeroPage,  SLO,    2, 5};}
        case 0x17: {return {ZeroPageX,  SLO,    2, 6};}
        case 0x0F: {return {Absolute,  SLO,    3, 6};}
        case 0x1F: {return {AbsoluteX,  SLO,    3, 7, DummyReadM};}
        case 0x1B: {return {AbsoluteY,  SLO,    3, 7, DummyReadM};}
        case 0x03: {return {IndexedIndirectX, SLO,  2, 8};}
        case 0x13: {return {IndexedIndirectY, SLO,  2, 8, DummyReadM};}

        case 0x27: {return {ZeroPage,  RLA,    2, 5};}
        case 0x37: {return {ZeroPageX,  RLA,    2, 6};}
        case 0x2F: {return {Absolute,  RLA,    3, 6};}
        case 0x3F: {return {AbsoluteX,  RLA,    3, 7, DummyReadM};}
        case 0x3B: {return {AbsoluteY,  RLA,    3, 7, DummyReadM};}
        case 0x23: {return {IndexedIndirectX, RLA,  2, 8};}
        case 0x33: {return {IndexedIndirectY, RLA,  2, 8, DummyReadM};}


        case 0x47: {return {ZeroPage,  SRE,    2, 5};}
        case 0x57: {return {ZeroPageX,  SRE,    2, 6};}
        case 0x4F: {return {Absolute,  SRE,    3, 6};}
        case 0x5F: {return {AbsoluteX,  SRE,    3, 7, DummyReadM};}
        case 0x5B: {return {AbsoluteY,  SRE,    3, 7, DummyReadM};}
        case 0x43: {return {IndexedIndirectX, SRE,  2, 8};}
        case 0x53: {return {IndexedIndirectY, SRE,  2, 8, DummyReadM};}

        case 0x67: {return {ZeroPage,  RRA,    2, 5};}
        case 0x77: {return {ZeroPageX,  RRA,    2, 6};}
        case 0x6F: {return {Absolute,  RRA,    3, 6};}
        case 0x7F: {return {AbsoluteX,  RRA,    3, 7, DummyReadM};}
        case 0x7B: {return {AbsoluteY,  RRA,    3, 7, DummyReadM};}
        case 0x63: {return {IndexedIndirectX, RRA,  2, 8};}
        case 0x73: {return {IndexedIndirectY, RRA,  2, 8, DummyReadM};}

        case 0x87: {return {ZeroPage,  AAX,    2, 3};}
        case 0x8F: {return {Absolute,  AAX,    3, 4};}
        case 0x97: {return {ZeroPageY,  AAX,    2, 4};}
        case 0x83: {return {IndexedIndirectX, AAX,  2, 6};}

        case 0xA7: {return {ZeroPage,  LAX,    2, 3};}
        case 0xB7: {return {ZeroPageY,  LAX,    2, 4};}
        case 0xAF: {return {Absolute,  LAX,    3, 4};}
        case 0xBF: {return {AbsoluteY,  LAX,    3, 4, CrossPageM};} //*
        case 0xA3: {return {IndexedIndirectX, LAX,  2, 6};}
        case 0xB3: {return {IndexedIndirectY, LAX,  2, 5, CrossPageM};}//*

        case 0xC7: {return {ZeroPage,  DCP,    2, 5};}
        case 0xD7: {return {ZeroPageX,  DCP,    2, 6};}
        case 0xCF: {return {Absolute,  DCP,    3, 6};}
        case 0xDF: {return {AbsoluteX,  DCP,    3, 7, DummyReadM};}
        case 0xDB: {return {AbsoluteY,  DCP,    3, 7, DummyReadM};}
        case 0xC3: {return {IndexedIndirectX, DCP,  2, 8};}
        case 0xD3: {return {IndexedIndirectY, DCP,  2, 8, DummyReadM};}


        case 0xE7: {return {ZeroPage,  ISC,    2, 5};}
        case 0xF7: {return {ZeroPageX,  ISC,    2, 6};}
        case 0xEF: {return {Absolute,  ISC,    3, 6};}
        case 0xFF: {return {AbsoluteX,  ISC,    3, 7, DummyReadM};}
        case 0xFB: {return {AbsoluteY,  ISC,    3, 7, DummyReadM};}
        case 0xE3: {return {IndexedIndirectX, ISC,  2, 8};}
        case 0xF3: {return {IndexedIndirectY, ISC,  2, 8, DummyReadM};}

        case 0x9C: {return {AbsoluteX,  SYA,    3, 5, DummyReadM};}
        case 0x9E: {return {AbsoluteY,  SXA,    3, 5, DummyReadM};}

        case 0x86: {return {ZeroPage, STX, 2, 3};}
        case 0x96: {return {ZeroPageY, STX, 2, 4};}
        case 0x8E: {return {Absolute, STX, 3, 4};}

        case 0x84: {return {ZeroPage, STY, 2, 3};}
        case 0x94: {return {ZeroPageX, STY, 2, 4};}
        case 0x8C: {return {Absolute, STY, 3, 4};}

        case 0x24: {return {ZeroPage, BIT, 2, 3};}
        case 0x2C: {return {Absolute, BIT, 3, 4};}

        case 0x4C: {return {Absolute, JMP, 3, 3};}
        case 0x6C: {return {Indirect, JMP, 3, 5};}

        case 0x20: {return {Absolute, JSR, 3, 6};}

        case 0x60: {return {Implied, RTS, 1, 6};}
        
        case 0x48: {return {Implied, PHA, 1, 3};}
        case 0x08: {return {Implied, PHP, 1, 3};}
        case 0x68: {return {Implied, PLA, 1, 4};}
        case 0x28: {return {Implied, PLP, 1, 4};}

        case 0x38: {return {Implied, SEC, 1, 2};}
        case 0xF8: {return {Implied, SED, 1, 2};}
        case 0x78: {return {Implied, SEI, 1, 2};}
        case 0xAA: {return {Implied, TAX, 1, 2};}
        case 0xA8: {return {Implied, TAY, 1, 2};}
        case 0xBA: {return {Implied, TSX, 1, 2};}
        case 0x8A: {return {Implied, TXA, 1, 2};}
        case 0x9A: {return {Implied, TXS, 1, 2};}
        case 0x98: {return {Implied, TYA, 1, 2};}
        case 0x40: {return {Implied, RTI, 1, 6};}
        case 0xE8: {return {Implied, INX, 1, 2};}
        case 0xC8: {return {Implied, INY, 1, 2};}

        case 0xEA:
        case 0x1A:
        case 0x3A:
        case 0x5A:
        case 0x7A:
        case 0xDA:
        case 0xFA:
            {return {Implied, NOP, 1, 2};}

        case 0x80:
        case 0x82:
        case 0x89:
        case 0xC2:
        case 0xE2:
             {return {Implied, DOP, 2, 2};}

        case 0x04:
        case 0x44:
        case 0x64:
            {return {ZeroPage, DOP, 2, 3};}

        case 0x14:
        case 0x34:
        case 0x54:
        case 0x74:
        case 0xD4:
        case 0xF4:
            {return {ZeroPageX, DOP, 2, 4};}

        case 0x0C:
            {return {Absolute, TOP, 3, 4};}

        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            {return {AbsoluteX, TOP, 3, 4};} //*

        case 0xCA: {return {Implied, DEX, 1, 2};}
        case 0x88: {return {Implied, DEY, 1, 2};}
        case 0x18: {return {Implied, CLC, 1, 2};}
        case 0xD8: {return {Implied, CLD, 1, 2};}
        case 0x58: {return {Implied, CLI, 1, 2};}
        case 0xB8: {return {Implied, CLV, 1, 2};}
        case 0x00: {return {Implied, BRK, 2, 7};}
    }
    // TODO: maybe error or optional
    return {};
}

void CPU::push_addr_step(AddressingStep step) {
    switch (step) {
        case LowByte: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.arg1 = bus.read_prg(cpu.pc + 1);
            });
            break;
        }
        case HighByte:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.arg2 = bus.read_prg(cpu.pc + 2);
            });
            break;
        }
        case AddrPlusX: {
            iop.push_front([](Bus &bus, CPU& cpu){
                cpu.as.result = (unsigned short)((cpu.as.arg1 + cpu.x) % 256);
            });
            break;
        }
        case AddrPlusY: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.result = (unsigned short)((cpu.as.arg1 + cpu.y) % 256);
            });
            break;
        }
        case AbsX: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.arg2 = bus.read_prg(cpu.pc + 2);
                unsigned short args_as_addr = ((cpu.as.arg1) | (cpu.as.arg2 << 8));
                unsigned short addr = args_as_addr + cpu.x;
                if ((cpu.di.penalty_mode == CrossPageM  && (cpu.as.arg2 != ((addr >> 8) & 0xff))) || cpu.di.penalty_mode == DummyReadM){
                    cpu.as.result = args_as_addr;
                    cpu.as.addr_to_add = cpu.x;
                    ++cpu.di.cycles;
                    cpu.update_arg();
                    cpu.iop.push_back([](Bus& bus, CPU& cpu){
                        auto arg = cpu.dummy_read(bus, cpu.arg);
                        cpu.as.result = arg.addr;
                    }); // PageCross
                } else {
                    cpu.as.result = addr;
                    cpu.as.addr_to_add = 0;
                }
            });

            break;
        }
        case AbsY: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.arg2 = bus.read_prg(cpu.pc + 2);
                unsigned short args_as_addr = ((cpu.as.arg1) | (cpu.as.arg2 << 8));
                unsigned short addr = args_as_addr + cpu.y;
                if ((cpu.di.penalty_mode == CrossPageM && (cpu.as.arg2 != ((addr >> 8) & 0xff))) || cpu.di.penalty_mode == DummyReadM){
                    cpu.as.result = args_as_addr;
                    cpu.as.addr_to_add = cpu.y;
                    ++cpu.di.cycles;
                    cpu.update_arg();
                    cpu.iop.push_back([](Bus& bus, CPU& cpu){
                        auto arg = cpu.dummy_read(bus, cpu.arg);
                        cpu.as.result = arg.addr;
                    }); // PageCross
                } else {
                    cpu.as.result = addr;
                    cpu.as.addr_to_add = 0;
                }
            });
            break;
        }
        case IndXReadLow: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.ind_low = bus.read_prg((cpu.as.arg1 + cpu.x) % 256);
            });
            break;
        }
        case IndXReadHigh: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.tmp_h = bus.read_prg((cpu.as.arg1 + cpu.x + 1) % 256)*256;
            });
            break;
        }
        case IndXRes: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.result = cpu.as.ind_low + cpu.as.tmp_h;
            });
            break;
        }
        case IndYReadLow: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.ind_low = bus.read_prg((cpu.as.arg1));
            });
            break;
        }
        case IndYReadHigh: {
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char h = bus.read_prg((cpu.as.arg1 + 1) % 256);
                unsigned short addr = (cpu.as.ind_low | (h << 8));
                unsigned short res_addr = addr + cpu.y;
                if ((cpu.di.penalty_mode == CrossPageM && (h != ((res_addr >> 8) & 0xff)) || cpu.di.penalty_mode == DummyReadM)){
                    cpu.as.result = addr;
                    cpu.as.addr_to_add = cpu.y;
                    ++cpu.di.cycles;
                    cpu.update_arg();
                    cpu.iop.push_back([](Bus& bus, CPU& cpu){
                        auto arg = cpu.dummy_read(bus, cpu.arg);
                        cpu.as.result = arg.addr;
                    }); // PageCross
                } else {
                    cpu.as.result = res_addr;
                    cpu.as.addr_to_add = 0;
                }
            });
            break;
        }
        case Abs: {
            iop.push_front([](Bus& bus, CPU& cpu){
                auto h = bus.read_prg(cpu.pc + 2);
                cpu.as.result = ((cpu.as.arg1) | h << 8);
            });
            break;
        }
        case Zp: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.result = bus.read_prg(cpu.pc + 1);
            });
            break;
        }
    }
}

void CPU::fill_address_ops() {
    static constexpr Instruction cmds[] = {
            BRK,
            CLC, CLD, CLI, CLV, DEX, DEY, INX, INY,
            PHA,  PHP, PLA, PLP, RTI, RTS, SEC, SEI, SED,
            TAX,
            TAY,
            TSX,
            TXA,
            TXS,
            TYA,
    };
    if (std::any_of(std::begin(cmds), std::end(cmds), [i = di.i](auto el){return i == el;})) return;

    switch (di.addr_mode) {
        case ZeroPageX: {push_addr_step(LowByte); push_addr_step(AddrPlusX); break;}
        case ZeroPageY: {push_addr_step(LowByte); push_addr_step(AddrPlusY); break;}
        case AbsoluteX: {push_addr_step(LowByte); push_addr_step(AbsX); break;}
        case AbsoluteY: {push_addr_step(LowByte); push_addr_step(AbsY); break;}
        case IndexedIndirectX: {push_addr_step(LowByte);push_addr_step(IndXReadLow);push_addr_step(IndXReadHigh); push_addr_step(IndXRes); break;}
        case IndexedIndirectY: {push_addr_step(LowByte);push_addr_step(IndYReadLow);push_addr_step(IndYReadHigh); break;}
        case Absolute: {push_addr_step(LowByte);push_addr_step(Abs); break;}
        case ZeroPage: {push_addr_step(Zp); break;}
        case Immediate:
        case Relative:  {as.result = pc + 1; break;}
        case Accumulator: {break;}
    }
}

void CPU::fill_exec_ops() {
    switch(di.i) {
        case ADC: {
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                unsigned short res = cpu.a + val + cpu.sr.get(StatusReg::C);
                cpu.sr.set(StatusReg::C, res > 0xff);
                cpu.sr.set(StatusReg::Z, (res & 0xff) == 0);
                cpu.sr.set(StatusReg::V, (res ^ cpu.a) & (res ^ val) & 0x80);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.a = res & 0xff;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case AND:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                cpu.a &= val;
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case ASL:{
            if (di.addr_mode == Accumulator){
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val =  cpu.read(bus, cpu.arg);
                    unsigned char res = val << 1;
                    cpu.sr.set(StatusReg::C, val & 0x80);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, res & 0x80);
                    cpu.write(bus, cpu.arg, res & 0xff);
                    cpu.pc += cpu.di.bytes;
                });
            } else {
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val =  cpu.read(bus, cpu.arg);
                    cpu.as.write_value = val; // RMW
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    unsigned char res = cpu.as.write_value << 1;
                    cpu.sr.set(StatusReg::C, cpu.as.write_value & 0x80);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, res & 0x80);
                    cpu.as.write_value = res & 0xff;
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    cpu.pc += cpu.di.bytes;
                });
            }
            break;
        }
        case SLO:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = val << 1;
                cpu.a |= res;
                cpu.sr.set(StatusReg::C, val & 0x80);
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.as.write_value = res & 0xff;
                cpu.as.old_value = val;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value); // RMW
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case RLA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                unsigned char res = (val << 1) | cpu.sr.get(StatusReg::C);
                cpu.a &= res;
                cpu.sr.set(StatusReg::C, val & 0x80);
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.as.write_value = res & 0xff;
                cpu.as.old_value = val;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value); // RMW
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case SRE:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = val >> 1;
                cpu.a ^= res;
                cpu.sr.set(StatusReg::C, val & 0x1);
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.as.write_value = res & 0xff;
                cpu.as.old_value = val;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value); // RMW
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case RRA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                unsigned char ror_res = (val >> 1) | (cpu.sr.get(StatusReg::C) << 7);
                cpu.sr.set(StatusReg::C, val & 0x1);
                cpu.sr.set(StatusReg::Z, ror_res == 0);
                cpu.sr.set(StatusReg::N, ror_res & 0x80);
                cpu.as.write_value = ror_res;
                cpu.as.old_value = val;
                val = ror_res;
                unsigned short res = (cpu.a + val + cpu.sr.get(StatusReg::C));
                cpu.sr.set(StatusReg::C, res > 0xff);
                cpu.sr.set(StatusReg::Z, (res & 0xff) == 0);
                cpu.sr.set(StatusReg::V, (res ^ cpu.a) & (res ^ val) & 0x80);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.a = res & 0xff;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value); // RMW
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value); // RMW
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case AAX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.a & cpu.x);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case LAX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                cpu.a = val;
                cpu.x = val;
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case DCP:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                unsigned char res = val - 1;
                cpu.sr.set(StatusReg::C, cpu.a >= res);
                cpu.sr.set(StatusReg::Z, cpu.a == res);
                cpu.sr.set(StatusReg::N, (cpu.a - res) & 0x80);
                cpu.as.write_value = res & 0xff;
                cpu.as.old_value = val;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value); // RMW
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case ISC:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                cpu.as.old_value = val;
                val += 1;
                cpu.as.write_value = val;
                int res = cpu.a - val - (1 - cpu.sr.get(StatusReg::C));
                cpu.sr.set(StatusReg::C, res >= 0);
                cpu.sr.set(StatusReg::Z, (res & 0xff) == 0);
                cpu.sr.set(StatusReg::V, (((res & 0xff) ^ cpu.a) & ((res & 0xff) ^ (~val))) & 0x80);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.a = res & 0xff;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case SYA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char res = cpu.y & (((cpu.arg.addr >> 8) & 0xff) + 1);
                if (res > 0){
                     cpu.as.write_value = res & 0xff;
                     cpu.iop.push_front([](Bus& bus, CPU& cpu){
                         ++cpu.di.cycles;
                         cpu.write(bus, cpu.arg, cpu.as.write_value);
                         cpu.pc += cpu.di.bytes;
                     });
                } else {
                    cpu.pc += cpu.di.bytes;
                }
            });
            break;
        }
        case SXA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char res = cpu.x & (((cpu.arg.addr >> 8) & 0xff) + 1);
                if (res > 0){
                    cpu.as.write_value = res & 0xff;
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        cpu.write(bus, cpu.arg, cpu.as.write_value);
                        ++cpu.di.cycles;
                        cpu.pc += cpu.di.bytes;
                    });
                } else {
                    cpu.pc += cpu.di.bytes;
                }
            });
            break;
        }
        case BCC:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (!cpu.sr.get(StatusReg::C)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BCS:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (cpu.sr.get(StatusReg::C)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BEQ:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (cpu.sr.get(StatusReg::Z)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BMI: {
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (cpu.sr.get(StatusReg::N)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BNE:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (!cpu.sr.get(StatusReg::Z)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BPL:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (!cpu.sr.get(StatusReg::N)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BVC:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (!cpu.sr.get(StatusReg::V)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BVS:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
                if (cpu.sr.get(StatusReg::V)){
                    cpu.iop.push_front([](Bus& bus, CPU& cpu){
                        unsigned char h = (cpu.pc >> 8) & 0xff;
                        cpu.pc += (char)(cpu.read(bus, cpu.arg));
                        if (h != ((cpu.pc >> 8) & 0xff)) {
                            ++cpu.di.cycles;
                            cpu.iop.push_back([](Bus &bus, CPU &cpu) {}); // PageCross
                        }
                    });
                }
            });
            break;
        }
        case BIT:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                unsigned char res = cpu.a & val;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::V, val & 0x40);
                cpu.sr.set(StatusReg::N, val & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case BRK:{
            iop.push_front([](Bus& bus, CPU& cpu){
                bus.write(0x100 + cpu.sp, ((cpu.pc + 2) >> 8) & 0xff);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                --cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                bus.write(0x100 + cpu.sp, (cpu.pc + 2) & 0xff);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                --cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.sr.set(StatusReg::B, true);
                bus.write(0x100 + cpu.sp, cpu.sr.get_status());
                cpu.sr.set(StatusReg::I, true);
                --cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc = (bus.read_prg(constants::IRQ_INTERRUPT_HANDLER_L + 1) << 8) |
                          bus.read_prg(constants::IRQ_INTERRUPT_HANDLER_L);
            });
            break;}
        case CLC:{iop.push_front([](Bus& bus, CPU& cpu){ cpu.sr.set(StatusReg::C, 0); cpu.pc+=1; }); break;}
        case CLD:{iop.push_front([](Bus& bus, CPU& cpu){ cpu.sr.set(StatusReg::D, 0); cpu.pc+=1; }); break;}
        case CLI:{iop.push_front([](Bus& bus, CPU& cpu){ cpu.sr.set(StatusReg::I, 0); cpu.pc+=1; }); break;}
        case CLV:{iop.push_front([](Bus& bus, CPU& cpu){ cpu.sr.set(StatusReg::V, 0); cpu.pc+=1; }); break;}
        case CMP:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = cpu.a - val;
                cpu.sr.set(StatusReg::C, cpu.a >= val);
                cpu.sr.set(StatusReg::Z, cpu.a == val);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case CPX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = cpu.x - val;
                cpu.sr.set(StatusReg::C, cpu.x >= val);
                cpu.sr.set(StatusReg::Z, cpu.x == val);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case CPY:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = cpu.y - val;
                cpu.sr.set(StatusReg::C, cpu.y >= val);
                cpu.sr.set(StatusReg::Z, cpu.y == val);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case DEC:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = val - 1;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.as.write_value = res & 0xff;
                cpu.as.old_value = val; // RMW
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case DEX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char res = cpu.x - 1;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.x = res;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case DEY:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char res = cpu.y - 1;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.y = res;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case EOR:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = cpu.a ^ val;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.a = res;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case INC:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                unsigned char res = val + 1;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.as.write_value = res & 0xff;
                cpu.as.old_value = val;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.old_value);// RMW
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.as.write_value);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case INX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char res = cpu.x + 1;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.x = res;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case INY:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char res = cpu.y + 1;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.y = res;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case JMP:{
            // todo
            if (di.addr_mode == Absolute){
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.pc = (cpu.as.arg1 | (bus.read_prg(cpu.pc + 2) << 8));
                });
            } else { // Indirect
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.as.arg1 = bus.read_prg(cpu.pc + 1);
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.as.result = (cpu.as.arg1 | (bus.read_prg(cpu.pc + 2) << 8));
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.as.tmp_l = bus.read_prg(cpu.as.result);
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.as.tmp_h = bus.read_prg((cpu.as.result & 0xff) == 0xff ? (cpu.as.result & 0xff00) : cpu.as.result + 1);
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.pc = cpu.as.tmp_l | (cpu.as.tmp_h << 8);
                });
            }
            break;
        }
        case JSR:{
            // TODO: maybe check
            iop.push_front([](Bus& bus, CPU& cpu){
                bus.write(0x100 + cpu.sp, ((cpu.pc + 2) >> 8) & 0xff); --cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                bus.write(0x100 + cpu.sp, (cpu.pc + 2) & 0xff); --cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc = cpu.arg.addr;
            });
            break;
        }
        case LDA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.a = cpu.read(bus, cpu.arg);
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case LDX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.x = cpu.read(bus, cpu.arg);
                cpu.sr.set(StatusReg::Z, cpu.x == 0);
                cpu.sr.set(StatusReg::N, cpu.x & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case LDY:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.y = cpu.read(bus, cpu.arg);
                cpu.sr.set(StatusReg::Z, cpu.y == 0);
                cpu.sr.set(StatusReg::N, cpu.y & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case LSR:{
            if (di.addr_mode == Accumulator){
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val = cpu.read(bus, cpu.arg);
                    unsigned char res = val >> 1;
                    cpu.sr.set(StatusReg::C, val & 0x1);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, 0);
                    cpu.write(bus, cpu.arg,  res & 0xff);
                    cpu.pc += cpu.di.bytes;
                });
            } else {
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val = cpu.read(bus, cpu.arg);
                    cpu.as.write_value = val; // RMW
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    unsigned char res = cpu.as.write_value >> 1;
                    cpu.sr.set(StatusReg::C, cpu.as.write_value & 0x1);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, 0);
                    cpu.as.write_value = res & 0xff;
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    cpu.pc += cpu.di.bytes;
                });
            }
            break;
        }
        case ASR: {
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                unsigned char res = cpu.a & val;
                unsigned char shifted_res = res >> 1;
                cpu.sr.set(StatusReg::C, res & 0x1);
                cpu.sr.set(StatusReg::Z, shifted_res == 0);
                cpu.sr.set(StatusReg::N, shifted_res & 0x80);
                cpu.a = shifted_res;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
            // TODO: check
        case ATX: {
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                cpu.a = val;
                cpu.x = cpu.a;
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }

        case AXS: {
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                int tmp = (cpu.a & cpu.x) - val;
                cpu.sr.set(StatusReg::C, tmp >= 0);
                cpu.sr.set(StatusReg::Z, (tmp & 0xff) == 0);
                cpu.sr.set(StatusReg::N, tmp & 0x80);
                cpu.x = tmp & 0xff;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case ARR: {
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                cpu.a = (cpu.a & val);
                cpu.a = (cpu.a >> 1) | (cpu.sr.get(StatusReg::C) << 7);
                cpu.sr.set(StatusReg::C, (cpu.a >> 6) & 0x1);
                cpu.sr.set(StatusReg::V, ((cpu.a >> 6) & 0x1) ^  ((cpu.a >> 5) & 0x1));
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case AAC:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                cpu.a &= val;
                cpu.sr.set(StatusReg::C, cpu.a & 0x80);
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case NOP:
        case DOP:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc+=cpu.di.bytes;
            });
            break;}

        case TOP:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.dummy_read(bus, cpu.arg);
                cpu.pc+=cpu.di.bytes;
            });
            break;}

        case ORA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val =  cpu.read(bus, cpu.arg);
                unsigned char res = cpu.a | val;
                cpu.sr.set(StatusReg::Z, res == 0);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.a = res;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case PHA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                bus.write(0x100 + cpu.sp, cpu.a);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                --cpu.sp;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case PHP:{
            iop.push_front([](Bus& bus, CPU& cpu){
                bus.write(0x100 + cpu.sp, cpu.sr.get_status());
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                --cpu.sp;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case PLA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.a = bus.read_prg(0x100 + cpu.sp);
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case PLP:{
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                // TODO: Interrupt disable flag The effect of changing this flag is delayed 1 instruction.
                cpu.sr.set_status(bus.read_prg(0x100 + cpu.sp));
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case ROL:{
            if (di.addr_mode == Accumulator){
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val = cpu.read(bus, cpu.arg);
                    bool prev_c = cpu.sr.get(StatusReg::C);
                    unsigned char res = (val << 1) | prev_c;
                    cpu.sr.set(StatusReg::C, val & 0x80);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, res & 0x80);
                    cpu.write(bus, cpu.arg, res & 0xff);
                    cpu.pc += cpu.di.bytes;
                });
            } else {
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val = cpu.read(bus, cpu.arg);
                    cpu.as.write_value = val; // RMW
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    bool prev_c = cpu.sr.get(StatusReg::C);
                    unsigned char res = (cpu.as.write_value << 1) | prev_c;
                    cpu.sr.set(StatusReg::C, cpu.as.write_value & 0x80);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, res & 0x80);
                    cpu.as.write_value = res & 0xff;
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    cpu.pc += cpu.di.bytes;
                });
            }
            break;
        }
        case ROR:{
            if (di.addr_mode == Accumulator){
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val = cpu.read(bus, cpu.arg);
                    bool prev_c = cpu.sr.get(StatusReg::C);
                    unsigned char res = (val >> 1) | (prev_c << 7) ;
                    cpu.sr.set(StatusReg::C, val & 0x1);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, res & 0x80);
                    cpu.write(bus, cpu.arg, res & 0xff);
                    cpu.pc += cpu.di.bytes;
                });
            } else {
                iop.push_front([](Bus& bus, CPU& cpu){
                    unsigned char val = cpu.read(bus, cpu.arg);
                    cpu.as.write_value = val;
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    bool prev_c = cpu.sr.get(StatusReg::C);
                    unsigned char res = (cpu.as.write_value >> 1) | (prev_c << 7) ;
                    cpu.sr.set(StatusReg::C, cpu.as.write_value & 0x1);
                    cpu.sr.set(StatusReg::Z, res == 0);
                    cpu.sr.set(StatusReg::N, res & 0x80);
                    cpu.as.write_value = res & 0xff;
                });
                iop.push_front([](Bus& bus, CPU& cpu){
                    cpu.write(bus, cpu.arg, cpu.as.write_value);
                    cpu.pc += cpu.di.bytes;
                });
            }
            break;
        }
        case RTI:{
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.sr.set_status(bus.read_prg(0x100 + cpu.sp));
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.tmp_l = bus.read_prg(0x100 + cpu.sp);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.sp; unsigned char h = bus.read_prg(0x100 + cpu.sp);
                cpu.pc = cpu.as.tmp_l | (h << 8);
            });
            break;
        }
        case RTS:{
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.as.tmp_l = bus.read_prg(0x100 + cpu.sp);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.sp;
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char h = bus.read_prg(0x100 + cpu.sp);
                cpu.pc = cpu.as.tmp_l | (h << 8);
            });
            iop.push_front([](Bus& bus, CPU& cpu){
                ++cpu.pc;
            });
            break;
        }
        case SBC:{
            iop.push_front([](Bus& bus, CPU& cpu){
                unsigned char val = cpu.read(bus, cpu.arg);
                bool prev_c = cpu.sr.get(StatusReg::C);
                int res = (cpu.a - val - (!prev_c));
                cpu.sr.set(StatusReg::C, res >= 0);
                cpu.sr.set(StatusReg::Z, (res & 0xff) == 0);
                cpu.sr.set(StatusReg::V, (((res & 0xff) ^ cpu.a) & ((res & 0xff) ^ (~val))) & 0x80);
                cpu.sr.set(StatusReg::N, res & 0x80);
                cpu.a = res & 0xff;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case SEC:{iop.push_front([](Bus& bus, CPU& cpu){cpu.sr.set(StatusReg::C, true); cpu.pc+=cpu.di.bytes; }); break;}
        case SED:{iop.push_front([](Bus& bus, CPU& cpu){cpu.sr.set(StatusReg::D, true); cpu.pc+=cpu.di.bytes; }); break;}
        case SEI:{iop.push_front([](Bus& bus, CPU& cpu){cpu.sr.set(StatusReg::I, true); cpu.pc+=cpu.di.bytes; }); break;}
        case STA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.a);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case STX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.x);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case STY:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.write(bus, cpu.arg, cpu.y);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case TAX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.x = cpu.a;
                cpu.sr.set(StatusReg::Z, cpu.x == 0);
                cpu.sr.set(StatusReg::N, cpu.x & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case TAY:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.y = cpu.a;
                cpu.sr.set(StatusReg::Z, cpu.y == 0);
                cpu.sr.set(StatusReg::N, cpu.y & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case TSX:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.x = cpu.sp;
                cpu.sr.set(StatusReg::Z, cpu.x == 0);
                cpu.sr.set(StatusReg::N, cpu.x & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case TXA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.a = cpu.x;
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case TXS:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.sp = cpu.x;
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
        case TYA:{
            iop.push_front([](Bus& bus, CPU& cpu){
                cpu.a = cpu.y;
                cpu.sr.set(StatusReg::Z, cpu.a == 0);
                cpu.sr.set(StatusReg::N, cpu.a & 0x80);
                cpu.pc += cpu.di.bytes;
            });
            break;
        }
    }
}


CPU::AddressingResult CPU::get_arg_by_addr(Bus &bus, CPU::AddressingMode am, unsigned short pc_addr) {
    auto arg1 = bus.unsafe_read_prg(pc_addr + 1);
    auto arg2 = bus.unsafe_read_prg(pc_addr + 2);
    unsigned short args_as_addr = ((arg1) | arg2 << 8);
    switch(am){
        case ZeroPageX: return {(unsigned short)((arg1 + x) % 256), false, false};
        case ZeroPageY: return {(unsigned short)((arg1 + y) % 256), false, false};
        case AbsoluteX: {
            unsigned short addr = args_as_addr + x;
            return {addr, false, false};
        }
        case AbsoluteY:{
            unsigned short addr = args_as_addr + y;
            return {addr, false, false};
        }
        case IndexedIndirectX: return
                    {(unsigned short)(bus.unsafe_read_prg((arg1 + x) % 256) + bus.unsafe_read_prg((arg1 + x + 1) % 256)* 256), false, false};
        case IndexedIndirectY: {
            unsigned char l = bus.unsafe_read_prg(arg1);
            unsigned char h = bus.unsafe_read_prg((arg1 + 1) % 256);
            unsigned short addr = (l | (h << 8));
            unsigned short res_addr = addr + y;
            return {res_addr, false, false};
        }
        case Immediate: return { (unsigned short)(pc_addr + 1), false, false};
        case ZeroPage: return {arg1, false, false};
        case Absolute: return {  args_as_addr, false, false};
        case Relative: return { (unsigned short)(pc_addr + 1), false, false};
        case Accumulator: return {0, false, true};
    }
    throw std::exception();
}

void CPU::push_interrupts_ops(Bus& bus) {
    iop.push_front([](Bus& bus, CPU& cpu){
        bus.write(0x100 + cpu.sp, (cpu.pc >> 8) & 0xff);
    });
    iop.push_front([](Bus& bus, CPU& cpu){
        --cpu.sp;
    });
    iop.push_front([](Bus& bus, CPU& cpu){
        bus.write(0x100 + cpu.sp, cpu.pc & 0xff);
    });
    iop.push_front([](Bus& bus, CPU& cpu){
        --cpu.sp;
    });
    iop.push_front([](Bus& bus, CPU& cpu){
        cpu.sr.set(StatusReg::B, false);
        cpu.sr.set(StatusReg::Unused, true);
        bus.write(0x100 + cpu.sp, cpu.sr.reg);
        cpu.sr.set(StatusReg::I, true);
    });
    iop.push_front([](Bus& bus, CPU& cpu){
        --cpu.sp;
    });
    iop.push_front([](Bus& bus, CPU& cpu){
        cpu.pc = (bus.read_prg(cpu.interrupt_addresses[cpu.as.interrupt]) | (bus.read_prg(cpu.interrupt_addresses[cpu.as.interrupt] + 1) << 8));
        cpu.interrupt_requests[cpu.as.interrupt] = false;
    });
}

CPU::AddressingResult CPU::dummy_read(Bus& bus, AddressingResult arg) {
     bus.read_prg( (arg.addr & 0xff00) | (arg.addr + arg.add_to_addr) & 0xff);
     arg.addr = arg.addr + arg.add_to_addr;
     // todo: check - addr+1
     return arg; // update address
}

void CPU::update_arg() {
    arg = AddressingResult{
        .addr = as.result,
        .page_crossed = di.penalty_mode == CrossPageM,
        .is_accumulator = di.addr_mode == Accumulator,
        .add_to_addr = as.addr_to_add,
    };
}

void CPU::iop_exec_step(Bus& bus) {
    update_arg();
    if (!iop.empty()){
        auto f = iop.back(); iop.pop_back();
        f(bus, *this);
    }
}


void CPU::interrupt_handle(Bus& bus) {
    if (bus.ppustatus & 0x80 && bus.ppuctrl & 0x80 && bus.nmi_interrupt_flip_flop) {
        interrupt_requests[NMI] = true;
        sr.set(StatusReg::I, false);
        bus.nmi_interrupt_flip_flop = false;
    } else if (bus.irq_interrupt_flip_flop && !sr.get(StatusReg::I)) {
        interrupt_requests[IRQ] = true;
        bus.irq_interrupt_flip_flop = false;
    }

    bool interrupt_in_progress = false;
    if (!sr.get(StatusReg::I)) {
        for (int i = 0; i < InterruptsCount; ++i) {
            if (interrupt_requests[i]) {
                as.interrupt = static_cast<Interrupt>(i);
                push_interrupts_ops(bus);
                interrupt_in_progress = true;
                break;
            }
        }
    }
    istep = interrupt_in_progress ? Execute : Fetch;
}

// todo: finish
void CPU::tick_step(Bus& bus){
#if DEBUG_MODE
    if (!debug_run) return;
#endif
    switch (istep) {
        case Fetch: {
            if (bus.dmc_dma_in_progress)
                return;
            if (bus.dma_active) {
                if (bus.dma_start){
                    bus.dma_start = false;
                    bus.alignment = !(cycles_count & 1);
                    return;
                }
                bus.tick_dma();
#if DEBUG_MODE
                if (!bus.dma_active) assert(cycles_count & 1);
#endif
                return;
            }
            di = decode(bus.read_prg( pc));
            current_iop_index = 0;
            fill_address_ops();
            fill_exec_ops();
            if (di.i == JMP) iop_exec_step(bus); // todo: check
#if DEBUG_MODE
            pc_watch_count += (pc == pc_watch_address);
#endif
            istep = Execute;
            ++bus.instructions_count;
// todo: finish
            --di.cycles;
            break;
        }
        case Execute: {
            if (bus.dma_active) {
                if (bus.dma_start){
                    bus.dma_start = false;
                    bus.alignment = !(cycles_count & 1);
                    return;
                }
                bus.tick_dma();
#if DEBUG_MODE
                if (!bus.dma_active) assert(cycles_count & 1);
#endif
                return;
            }
            if (bus.dmc_dma_in_progress)
                return;
            if (!iop.empty()){
                iop_exec_step(bus);
                ++current_iop_index;
                --di.cycles;
                if (!iop.empty()) return;
            }
            interrupt_handle(bus);
#if DEBUG_MODE
            if (istep == Fetch) debug_run = false; // todo: check
#endif
            break;
        }
    }

}

unsigned char CPU::read(Bus &bus, AddressingResult ar) {
    if (ar.is_accumulator) return a;
    return bus.read_prg(ar.addr);
}

void CPU::write(Bus& bus, AddressingResult ar, unsigned char value){
    if (ar.is_accumulator) {a = value; return;}
    bus.write(ar.addr, value);
}

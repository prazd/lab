#include "CPU.hpp"
#include <iostream>

bool CPU::StatusReg::get(Flag flag) {
    return (reg >> flag) & 1;
}

void CPU::StatusReg::set(Flag flag, bool value) {
    if (value)
        reg |= (1 << flag);
    else 
        reg &= ~(1 << flag);
}

// https://www.nesdev.org/wiki/Instruction_reference
CPU::DecodedInstruction CPU::decode(unsigned char i) {
    switch(i){
        case 0x69: {return {Immediate, ADC,  2, 2};}
        case 0x65: {return {ZeroPage,  ADC,  2, 3};}
        case 0x75: {return {ZeroPageX, ADC,  2, 4};}
        case 0x6d: {return {Absolute,  ADC,  3, 4};}
        case 0x7d: {return {AbsoluteX, ADC,  3, 4};} // 5 if page crossed
        case 0x79: {return {AbsoluteY, ADC,  3, 4};} // 5 if page crossed
        case 0x61: {return {IndexedIndirectX, ADC, 2, 6};}
        case 0x71: {return {IndexedIndirectY, ADC, 2, 5};} // 6 if page crossed

        case 0x29: {return {Immediate, AND, 2, 2};}
        case 0x25: {return {ZeroPage,  AND, 2, 3};}
        case 0x35: {return {ZeroPageX, AND, 2, 4};}
        case 0x2d: {return {Absolute,  AND, 3, 4};}
        case 0x3d: {return {AbsoluteX, AND, 3, 4};} // 5 if page crossed
        case 0x39: {return {AbsoluteY, AND, 3, 4};} // 5 if page crossed
        case 0x21: {return {IndexedIndirectX,  AND, 2, 6};}
        case 0x31: {return {IndexedIndirectY,  AND, 2, 5};} // 6 if page crossed

        case 0x0A: {return {Accumulator, ASL, 1, 2};}
        case 0x06: {return {ZeroPage,    ASL, 2, 5};}
        case 0x16: {return {ZeroPageX,   ASL, 2, 6};}
        case 0x0E: {return {Absolute,    ASL, 3, 6};}
        case 0x1E: {return {AbsoluteX,   ASL, 3, 7};}

        case 0x90: {return {Relative, BCC, 2, 2};} // (3 if branch taken, 4 if page crossed)*
        case 0xB0: {return {Relative, BCS, 2, 2};} // (3 if branch taken, 4 if page crossed)*
        case 0xF0: {return {Relative, BEQ, 2, 2};} // (3 if branch taken, 4 if page crossed)*
        case 0x30: {return {Relative, BMI, 2, 2};} // (3 if branch taken, 4 if page crossed)*
        case 0xD0: {return {Relative, BNE, 2, 2};} // (3 if branch taken, 4 if page crossed)*
        case 0x10: {return {Relative, BPL, 2, 2};} // (3 if branch taken, 4 if page crossed)*
        case 0x50: {return {Relative, BVC, 2, 2};} // (3 if branch taken, 4 if page crossed)*
        case 0x70: {return {Relative, BVS, 2, 2};} // (3 if branch taken, 4 if page crossed)*

        case 0xC9: {return {Immediate, CMP,        2, 2};}
        case 0xC5: {return {ZeroPage, CMP,         2, 3};}
        case 0xD5: {return {ZeroPageX, CMP,        2, 4};}
        case 0xCD: {return {Absolute, CMP,         3, 4};}
        case 0xDD: {return {AbsoluteX, CMP,        3, 4};} // 5 if page crossed
        case 0xD9: {return {AbsoluteY, CMP,        3, 4};} // 5 if page crossed
        case 0xC1: {return {IndexedIndirectX, CMP, 2, 6};}
        case 0xD1: {return {IndexedIndirectY, CMP, 2, 5};} // 6 if page crossed

        case 0xE0: {return {Immediate, CPX, 2, 2};}
        case 0xE4: {return {ZeroPage,  CPX, 2, 3};}
        case 0xEC: {return {Absolute,  CPX, 3, 4};}

        case 0xC0: {return {Immediate, CPY, 2, 2};}
        case 0xC4: {return {ZeroPage,  CPY, 2, 3};}
        case 0xCC: {return {Absolute,  CPY, 3, 4};}

        case 0xC6: {return {ZeroPage,  DEC, 2, 5};}
        case 0xD6: {return {ZeroPageX,  DEC, 2, 6};}
        case 0xCE: {return {Absolute,  DEC, 3, 6};}
        case 0xDE: {return {AbsoluteX,  DEC, 3, 7};}

        case 0x49: {return {Immediate,           EOR, 2, 2};}
        case 0x45: {return {ZeroPage,            EOR, 2, 3};}
        case 0x55: {return {ZeroPageX,           EOR, 2, 4};}
        case 0x4D: {return {Absolute,            EOR, 3, 4};}
        case 0x5D: {return {AbsoluteX,           EOR, 3, 4};} // 5 if page crossed
        case 0x59: {return {AbsoluteY,           EOR, 3, 4};} // 5 if page crossed
        case 0x41: {return {IndexedIndirectX,    EOR, 2, 6};}
        case 0x51: {return {IndexedIndirectY,    EOR, 2, 5};} // 6 if page crossed

        case 0xE6: {return {ZeroPage,    INC, 2, 5};}
        case 0xF6: {return {ZeroPageX,   INC, 2, 6};}
        case 0xEE: {return {Absolute,    INC, 3, 6};}
        case 0xFE: {return {AbsoluteX,   INC, 3, 7};}

        case 0xA9: {return {Immediate, LDA,         2, 2};}
        case 0xA5: {return {ZeroPage, LDA,          2, 3};}
        case 0xB5: {return {ZeroPageX, LDA,         2, 4};}
        case 0xAD: {return {Absolute, LDA,          3, 4};}
        case 0xBD: {return {AbsoluteX, LDA,         3, 4};} // 5 if page crossed
        case 0xB9: {return {AbsoluteY, LDA,         3, 4};} // 5 if page crossed
        case 0xA1: {return {IndexedIndirectX, LDA,  2, 6};} 
        case 0xB1: {return {IndexedIndirectY, LDA,  2, 5};} // 6 if page crossed


        case 0xA2: {return {Immediate, LDX,  2, 2};}
        case 0xA6: {return {ZeroPage, LDX,   2, 3};}
        case 0xB6: {return {ZeroPageY, LDX,  2, 4};}
        case 0xAE: {return {Absolute, LDX,   3, 4};}
        case 0xBE: {return {AbsoluteY, LDX,  3, 4};} // 5 if page crossed

        case 0xA0: {return {Immediate, LDY, 2, 2};}
        case 0xA4: {return {ZeroPage, LDY,  2, 3};}
        case 0xB4: {return {ZeroPageX, LDY, 2, 4};}
        case 0xAC: {return {Absolute, LDY,  3, 4};}
        case 0xBC: {return {AbsoluteX, LDY, 3, 4};}  // 5 if page crossed

        case 0x4A: {return {Accumulator, LSR, 1, 2};}
        case 0x46: {return {ZeroPage, LSR,    2, 5};}
        case 0x56: {return {ZeroPageX, LSR,    2, 6};}
        case 0x4E: {return {Absolute, LSR,    3, 6};}
        case 0x5E: {return {AbsoluteX, LSR,   3, 7};}

        case 0x09: {return {Immediate, ORA,         2, 2};}
        case 0x05: {return {ZeroPage,  ORA,         2, 3};}
        case 0x15: {return {ZeroPageX, ORA,         2, 4};}
        case 0x0D: {return {Absolute,  ORA,         3, 4};}
        case 0x1D: {return {AbsoluteX, ORA,         3, 4};}  // 5 if page crossed
        case 0x19: {return {AbsoluteY, ORA,         3, 4};}  // 5 if page crossed
        case 0x01: {return {IndexedIndirectX, ORA,  2, 6};} 
        case 0x11: {return {IndexedIndirectY, ORA,  2, 5};}  // 6 if page crossed

        case 0x2A: {return {Accumulator, ROL, 1, 2};}
        case 0x26: {return {ZeroPage, ROL,    2, 5};}
        case 0x36: {return {ZeroPageX, ROL,   2, 6};}
        case 0x2E: {return {Absolute, ROL,    3, 6};}
        case 0x3E: {return {AbsoluteX, ROL,   3, 7};}

        case 0x6A: {return {Accumulator, ROR,  1, 2};}
        case 0x66: {return {ZeroPage,  ROR,    2, 5};}
        case 0x76: {return {ZeroPageX, ROR,    2, 6};}
        case 0x6E: {return {Absolute,  ROR,    3, 6};}
        case 0x7E: {return {AbsoluteX, ROR,    3, 7};}

        case 0xE9: {return {Immediate, SBC,         2, 2};}
        case 0xE5: {return {ZeroPage,  SBC,         2, 3};}
        case 0xF5: {return {ZeroPageX, SBC,         2, 4};}
        case 0xED: {return {Absolute,  SBC,         3, 4};}
        case 0xFD: {return {AbsoluteX, SBC,         3, 4};}  // 5 if page crossed
        case 0xF9: {return {AbsoluteY, SBC,         3, 4};}  // 5 if page crossed
        case 0xE1: {return {IndexedIndirectX, SBC,  2, 6};} 
        case 0xF1: {return {IndexedIndirectY, SBC,  2, 5};}  // 6 if page crossed

        case 0x85: {return {ZeroPage,  STA,    2, 3};}
        case 0x95: {return {ZeroPageX, STA,    2, 4};}
        case 0x8D: {return {Absolute,  STA,    3, 4};}
        case 0x9D: {return {AbsoluteX, STA,    3, 5};} 
        case 0x99: {return {AbsoluteY, STA,    3, 5};} 
        case 0x81: {return {IndexedIndirectX, STA,  2, 6};} 
        case 0x91: {return {IndexedIndirectY, STA,  2, 6};}

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
        case 0xEA: {return {Implied, NOP, 1, 2};}
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


CPU::AddressingResult CPU::get_arg(const Memory& mem, AddressingMode am, bool cross_check) {
    auto arg1 = mem.read(pc + 1);
    auto arg2 = mem.read(pc + 2);
    unsigned short args_as_addr = ((arg1) | arg2 << 8);
    switch(am){
        case ZeroPageX: return {mem.read((arg1 + x) % 256), false};
        case ZeroPageY: return {mem.read((arg1 + y) % 256), false};
        case AbsoluteX: {
            unsigned short addr = args_as_addr + x;
            return {mem.read(addr), cross_check && ((args_as_addr & 0xff00) != (addr & 0xff00))};
        }
        case AbsoluteY:{
            unsigned short addr = args_as_addr + y;
            return {mem.read(addr), cross_check && ((args_as_addr & 0xff00) != (addr & 0xff00))};
        }
        case IndexedIndirectX: return
                    {mem.read(
                    mem.read((arg1 + x) % 256) + mem.read((arg1 + x + 1) % 256)
                * 256), false};
        case IndexedIndirectY: {
            unsigned char l =  mem.read(arg1);
            unsigned char h =  mem.read((arg1 + 1) % 256);
            unsigned short addr = (l | (h << 8));
            unsigned short res_addr = addr + y;
            return {mem.read(res_addr), cross_check && ((addr & 0xff00) != ((res_addr & 0xff00)))};
        }
        case Immediate: return {arg1, false};
        case ZeroPage: return {mem.read(arg1), false};
        case Absolute: return {mem.read(args_as_addr), false};
        case Accumulator: return {a, false};
    }
    throw std::exception();
}

void CPU::execute(Memory& mem, DecodedInstruction di){
    switch(di.i) {
        case ADC: {
            auto arg = get_arg(mem, di.addr_mode, true);
            unsigned short res = arg.value + sr.get(StatusReg::C);
            sr.set(StatusReg::C, res > 0xff);
            sr.set(StatusReg::Z, (res & 0xff) == 0);
            sr.set(StatusReg::V, (res ^ a) & (res ^ arg.value) & 0x80);
            sr.set(StatusReg::N, res & 0x80);
            a = res & 0xff;
            pc += di.bytes;
            break;
        }
        case AND:{
            auto arg = get_arg(mem, di.addr_mode, true);
            a &= arg.value;
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            break;
        }
        case ASL:{break;}
        case BCC:{break;}
        case BCS:{break;}
        case BEQ:{break;}
        case BIT:{break;}
        case BMI:{break;}
        case BNE:{break;}
        case BPL:{break;}
        case BRK:{break;}
        case BVC:{break;}
        case BVS:{break;}
        case CLC:{break;}
        case CLD:{break;}
        case CLI:{break;}
        case CLV:{break;}
        case CMP:{break;}
        case CPX:{break;}
        case CPY:{break;}
        case DEC:{break;}
        case DEX:{break;}
        case DEY:{break;}
        case EOR:{break;} 
        case INC:{break;} 
        case INX:{break;} 
        case INY:{break;}
        case JMP:{break;}
        case JSR:{break;} 
        case LDA:{break;} 
        case LDX:{break;} 
        case LDY:{break;}
        case LSR:{break;}
        case NOP:{break;}
        case ORA:{break;}
        case PHA:{break;}
        case PHP:{break;}
        case PLA:{break;}
        case PLP:{break;}
        case ROL:{break;}
        case ROR:{break;}
        case RTI:{break;}
        case RTS:{break;}
        case SBC:{break;}
        case SEC:{break;}
        case SED:{break;}
        case SEI:{break;}
        case STA:{break;}
        case STX:{break;}
        case STY:{break;}
        case TAX:{break;}
        case TAY:{break;}
        case TSX:{break;}
        case TXA:{break;}
        case TXS:{break;}
        case TYA:{break;}
    }
}
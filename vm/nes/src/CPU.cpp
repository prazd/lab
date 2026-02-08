#include <iostream>
#include "CPU.hpp"
#include "Constants.hpp"

void CPU::Memory::write(Bus& bus, unsigned short addr, unsigned char value) {
    adjust_if_mirror(addr);
    if (std::any_of(std::begin(write_mmio_addr), std::end(write_mmio_addr), [addr](auto el){return el == addr;})
    || addr >= 0x8000 && addr <= 0xffff){
        bus.write(addr, value);
        return;
    }
    map[addr] = value;
}

unsigned char CPU::Memory::read(Bus& bus, unsigned short addr) const {
    adjust_if_mirror(addr);
    if (std::any_of(std::begin(write_mmio_addr), std::end(write_mmio_addr), [addr](auto el){return el == addr;})
        || addr >= 0x8000 && addr <= 0xffff){
        return bus.read_prg(addr);
    }
    return map[addr];
}

unsigned char CPU::Memory::unsafe_read(Bus& bus, unsigned short addr) const {
    adjust_if_mirror(addr);
    if (std::any_of(std::begin(write_mmio_addr), std::end(write_mmio_addr), [addr](auto el){return el == addr;})
        || addr >= 0x8000 && addr <= 0xffff){
        return bus.unsafe_read_prg(addr);
    }
    return map[addr];
}


void CPU::Memory::adjust_if_mirror(unsigned short& addr) {
    if (addr >= constants::RAM_MIRROR1_BEGIN && addr <= constants::RAM_MIRROR1_END)
        addr-=constants::RAM_MIRROR1_BEGIN;
    else if (addr >= constants::RAM_MIRROR2_BEGIN && addr <= constants::RAM_MIRROR2_END)
        addr-=constants::RAM_MIRROR2_BEGIN;
    else if (addr >= constants::RAM_MIRROR3_BEGIN && addr <= constants::RAM_MIRROR3_END)
        addr-=constants::RAM_MIRROR3_BEGIN;
    else if (addr >= constants::PPU_REGS_MIRROR_BEGIN && addr <= constants::PPU_REGS_MIRROR_END){
        addr = constants::PPU_REGS_BEGIN + (addr % 8);
    }
}

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

        case 0x0B:
        case 0x2B:
                    {return {Immediate, AAC, 2, 2};}

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


        case 0x6B: {return {Immediate, ARR, 2, 2};}

        case 0xAB: {return {Immediate, ATX, 2, 2};}
        case 0xCB: {return {Immediate, AXS, 2, 2};}

        case 0x4B: {return {Immediate, ASR, 2, 2};}
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

        case 0xEB:
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


        case 0x07: {return {ZeroPage,  SLO,    2, 3};}
        case 0x0F: {return {Absolute,  SLO,    3, 4};}
        case 0x1F: {return {AbsoluteX,  SLO,    3, 5};}
        case 0x1B: {return {AbsoluteY,  SLO,    3, 5};}
        case 0x03: {return {IndexedIndirectX, SLO,  2, 6};}
        case 0x13: {return {IndexedIndirectY, SLO,  2, 6};}

        case 0x17: {return {ZeroPageX,  SLO,    2, 4};}

        case 0x27: {return {ZeroPage,  RLA,    2, 3};}
        case 0x2F: {return {Absolute,  RLA,    3, 4};}
        case 0x3F: {return {AbsoluteX,  RLA,    3, 5};}
        case 0x3B: {return {AbsoluteY,  RLA,    3, 5};}
        case 0x23: {return {IndexedIndirectX, RLA,  2, 6};}
        case 0x33: {return {IndexedIndirectY, RLA,  2, 6};}

        case 0x37: {return {ZeroPageX,  RLA,    2, 4};}

        case 0x47: {return {ZeroPage,  SRE,    2, 3};}
        case 0x4F: {return {Absolute,  SRE,    3, 4};}
        case 0x5F: {return {AbsoluteX,  SRE,    3, 5};}
        case 0x5B: {return {AbsoluteY,  SRE,    3, 5};}
        case 0x57: {return {ZeroPageX,  SRE,    2, 4};}
        case 0x43: {return {IndexedIndirectX, SRE,  2, 6};}
        case 0x53: {return {IndexedIndirectY, SRE,  2, 6};}

        case 0x67: {return {ZeroPage,  RRA,    2, 5};}
        case 0x6F: {return {Absolute,  RRA,    3, 5};} // TODO: check
        case 0x7F: {return {AbsoluteX,  RRA,    3, 5};} // TODO: check
        case 0x7B: {return {AbsoluteY,  RRA,    3, 5};} // TODO: check
        case 0x63: {return {IndexedIndirectX, RRA,  2, 6};}
        case 0x73: {return {IndexedIndirectY, RRA,  2, 6};}

        case 0x77: {return {ZeroPageX,  RRA,    2, 6};}

        case 0x87: {return {ZeroPage,  AAX,    2, 3};}
        case 0x8F: {return {Absolute,  AAX,    3, 4};}
        case 0x97: {return {ZeroPageY,  AAX,    2, 4};}
        case 0x83: {return {IndexedIndirectX, AAX,  2, 6};}

        case 0xA7: {return {ZeroPage,  LAX,    2, 3};}
        case 0xAF: {return {Absolute,  LAX,    3, 4};}
        case 0xBF: {return {AbsoluteY,  LAX,    3, 5};}
        case 0xB7: {return {ZeroPageY,  LAX,    2, 4};}
        case 0xA3: {return {IndexedIndirectX, LAX,  2, 6};}
        case 0xB3: {return {IndexedIndirectY, LAX,  2, 6};}

        case 0xC7: {return {ZeroPage,  DCP,    2, 3};}
        case 0xCF: {return {Absolute,  DCP,    3, 4};}
        case 0xDF: {return {AbsoluteX,  DCP,    3, 5};}
        case 0xDB: {return {AbsoluteY,  DCP,    3, 5};}
        case 0xC3: {return {IndexedIndirectX, DCP,  2, 6};}
        case 0xD3: {return {IndexedIndirectY, DCP,  2, 6};}

        case 0xD7: {return {ZeroPageX,  DCP,    2, 4};}
        case 0xE7: {return {ZeroPage,  ISC,    2, 3};}
        case 0xEF: {return {Absolute,  ISC,    3, 4};}
        case 0xFF: {return {AbsoluteX,  ISC,    3, 5};}
        case 0xFB: {return {AbsoluteY,  ISC,    3, 5};}
        case 0xF7: {return {ZeroPageX,  ISC,    2, 4};}
        case 0xE3: {return {IndexedIndirectX, ISC,  2, 6};}
        case 0xF3: {return {IndexedIndirectY, ISC,  2, 6};}

        case 0x9C: {return {AbsoluteX,  SYA,    3, 5};}
        case 0x9E: {return {AbsoluteY,  SXA,    3, 5};}

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
            {return {ZeroPage, DOP, 2, 2};}

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
            {return {AbsoluteX, TOP, 3, 5};}

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

CPU::AddressingResult CPU::get_arg_pc(Bus& bus, AddressingMode am, bool cross_check) {
    auto arg1 = mem.read(bus,pc + 1);
    auto arg2 = mem.read(bus,pc + 2);
    unsigned short args_as_addr = ((arg1) | arg2 << 8);
    switch(am){
        case ZeroPageX: return {(unsigned short)((arg1 + x) % 256), false, false};
        case ZeroPageY: return {(unsigned short)((arg1 + y) % 256), false, false};
        case AbsoluteX: {
            unsigned short addr = args_as_addr + x;
            if (cross_check && (arg2 != ((addr >> 8) & 0xff))){
                return {args_as_addr, true, false, x};
            }
            return {addr, false, false, 0};
        }
        case AbsoluteY:{
            unsigned short addr = args_as_addr + y;
            if (cross_check && (arg2 != ((addr >> 8) & 0xff))){
                return {args_as_addr, true, false, y};
            }
            return {addr, false, false, 0};
        }
        case IndexedIndirectX: return
                    {(unsigned short)(mem.read(bus,(arg1 + x) % 256) + mem.read(bus,(arg1 + x + 1) % 256)* 256), false, false};
        case IndexedIndirectY: {
            unsigned char l = mem.read(bus,arg1);
            unsigned char h = mem.read(bus,(arg1 + 1) % 256);
            unsigned short addr = (l | (h << 8));
            unsigned short res_addr = addr + y;
            if (cross_check && (h != ((res_addr >> 8) & 0xff))){
                return {addr, true, false, y};
            }
            return {res_addr, false, false, 0};
        }
        case Immediate: return { (unsigned short)(pc + 1), false, false};
        case ZeroPage: return {arg1, false, false};
        case Absolute: return {  args_as_addr, false, false};
        case Relative: return { (unsigned short)(pc + 1), false, false};
        case Accumulator: return {0, false, true};
    }
    throw std::exception();
}

CPU::AddressingResult CPU::get_arg_by_addr(Bus &bus, CPU::AddressingMode am, unsigned short pc_addr) {
    auto arg1 = mem.read(bus,pc_addr + 1);
    auto arg2 = mem.read(bus,pc_addr + 2);
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
                    {(unsigned short)(mem.read(bus,(arg1 + x) % 256) + mem.read(bus,(arg1 + x + 1) % 256)* 256), false, false};
        case IndexedIndirectY: {
            unsigned char l = mem.read(bus,arg1);
            unsigned char h = mem.read(bus,(arg1 + 1) % 256);
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

void CPU::handle_interrupt(Bus& bus, Interrupt i) {
    mem.write(bus,0x100 + sp, (pc >> 8) & 0xff); --sp;
    mem.write(bus,0x100 + sp, pc & 0xff); --sp;
    sr.set(StatusReg::B, false);
    sr.set(StatusReg::Unused, true);
    mem.write(bus,0x100 + sp, sr.reg); --sp;
    sr.set(StatusReg::I, true);
    pc = (mem.read(bus, interrupt_addresses[i]) | (mem.read(bus, interrupt_addresses[i] + 1) << 8));
    interrupt_requests[i] = false;
}

CPU::AddressingResult CPU::dummy_read(Bus& bus, AddressingResult arg) {
     // TODO: check
     mem.read(bus, (arg.addr & 0xff00) | (arg.addr + arg.add_to_addr) & 0xff);
     arg.addr = arg.addr + arg.add_to_addr;
     return (arg.addr = (((((arg.addr & 0xff00) >> 8) + 1) << 8) | arg.addr & 0xff), arg); // update address
}

void CPU::tick(Bus& bus){
#if DEBUG_MODE
    ++cycles_count;
#endif
#if UI_DEBUGGER
    if (!debug_run) return;
#endif
    if (!execute_in_progress) {
        if (bus.ppustatus.reg & 0x80 && bus.ppuctrl.reg & 0x80){
            interrupt_requests[NMI] = true;
        }
        if (bus.dma_active){
            // Every cpu cycle
            bus.dma_tick(mem.read(bus, bus.dma_address + bus.dma_index));
            return;
        }

        if (!sr.get(StatusReg::I)){
            // TODO:
            // The NES has an interrupt latency of 7 cycles, which means it takes 7 CPU cycles to begin
            // executing the interrupt handler
            for (int i = 0; i < InterruptsCount; ++i){
                if (interrupt_requests[i]){
                    if (!interrupt_handling_in_progress) {
                        interrupt_handling_cycle = 6;
                        interrupt_handling_in_progress = true;
                    } else if (!(--interrupt_handling_cycle)) {
                        handle_interrupt(bus, static_cast<Interrupt>(i));
                        interrupt_handling_in_progress = false;
                    }
                    break;
                }
            }
        }

        if (interrupt_handling_in_progress) return;

        di = decode(mem.read(bus, pc));
        execute_in_progress = true;
        --di.cycles;
    }

    if (di.cycles){
        --di.cycles;
        return;
    }

    if (instruction_execute_done) {
        --additional_cycles;
        if (additional_cycles){
            return;
        }
        execute_in_progress = false;
        instruction_execute_done = false;
#if UI_DEBUGGER
        debug_run = false;
#endif
        return;
    }

    switch(di.i) {
        case ADC: {
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val = read(bus, arg);
            unsigned short res = a + val + sr.get(StatusReg::C);
            sr.set(StatusReg::C, res > 0xff);
            sr.set(StatusReg::Z, (res & 0xff) == 0);
            sr.set(StatusReg::V, (res ^ a) & (res ^ val) & 0x80);
            sr.set(StatusReg::N, res & 0x80);
            a = res & 0xff;
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case AND:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val =  read(bus, arg);
            a &= val;
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case ASL:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = val << 1;
            sr.set(StatusReg::C, val & 0x80);
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            write(bus, arg, res & 0xff);
            pc += di.bytes;
            break;
        }
        case SLO:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = val << 1;
            a |= res;
            sr.set(StatusReg::C, val & 0x80);
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            write(bus, arg, res & 0xff);
            pc += di.bytes;
            break;
        }
        case RLA:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = (val << 1) | sr.get(StatusReg::C);
            a &= res;
            sr.set(StatusReg::C, val & 0x80);
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            write(bus, arg, res & 0xff);
            pc += di.bytes;
            break;
        }
        case SRE:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = val >> 1;
            a ^= res;
            sr.set(StatusReg::C, val & 0x1);
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            write(bus, arg, res & 0xff);
            pc += di.bytes;
            break;
        }
        case RRA:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            unsigned char ror_res = (val >> 1) | (sr.get(StatusReg::C) << 7);
            sr.set(StatusReg::C, val & 0x1);
            sr.set(StatusReg::Z, ror_res == 0);
            sr.set(StatusReg::N, ror_res & 0x80);
            write(bus, arg, ror_res);
            val = read(bus, arg);
            unsigned short res = (a + val + sr.get(StatusReg::C));
            sr.set(StatusReg::C, res > 0xff);
            sr.set(StatusReg::Z, (res & 0xff) == 0);
            sr.set(StatusReg::V, (res ^ a) & (res ^ val) & 0x80);
            sr.set(StatusReg::N, res & 0x80);
            a = res & 0xff;
            pc += di.bytes;
            break;
        }
        case AAX:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            write(bus, arg, a & x);
            pc += di.bytes;
            break;
        }
        case LAX:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            a = val;
            x = val;
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            break;
        }
        case DCP:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            unsigned char res = val - 1;
            sr.set(StatusReg::C, a >= res);
            sr.set(StatusReg::Z, a == res);
            sr.set(StatusReg::N, (a - res) & 0x80);
            write(bus, arg, res & 0xff);
            pc += di.bytes;
            break;
        }
        case ISC:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            write(bus, arg, val + 1);
            val = read(bus, arg);
            int res = a - val - (1 - sr.get(StatusReg::C));
            sr.set(StatusReg::C, res >= 0);
            sr.set(StatusReg::Z, (res & 0xff) == 0);
            sr.set(StatusReg::V, (((res & 0xff) ^ a) & ((res & 0xff) ^ (~val))) & 0x80);
            sr.set(StatusReg::N, res & 0x80);
            a = res & 0xff;
            pc += di.bytes;
            break;
        }
        case SYA:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char res = y & (((arg.addr >> 8) & 0xff) + 1);
            if (res > 0)
                write(bus, arg, res);
            pc += di.bytes;
            break;
        }
        case SXA:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char res = x & (((arg.addr >> 8) & 0xff) + 1);
            if (res > 0)
                write(bus, arg, res);
            pc += di.bytes;
            break;
        }
        case BCC:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (!sr.get(StatusReg::C)){
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }

            break;
        }
        case BCS:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (sr.get(StatusReg::C)){
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }
            break;
        }
        case BEQ:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (sr.get(StatusReg::Z)){
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }

            break;
        }
        case BMI: {
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (sr.get(StatusReg::N)){
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }

            break;
        }
        case BNE:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (!sr.get(StatusReg::Z)) {
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }

            break;
        }
        case BPL:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (!sr.get(StatusReg::N)){
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }

            break;
        }
        case BVC:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (!sr.get(StatusReg::V)){
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }

            break;
        }
        case BVS:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            pc += di.bytes;
            if (sr.get(StatusReg::V)){
                pc += (char)(read(bus, arg));
                additional_cycles = 1 + (arg.page_crossed);
            }
            break;
        }

        case BIT:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val = read(bus, arg);
            unsigned char res = a & val;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::V, val & 0x40);
            sr.set(StatusReg::N, val & 0x80);
            pc += di.bytes;
            break;
        }
        case BRK:{
            if (!interrupt_requests[IRQ]){
                sr.set(StatusReg::I, false);
                interrupt_requests[IRQ] = true;
            }
            break;}

        case CLC:{sr.set(StatusReg::C, 0); pc+=1; break;}
        case CLD:{sr.set(StatusReg::D, 0); pc+=1; break;}
        case CLI:{sr.set(StatusReg::I, 0); pc+=1; break;}
        case CLV:{sr.set(StatusReg::V, 0); pc+=1; break;}
        case CMP:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val =  read(bus, arg);
            unsigned char res = a - val;
            sr.set(StatusReg::C, a >= val);
            sr.set(StatusReg::Z, a == val);
            sr.set(StatusReg::N, res & 0x80);
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case CPX:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = x - val;
            sr.set(StatusReg::C, x >= val);
            sr.set(StatusReg::Z, x == val);
            sr.set(StatusReg::N, res & 0x80);
            pc += di.bytes;
            break;
        }
        case CPY:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = y - val;
            sr.set(StatusReg::C, y >= val);
            sr.set(StatusReg::Z, y == val);
            sr.set(StatusReg::N, res & 0x80);
            pc += di.bytes;
            break;
        }
        case DEC:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = val - 1;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            write(bus, arg, res);
            pc += di.bytes;
            break;
        }
        case DEX:{
            unsigned char res = x - 1;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            x = res;
            pc += di.bytes;
            break;
        }
        case DEY:{
            unsigned char res = y - 1;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            y = res;
            pc += di.bytes;
            break;
        }
        case EOR:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val =  read(bus, arg);
            unsigned char res = a ^ val;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            a = res;
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case INC:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            unsigned char val =  read(bus, arg);
            unsigned char res = val + 1;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            write(bus, arg, res);
            pc += di.bytes;
            break;
        }
        case INX:{
            unsigned char res = x + 1;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            x = res;
            pc += di.bytes;
            break;
        }
        case INY:{
            unsigned char res = y + 1;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            y = res;
            pc += di.bytes;
            break;
        }
        case JMP:{
            if (di.addr_mode == Absolute){
                pc = (mem.read(bus,pc + 1) | (mem.read(bus,pc + 2) << 8));
            } else { // Indirect
                unsigned short arg_addr = (mem.read(bus,pc + 1) | (mem.read(bus,pc + 2) << 8));
                unsigned char l = mem.read(bus,arg_addr);
                unsigned char h = mem.read(bus,(arg_addr & 0xff) == 0xff ? (arg_addr & 0xff00) : arg_addr + 1);
                pc = l | (h << 8);
            }
            break;
        }
        case JSR:{
            // TODO: maybe check
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            mem.write(bus,0x100 + sp, ((pc + 2) >> 8) & 0xff); --sp;
            mem.write(bus,0x100 + sp, (pc + 2) & 0xff); --sp;
            pc = arg.addr;
            break;
        }
        case LDA:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            a = read(bus, arg);
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case LDX:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            x = read(bus, arg);
            sr.set(StatusReg::Z, x == 0);
            sr.set(StatusReg::N, x & 0x80);
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case LDY:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            y = read(bus, arg);
            sr.set(StatusReg::Z, y == 0);
            sr.set(StatusReg::N, y & 0x80);
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case LSR:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val =  read(bus, arg);
            unsigned char res = val >> 1;
            sr.set(StatusReg::C, val & 0x1);
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, 0);
            write(bus, arg, res);
            pc += di.bytes;
            break;
        }
        case ASR: {
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            unsigned char res = a & val;
            unsigned char shifted_res = res >> 1;
            sr.set(StatusReg::C, res & 0x1);
            sr.set(StatusReg::Z, shifted_res == 0);
            sr.set(StatusReg::N, shifted_res & 0x80);
            a = shifted_res;
            pc += di.bytes;
            break;
        }
        // TODO: check
        case ATX: {
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            a = val;
            x = a;
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            break;
        }

        case AXS: {
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            int tmp = (a & x) - val;
            sr.set(StatusReg::C, tmp >= 0);
            sr.set(StatusReg::Z, (tmp & 0xff) == 0);
            sr.set(StatusReg::N, tmp & 0x80);
            x = tmp & 0xff;
            pc += di.bytes;
            break;
        }

        case ARR: {
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            a = (a & val);
            a = (a >> 1) | (sr.get(StatusReg::C) << 7);
            sr.set(StatusReg::C, (a >> 6) & 0x1);
            sr.set(StatusReg::V, ((a >> 6) & 0x1) ^  ((a >> 5) & 0x1));
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            break;
        }
        case AAC:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val =  read(bus, arg);
            a &= val;
            sr.set(StatusReg::C, a & 0x80);
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            break;
        }

        case NOP:
        case DOP:
        case TOP:{pc+=di.bytes; break;}

        case ORA:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val =  read(bus, arg);
            unsigned char res = a | val;
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            a = res;
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case PHA:{
            mem.write(bus,0x100 + sp, a); --sp;
            pc += di.bytes;
            break;
        }
        case PHP:{
            mem.write(bus,0x100 + sp, sr.get_status()); --sp;
            pc += di.bytes;
            break;
        }
        case PLA:{
            ++sp; a = mem.read(bus,0x100 + sp);
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            break;
        }
        case PLP:{
            ++sp;
            // TODO: Interrupt disable flag The effect of changing this flag is delayed 1 instruction.
            sr.set_status(mem.read(bus,0x100 + sp));
            pc += di.bytes;
            break;
        }
        case ROL:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            bool prev_c = sr.get(StatusReg::C);
            unsigned char res = (val << 1) | prev_c;
            sr.set(StatusReg::C, val & 0x80);
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            write(bus, arg, res);
            pc += di.bytes;
            break;
        }
        case ROR:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            unsigned char val = read(bus, arg);
            bool prev_c = sr.get(StatusReg::C);
            unsigned char res = (val >> 1) | (prev_c << 7) ;
            sr.set(StatusReg::C, val & 0x1);
            sr.set(StatusReg::Z, res == 0);
            sr.set(StatusReg::N, res & 0x80);
            write(bus, arg, res);
            pc += di.bytes;
            break;
        }
        case RTI:{
            ++sp; sr.set_status(mem.read(bus,0x100 + sp));
            ++sp; unsigned char l = mem.read(bus,0x100 + sp);
            ++sp; unsigned char h = mem.read(bus,0x100 + sp);
            pc = l | (h << 8);
            break;
        }
        case RTS:{
            ++sp; unsigned char l = mem.read(bus,0x100 + sp);
            ++sp; unsigned char h = mem.read(bus,0x100 + sp);
            pc = l | (h << 8);
            ++pc;
            break;
        }
        case SBC:{
            auto arg = get_arg_pc(bus, di.addr_mode, true);
            if (arg.page_crossed) arg = dummy_read(bus, arg);
            unsigned char val = read(bus, arg);
            bool prev_c = sr.get(StatusReg::C);
            int res = (a - val - (!prev_c));
            sr.set(StatusReg::C, res >= 0);
            sr.set(StatusReg::Z, (res & 0xff) == 0);
            sr.set(StatusReg::V, (((res & 0xff) ^ a) & ((res & 0xff) ^ (~val))) & 0x80);
            sr.set(StatusReg::N, res & 0x80);
            a = res & 0xff;
            pc += di.bytes;
            if (arg.page_crossed) additional_cycles = 1;
            break;
        }
        case SEC:{sr.set(StatusReg::C, true); pc+=di.bytes; break;}
        case SED:{sr.set(StatusReg::D, true); pc+=di.bytes; break;}
        case SEI:{sr.set(StatusReg::I, true); pc+=di.bytes; break;}
        case STA:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            write(bus, arg, a);
            pc += di.bytes;
            break;
        }
        case STX:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            write(bus, arg, x);
            pc += di.bytes;
            break;
        }
        case STY:{
            auto arg = get_arg_pc(bus, di.addr_mode, false);
            write(bus, arg, y);
            pc += di.bytes;
            break;
        }
        case TAX:{
            x = a;
            sr.set(StatusReg::Z, x == 0);
            sr.set(StatusReg::N, x & 0x80);
            pc += di.bytes;
            break;
        }
        case TAY:{
            y = a;
            sr.set(StatusReg::Z, y == 0);
            sr.set(StatusReg::N, y & 0x80);
            pc += di.bytes;
            break;
        }
        case TSX:{
            x = sp;
            sr.set(StatusReg::Z, x == 0);
            sr.set(StatusReg::N, x & 0x80);
            pc += di.bytes;
            break;
        }
        case TXA:{
            a = x;
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            break;
        }
        case TXS:{
            sp = x;
            pc += di.bytes;
            break;
        }
        case TYA:{
            a = y;
            sr.set(StatusReg::Z, a == 0);
            sr.set(StatusReg::N, a & 0x80);
            pc += di.bytes;
            break;
        }
    }
    instruction_execute_done = true;
    if (!additional_cycles) {
        execute_in_progress = false;
        instruction_execute_done = false;
#if UI_DEBUGGER
        debug_run = false;
#endif
    }
}

unsigned char CPU::read(Bus &bus, AddressingResult ar) {
    if (ar.is_accumulator) return a;
    return mem.read(bus, ar.addr);
}

void CPU::write(Bus& bus, AddressingResult ar, unsigned char value){
    if (ar.is_accumulator) {a = value; return;}
    mem.write(bus, ar.addr, value);
}

#include "CPU.hpp"

bool CPU::ProcessorStatusReg::read_p(CPU::ProcessorStatusReg::Flag flag) {
    return (reg >> flag) & 1;
}

void CPU::ProcessorStatusReg::set_p(CPU::ProcessorStatusReg::Flag flag, bool value) {
    if (value)
        reg |= (1 << flag);
    else 
        reg &= ~(1 << flag);
}
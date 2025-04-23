#include <iostream>
#include <array>
#include <limits>
#include <cassert>

enum Registers {R0, R1, R2, R3, R4, R5, R6, R7};
enum Instructions {ADD, AND, BR, JMP, JSR, JSRR, LD, LDI, LDR, LEA, NOT, RET, ST, STI, STR, TRAP};
constexpr std::string_view mnemonics[] = {"ADD", "AND", "BR", "JMP", "JSR", "JSRR", "LD", "LDI", "LDR", "LEA", "NOT", "RET", "ST", "STI", "STR", "TRAP"};
constexpr std::string_view reg_str[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7"};

struct Parser {
    static auto instruction(unsigned short i) {
        return (i >> 12);
    }
    static auto destination(unsigned short i) {
        return (((0b111 << 9) & i) >> 9);
    }
    static auto source1(unsigned short i) {
        return (((0b111 << 6) & i) >> 6);
    }

    static auto source2(unsigned short i) {
        return (0b111 & i);
    }

    static auto offset6(unsigned short i) {
        return (0b111111 & i);
    }

    static auto offset9(unsigned short i) {
        return (0b111111111 & i);
    }

    static auto offset11(unsigned short i) {
        return (0b11111111111 & i);
    }
};

struct LC3 {
    unsigned short reg[8]{};
    std::array<unsigned short, std::numeric_limits<unsigned short>::max() + 1> mem{};
    unsigned short* pc = mem.data();
    void execute(unsigned short i) {
        switch(Parser::instruction(i)) {
            case LD:  {reg[Parser::destination(i)] = mem[Parser::offset9(i)]; break;}
            case LDI: {reg[Parser::destination(i)] = pc[Parser::offset9(i)]; break;}
        }
    }
};

int main() {
    unsigned short i = 0b0000001010000000;
    assert(Parser::instruction(i) == ADD);
    assert(Parser::destination(i) == R1);
    assert(Parser::source1(i) == R2);
}
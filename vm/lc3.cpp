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

    static auto source(unsigned short i) {
        return (((0b111 << 9) & i) >> 9);
    }

    static auto source1(unsigned short i) {
        return (((0b111 << 6) & i) >> 6);
    }

    static auto baseR(unsigned short i) {
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

    static short signed_extend(unsigned short i, int n) {
        if ((i >> (n - 1)) & 1) {
            return i | (~((1 << n) - 1));
        } else {
            return i;
        }
    }

    static auto offset11(unsigned short i) {
        return (0b11111111111 & i);
    }
};

struct LC3 {
    unsigned short reg[8]{};
    std::array<unsigned short, std::numeric_limits<unsigned short>::max() + 1> mem{};
    unsigned short pc{};
    void execute(unsigned short i) {
        switch(Parser::instruction(i)) {
            // mem
            case LD:  {
                auto address = pc + Parser::signed_extend(i, 9);
                reg[Parser::destination(i)] = mem[address]; 
                break;
            }
            case LDI: {
                auto address1 = pc + Parser::signed_extend(i, 9);
                reg[Parser::destination(i)] = mem[mem[address1]];
                break;
            }
            case LDR: {
                reg[Parser::destination(i)] = mem[Parser::signed_extend(i, 6) + reg[Parser::baseR(i)]];
                break;
            }
            case LEA: {
                reg[Parser::destination(i)] = pc + Parser::signed_extend(i, 9);
                break;
            }
            case ST: {
                mem[pc + Parser::signed_extend(i, 9)] = reg[Parser::source(i)];
                break;
            }
            case STI: {
                auto address = pc + Parser::signed_extend(i, 9);
                mem[mem[address]] = reg[Parser::destination(i)];
                break;
            }
            case STR: {
                mem[Parser::signed_extend(i, 6) + reg[Parser::baseR(i)]] = reg[Parser::source(i)];
                break;
            }
        }
    }
};

int main() {
    unsigned short i = 0b0000001010000000;
    assert(Parser::instruction(i) == ADD);
    assert(Parser::destination(i) == R1);
    assert(Parser::source1(i) == R2);
    assert(Parser::signed_extend(0b011111111, 9) == 255);
    assert(Parser::signed_extend(0b111000000, 9) == -64);
    assert(Parser::signed_extend(0b100000000, 9) == -256);
}
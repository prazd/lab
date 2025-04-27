#include <iostream>
#include <array>
#include <limits>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <vector>
#include <string_view>

enum Registers {R0, R1, R2, R3, R4, R5, R6, R7};
enum Instructions {ADD = 0b0001, AND = 0b0101, BR = 0b0000, JMP = 0b1100, JSR = 0b0100, LD = 0b0010, LDI = 0b1010, LDR = 0b0110, LEA = 0b1110, NOT = 0b1001, RET = 0b1100, ST = 0b0011, STI = 0b1011, STR = 0b0111, TRAP = 0b1111};
std::unordered_map<int, std::string_view> mnemonics = 
    { {ADD, "ADD"},
      {AND, "AND"}, 
      {BR, "BR"}, 
      {JMP, "JMP"}, 
      {JSR, "JSR"},  
      {LD, "LD"}, 
      {LDI, "LDI"}, 
      {LDR, "LDR"}, 
      {LEA, "LEA"}, 
      {NOT, "NOT"}, 
      {RET, "RET"}, 
      {ST, "ST"}, 
      {STI, "STI"}, 
      {STR, "STR"}, 
      {TRAP, "TRAP"}
    };    

constexpr std::string_view reg_str[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7"};

struct Parser {
    static unsigned short instruction(unsigned short i) {
        return (i >> 12);
    }
    static unsigned short destination(unsigned short i) {
        return (((0b111 << 9) & i) >> 9);
    }

    static unsigned short source(unsigned short i) {
        return (((0b111 << 9) & i) >> 9);
    }

    static unsigned short source1(unsigned short i) {
        return (((0b111 << 6) & i) >> 6);
    }

    static unsigned short source2(unsigned short i) {
        return (0b111 & i);
    }

    static unsigned short baseR(unsigned short i) {
        return (((0b111 << 6) & i) >> 6);
    }

    static bool bit_one(unsigned short i, int n) {
        return ((1 << n) & i);
    }

    static short nbits_sign_extend(short i, int n) {
        i &= (1 << n) - 1;
        if ((i >> (n - 1)) & 1) {
            return i | (~((1 << n) - 1));
        } else {
            return i;
        }
    }

    static unsigned short n_bits(unsigned short i, int n) {
        return i & ((1 << n) - 1);
    }

    static unsigned short nzp(unsigned short i) {
        return ((0b111) & (i >> 9));
    }

    static bool nzp_g_n(unsigned short i) {
        return ((i >> 2) & 1);
    }

    static bool nzp_g_z(unsigned short i) {
        return ((i >> 1) & 1);
    }

    static bool nzp_g_p(unsigned short i) {
        return (i & 1);
    }
};

struct LC3 {
    unsigned short reg[8]{};
    std::array<unsigned short, std::numeric_limits<unsigned short>::max() + 1> mem{};
    unsigned short pc{};
    enum {N = 0, Z = 1, P = 2};
    std::array<bool, 3> nzp{};

    void update_nzp(short value) {
        nzp = {};
        if (value > 0)
            nzp[P] = 1;
        else if (value < 0)
            nzp[N] = 1;
        else 
            nzp[Z] = 1;
    }
    
    void execute(unsigned short i) {
        ++pc;
        switch(Parser::instruction(i)) {
            // memory
            case LD:  {
                auto address = pc + Parser::nbits_sign_extend(i, 9);
                reg[Parser::destination(i)] = mem[address]; 
                break;
            }
            case LDI: {
                auto address1 = pc + Parser::nbits_sign_extend(i, 9);
                reg[Parser::destination(i)] = mem[mem[address1]];
                break;
            }
            case LDR: {
                reg[Parser::destination(i)] = mem[Parser::nbits_sign_extend(i, 6) + reg[Parser::baseR(i)]];
                break;
            }
            case LEA: {
                reg[Parser::destination(i)] = pc + Parser::nbits_sign_extend(i, 9);
                break;
            }
            case ST: {
                mem[pc + Parser::nbits_sign_extend(i, 9)] = reg[Parser::source(i)];
                break;
            }
            case STI: {
                auto address = pc + Parser::nbits_sign_extend(i, 9);
                mem[mem[address]] = reg[Parser::destination(i)];
                break;
            }
            case STR: {
                mem[Parser::nbits_sign_extend(i, 6) + reg[Parser::baseR(i)]] = reg[Parser::source(i)];
                break;
            }
            // arithmetic
            case ADD: {
                if (Parser::bit_one(i, 5))
                    reg[Parser::destination(i)] = reg[Parser::source1(i)] + Parser::nbits_sign_extend(i, 5);
                else
                    reg[Parser::destination(i)] = reg[Parser::source1(i)] + reg[Parser::source2(i)];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            case AND: {
                if (Parser::bit_one(i, 5))
                    reg[Parser::destination(i)] = (reg[Parser::source1(i)] & Parser::nbits_sign_extend(i, 5));
                else
                    reg[Parser::destination(i)] = (reg[Parser::source1(i)] & reg[Parser::source2(i)]);
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            case NOT: {
                reg[Parser::destination(i)] = ~reg[Parser::source(i)];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            // control flow
            case BR: {
                auto nzp_i = Parser::nzp(i);
                if ((nzp[N] && Parser::nzp_g_n(nzp_i)) || 
                    (nzp[Z] && Parser::nzp_g_z(nzp_i)) || 
                    (nzp[P] && Parser::nzp_g_p(nzp_i))) {
                    pc += Parser::nbits_sign_extend(i, 9);
                }
                break;
            }
            case JMP: {
                pc = reg[Parser::baseR(i)];
                break;
            }
            case JSR: {
                if (Parser::bit_one(i, 11))
                    reg[R7] = std::exchange(pc, pc + Parser::nbits_sign_extend(i, 11));
                else
                    reg[R7] = std::exchange(pc, reg[Parser::baseR(i)]); // JSRR
                break;
            }
            // todo: trap
        }
    }

    void load_instructions(const auto& instructions) {
        auto counter = 0x3000;
        for (auto el: instructions)
            mem[counter++] = el;
        pc = 0x3000;
    }
public:
    void run(const auto& instructions) {
        load_instructions(instructions);
        while (mem[pc]) {execute(mem[pc]);};
    }

    unsigned short read_mem(unsigned short index) const {
        return mem[index];
    }

    unsigned short read_pc() const {
        return pc;
    }

    unsigned short read_r1() const {
        return reg[R1];
    }
    void print_regs() {
        for (size_t i = 0; i < 8; ++i)
            std::cout << reg_str[i] << ":" << reg[i] << '\n';
    }
};



// constexpr std::string_view raw = R"(
//         ADD R1, R0, #1
//         ADD R1, R1, #4
//         ADD R2, R0, #3
//         ADD R2, R2, #2
//         ADD R2, R2, #2
//         ADD R3, R0, #10
//         ADD R1, R1, R2
//         ADD R1, R1, R3
//         ST R1, PC+1
// )";

int main() {
    std::array<unsigned short, 10> program = {
        0b0001'001'000'1'00001, // ADD R1, R0, #1
        0b0001'001'001'1'00100, // ADD R1, R1, #4
    
        0b0001'010'000'1'00011, // ADD R2, R0, #3
        0b0001'010'010'1'00010, // ADD R2, R2, #2
        0b0001'010'010'1'00010, // ADD R2, R2, #2
    
        0b0001'011'000'1'01010, // ADD R3, R0, #10
    
        0b0001'001'001'0'00'010, // ADD R1, R1, R2
        0b0001'001'001'0'00'011, // ADD R1, R1, R3
    
        0b0011'001'100000001,    // ST R1, PC+1
        0b00000000000000000     // пустышка
    };
    // to_instructions(raw);
    LC3 vm;
    vm.run(program);
    assert(vm.read_mem(12042) == 22);
}
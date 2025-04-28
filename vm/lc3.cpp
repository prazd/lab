#include <iostream>
#include <array>
#include <limits>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <vector>
#include <string_view>
#include <iomanip>

#define DEBUG 1

enum Registers {R0, R1, R2, R3, R4, R5, R6, R7};
enum Instructions: unsigned short {ADD = 0b0001, AND = 0b0101, BR = 0b0000, JMP = 0b1100, JSR = 0b0100, LD = 0b0010, LDI = 0b1010, LDR = 0b0110, LEA = 0b1110, NOT = 0b1001, RET = 0b1100, ST = 0b0011, STI = 0b1011, STR = 0b0111, TRAP = 0b1111};
std::unordered_map<std::string_view, unsigned short> mnemonics = 
    { {"ADD", ADD},
      {"AND", AND}, 
      {"BR",   BR}, 
      {"JMP", JMP}, 
      {"JSR", JSR},  
      {"LD",   LD}, 
      {"LDI", LDI}, 
      {"LDR", LDR}, 
      {"LEA", LEA}, 
      {"NOT", NOT}, 
      {"RET", RET}, 
      {"ST",   ST}, 
      {"STI", STI}, 
      {"STR", STR}, 
      {"TRAP", TRAP}
    };    

#if DEBUG
std::unordered_map<unsigned short, std::string_view> debug_mnemonics = 
    { {ADD, "ADD"},
      {AND, "AND"}, 
      {BR,  "BR"}, 
      {JMP, "JMP"}, 
      {JSR, "JSR"},  
      {LD,  "LD"}, 
      {LDI, "LDI"}, 
      {LDR, "LDR"}, 
      {LEA, "LEA"}, 
      {NOT, "NOT"}, 
      {RET, "RET"}, 
      {ST,  "ST"}, 
      {STI, "STI"}, 
      {STR, "STR"}, 
      {TRAP,"TRAP"}
    };   
#endif  
std::unordered_map<std::string_view, unsigned short> regs = 
    { {"R0", R0},
      {"R1", R1}, 
      {"R2", R2}, 
      {"R3", R3}, 
      {"R4", R4},  
      {"R5", R5}, 
      {"R6", R6}, 
      {"R7", R7},
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
        switch(Parser::instruction(i)) {
            // memory
            case LD:  {
                ++pc;
                auto address = pc + Parser::nbits_sign_extend(i, 9);
                reg[Parser::destination(i)] = mem[address];
                break;
            }
            case LDI: {
                ++pc;
                auto address1 = pc + Parser::nbits_sign_extend(i, 9);
                reg[Parser::destination(i)] = mem[mem[address1]];
                break;
            }
            case LDR: {
                reg[Parser::destination(i)] = mem[Parser::nbits_sign_extend(i, 6) + reg[Parser::baseR(i)]];
                break;
            }
            case LEA: {
                ++pc;
                reg[Parser::destination(i)] = pc + Parser::nbits_sign_extend(i, 9);
                break;
            }
            case ST: {
                ++pc;
                mem[pc + Parser::nbits_sign_extend(i, 9)] = reg[Parser::source(i)];
                break;
            }
            case STI: {
                ++pc;
                auto address = pc + Parser::nbits_sign_extend(i, 9);
                mem[mem[address]] = reg[Parser::destination(i)];
                break;
            }
            case STR: {
                ++pc;
                mem[Parser::nbits_sign_extend(i, 6) + reg[Parser::baseR(i)]] = reg[Parser::source(i)];
                break;
            }
            // arithmetic
            case ADD: {
                ++pc;
                if (Parser::bit_one(i, 5))
                    reg[Parser::destination(i)] = reg[Parser::source1(i)] + Parser::nbits_sign_extend(i, 5);
                else
                    reg[Parser::destination(i)] = reg[Parser::source1(i)] + reg[Parser::source2(i)];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            case AND: {
                ++pc;
                if (Parser::bit_one(i, 5))
                    reg[Parser::destination(i)] = (reg[Parser::source1(i)] & Parser::nbits_sign_extend(i, 5));
                else
                    reg[Parser::destination(i)] = (reg[Parser::source1(i)] & reg[Parser::source2(i)]);
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            case NOT: {
                ++pc;
                reg[Parser::destination(i)] = ~reg[Parser::source1(i)];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            // control flow
            case BR: {
                ++pc;
                auto nzp_i = Parser::nzp(i);
                if ((nzp[N] && Parser::nzp_g_n(nzp_i)) || 
                    (nzp[Z] && Parser::nzp_g_z(nzp_i)) || 
                    (nzp[P] && Parser::nzp_g_p(nzp_i))) {
                        pc = pc + Parser::nbits_sign_extend(i, 9);
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
        }
    }

    void load_instructions(const auto& instructions) {
        auto counter = 0x3000;
        for (auto el: instructions)
            mem[counter++] = el;

        #if DEBUG    
        for (size_t i = 0; i < instructions.size(); ++i)
            // std::cout << "0x" << std::hex << instructions[i] << ",\n";
        #endif

        pc = 0x3000;
    }
public:
    void run(const auto& instructions) {
        load_instructions(instructions);
        #if DEBUG
        int counter{};
        #endif
        while (mem[pc]) { 
            #if DEBUG
                std::cout << "\n---------------------STEP "<< counter++ << "----------------\n"; 
                std::cout << "Instruction:" << debug_mnemonics[Parser::instruction(mem[pc])] << '\n'; 
                execute(mem[pc]);
                print_regs(); 
                std::cout << "\n--------------------------\n";
            #else 
                execute(mem[pc]);
            #endif
        }
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

std::vector<std::vector<std::string_view>> split(std::string_view raw) {
    std::vector<std::vector<std::string_view>> res;
    std::vector<std::string_view> tmp;

    size_t begin = 0;
    size_t end = 0;
    bool searching = true;

    for (auto el: raw) {
        if (el != ' ' && el != '\n' && searching){
            ++end;
            searching = false;
            continue;
        } else if (searching){
            ++begin;
            ++end;
            continue;
        }

        if (el == ' ' && !searching) {
            tmp.push_back({raw.begin() + begin,  *(raw.begin() + end - 1) == ',' ? raw.begin() + end - 1 : raw.begin() + end});
            ++end;
            begin = end;
            continue;
        }
        
        if (el == '\n' && !searching) {
            tmp.push_back({raw.begin() + begin,  *(raw.begin() + end - 1) == ',' ? raw.begin() + end - 1 : raw.begin() + end});
            ++end;
            begin = end;
            res.push_back(tmp);
            tmp = {};
            searching = true;
            continue;
        }
        ++end;
    }
    return res;

}

bool isnum(char el) {
    constexpr std::array<char, 10> nums{'0','1','2','3','4','5','6','7','8','9'};
    for (auto n: nums){
        if (el == n){
            return true;
        }
    }
    return false;
}

int atoi(std::string_view s) {
    int res{};
    int tmp{};
    bool gz = true;
    bool update = false;
    auto imax = std::numeric_limits<int>::max();
    auto imin = std::numeric_limits<int>::min();
    auto ret_func = [&gz, &res]{return gz ? res : res * -1;};
    for (auto el: s) {
        if (el == '-') {if (update) return ret_func(); update = true; gz = false; continue;};
        if (!isnum(el)) return ret_func();
        tmp = el - '0';
        if (!res) {
            res = tmp;
            update = true;
            continue;
        }
        if (imax / 10 < res || ((imax / 10 == res && tmp > 7)))
            return gz ? imax : imin;
        
        res *= 10;
        res += tmp;
        update = true;
    }
    return ret_func();
}

std::vector<unsigned short> to_instructions(std::string_view raw) {
    auto splited = split(raw);
    std::vector<unsigned short> instructions;
    unsigned short current{};
    for (auto&& raw: splited) {
        auto i = raw[0];
        current = mnemonics[i] << 12; 
        if (i == "AND" || i == "ADD"){
            current |= regs[raw[1]] << 9; // DR
            current |= regs[raw[2]] << 6; // SR1
            if (raw[3][0] == '#') {
                current |= 1 << 5;
                current |= atoi({raw[3].begin() + 1, raw[3].end()}); // imm5
            }
            else{current |= regs[raw[3]];} // SR2
        }
        if (i == "LD" || i == "LDI" || i == "ST" || i == "LEA" || i == "STI") {
            current |= regs[raw[1]] << 9;
            current |= atoi({raw[2].begin() + 1, raw[2].end()}); // offset9
        }

        if (i == "STR" || i == "LDR") {
            current |= regs[raw[1]] << 9; // SR
            current |= regs[raw[2]] << 6; // baseR
            current |= atoi({raw[3].begin() + 1, raw[3].end()}); // offset6
        }

        if (i == "JMP" || i == "JSRR") {
            current |= regs[raw[1]] << 6; // baseR
        }
        if (i == "JSR") {
            current |= 1 << 11;
            current |= atoi({raw[1].begin() + 1, raw[1].end()}); // offset11
        }

        if (i == "NOT") {
            current |= regs[raw[1]] << 9; // DR
            current |= regs[raw[2]] << 6; // SR
        }

        // todo: labels
        if (i[0] == 'B' && i[1] == 'R') {
            current = mnemonics["BR"] << 12; 
            for (int j = 2; j < std::size(i); ++j) {
                if (i[j] == 'n')
                    current |= 1 << 11;
                else if (i[j] == 'z')
                    current |= 1 << 10;
                else if (i[j] == 'p')
                    current |= 1 << 9;
            }
            current |= atoi({raw[1].begin() + 1, raw[1].end()}); // offset9
        }

        
        instructions.push_back(current);
        current = 0;
    }
    return instructions;
}

constexpr std::string_view raw = R"(
        LD R0, #13
        LD R1, #13
        ADD R2, R0, R1
        AND R3, R0, R1
        NOT R4, R2
        BRz #2
        ADD R2, R2, #1
        ADD R5, R5, #0
        JSR #5
        LDR R5, R6, #0
        STR R5, R6, #1
        JMP R7
        LEA R6, #4
        LDI R1, #2
        STI R2, #0
)";

int main() {
    LC3 vm;
    vm.run(to_instructions(raw));
    vm.print_regs();
}
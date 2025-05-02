#include <iostream>
#include <array>
#include <limits>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <vector>
#include <string_view>
#include <iomanip>

#define DEBUG 0
constexpr unsigned short PC_START = 0x3000;
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
   {{"R0", R0},
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
    unsigned short pc = PC_START;
    bool running = false;
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
        if (i == 0xF025) {
            running = false;
            return;
        }
        switch(Parser::instruction(i)) {
            // memory
            case LD:  {
                auto address = pc + Parser::nbits_sign_extend(i, 9);
                reg[Parser::destination(i)] = mem[address];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            case LDI: {
                auto address1 = pc + Parser::nbits_sign_extend(i, 9);
                reg[Parser::destination(i)] = mem[mem[address1]];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            case LDR: {
                reg[Parser::destination(i)] = mem[Parser::nbits_sign_extend(i, 6) + reg[Parser::baseR(i)]];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            case LEA: {
                reg[Parser::destination(i)] = pc + Parser::nbits_sign_extend(i, 9);
                update_nzp(reg[Parser::destination(i)]);
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
                reg[Parser::destination(i)] = ~reg[Parser::source1(i)];
                update_nzp(reg[Parser::destination(i)]);
                break;
            }
            // control flow
            case BR: {
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
        #if DEBUG    
        std::cout << "Instructions(hex):\n";
        for (size_t i = 0; i < instructions.size(); ++i)
            std::cout << "0x" << std::hex << instructions[i] << std::dec << ",\n";
        #endif
        auto counter = PC_START;
        #if DEBUG
        std::cout << "\nADDRESS : INSTRUCTION\n";
        #endif
        for (auto el: instructions) {
            mem[counter++] = el;
            #if DEBUG
            std::cout << " | " << std::hex << "0x" << counter << " | " << "0x" << el << " |\n";
            #endif
        }
        #if DEBUG
        std::cout << std::dec;
        #endif
    }
public:
    void run(const auto& instructions) {
        running = true;
        load_instructions(instructions);
        #if DEBUG
        int counter{};
        #endif
        while (mem[pc]) { 
            #if DEBUG
                std::cout << "\n---------------------STEP "<< counter++ << "----------------------\n"; 
                std::cout << "Instruction:" << debug_mnemonics[Parser::instruction(mem[pc])] << '\n'; 
                execute(mem[pc++]);
                print_regs(); 
            #else 
                execute(mem[pc++]);
            #endif
        }
        #if DEBUG
        std::cout << "\n";
        #endif
        running = false;
    }

    unsigned short read_mem(unsigned short index) const {
        return mem[index];
    }

    unsigned short read_pc() const {
        return pc;
    }

    unsigned short read_reg(int i) const {
        return reg[i];
    }

    void print_regs() {
        for (size_t i = 0; i < 8; ++i)
            std::cout << reg_str[i] << ":" << reg[i] << '\n';
    }
};

std::vector<std::vector<std::string_view>> split(std::string_view raw) {
    std::vector<std::vector<std::string_view>> res;
    res.reserve(std::count(raw.begin(), raw.end(), '\n'));
    std::vector<std::string_view> tmp;
    tmp.reserve(4);

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
            tmp.clear();
            searching = true;
            continue;
        }
        ++end;
    }
    return res;

}

int label_index(const auto& data, std::string_view label) {
    for (int i = 0; i < data.size(); ++i) {
        if (*(data[i][0].end()-1) == ':' && std::equal(data[i][0].begin(), data[i][0].end()-1, label.begin())) {
            return i - 1;
        }
    }
    return -1;
}

std::vector<unsigned short> to_instructions(std::string_view raw) {
    auto splited = split(raw);
    
    std::vector<unsigned short> instructions;
    instructions.reserve(std::count(raw.begin(), raw.end(), '\n'));

    unsigned short current{};

    for (size_t k = 0; k < splited.size(); ++k) {
        auto raw = splited[k];
        
        auto i = raw[0];
        if (i[std::size(raw[0]) - 1] == ':') continue;
        current = mnemonics[i] << 12; 
        if (i == "HALT"){
            current = 0xF025; // HALT
        }
        if (i == "AND" || i == "ADD"){
            current |= regs[raw[1]] << 9; // DR
            current |= regs[raw[2]] << 6; // SR1
            if (raw[3][0] == '#') {
                current |= 1 << 5;
                current |= std::atoi(std::string_view{raw[3].begin() + 1, raw[3].end()}.data()); // imm5
            }
            else{current |= regs[raw[3]];} // SR2
        }
        if (i == "LD" || i == "LDI" || i == "ST" || i == "LEA" || i == "STI") {
            current |= regs[raw[1]] << 9;
            current |= std::atoi(std::string_view{raw[2].begin() + 1, raw[2].end()}.data()); // offset9
        }

        if (i == "STR" || i == "LDR") {
            current |= regs[raw[1]] << 9; // SR
            current |= regs[raw[2]] << 6; // baseR
            current |= std::atoi(std::string_view{raw[3].begin() + 1, raw[3].end()}.data()); // offset6
        }

        if (i == "JMP" || i == "JSRR") {
            current |= regs[raw[1]] << 6; // baseR
        }
        if (i == "JSR") {
            current |= 1 << 11;
            current |= raw[1][0] == '#' ?
                std::atoi(std::string_view{raw[1].begin() + 1, raw[1].end()}.data()) :  // num
                label_index(splited, {raw[1].begin(), raw[1].end()}) - k; // label
        }

        if (i == "NOT") {
            current |= regs[raw[1]] << 9; // DR
            current |= regs[raw[2]] << 6; // SR
        }

    
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
            current |= raw[1][0] == '#' ? 
            std::atoi(std::string_view{raw[1].begin() + 1, raw[1].end()}.data()) : // num
            label_index(splited, {raw[1].begin(), raw[1].end()}) - k; //label
        }
        
        instructions.push_back(current);
        current = 0;
    }
    return instructions;
}

constexpr std::string_view raw = R"(
    LD R0, #13
    LD R1, #12
    LEA R6, #11
    ADD R2, R0, R1
    AND R3, R0, R1
    NOT R4, R2
    BRz SKIP
    ADD R2, R2, #1
    ADD R5, R5, #0
SKIP:
    JSR SUBR
    LDR R5, R6, #0
    STR R5, R6, #1
    JMP R7
    LDI R1, #2
    STI R2, #1
SUBR:
    ST R2, #0
    ADD R0, R2, #0
    NOT R0, R0
    ADD R0, R0, #1
    HALT
)";

int main() {
    LC3 vm;
    vm.run(to_instructions(raw));
    assert(vm.read_reg(R0) == 19455);
    assert(vm.read_reg(R1) == 46081);
    assert(vm.read_reg(R2) == 26627);
    assert(vm.read_reg(R3) == 46081);
    assert(vm.read_reg(R4) == 0);
    assert(vm.read_reg(R5) == 0);
    assert(vm.read_reg(R6) == 12302);
    assert(vm.read_reg(R7) == 12298);
    vm.print_regs();
}
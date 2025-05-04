#include <iostream>
#include <array>
#include <limits>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <vector>
#include <string_view>
#include <iomanip>
#include <cctype>
#include <iterator>

#define DEBUG 0
constexpr unsigned short PC_START = 0x3000;
enum Registers {R0, R1, R2, R3, R4, R5, R6, R7};
enum Instructions: unsigned short {ADD = 0b0001, AND = 0b0101, BR = 0b0000, JMP = 0b1100, JSR = 0b0100, LD = 0b0010, LDI = 0b1010, LDR = 0b0110, LEA = 0b1110, NOT = 0b1001, RET = 0b1100, ST = 0b0011, STI = 0b1011, STR = 0b0111, TRAP = 0b1111};
enum Trapvect8 {TGETC = 0x20, TOUT = 0x21, TPUTS = 0x22, TIN = 0x23, TPUTSP = 0x24, THALT = 0x25};

std::unordered_map<std::string_view, unsigned short> trapvec_table = {
    {"GETC",  TGETC},
    {"OUT",   TOUT},
    {"PUTS",  TPUTS},
    {"IN",    TIN},
    {"PUTSP", TPUTSP},
    {"HALT",  THALT},
};

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
      {"TRAP", TRAP},
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
    static inline unsigned short reg[8]{};
    static inline std::array<unsigned short, std::numeric_limits<unsigned short>::max() + 1> mem{};
    unsigned short pc = PC_START;
    bool running = false;
    enum {N = 0, Z = 1, P = 2};
    std::array<bool, 3> nzp{};

    using trap_func = void(LC3::*)();


    void trap_getc() {
        char s;
        std::cin >> s;
        reg[R0] = static_cast<unsigned short>(s);
    }
    void trap_out() {std::cout << "OUT:" << static_cast<char>(reg[R0] & 0xff);}
    void trap_puts() {
        for (unsigned short i = reg[R0]; mem[i]; ++i) 
            std::cout << static_cast<char>(mem[i] & 0xff);
    }
    void trap_in() {
        char s;
        std::cin >> s;
        std::cout << "\nData:" << s << '\n';
        reg[R0] = static_cast<unsigned short>(s);
    }
    void trap_putsp() {
        for (unsigned short i = reg[R0]; mem[i]; ++i) {
            auto word = mem[i];
            char l = 0xff & word;
            char h = 0xff & (word>>8);
            std::cout << l;
            if (h) std::cout << h;
        }
    }
    void trap_halt() {
        running = false;
    }
    
    const static inline trap_func trap_instructions[6] = {
        &LC3::trap_getc, 
        &LC3::trap_out, 
        &LC3::trap_puts, 
        &LC3::trap_in, 
        &LC3::trap_putsp, 
        &LC3::trap_halt,
    };

    // offset table for trap instruction
    std::unordered_map<unsigned short, unsigned short> trap_offset_table = {
        {TGETC,  0},
        {TOUT,   1},
        {TPUTS,  2},
        {TIN,    3},
        {TPUTSP, 4},
        {THALT,  5},
    };


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

            case TRAP: {
                ((*this).**(((trap_func*)mem.data()) + trap_offset_table[i & 0xff]))();
                break;
            }
        }
    }
    void load_instructions(const auto& instructions) {
        // load TRAP
        std::copy(std::begin(trap_instructions), std::end(trap_instructions), (trap_func*)mem.data());

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


struct Lexer {
    enum TokenType {
        Mnemonic,
        LabelAnnotate,
        LabelUse,
        Immediate,
        Register,
        EndOfLine,
    };

    constexpr static std::string_view tt_str[] = {"Mnemonic", "LabelAnnotate", "LabelUse", "Immediate", "Register", "EndOfLine"};

    struct Token {
        TokenType type;
        std::string_view value;
    };

    static auto lex(std::string_view line) {
        std::vector<Token> result;

        for (size_t i = 0; i < line.size();) {

            if (line[i] == 'R' && std::isdigit(line[i+1])) {
                result.push_back({Register, line.substr(std::exchange(i, i + 2), 2)});
                continue;
            }

            if (std::isalpha(line[i])) {
                auto s = i;
                size_t sz{};
                while(std::isalpha(line[s])){++sz; ++s;};

                if (line[s] == ':') {
                    result.push_back({LabelAnnotate, line.substr(std::exchange(i, i + sz), sz)});
                    continue;
                }

                if (line[s] == ' ' || line[s] == '\n') {
                    if (line[i] == 'B' && line[i + 1] == 'R'){
                        if (mnemonics.contains(line.substr(i, 2)))
                            result.push_back({Mnemonic, line.substr(std::exchange(i, i + sz), sz)});
                        continue;
                    }

                    if (mnemonics.contains(line.substr(i, sz)) || trapvec_table.contains(line.substr(i, sz)))
                        result.push_back({Mnemonic, line.substr(std::exchange(i, i + sz), sz)});
                    else
                        result.push_back({LabelUse, line.substr(std::exchange(i, i + sz), sz)});
                    continue;
                }
            }

            if (line[i] == '#') {
                auto s = ++i;
                size_t sz{};
                while(std::isdigit(line[s])){++sz; ++s;};
                result.push_back({Immediate, line.substr(std::exchange(i, i + sz), sz)});
                continue;
            }

            if (line[i] == '\n') {
                result.push_back({EndOfLine, line.substr(i++, 1)});
                continue;
            }
            ++i;
        }
        return result;
    }
};

auto split_tokens(const std::vector<Lexer::Token>& tokens) {
    std::vector<std::vector<Lexer::Token>> res;
    std::vector<Lexer::Token> tmp;
    for (auto& el: tokens) {
        if (el.type == Lexer::EndOfLine && !tmp.empty()) {
            res.push_back(std::exchange(tmp, {}));
        } else if (el.type != Lexer::EndOfLine){
            tmp.push_back(el);
        }
    }
    return res;
}

auto label_tab(const auto& stokens) {
    std::unordered_map<std::string_view, size_t> res;
    for (size_t i = 0; i < stokens.size(); ++i) {
        if (stokens[i][0].type == Lexer::LabelAnnotate) {
            res[stokens[i][0].value] = i;
        }
    }
    return res;
}

std::vector<unsigned short> parse_tokens(const std::vector<Lexer::Token>& tokens) {
    std::vector<unsigned short> instructions;
    unsigned short current{};

    auto stokens = split_tokens(tokens);
    auto ltab = label_tab(stokens);

    size_t k = 0;
    for (auto&& raw: stokens) {
        k++;
        auto t = raw[0];
        if (t.type == Lexer::Mnemonic) {
            auto value = t.value;

            if (trapvec_table.contains(value)) {
                current = mnemonics["TRAP"] << 12;
                current |= trapvec_table[value];
                instructions.push_back(std::exchange(current, 0));
                continue;
            };

            current = mnemonics[value] << 12;
      
            if (value == "AND" || value == "ADD") {
                current |= regs[raw[1].value] << 9; // DR
                current |= regs[raw[2].value] << 6; // SR1
                if (raw[3].type == Lexer::Immediate) {
                    current |= 1 << 5;
                    current |= std::atoi(raw[3].value.data()); // imm5
                }
                else{current |= regs[raw[3].value];} // SR2
                instructions.push_back(std::exchange(current, 0));
                continue;
            }

            if (value == "LD" || value == "LDI" || value == "ST" || value == "LEA" || value == "STI") {
                current |= regs[raw[1].value] << 9;
                current |= std::atoi(raw[2].value.data()); // offset9
                instructions.push_back(std::exchange(current, 0));
                continue;
            }

            if (value == "STR" || value == "LDR") {
                current |= regs[raw[1].value] << 9; // SR
                current |= regs[raw[2].value] << 6; // baseR
                current |= std::atoi(raw[3].value.data()); // offset6
                instructions.push_back(std::exchange(current, 0));
                continue;
            }

            if (value == "JMP" || value == "JSRR") {
                current |= regs[raw[1].value] << 6; // baseR
                instructions.push_back(std::exchange(current, 0));
                continue;
            }

            if (value == "JSR") {
                current |= 1 << 11;
                current |= raw[1].type == Lexer::Immediate ?
                           std::atoi(raw[1].value.data()) :  // num
                           ltab[raw[1].value] - k;
                instructions.push_back(std::exchange(current, 0));
                continue;
            }

            if (value == "NOT") {
                current |= regs[raw[1].value] << 9; // DR
                current |= regs[raw[2].value] << 6; // SR
                instructions.push_back(std::exchange(current, 0));
                continue;
            }

            if (value[0] == 'B' && value[1] == 'R') {
                current = mnemonics["BR"] << 12;
                for (int j = 2; j < std::size(value); ++j) {
                    if (value[j] == 'n')
                        current |= 1 << 11;
                    else if (value[j] == 'z')
                        current |= 1 << 10;
                    else if (value[j] == 'p')
                        current |= 1 << 9;
                }
                current |= raw[1].type == Lexer::Immediate ?
                           std::atoi(raw[2].value.data()) : // num
                           ltab[raw[1].value] - k;
                instructions.push_back(std::exchange(current, 0));
                continue;
            }


        }
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


void print_lex(std::string_view r) {
    auto res = Lexer::lex(r);
    for (auto& el: res)
        std::cout << "{ Type:" << Lexer::tt_str[el.type] << "  Value:" << el.value << "}" << '\n';
}

int main() {
    LC3 vm;
    vm.run(parse_tokens(Lexer::lex(raw)));
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
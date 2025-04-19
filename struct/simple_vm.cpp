#include <iostream>
#include <cassert>
#include <vector>
#include <string_view>

#define DEBUG 0

struct Instruction {
    unsigned char opcode;
    unsigned char arg1;
    unsigned char arg2;
};

enum Registers{R0, R1, R2, R3, R4, R5, R6, R7};

enum Opcodes {
    HLT   =   0x00,
    LOAD  =   0x01,
    STORE =   0x02,
    ADD   =   0x03,
    SUB   =   0x04,
    JMP   =   0x05,
    JZ    =   0x06,
    SET   =   0x07
};

constexpr Opcodes opcodes[] = {HLT, LOAD, STORE, ADD, SUB, JMP, JZ, SET};
constexpr std::string_view mnemonics[] = {"HLT", "LOAD", "STORE", "ADD", "SUB", "JMP", "JZ", "SET"};

constexpr Registers registers[] = {R0, R1, R2, R3, R4, R5, R6, R7};
constexpr std::string_view reg_str[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7"};

struct SimpleVM {
    unsigned offset = 0;
    std::array<unsigned char, 8> regs{};
    std::array<unsigned char, 256> mem{};
    Instruction* current;
    void base_call(const Instruction& ins) {
        switch(ins.opcode) {
            case LOAD: {
                regs[ins.arg1] = mem[ins.arg2 + offset];
                break;
            }
            case STORE: {
                mem[ins.arg1 + offset] = regs[ins.arg2];
                break;
            }
            case ADD: {
                regs[ins.arg1] = regs[ins.arg1] + regs[ins.arg2];
                break;
            }
            case SUB: {
                regs[ins.arg1] = regs[ins.arg1] - regs[ins.arg2];
                break;
            }
            case SET: {
                regs[ins.arg1] = ins.arg2;
                break;
            }
            default: break;
        }
    }

    void load_instructions(const auto& instructions) {
        size_t i = 0;
        for (size_t j = 0; j < instructions.size(); ++j){
            mem[i++] = instructions[j].opcode;
            mem[i++] = instructions[j].arg1;
            mem[i++] = instructions[j].arg2;
        }
        offset = instructions.size() * 3 + 4;
        #if DEBUG
        print_mem(40);
        #endif
    }

    void execute() {
        current = reinterpret_cast<Instruction*>(mem.data());
        while(current->opcode != HLT) {
            switch(current->opcode) {
                case JMP: {
                    current = reinterpret_cast<Instruction*>(&mem[current->arg1]);
                    break;
                }
                case JZ: {
                    if (!regs[current->arg1]) {
                        #if DEBUG
                        std::cout << "REG:" << (unsigned)regs[current->arg1] << '\n' << "INS:" << (unsigned)current->arg2 << '\n';
                        #endif
                        current = reinterpret_cast<Instruction*>(&mem[current->arg2]);
                        break;
                    }
                }
                default: {
                    base_call(*current++);
                    break;
                }
            }
        }
    }

    void print_mem(size_t sz) {
        for (size_t i = 0; i < sz; ++i) {
            std::cout << i << ": " << (unsigned)mem[i] << '\n';
        }
    }
    
    auto get_mem_value(size_t i) const {
        return mem[i + offset];
    }
};

auto split_to_ranges(std::string_view raw) {
    std::vector<std::vector<std::string_view>> range;
    size_t i = 0;
    size_t j = 0;
    std::vector<std::string_view> tmp;
    bool found = false;
    for (size_t k = 0; k < raw.size(); ++k, ++j) {
        if (!found) {
            if (!std::isalpha(raw[k])) {
                ++i;
            } else {
                found = true;
                continue;
            }
        }
        
        if (raw[k] == ' ' && found) {
            tmp.push_back(std::string_view{raw.begin() + i, raw.begin() + j});
            i = j+1;
            continue;
        }
        
        if (raw[k] == ';' && found) {
            tmp.push_back(std::string_view{raw.begin() + i, raw.begin() + j});
            range.push_back(tmp);
            tmp = {};
            i = j+1;
            found = false;
        }

    }
    return range;
}


int get_index(std::string_view m, const auto& arr) {
    for (int i = 0; i < std::size(arr); ++i) 
        if (m == arr[i]) return i;
    return -1;    
}

auto parse(std::string_view raw) {
    std::vector<Instruction> res;
    std::string tmp;
    Instruction ins;
    auto ranges = split_to_ranges(raw);
    for (auto&& raw: ranges){
        unsigned char a1{};
        unsigned char a2{};
        unsigned char opcode = get_index(raw[0], mnemonics);
        switch (opcode) {
            case LOAD: 
            case SET:
            case JZ:
            {
                a1 = get_index(raw[1], reg_str);
                a2 = std::stoi(raw[2].data());
                break;
            }
            case STORE:
            case JMP:
            {
                a1 = std::stoi(raw[1].data());
                a2 = get_index(raw[2], reg_str);
                break;
            }
            case ADD: 
            case SUB:
            {
                a1 = get_index(raw[1], reg_str);
                a2 = get_index(raw[2], reg_str);
                break;
            }
            default: break;
        }
        #if DEBUG
        std::cout << "OPCODE: " << mnemonics[opcode] << " " << "ARG1: " << (unsigned)a1 << " " << "ARG2: " << (unsigned)a2 << '\n';
        #endif
        res.push_back(Instruction{.opcode = opcode, .arg1 = a1, .arg2 = a2});
    }
    return res;
}

constexpr std::string_view raw = R"(
        SET R0 160;
        SET R1 10;
        SET R2 5;
        SET R3 1;
        JZ R2 24;
        ADD R1 R2;
        SUB R2 R3;
        JMP 12;
        ADD R0 R1;
        STORE 0 R0;
)";

int main() {
    SimpleVM sm;
    sm.load_instructions(parse(raw));
    sm.execute();
    assert(sm.get_mem_value(0) == 185);
}
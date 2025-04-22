#include <iostream>
#include <cassert>
#include <vector>
#include <string_view>
#include <array>

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

constexpr std::array<char, 52> alphas{
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

constexpr Opcodes opcodes[] = {HLT, LOAD, STORE, ADD, SUB, JMP, JZ, SET};
constexpr std::string_view mnemonics[] = {"HLT", "LOAD", "STORE", "ADD", "SUB", "JMP", "JZ", "SET"};

constexpr Registers registers[] = {R0, R1, R2, R3, R4, R5, R6, R7};
constexpr std::string_view reg_str[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7"};

struct SimpleVM {
    std::array<unsigned char, 8>                           regs{};
    std::array<unsigned char, 128>                         mem{};
    std::array<Instruction, 128/sizeof(Instruction)>       text{};
    Instruction* current{};
    size_t commands{};
    consteval SimpleVM(){}
    consteval void base_call(const Instruction& ins) {
        switch(ins.opcode) {
            case LOAD: {
                regs[ins.arg1] = mem[ins.arg2];
                break;
            }
            case STORE: {
                mem[ins.arg1] = regs[ins.arg2];
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

    consteval void load_instructions(const auto& instructions) {
        assert(instructions.size() < 128/sizeof(Instruction));
        std::copy(instructions.begin(), instructions.end(), text.begin());
    }

    consteval void execute() {
        current = text.data();
        while(current->opcode != HLT) {
            switch(current->opcode) {
                case JMP: {
                    current = text.data() + current->arg1;
                    break;
                }
                case JZ: {
                    current = !regs[current->arg1] ? text.data() + current->arg2 : current + 1;
                    break;
                }
                default: {
                    base_call(*current++);
                    break;
                }
            }
        }
    }
    
    consteval auto get_mem_value(size_t i) const {
        return mem[i];
    }
};

consteval bool isalpha(char el) {
    for (auto a: alphas){
        if (el == a){
            return true;
        }
    }
    return false;
}

consteval auto split_to_ranges(std::string_view raw) {
    std::vector<std::array<std::string_view, 3>> range{};
    size_t i = 0;
    size_t j = 0;
    std::array<std::string_view, 3> tmp{};
    bool found = false;
    size_t current = 0;
    for (size_t k = 0; k < raw.size(); ++k, ++j) {
        if (!found) {
            if (!isalpha(raw[k])) {
                ++i;
            } else {
                found = true;
                continue;
            }
        }
        
        if (raw[k] == ' ' && found) {
            tmp[current++] = std::string_view{raw.begin() + i, raw.begin() + j};
            i = j+1;
            continue;
        }
        
        if (raw[k] == ';' && found) {
            tmp[current] = std::string_view{raw.begin() + i, raw.begin() + j};
            range.push_back(tmp);
            current = 0;
            tmp = {};
            i = j+1;
            found = false;
        }

    }
    return range;
}

consteval bool isnum(char el) {
    constexpr std::array<char, 10> nums{'0','1','2','3','4','5','6','7','8','9'};
    for (auto n: nums){
        if (el == n){
            return true;
        }
    }
    return false;
}

consteval int atoi(std::string_view s) {
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


consteval int get_index(std::string_view m, const auto& arr) {
    for (int i = 0; i < std::size(arr); ++i) 
        if (m == arr[i]) return i;
    return -1;    
}

consteval auto parse(std::string_view s) {
    std::vector<Instruction> res;
    std::string tmp;
    Instruction ins;
    auto ranges = split_to_ranges(s);
    for (auto& raw: ranges){
        unsigned char a1{};
        unsigned char a2{};
        unsigned char opcode = get_index(raw[0], mnemonics);
        switch (opcode) {
            case LOAD: 
            case SET:
            case JZ:
            case HLT:
            {
                a1 = get_index(raw[1], reg_str);
                a2 = atoi(raw[2]);
                break;
            }
            case STORE:
            case JMP:
            {
                a1 = atoi(raw[1]);
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
        res.push_back(Instruction{.opcode = opcode, .arg1 = a1, .arg2 = a2});
    }
    
    return res;
} 

constexpr std::string_view raw = R"(
        SET R0 160;
        SET R1 10;
        SET R2 12;
        SET R3 1;
        JZ R2 8;
        ADD R1 R2;
        SUB R2 R3;
        JMP 4;
        ADD R0 R1;
        STORE 0 R0;
        HLT;
)";

consteval auto exec() {
    SimpleVM vm;
    vm.load_instructions(parse(raw));
    vm.execute();
    return vm.get_mem_value(0);
}

int main() {
    static_assert(exec() == 248);
}
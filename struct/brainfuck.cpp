#include <iostream>
#include <iterator>
#include <array>
#include <vector>

constexpr size_t mem_sz = 30'000;
enum Command {RightShift, LeftShift, Plus, Minus, Dot, Comma, SLoop, ELoop};
constexpr Command commands[] = {RightShift, LeftShift, Plus, Minus, Dot, Comma, SLoop, ELoop};
constexpr char commands_str[] = {'>', '<', '+', '-', '.', ',', '[', ']'};

class VM {
    static inline std::array<unsigned char, mem_sz> memory;
    decltype(memory.begin()) current;
    void base_operation_call(Command cmd) {
        switch(cmd) {
            case RightShift: {
                ++current;
                break;
            }
            case LeftShift: {
                if (current != memory.begin()) --current;
                break;
            }
            case Plus: {
                ++(*current);
                break;
            }
            case Minus: {
                --(*current);
                break;
            }
            case Dot: {
                std::cout << *current;
                break;
            }
            case Comma: {
                unsigned char info;
                std::cin >> info;
                *current = info;
                break;
            }
            default: break;
        }
    }
public:    
    VM(): current(memory.begin()){}
    
    // TODO: loop levels
    void execute(const auto& cmds) {
        bool loop_seed = false;
        decltype(current) loop_begin;
        size_t start{};
        size_t end{};
        for (size_t i = 0; i < std::size(cmds); ++i) {
            if (cmds[i] == SLoop) {
                start = i;
                loop_seed = true;
                loop_begin = current;
                continue;
            }
            if (cmds[i] == ELoop) {
               while(*loop_begin){
                 for (size_t s = start; s != i; ++s)
                    base_operation_call(cmds[s]);
               }
               loop_seed = false;
               continue;
            }
            if (!loop_seed) base_operation_call(cmds[i]);
        }
    }
};

constexpr std::string_view raw = "++++++++++[>++++++>+++++++++++>+++++++++>++++++++++>+++++++++++>++++++++++>+++++++++++>++++++++++>+++++++++++>+++>++++++++++>+++++++++++>+++>++++++++++>++++++++++++>+++++++++++>+++<<<<<<<<<<<<<<<<<-]>++++++.>++++.>+++++++.>+++++.>.>++.>+++++++.>-.>---.>++.>+++++.>+++++.>++.>++.>---.>.>+++.";

auto get_index(char el) {
    for (int i = 0; i < std::size(commands_str);++i) {
        if (el == commands_str[i]) return i;
    }
    return -1;
}

auto parse_commands(std::string_view raw) {
    std::vector<Command> result;
    for (auto el: raw) {
        result.push_back(commands[get_index(el)]);
    }
    return result;
}


int main() {
    VM{}.execute(parse_commands(raw));
}
#include <iostream>
#include <iterator>
#include <array>
#include <vector>
#include <span>

constexpr size_t mem_sz_bytes = 30'000;
enum Command {RightShift, LeftShift, Plus, Minus, Dot, Comma, SLoop, ELoop};
constexpr Command commands[] = {RightShift, LeftShift, Plus, Minus, Dot, Comma, SLoop, ELoop};
constexpr char commands_str[] = {'>', '<', '+', '-', '.', ',', '[', ']'};

class VM {
    static inline std::array<unsigned char, mem_sz_bytes> memory{};
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
                char info;
                std::cin.get(info);
                *current = static_cast<unsigned char>(info);
                break;
            }
            default: break;
        }
    }

    int get_loop_end(std::span<Command> cmds) {
        size_t scope{};
        for (int i = 0; i < std::size(cmds); ++i) {
            if (cmds[i] == SLoop) {
                ++scope;
                continue;
            }
            if (cmds[i] == ELoop) {
                if (!scope) {
                    return i;
                }
                --scope;
            }
        }
        return -1;
    }

    void handler(std::span<Command> cmds) {
        for (size_t i = 0; i < std::size(cmds); ++i) {
            if (cmds[i] == SLoop){
                loop_execute({cmds.begin() + i + 1, cmds.begin() + i + 1 + get_loop_end({cmds.begin() + i + 1, cmds.end()})});
                i += get_loop_end({cmds.begin() + i + 1, cmds.end()});
            } else {
                base_operation_call(cmds[i]);
            }
        }
    }

    void loop_execute(std::span<Command> cmds) {
        while(*current) handler(cmds);
    }
  
public:    
    VM(): current(memory.begin()){}
    void execute(auto& cmds) {
        handler(cmds);
    }
};

constexpr std::string_view raw = "++++++++++[>++++++>+++++++++++>+++++++++>++++++++++>+++++++++++>++++++++++>+++++++++++>++++++++++>+++++++++++>+++>++++++++++>+++++++++++>+++>++++++++++>++++++++++++>>>>>>+++<<<<<<<<<<<<<<<<<<<<<-]>++++++.>++++.>+++++++.>+++++.>.>++.>+++++++.>-.>---.>++.>+++++.>+++++.>++.>++.>---.>+++++++[>+++[>+++[>+++[>++++++<-]<-]<-]<-]>>>>.>+++.";

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
    auto commands = parse_commands(raw);
    VM{}.execute(commands);
}
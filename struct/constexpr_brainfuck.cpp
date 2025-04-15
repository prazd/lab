#include <iostream>
#include <iterator>
#include <array>
#include <vector>
#include <span>

constexpr size_t mem_sz_bytes = 30'000;
enum Command {RightShift, LeftShift, Plus, Minus, Dot, SLoop, ELoop};
constexpr Command commands[] = {RightShift, LeftShift, Plus, Minus, Dot, SLoop, ELoop};
constexpr char commands_str[] = {'>', '<', '+', '-', '.', '[', ']'};

consteval auto get_index(char el) {
    for (int i = 0; i < std::size(commands_str);++i) {
        if (el == commands_str[i]) return i;
    }
    return -1;
}

consteval auto parse_commands(std::string_view raw) {
    std::vector<Command> result;
    for (size_t i = 0; i < raw.size(); ++i) {
        result.push_back(commands[get_index(raw[i])]);
    }
    return result;
}

consteval auto calc_prints(std::string_view raw) {
    size_t res{};
    for (auto el: raw) {
        res += el == '.' ? 1 : 0;
    }
    return res;
}

consteval auto calc_syms(std::string_view raw) {
    size_t res{};
    for (auto el: raw) {++res;}
    return res;
}

template <size_t output_sz>
class VM {
    std::array<unsigned char, mem_sz_bytes> memory{};
    std::array<char, output_sz> output{};
    decltype(output.begin()) current_output = output.begin();
    decltype(memory.begin()) current;
    consteval void base_operation_call(Command cmd) {
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
                *current_output = *current;
                ++current_output;
                break;
            }
            default: break;
        }
    }

    consteval int get_loop_end(std::span<Command> cmds) {
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

    consteval void handler(std::span<Command> cmds) {
        for (size_t i = 0; i < std::size(cmds); ++i) {
            if (cmds[i] == SLoop){
                loop_execute({cmds.begin() + i + 1, cmds.begin() + i + 1 + get_loop_end({cmds.begin() + i + 1, cmds.end()})});
                i += get_loop_end({cmds.begin() + i + 1, cmds.end()});
            } else {
                base_operation_call(cmds[i]);
            }
        }
    }

    consteval void loop_execute(std::span<Command> cmds) {
        while(*current) handler(cmds);
    }
  
public:    
    consteval VM(): current(memory.begin()){}
    consteval auto execute(std::string_view raw) {
        auto commands = parse_commands(raw);
        handler(commands);
        return output;
    }
};

consteval auto execute() {
    constexpr std::string_view input = "+++++++[>+++[>+++[>+++[>++++++<-]<-]<-]<-]>>>>.";
    VM<calc_prints(input)> vm;
    return vm.execute(input);
}

int main() {
    constexpr auto output = execute();
    static_assert(output == std::array<char, 1>{'n'});
}
#include <iostream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <charconv>
#include "Utils.hpp"
#include "Dwarf.hpp"
#include "Macho.hpp"

struct CLI {
    DebugLineParseVM vm;
    const char* filename;
    std::vector<char> f;

    static inline std::vector<uintptr_t> br_addresses;
    static inline std::vector<unsigned char> original_instr_value;

    static inline std::vector<char*> process_args_list = {nullptr};
    static inline std::vector<char*> process_env_list  = {nullptr};

    unsigned char last_instr = 0;

    static void sigtrap_handler(int s){
        std::cout << "Sigtrap handler!\n";
    }
    CLI(const char* filename): filename(filename), f(read_file(filename)){
        vm.parse_header(f, find_section(f, "__DWARF", "__debug_line"));
    }
    enum Command{Run, Break, Unknown};
    Command parse_cmd(std::string_view sv){
        if (sv == "break") return Break;
        else if (sv == "run") return Run;
        return Unknown;
    }
    void handle_cmd(const std::vector<std::string_view>& args){
        if (args.empty()) return;
        auto cmd = parse_cmd(args[0]);
        switch(cmd) {
            case Break: {
                if (args.size() != 3) {
                    std::cout << "args sz parse error\n"; 
                    break;
                }
                size_t line{};
                auto [ptr, ec] = std::from_chars(args[2].begin(), args[2].end(), line);
                if (ec == std::errc()){
                    auto addr = vm.get_address_by_file_and_line(std::string(args[1]), line);
                    std::cout << "Address:" << addr << '\n';
                    br_addresses.push_back((uintptr_t)addr);
                } else 
                    std::cout << "line parse error\n";
                break;
            }
            case Unknown: {std::cout << "Unknown cmd\n"; break;}
            case Run:     {
                std::cout << "Run:\n";
                auto s = std::string(args[1]);
                auto pid = fork();
                if (pid == 0){
                    std::cout << "prog:" << s.data() << '\n';
                    ptrace(PT_TRACE_ME, 0,0,0);
                    int val = execve(s.data(), process_args_list.data(), process_env_list.data());
                    if (val == -1){
                        std::cout << "execve error\n";
                    }
                } else {
                    // TODO: finish
                    // use valid address
                    // slide = actual_load_address - mach_o_preffered_address
                    // valid_address = dwarf_address + actual_load_address
                    waitpid(pid, NULL, 0);
                    for (size_t i = 0, e = br_addresses.size(); i != e; ++i){
                        size_t qword = ptrace(PT_READ_D, pid, (char*)br_addresses[i], 0);
                        std::cout << "Original qword:" << std::hex << qword << '\n';
                        original_instr_value.push_back(char(qword & 0xff));
                        qword &= 0xffffffffffffff00;
                        qword |= 0xcc;
                        ptrace(PT_WRITE_D, pid, (char*)(br_addresses[i]), qword); // int3
                    }
                    ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);
                }
                for (size_t i = 0, e = br_addresses.size(); i != e; ++i){
                    std::cout << "Addr:" << br_addresses[i] << " Val:" << std::hex << 
                        std::setw(2) << std::setfill('0') << (int)original_instr_value[i] << '\n';
                }
                break;
            }
        }
    }

    auto split(const std::string& s){
        std::vector<std::string_view> res;
        if (s.empty()) return res;
        auto s_b = s.begin();
        for (auto it = s.begin(), ite = s.end(); it != ite; ++it){
            if (*it == ' '){
                res.push_back(std::string_view{s_b, it});
                s_b = it + 1;
            }
        }
        res.push_back(std::string_view{s_b, s.end()});
        return res;
    }

    void run(){
        std::string str;
        for(;;){
            std::cout << ">>";
            std::getline(std::cin, str);
            handle_cmd(split(str));
            str.clear();
        }
    }
};


int main(int argc, char** argv){
    signal(SIGTRAP, CLI::sigtrap_handler);
    if (argc != 2) std::abort();
    CLI cli(argv[1]);
    print_macho_header(cli.f);
    print_dwarf_cu_header(cli.f, find_section(cli.f, "__DWARF", "__debug_info"));
    cli.run();
}

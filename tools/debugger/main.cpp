#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include <utility>
#include <charconv>
#include "Utils.hpp"
#include "Dwarf.hpp"
#if defined(__APPLE__)
#include "Macho.hpp"
#elif defined(__linux__)
#include "Elf.hpp"
#endif

struct CLI {
    DebugLineParseVM vm;
    const char* filename;
    std::vector<char> f;

    static inline std::vector<uintptr_t> br_addresses;
    static inline std::vector<unsigned char> original_instr_value;

    static inline std::vector<char*> process_args_list = {nullptr};
    static inline std::vector<char*> process_env_list  = {nullptr};

    unsigned char last_instr = 0;
    
    CLI(const char* filename): filename(filename), f(read_file(filename)){
#if defined(__APPLE__)        
        vm.parse_header(f, macho_find_section(f, "__DWARF", "__debug_line").offset);
#elif defined(__linux__)
        vm.parse_header(f, elf_find_section_offset_by_name(f, ".debug_line"), elf_find_section_offset_by_name(f, ".debug_line_str"));
#endif
    }
    enum Command{Run, Break, Unknown};
    Command parse_cmd(std::string_view sv){
        if (sv == "break") return Break;
        else if (sv == "run") return Run;
        return Unknown;
    }

    static unsigned char patch_int3(std::vector<char>& vec, size_t inst){
        auto ptr = vec.data();
        auto sz = vec.size();
        size_t value{};
        for (size_t i = 0; i != sz; ++i, ++ptr){
            memcpy(&value, ptr, sizeof(value));
            if (value == inst)
                return std::exchange(*ptr, 0xcc); // int3
        }
        return 0;
    }

#if defined(__linux__)
    void elf_debug(std::string_view arg){
        auto s = std::string(arg);
        auto pid = fork();
        if (pid == 0){
            ptrace(PTRACE_TRACEME,0,0,0);
            execve(s.c_str(), process_args_list.data(), process_env_list.data());
        } else {
            waitpid(pid, 0, 0);
            // use valid address
            // slide = actual_load_address - mach_o_preffered_address
            // valid_address = dwarf_address + slide
            // 0x1195
            auto min_load_address = elf_find_min_load_address(f);
            auto elf_header = parse_to<Elf64Header>(f.data());
            std::string proc_path = "/proc/";
            proc_path += std::to_string(pid);
            proc_path += std::string("/maps");
            auto res = read_file_str(proc_path.c_str());
            for (const auto& el: res){
                std::cout << el << '\n';
            }
            std::cout << "ProcMapEntry:\n";
            std::vector<ProcMapEntry> proc_map_entry;
            ProcMapEntry pme;
            for (const auto& el: res){
                pme.parse_from_line(el);
                proc_map_entry.push_back(pme);
            }
            for (const auto& el: proc_map_entry){el.print(); std::cout << '\n';}

            // size_t header_begin{};
            // memcpy(&header_begin, &elf_header, sizeof(header_begin));
            // std::cout << "Header begin:" << header_begin << '\n';
            // auto regs = linux_read_regs(pid);
            // std::cout << "RIP:" << regs.rip << '\n';

            // auto base_addr = elf_base_image_address(pid, regs.rip, header_begin);
            // std::cout << "BASE ADDR:" << base_addr << '\n';
            // uintptr_t addr = base_addr - min_load_address + 0x1195;
            // std::cout << "ADDR:" << addr << '\n';

            // auto res = ptrace(PTRACE_PEEKDATA, pid, 0x55555555519f, NULL);
            // std::cout << "DATA:" << std::hex << res << '\n';
            ptrace(PTRACE_CONT, pid, 1, 0);
        }
    }
#endif

#if defined(__APPLE__)
    void macho_debug(std::string_view arg){
        int status = 0;
        auto s = std::string(arg);
        auto d_f = read_file(s.c_str());
        // int pid{};
                // posix_spawnattr_t attr;
                // if (posix_spawnattr_init(&attr) != 0){
                //     std::cout << "posix_spawnattr_init error\n";
                //     break;
                // }
                // if (posix_spawnattr_setflags(&attr, POSIX_SPAWN_START_SUSPENDED) != 0){
                //     std::cout << "posix_spawnattr_setflags error\n";
                //     break;
                // }
                // int res = posix_spawn(&pid, s.data(), NULL, &attr, process_args_list.data(), process_env_list.data());
                // if (res != 0){
                //     std::cout << "posix_spawn error\n";
                //     break;
                // }
                // if (waitpid(pid, &status, WUNTRACED) == -1){
                //     std::cout << "waitpid error\n";
                //     break;
                // }
                // // ....
                //    if (kill(pid, SIGCONT) != 0){
                //         std::cout << "SIGCONT error\n";
                //         break;
                //     }

                //     waitpid(pid, &status, 0);
                //     posix_spawnattr_destroy(&attr);
                const char* fn = "check";
                std::cout << "prog:" << s.data() << '\n';
                auto e_f = read_file(s.data());
                auto o_f = std::ofstream(fn, std::ios::binary);
                if (!o_f.is_open()){std::cout << "file is not open"; return;}
                unsigned char prev_value = patch_int3(e_f, 0xb00000004f3d8d48);

                o_f.write(e_f.data(), e_f.size());
                o_f.close();

                std::filesystem::permissions(std::filesystem::path(fn),
                    std::filesystem::perms::owner_exec |
                    std::filesystem::perms::group_exec | 
                    std::filesystem::perms::others_exec,
                    std::filesystem::perm_options::add
                );
                std::cout << "Create file" << std::endl;

                auto pid = fork();
                if (pid == 0){
                    if (ptrace(PT_TRACE_ME, 0,0,0) == -1){
                        std::cout << "Error:" << errno  << '\n';
                        return;
                    }
                    int val = execve(fn, process_args_list.data(), process_env_list.data());
                    if (val == -1){
                        std::cout << "execve error\n";
                    }
                } else {
                    // TODO: finish
                    // use valid address
                    // slide = actual_load_address - mach_o_preffered_address
                    // valid_address = dwarf_address + slide
                    for (;;){
                        waitpid(pid, &status, 0);
                        std::cout << "Step" << std::endl;
                        auto r = macho_read_proc_regs(pid);
                        auto val = macho_read_proc_mem<size_t>(pid, r.__rip);
                        if (val == 0xb00000004f3d8d){
                            auto actual_load_address = macho_find_proc_base_address(pid, (uintptr_t)r.__rip);
                            if (actual_load_address == 0){
                                std::cout << "Addr not found\n";
                                break;
                            }
                            std::cout << "Actual load address:" << actual_load_address << std::endl;
                            std::cout << "!!!FOUND!!!" << " Address:" << r.__rip << std::endl;
                            macho_write_proc_mem_text(pid, prev_value, r.__rip-1);
                            break;
                        } else {
                            std::cout << "RIP:" << r.__rip << " val:" << val << std::endl;

                        }
                        ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);
                    }
                
            
                    // auto ins_addr = find_instructions(pid, 0x480000000afc45c7,(uintptr_t)r.__rip);
                    // std::cout << "Ins addr:" << ins_addr << std::endl;
                    // // 100000000 - vm addr
                    // // 100003f78 - intruction addr
   
                    // // std::cout << "RES:" << res
                    // ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);

                }
                for (size_t i = 0, e = br_addresses.size(); i != e; ++i){
                    std::cout << "Addr:" << br_addresses[i] << " Val:" << std::hex << 
                        std::setw(2) << std::setfill('0') << (int)original_instr_value[i] << '\n';
                }
    }
#endif

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
#if defined(__APPLE__)
                macho_debug(args[1]);  
#elif defined(__linux__)
                elf_debug(args[1]);
#endif

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
    if (argc != 2) std::abort();
    CLI cli(argv[1]);
#if defined(__APPLE__)
    print_macho_header(cli.f);
    print_dwarf_cu_header_macho(cli.f, macho_find_section(cli.f, "__DWARF", "__debug_info"));
    auto seg = macho_find_segment_header(cli.f, "__TEXT");
    seg.print();
    cli.run();
#elif defined (__linux__)
    cli.run();
#endif
}

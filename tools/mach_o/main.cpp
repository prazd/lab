#include <iostream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_region.h>
#include <chrono>
#include <thread>
#include <charconv>
#include "Utils.hpp"
#include "Dwarf.hpp"
#include "Macho.hpp"

struct CLI {
    MachOHeader macho_header;
    SegmentCommand64 text_seg;
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
        macho_header = parse_macho_header(f);
        text_seg = find_segment_header(f, "__TEXT");
    }
    enum Command{Run, Break, Unknown};
    Command parse_cmd(std::string_view sv){
        if (sv == "break") return Break;
        else if (sv == "run") return Run;
        return Unknown;
    }


    // TODO: finish
    uintptr_t find_proc_text_segment(size_t pid, char* ptr){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << std::endl;
            return 0;
        }
        ptr += sizeof(MachOHeader);
        vm_offset_t buffer_address = 0;
        mach_msg_type_number_t count;
        mach_vm_size_t sz_lc = sizeof(LoadCommands);
        mach_vm_size_t sz_sc = sizeof(SegmentCommand64);
        LoadCommands lc;
        SegmentCommand64 sc;
        unsigned s = 0;
        for(;;){
            std::cout << "Address:" << (void*)ptr << std::endl;
            if (s == 0){
                kr = mach_vm_read(task, (mach_vm_address_t)ptr, sz_lc, &buffer_address, &count);
                if (kr != KERN_SUCCESS){
                    std::cout << "mach_vm_read:" << mach_error_string(kr) << '\n';
                    return 0;
                }
                memcpy(&lc, (void*)buffer_address, count);
                vm_deallocate(mach_task_self(), buffer_address, count);
                s += (lc.ncmds == LC_SEGMENT_64);
                ptr += s == 0 ? lc.szcmds : 0;
            } else if (s == 1){
                kr = mach_vm_read(task, (mach_vm_address_t)ptr, sz_sc, &buffer_address, &count);
                if (kr != KERN_SUCCESS){
                    std::cout << "mach_vm_read:" << mach_error_string(kr) << '\n';
                    return 0;
                }
                memcpy(&sc, (void*)buffer_address, count);
                vm_deallocate(mach_task_self(), buffer_address, count);
                std::cout << "Addr:" << (void*)ptr << " Segname:" << sc.segname << '\n';
                if (!strcmp(sc.segname, "__TEXT")){
                    return (uintptr_t)ptr;
                }
                ptr += s == 1 ? sc.cmdsz: 0;
            }
        }
        std::abort();
    }

    x86_thread_state64_t read_proc_regs(size_t pid){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << '\n';
            return x86_thread_state64_t{};
        }
        thread_act_port_array_t thread_list;
        mach_msg_type_number_t thread_count;

        kr = task_threads(task, &thread_list, &thread_count);
        if (kr != KERN_SUCCESS){
            std::cout << "task_threads:" << mach_error_string(kr) << '\n';
            return x86_thread_state64_t{};
        }
        if (thread_count > 0){
            x86_thread_state64_t state;
            mach_msg_type_number_t s_count = x86_THREAD_STATE64_COUNT;
            kr = thread_get_state(thread_list[0], x86_THREAD_STATE64, (thread_state_t)&state, &s_count);
            if (kr != KERN_SUCCESS){
                std::cout << "thread_get_state:" << mach_error_string(kr) << '\n';
                return x86_thread_state64_t{};
            }
            std::cout << "Read state succesfully\n";
            return state;
        } 
        std::cout << "Thread count:" << thread_count << '\n';
        return x86_thread_state64_t{};
    }

    template <typename T>
    T read_proc_mem(size_t pid, uintptr_t address){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << '\n';
            return 0;
        }
        vm_offset_t buffer_address = 0;
        mach_msg_type_number_t count;
        mach_vm_size_t sz = sizeof(T);
        T val{};
        kr = mach_vm_read(task, (mach_vm_address_t)address, sz, &buffer_address, &count);
        if (kr != KERN_SUCCESS){
            std::cout << "mach_vm_read:" << mach_error_string(kr) << '\n';
            return 0;
        }
        memcpy(&val, (void*)buffer_address, count);
        vm_deallocate(mach_task_self(), val, count);
        return val;
    }

    uintptr_t find_proc_base_address(size_t pid, uintptr_t rip_address){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << '\n';
            return 0;
        }
        vm_offset_t buffer_address = 0;
        mach_msg_type_number_t count;
        mach_vm_size_t sz = sizeof(unsigned);
        unsigned val{};
        rip_address &= 0xfffffffffffff000;
        for (;;){
            kr = mach_vm_read(task, (mach_vm_address_t)rip_address, sz, &buffer_address, &count);
            if (kr != KERN_SUCCESS){
                std::cout << "mach_vm_read:" << mach_error_string(kr) << '\n';
                return 0;
            }
            memcpy(&val, (void*)buffer_address, count);
            vm_deallocate(mach_task_self(), buffer_address, count);
            if (val == macho_header.magic_number){
                return rip_address;
            }
            rip_address -= 0x1000; // page size 4kb
        }
        return 0;
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
                auto d_f = read_file(s.c_str());
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
                    // valid_address = dwarf_address + slide
                    waitpid(pid, NULL, 0);
                    auto r = read_proc_regs(pid);
                    std::cout << "RIP:" << r.__rip << '\n';
                    
                    // TODO: check magic number
                    auto actual_load_address = find_proc_base_address(pid, (uintptr_t)r.__rip);
                    if (actual_load_address == 0){
                        std::cout << "Addr not found\n";
                        break;
                    }
                    
                    // 100000000 - vm addr
                    // 100003f78 - intruction addr
                    // auto text = find_proc_text_segment(pid, (char*)actual_load_address); 
                    uintptr_t slide = actual_load_address - (uintptr_t)text_seg.vmaddr;
                    std::cout << "Slide:" << slide << '\n';
                    // std::cout << "TEXT:" << text << '\n';
                    uintptr_t valid_address = (uintptr_t)(0x100003f78) + slide;
                    std::cout << "Valid address:" << valid_address << '\n';
                    auto res = read_proc_mem<size_t>(pid, valid_address);
                    std::cout << "RES:" << res;
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
    auto seg = find_segment_header(cli.f, "__TEXT");
    seg.print();
    cli.run();
}

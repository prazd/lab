#include <iostream>
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <filesystem>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utility>
#include <signal.h>
#include <spawn.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
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

    template <typename T>
    T read_proc_mem(size_t pid, uintptr_t address){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << '\n';
            return T{};
        }
        vm_offset_t buffer_address = 0;
        mach_msg_type_number_t count;
        mach_vm_size_t sz = sizeof(T);
        T val{};
        kr = mach_vm_read(task, (mach_vm_address_t)address, sz, &buffer_address, &count);
        if (kr != KERN_SUCCESS){
            std::cout << "mach_vm_read:" << mach_error_string(kr) << '\n';
            return T{};
        }
        memcpy(&val, (void*)buffer_address, count);
        vm_deallocate(mach_task_self(), buffer_address, count);
        return val;
    }
    
    void write_proc_mem_text(size_t pid, unsigned char value, uintptr_t address){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << '\n';
            return;
        }

        vm_region_submap_info_data_64_t info;
        mach_vm_address_t addr = 0;
        mach_vm_size_t ssz = 0;
        natural_t nesting_depth = 0;
        mach_msg_type_number_t info_cnt = VM_REGION_SUBMAP_INFO_COUNT_64;
        kr = mach_vm_region_recurse(task, &addr, &ssz,&nesting_depth, (vm_region_info_t)&info, &info_cnt);
        if (kr != KERN_SUCCESS){
            std::cout << "vm_region_recurse:" << mach_error_string(kr) << '\n';
            return;
        }
        std::cout << "info.protection:" << info.protection << std::endl;
        std::cout << "info.max_protection:" << info.max_protection << std::endl;
        

        // TODO: test
        mach_vm_size_t sz = sizeof(value);
        kr = mach_vm_protect(task, address, sz, false, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
        if (kr != KERN_SUCCESS){
            std::cout << "mach_vm_protect:" << mach_error_string(kr) << '\n';
            return;
        }

        kr = mach_vm_write(task, (mach_vm_address_t)address, (vm_offset_t)&value,  sz);
        if (kr != KERN_SUCCESS){
            std::cout << "mach_vm_write:" << mach_error_string(kr) << '\n';
            return;
        }

        kr = mach_vm_protect(task, address, sz, false, VM_PROT_READ | VM_PROT_EXECUTE);
        if (kr != KERN_SUCCESS){
            std::cout << "mach_vm_protect:" << mach_error_string(kr) << '\n';
            return;
        }

    }

    void single_step_proc_set(size_t pid, bool v){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << '\n';
            return;
        }
        thread_act_port_array_t thread_list;
        mach_msg_type_number_t thread_count;
        kr = task_threads(task, &thread_list, &thread_count);
        if (kr != KERN_SUCCESS){
            std::cout << "task_threads:" << mach_error_string(kr) << '\n';
            return;
        }
        if (thread_count > 0){
            x86_thread_state64_t state;
            mach_msg_type_number_t s_count = x86_THREAD_STATE64_COUNT;
            kr = thread_get_state(thread_list[0], x86_THREAD_STATE64, (thread_state_t)&state, &s_count);
            if (kr != KERN_SUCCESS){
                std::cout << "thread_get_state:" << mach_error_string(kr) << '\n';
                return;
            }
            state.__rflags &= ~((size_t)(1 << 8));
            state.__rflags |= (v << 8);
            kr = thread_set_state(thread_list[0], x86_THREAD_STATE64, (thread_state_t)&state, s_count);
            if (kr != KERN_SUCCESS){
                std::cout << "thread_set_state:" << mach_error_string(kr) << '\n';
            }
        } 
    }

    x86_thread_state64_t read_proc_regs(size_t pid){
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << " PID:" << pid << '\n';
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
            // std::cout << "Read state succesfully\n";
            return state;
        } 
        std::cout << "Thread count:" << thread_count << '\n';
        return x86_thread_state64_t{};
    }

    x86_thread_state64_t read_proc_regs(mach_port_t* task, size_t pid){
        thread_act_port_array_t thread_list;
        mach_msg_type_number_t thread_count;

        auto kr = task_threads(*task, &thread_list, &thread_count);
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
            // std::cout << "Read state succesfully\n";
            return state;
        } 
        std::cout << "Thread count:" << thread_count << '\n';
        return x86_thread_state64_t{};
    }

    uintptr_t find_instructions(size_t pid, size_t ins, uintptr_t rip_address){
        std::cout << "Find instructions" << std::endl;
        mach_port_t task;
        auto kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS){
            std::cout << "task_for_pid:" << mach_error_string(kr) << std::endl;
            return 0;
        }
        vm_offset_t buffer_address = 0;
        mach_msg_type_number_t count;
        mach_vm_size_t sz = sizeof(ins);
        size_t val{};
        int status{};
        for (;;){
            // std::cout << "Rip:" << rip_address << std::endl;
            kr = mach_vm_read(task, (mach_vm_address_t)rip_address, sz, &buffer_address, &count);
            if (kr != KERN_SUCCESS){
                std::cout << "mach_vm_read:" << mach_error_string(kr) << std::endl;
                return 0;
            }
            memcpy(&val, (void*)buffer_address, count);
            vm_deallocate(mach_task_self(), buffer_address, count);
            if (val == ins){
                std::cout << "Found!!!!" << std::endl;
                return rip_address;
            }
            ptrace(PT_STEP, pid, 0, 0);
            waitpid(pid, &status, 0);
            rip_address = (uintptr_t)read_proc_regs(pid).__rip;
        }
        return 0;
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
        size_t s_count = 1;
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
                auto header = read_proc_mem<MachOHeader>(pid, rip_address);
                MapNum<std::string_view, 0x18> mn(cpu_type_str, cpu_type_str + CPUTypeCount, cpu_type_num);
                std::cout << "MagicNumber:" << header.magic_number << '\n';
                std::cout << "CpuType:" << mn[header.cpu_type & 0x1f] << '\n';
                std::cout << "FileType:" << header.file_type << '\n';
                std::cout << "NumberOfLoadCommands:" << header.number_of_load_commands << '\n';
                std::cout << "SizeOfLoadCommands:" << header.size_of_load_commands << '\n';
                std::cout << "Flags:" << header.flags << '\n';
                std::cout << "Reseved:" << header.reserved << "\n\n";
                return rip_address;
            }
            rip_address -= 0x1000; // page size 4kb
        }
        return 0;
    }

    unsigned char patch(std::vector<char>& vec, size_t inst){
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
                int status = 0;
                auto s = std::string(args[1]);
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
                if (!o_f.is_open()){std::cout << "file is not open"; break;}
                unsigned char prev_value = patch(e_f, 0xb00000004f3d8d48);

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
                        break;
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
                        auto r = read_proc_regs(pid);
                        auto val = read_proc_mem<size_t>(pid, r.__rip);
                        if (val == 0xb00000004f3d8d){
                            auto actual_load_address = find_proc_base_address(pid, (uintptr_t)r.__rip);
                            if (actual_load_address == 0){
                                std::cout << "Addr not found\n";
                                break;
                            }
                            std::cout << "Actual load address:" << actual_load_address << std::endl;
                            std::cout << "!!!FOUND!!!" << " Address:" << r.__rip << std::endl;
                            write_proc_mem_text(pid, prev_value, r.__rip-1);
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
    print_macho_header(cli.f);
    print_dwarf_cu_header(cli.f, find_section(cli.f, "__DWARF", "__debug_info"));
    auto seg = find_segment_header(cli.f, "__TEXT");
    seg.print();
    cli.run();
}

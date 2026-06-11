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

    size_t debug_pid = 0;

    uintptr_t base_image_address = 0;
#if defined(__linux__)    
    std::vector<ProcMapEntry> proc_map_entry;
#endif
    struct BreakEntry {
        size_t line;
        uintptr_t address;
        std::string filename;
        size_t filename_index;
        size_t original_instruction_value;
        bool operator==(const BreakEntry& rhs){
            return rhs.line == line && address == rhs.address && filename_index == rhs.filename_index;
        }
        inline void print(){
            PRINT(line)
            PRINT(address)
            PRINT(filename_index)
            PRINT(original_instruction_value)
        }
    };
    bool next_br = false;
    static inline BreakEntry next_br_entry;
    std::vector<BreakEntry> br_entries;
    std::vector<char*> process_args_list = {nullptr};
    std::vector<char*> process_env_list  = {nullptr};

    unsigned char last_instr = 0;
    
    CLI(const char* filename): filename(filename), f(read_file(filename)){
#if defined(__APPLE__)        
        vm.parse_header(f, macho_find_section(f, "__DWARF", "__debug_line").offset);
#elif defined(__linux__)
        vm.parse_header(f, elf_find_section_offset_by_name(f, ".debug_line"), elf_find_section_offset_by_name(f, ".debug_line_str"));
#endif
    }
    enum Command{
        Run, Break, UnBreak, Next,
        Regs,
        List,
        PrintBreakpoints,
        PrintIncludeDirs,
        PrintFileEntries,
        PrintCommitedDwarfVMState,
        Continue,
        Unknown};

    Command parse_cmd(std::string_view sv){
        if      (sv == "break" || sv == "b")      return Break;
        else if (sv == "unbreak" || sv == "unb")  return UnBreak;
        else if (sv == "run" || sv == "r")        return Run;
        else if (sv == "continue" || sv == "c")   return Continue;
        else if (sv == "iregs" || sv == "ir")     return Regs;
        else if (sv == "next" || sv == "n")       return Next;
        else if (sv == "list" || sv == "l")       return List;

        else if (sv == "pbr")   return     PrintBreakpoints;
        else if (sv == "pincd") return     PrintIncludeDirs;
        else if (sv == "pfe")    return     PrintFileEntries;
        else if (sv == "pcds")    return    PrintCommitedDwarfVMState;
        return Unknown;
    }

    enum Mode {Main, Debug};
    Mode mode = Main;

    enum DebugProcState{
        ProcRunning, ProcStopped, ProcFinish, 
    };
    DebugProcState debug_proc_state = ProcFinish;

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
    void linux_print_current_source_line(BreakEntry& br){
        std::istringstream source_file(vm.lists.file_entries[br.filename_index].fbuf.data());
        size_t lines_count = 1;
        std::string l_str;
        while(std::getline(source_file, l_str)){
            if (lines_count == br.line)
                std::cout << ' ' << lines_count << "   " << l_str << '\n';
            ++lines_count;
        }
    }

    void linux_signal_handler(){
        for (;;){
            int status{};
            waitpid(debug_pid, &status, 0);
            if (WIFSTOPPED(status)){
                if (auto sig = WSTOPSIG(status); sig == SIGTRAP){
                    auto regs = linux_read_regs(debug_pid);
                    BreakEntry* be_ptr = nullptr;
                    if (next_br)
                        be_ptr = &next_br_entry;
                    else {
                        for (size_t i = 0, e = br_entries.size(); i < e; ++i){
                            if (regs.rip-1 == base_image_address + br_entries[i].address){
                                be_ptr = &br_entries[i];
                                break;
                            }
                        }
                }
                std::cout << "Handling...\n";
                std::cout << "int3 address found\n";
                size_t current_ins_val = ptrace(PTRACE_PEEKDATA, debug_pid, regs.rip-1, 0);
                auto prev_value = be_ptr->original_instruction_value;
                int err = ptrace(PTRACE_POKEDATA, debug_pid, regs.rip-1, (void*)prev_value);
                if (err == -1){std::cout << "PTRACE_POKEDATA error\n";}
                --regs.rip;
                linux_write_regs(debug_pid, regs);
                linux_print_current_source_line(*be_ptr);
#if DEBUG                            
                auto update_regs = linux_read_regs(debug_pid);
                assert(update_regs.rip == regs.rip);
                auto updated_ins_val = ptrace(PTRACE_PEEKDATA, debug_pid, update_regs.rip, 0);
                assert(updated_ins_val == prev_value);
#endif                            
                debug_proc_state = ProcStopped;
                break;
                }
            } else {
                next_br = false;
                debug_proc_state = ProcFinish;
                mode = Main;
                std::cout << "Finish\n";
                break;
            }
        }
    }

    void linux_patch_by_address(uintptr_t br_addr){
        uintptr_t addr = base_image_address + br_addr;
        auto val = ptrace(PTRACE_PEEKDATA, debug_pid, addr, 0);
        auto it = std::find_if(br_entries.begin(), br_entries.end(),[br_addr](auto& el){return el.address == br_addr;});
        if (it == std::end(br_entries)){
            std::cout << "int3 patch error: breakpoint address not found\n";
            return;
        }
        it->original_instruction_value = val;
        val &= 0xffffffffffffff00;
        val |= 0xcc; // int3
        if (ptrace(PTRACE_POKEDATA, debug_pid, addr, (void*)val) == -1)
            std::cout << "PTRACE_POKEDATA error\n";
        auto updated_value = ptrace(PTRACE_PEEKDATA, debug_pid, addr, 0);
        assert(val == updated_value);
    }

    void linux_patch_br_entry(uintptr_t br_addr){
        uintptr_t addr = base_image_address + br_addr;
        auto val = ptrace(PTRACE_PEEKDATA, debug_pid, addr, 0);
        next_br_entry.original_instruction_value = val;
        val &= 0xffffffffffffff00;
        val |= 0xcc; // int3
        if (ptrace(PTRACE_POKEDATA, debug_pid, addr, (void*)val) == -1)
            std::cout << "PTRACE_POKEDATA error\n";
        auto updated_value = ptrace(PTRACE_PEEKDATA, debug_pid, addr, 0);
        assert(val == updated_value);
    }

    void elf_start_debug(){
        debug_pid = fork();
        if (debug_pid == 0){
            ptrace(PTRACE_TRACEME,0,0,0);
            execve(filename, process_args_list.data(), process_env_list.data());
        } else {
            waitpid(debug_pid, 0, 0);
            // use valid address
            // slide = actual_load_address - mach_o_preffered_address
            // valid_address = dwarf_address + slide
            // 0x1195
            auto min_load_address = elf_find_min_load_address(f);
            auto elf_header = parse_to<Elf64Header>(f.data());
            std::string proc_path = "/proc/";
            proc_path += std::to_string(debug_pid);
            proc_path += std::string("/maps");
            auto res = read_file_str(proc_path.c_str());
            std::cout << "ProcMapEntry:\n";
            
            ProcMapEntry pme;
            for (const auto& el: res){
                pme.parse_from_line(el);
                proc_map_entry.push_back(pme);
            }

            unsigned long base_address = 0;
            for (const auto& el: proc_map_entry){
                if (el.permissions.get(1) && el.permissions.get(0x4) && el.path_name.contains(std::string_view(filename))){
                    base_address = el.vma_range.first - el.offset;
                }
            }
            size_t header_begin{};
            memcpy(&header_begin, &elf_header, sizeof(header_begin));

            size_t word = ptrace(PTRACE_PEEKDATA, debug_pid, base_address, 0);
            assert(word == header_begin);

            base_image_address = base_address - min_load_address;

            for (auto el: br_entries)
                linux_patch_by_address(el.address);
        
            ptrace(PTRACE_CONT, debug_pid, 1, 0);
            mode = Debug;
            debug_proc_state = ProcRunning;
            linux_signal_handler();
        }
    }
#endif

#if defined(__APPLE__)
    void macho_start_debug(){
        int status = 0;
        auto d_f = read_file(filename);
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
                std::cout << "prog:" << filename << '\n';
                auto e_f = read_file(filename);
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

                debug_pid = fork();
                if (debug_pid == 0){
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
                        waitpid(debug_pid, &status, 0);
                        std::cout << "Step" << std::endl;
                        auto r = macho_read_proc_regs(debug_pid);
                        auto val = macho_read_proc_mem<size_t>(debug_pid, r.__rip);
                        if (val == 0xb00000004f3d8d){
                            auto actual_load_address = macho_find_proc_base_address(debug_pid, (uintptr_t)r.__rip);
                            if (actual_load_address == 0){
                                std::cout << "Addr not found\n";
                                break;
                            }
                            std::cout << "Actual load address:" << actual_load_address << std::endl;
                            std::cout << "!!!FOUND!!!" << " Address:" << r.__rip << std::endl;
                            macho_write_proc_mem_text(debug_pid, prev_value, r.__rip-1);
                            break;
                        } else {
                            std::cout << "RIP:" << r.__rip << " val:" << val << std::endl;

                        }
                        ptrace(PT_CONTINUE, debug_pid, (caddr_t)1, 0);
                    }
                    // auto ins_addr = find_instructions(pid, 0x480000000afc45c7,(uintptr_t)r.__rip);
                    // std::cout << "Ins addr:" << ins_addr << std::endl;
                    // // 100000000 - vm addr
                    // // 100003f78 - intruction addr
   
                    // // std::cout << "RES:" << res
                    // ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);

                }
                for (size_t i = 0, e = br_entries.size(); i != e; ++i){
                    std::cout << "Addr:" << br_entries[i].address << " Val:" << std::hex << 
                        std::setw(2) << std::setfill('0') << (int)br_entries[i].original_instruction_value << '\n';
                }
    }
#endif

    void add_to_break_list(size_t line, const std::string& filename){
        auto addr = vm.get_address_by_file_and_line(filename, line);
        if (addr != nullptr){
            if (std::any_of(br_entries.begin(), br_entries.end(), [addr](auto& el){return (uintptr_t)el.address == (uintptr_t)addr;})){
                std::cout << "Already set\n";
            } else {
                auto f_index = vm.lists.get_file_entries_index(filename);
                if (f_index == -1)
                    std::cout << "add to break list failed: file not found\n";
                else 
                    br_entries.push_back({line, (uintptr_t)addr, filename, (size_t)f_index});
            }
        }  
    }

    void add_to_break_list(uintptr_t addr, size_t line, const std::string& filename){
        if (std::any_of(br_entries.begin(), br_entries.end(), [addr](auto& el){return (uintptr_t)el.address == (uintptr_t)addr;})){
            std::cout << "Already set\n";
        } else {
            auto f_index = vm.lists.get_file_entries_index(filename);
            if (f_index == -1)
                std::cout << "add to break list failed: file not found\n";
            else 
                br_entries.push_back({line, (uintptr_t)addr, filename, (size_t)f_index});
        }
    }

     void update_next_br_entry(uintptr_t addr, size_t line, const std::string& filename){
        auto f_index = vm.lists.get_file_entries_index(filename);
        if (f_index == -1)
            std::cout << "add to break list failed: file not found\n";
        else 
            next_br_entry = BreakEntry({line, (uintptr_t)addr, filename, (size_t)f_index});
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
                if (ec == std::errc{})
                    add_to_break_list(line, std::string(args[1]));
                else
                    std::cout << "line parse error\n";
                break;
            }
            case UnBreak: {
                if (args.size() != 3) {
                    std::cout << "args sz parse error\n";
                    break;
                }
                size_t line{};
                auto [ptr, ec] = std::from_chars(args[2].begin(), args[2].end(), line);
                if (ec == std::errc{}){               
                    auto addr = vm.get_address_by_file_and_line(std::string(args[1]), line);
                    if (addr != nullptr){
                        for (size_t i = 0, e = br_entries.size(); i != e; ++i){
                            if (br_entries[i].address == (uintptr_t)addr && br_entries[i].line == line){
                                br_entries.erase(std::remove(br_entries.begin(), br_entries.end(), br_entries[i]), br_entries.end());                              
                                std::cout << "del breakpoint\n";
                                break;
                            }
                        }
                    }
                } else
                    std::cout << "line parse error\n";
                break;
            }
            case Next: {
#if defined(__linux__)                             
                if (next_br){
                    if (auto addr = (uintptr_t)vm.get_address_by_file_and_line(next_br_entry.filename, next_br_entry.line + 1); addr){
                        update_next_br_entry(addr, next_br_entry.line + 1, next_br_entry.filename);
                        linux_patch_br_entry(addr);
                    }
                } else {
                auto regs = linux_read_regs(debug_pid);
                 for (size_t i = 0, e = br_entries.size(); i < e; ++i){
                    if (regs.rip == base_image_address + br_entries[i].address){
                            if (auto addr = (uintptr_t)vm.get_address_by_file_and_line(br_entries[i].filename, br_entries[i].line + 1); addr){
                                update_next_br_entry(addr, br_entries[i].line + 1, br_entries[i].filename);
                                linux_patch_br_entry(addr);
                                next_br = true;
                            }
                            break;
                        }
                    }
                }
                
                ptrace(PTRACE_CONT, debug_pid, 0, 0);
                linux_signal_handler();
#endif                
                break;
            }
            case Unknown: {std::cout << "Unknown cmd\n"; break;}
            case Run:     {
#if defined(__APPLE__)
                macho_start_debug();
#elif defined(__linux__)
                elf_start_debug();
#endif
                break;
            }
            case Continue: {
#if defined(__linux__)
                if (mode == Debug && debug_proc_state == ProcStopped){
                    ptrace(PTRACE_CONT, debug_pid, 0, 0);
                    debug_proc_state = ProcRunning;
                    next_br = false;
                    linux_signal_handler();
                }
                break;
#endif
            }
            case Regs: {
#if defined(__linux__)                
                if (mode == Debug && debug_proc_state == ProcStopped){
                    auto regs = linux_read_regs(debug_pid);
                    std::cout << std::hex;
                    std::cout << "RIP:" << regs.rip << '\n';
                    std::cout << "RAX:" << regs.rax << '\n';
                    std::cout << "RBX:" << regs.rbx << '\n';
                    std::cout << "RCX:" << regs.rcx << '\n';
                    std::cout << "RDX:" << regs.rdx << '\n';
                    std::cout << "RDI:" << regs.rdi << '\n';
                    std::cout << "RSI:" << regs.rsi << '\n';
                    std::cout << "RBP:" << regs.rbp << '\n';
                    std::cout << "RSP:" << regs.rsp << '\n';
                    std::cout << "R8:"  << regs.r8 << '\n';
                    std::cout << "R9:"  << regs.r9 << '\n';
                    std::cout << "R10:" << regs.r10 << '\n';
                    std::cout << "R11:" << regs.r11 << '\n';
                    std::cout << "R12:" << regs.r12 << '\n';
                    std::cout << "R13:" << regs.r13 << '\n';
                    std::cout << "R14:" << regs.r14 << '\n';
                    std::cout << "R15:" << regs.r15 << '\n';
                }
#endif                
                break;
            }
            case PrintBreakpoints: {
                std::cout << "\n====\n"; 
                for (auto& el: br_entries) {el.print(); std::cout << "\n====\n";}
                break;
            }
            case List: {
#if defined(__linux__)      
                if (debug_proc_state == ProcStopped){
                    auto regs = linux_read_regs(debug_pid);
                    for (size_t i = 0, e = br_entries.size(); i != e; ++i){
                        if (regs.rip == base_image_address + br_entries[i].address){
                            std::istringstream source_file(vm.lists.file_entries[br_entries[i].filename_index].fbuf.data());
                            size_t lines_count = 1;
                            std::string l_str;
                            while(std::getline(source_file, l_str)){
                                if (lines_count == br_entries[i].line)
                                    std::cout << "==>";
                                else 
                                    std::cout << "   ";
                                std::cout << ' ' << lines_count++ << "   " << l_str << '\n';
                            }
                        }
                    }
                }
#endif
                break;
            }

            case PrintFileEntries:{
                for (auto& el: vm.lists.file_entries)
                    el.print();
                break;
            }
            case PrintCommitedDwarfVMState:{
                for (auto& el: vm.commited_states)
                    el.print();
                break;
            }
            case PrintIncludeDirs: {
                for (size_t i = 0, e = vm.lists.include_directories.size(); i != e; ++i)
                    std::cout <<  vm.lists.include_directories[i] << '\n';
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
#if 1
    cli.run();
#else    
    auto debug_abbrev_sh = elf_find_section_header_by_name(cli.f, ".debug_abbrev");
    debug_abbrev_sh.print();
    std::cout << "\n";
    auto r = parse_debug_abbrev(cli.f, debug_abbrev_sh.sh_size, debug_abbrev_sh.sh_offset);
    for (auto& el: r) {el.print(); std::cout << "\n=====\n";}
    auto debug_info_sh = elf_find_section_header_by_name(cli.f, ".debug_info");
    debug_info_sh.print();
    std::cout << '\n';
    parse_debug_info_cu(cli.f, r, debug_info_sh.sh_size, debug_info_sh.sh_offset);
#endif    
#endif
}

#ifndef MACHO_HPP
#define MACHO_HPP
#include <chrono>
#include <thread>
#include <utility>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/ptrace.h>
#include "Mapnum.hpp"

struct MachOHeader {
    unsigned magic_number;
    unsigned cpu_type;
    unsigned cpu_subtype;
    unsigned file_type;
    unsigned number_of_load_commands;
    unsigned size_of_load_commands;
    unsigned flags;
    unsigned reserved;
};

struct LoadCommands {
    unsigned ncmds;
    unsigned szcmds;
};

enum CommandTypes {
    LC_SEGMENT=0x1,
    LC_SYMTAB=0x2,
    LC_THREAD=0x4,
    LC_UNIXTHREAD=0x5,
    LC_LOAD_DYLIB=0xC,
    LC_LOAD_DYLINKER=0xE,
    LC_UUID=0x1B,
    LC_SEGMENT_64=0x19,
    LC_CODE_SIGNATURE=0x1D,
    LC_BUILD_VERSION=0x32,
    LC_MAIN=0x80000028,
    LC_LOAD_WEAK_DYLIB=0x80000018,
};

struct Section64 {
    char sectionname[16];
    char segname[16];
    unsigned long addr;
    unsigned long sz;
    unsigned offset;
    unsigned align;
    unsigned reloff;
    unsigned nreloc;
    unsigned flags;
    unsigned reserved1;
    unsigned reserved2;
    unsigned reserved3; 
};

struct SegmentCommand64 {
    unsigned cmd;
    unsigned cmdsz;
    char segname[16];
    unsigned long vmaddr;
    unsigned long vmsize;
    unsigned long fileoff;
    unsigned long filesize;
    unsigned maxprot;
    unsigned initprot;
    unsigned nsects;
    unsigned flags;
    inline void print(){
        std::cout <<  "Cmd:"      << cmd << '\n';
        std::cout <<  "CmdSz:"    << cmdsz << '\n';
        std::cout <<  "Segname:"  << segname << '\n';
        std::cout <<  "Vmaddr:"   << vmaddr << '\n';
        std::cout <<  "Vmsize:"   << vmsize << '\n';
        std::cout <<  "Fileoff:"  << fileoff << '\n';
        std::cout <<  "Filesize:" << filesize << '\n';
        std::cout <<  "Maxprot:"  << maxprot << '\n';
        std::cout <<  "Initprot:" << initprot << '\n';
        std::cout <<  "Nsects:"   << nsects << '\n';
        std::cout <<  "Flags:"    << flags << '\n';
    }
};

enum CPUType {
    VAX=0x1, ROMP=0x2, NS32032=0x4,NS32332=0x5,MC680x0=0x6,x86=0x7,MIPS=0x8,NS32352=0x9,HP_PA=0xB,
    ARM=0xC,MC88000=0xD,SPARC=0xE,i860BE=0xF,i860LE=0x10,RS6000=0x11,POWERPC=0x12,RISCV=0x18, CPUTypeCount
};

constexpr size_t cpu_type_num[CPUTypeCount] = {
   VAX, ROMP, NS32032,NS32332,MC680x0,x86,MIPS,NS32352,HP_PA,
   ARM, MC88000,SPARC,i860BE,i860LE,RS6000,POWERPC,RISCV
};

constexpr std::string_view cpu_type_str[CPUTypeCount] = {
    "VAX", "ROMP", "NS32032","NS32332","MC680x0","x86","MIPS","NS32352","HP_PA",
    "ARM","MC88000","SPARC","i860BE","i860LE","RS6000","POWERPC","RISCV"
};

inline void print_macho_header(const std::vector<char>& f) {
    MachOHeader header;
    memcpy(&header, f.data(), sizeof(MachOHeader));
    MapNum<std::string_view, 0x18> mn(cpu_type_str, cpu_type_str + CPUTypeCount, cpu_type_num);
    std::cout << "MagicNumber:" << header.magic_number << '\n';
    std::cout << "CpuType:" << mn[header.cpu_type & 0x1f] << '\n';
    std::cout << "FileType:" << header.file_type << '\n';
    std::cout << "NumberOfLoadCommands:" << header.number_of_load_commands << '\n';
    std::cout << "SizeOfLoadCommands:" << header.size_of_load_commands << '\n';
    std::cout << "Flags:" << header.flags << '\n';
    std::cout << "Reseved:" << header.reserved << "\n\n";
}

inline MachOHeader parse_macho_header(const std::vector<char>& f) {
    MachOHeader header;
    memcpy(&header, f.data(), sizeof(MachOHeader));
    return header;
}

inline Section64 macho_find_section(const std::vector<char>& f, const char* segment, const char* section){
    auto ptr = f.data();
    auto e = ptr + f.size();
    ptr += sizeof(MachOHeader);
    LoadCommands lc;
    SegmentCommand64 sc;
    Section64 s64;
    unsigned s = 0;
    while(ptr != e){
        if (s == 0){
            memcpy(&lc, ptr, sizeof(LoadCommands));
            s += (lc.ncmds == LC_SEGMENT_64);
            ptr += s == 0 ? lc.szcmds : 0;
        } else if (s == 1){
            memcpy(&sc, ptr, sizeof(SegmentCommand64));
            std::cout << "Addr:" << (void*)ptr << " Segname:" << sc.segname << '\n';
            if (!strcmp(sc.segname, segment)){
                s += 1;
                ptr += sizeof(SegmentCommand64);
            }
            ptr += s == 1 ? sc.cmdsz: 0;
        } else if (s == 2) {
            memcpy(&s64, ptr, sizeof(Section64));
            std::cout << "Segname:" << s64.segname << " Sectionname:" << s64.sectionname << '\n';
            if (!strcmp(s64.sectionname, section)) return s64;
            ptr += sizeof(Section64);
        }
    }
    std::abort();
}

inline SegmentCommand64 macho_find_segment_header(const std::vector<char>& f, const char* segment){
    auto ptr = f.data();
    auto e = ptr + f.size();
    ptr += sizeof(MachOHeader);
    LoadCommands lc;
    SegmentCommand64 sc;
    unsigned s = 0;
    while(ptr != e){
        if (s == 0){
            memcpy(&lc, ptr, sizeof(LoadCommands));
            s += (lc.ncmds == LC_SEGMENT_64);
            ptr += s == 0 ? lc.szcmds : 0;
        } else if (s == 1){
            memcpy(&sc, ptr, sizeof(SegmentCommand64));
            std::cout << "Addr:" << (void*)ptr << " Segname:" << sc.segname << '\n';
            if (!strcmp(sc.segname, segment)){
                return sc;
            }
            ptr += s == 1 ? sc.cmdsz: 0;
        }
    }
    std::abort();
}


template <typename T>
inline T macho_read_proc_mem(size_t pid, uintptr_t address){
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


inline uintptr_t macho_find_proc_text_segment(size_t pid, char* ptr){
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
inline uintptr_t macho_find_proc_base_address(size_t pid, uintptr_t rip_address){
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
        if (val == 0xfeedfacf){ // magic number
            auto header = macho_read_proc_mem<MachOHeader>(pid, rip_address);
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




void macho_write_proc_mem_text(size_t pid, unsigned char value, uintptr_t address){
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


inline x86_thread_state64_t macho_read_proc_regs(mach_port_t* task, size_t pid){
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

inline x86_thread_state64_t macho_read_proc_regs(size_t pid){
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


inline uintptr_t macho_find_instructions(size_t pid, size_t ins, uintptr_t rip_address){
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
        rip_address = (uintptr_t)macho_read_proc_regs(pid).__rip;
    }
    return 0;
}


void macho_single_step_proc_set(size_t pid, bool v){
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

#endif
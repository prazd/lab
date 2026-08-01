#if defined(__APPLE__)
#ifndef MACHO_HPP
#define MACHO_HPP
#include <chrono>
#include <thread>
#include <utility>
#include <optional>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/ptrace.h>
#include <fstream>
#include <iostream>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <filesystem>

struct MachOHeader {
    unsigned magic_number{};
    unsigned cpu_type{};
    unsigned cpu_subtype{};
    unsigned file_type{};
    unsigned number_of_load_commands{};
    unsigned size_of_load_commands{};
    unsigned flags{};
    unsigned reserved{};
};

struct LoadCommands {
    unsigned ncmds{};
    unsigned szcmds{};
};


struct Section64 {
    char sectionname[16]{};
    char segname[16]{};
    unsigned long addr{};
    unsigned long sz{};
    unsigned offset{};
    unsigned align{};
    unsigned reloff{};
    unsigned nreloc{};
    unsigned flags{};
    unsigned reserved1{};
    unsigned reserved2{};
    unsigned reserved3{}; 
};

struct SegmentCommand64 {
    unsigned cmd{};
    unsigned cmdsz{};
    char segname[16]{};
    unsigned long vmaddr{};
    unsigned long vmsize{};
    unsigned long fileoff{};
    unsigned long filesize{};
    unsigned maxprot{};
    unsigned initprot{};
    unsigned nsects{};
    unsigned flags{};
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

struct SymtabCommand {
    unsigned cmd{};
    unsigned cmdsz{};
    unsigned symoff{};
    unsigned nsyms{};
    unsigned stroff{};
    unsigned strsize{};
    inline void print(){
        std::cout <<  "cmd:"      << cmd << '\n';
        std::cout <<  "cmdsz:"    << cmdsz << '\n';
        std::cout <<  "symoff:"  << symoff << '\n';
        std::cout <<  "nsyms:"   << nsyms << '\n';
        std::cout <<  "stroff:"   << stroff << '\n';
        std::cout <<  "strsize:"  << strsize << '\n';
    }
};

struct DySymtabCommand {
    unsigned cmd{};
    unsigned cmdsz{};
    unsigned ilocalsym{};
    unsigned nlocalsym{};
    unsigned iextdefsym{};
    unsigned nextdefsym{};
    unsigned iundefsym{};
    unsigned nundefsym{};
    unsigned tocoff{};
    unsigned ntoc{};
    unsigned modtaboff{};
    unsigned nmodtab{};
    unsigned extrefsymoff{};
    unsigned nextrefsyms{};
    unsigned indirectsymoff{};
    unsigned nindirectsyms{};
    unsigned extreloff{};
    unsigned nextrel{};
    unsigned locreloff{};
    unsigned nlocrel{};
};

inline MachOHeader parse_macho_header(const std::vector<char>& f) {
    MachOHeader header;
    memcpy(&header, f.data(), sizeof(MachOHeader));
    return header;
}

inline std::optional<Section64> macho_find_section(char* ptr, const char* segment, const char* section){
    MachOHeader header;
    memcpy(&header, ptr, sizeof(MachOHeader));
    LoadCommands lc;
    SegmentCommand64 sc;
    Section64 s64;
    unsigned s = 0;
    size_t current_nsec = 0;
    ptr += sizeof(MachOHeader);
    for (int i = 0; i < header.number_of_load_commands; ++i){
        if (s == 0){
            memcpy(&lc, ptr, sizeof(LoadCommands));
            s += (lc.ncmds == LC_SEGMENT_64);
            ptr += s == 0 ? lc.szcmds : 0;
            ++i;
        } else if (s == 1){
            memcpy(&sc, ptr, sizeof(SegmentCommand64));
            if (!strcmp(sc.segname, segment)){
                ++s;
                ptr += sizeof(SegmentCommand64);
                current_nsec = sc.nsects;
            }
            ptr += s == 1 ? sc.cmdsz: 0;
            ++i; // TODO: check
        } else if (s == 2) {
            memcpy(&s64, ptr, sizeof(Section64));
            if (!strcmp(s64.sectionname, section)) return s64;
            ptr += sizeof(Section64);
            if (current_nsec) --current_nsec;
            if (!current_nsec) break;
        }
    }
    return std::nullopt;
}

inline std::optional<SegmentCommand64> macho_find_segment_header(char* ptr, const char* segment){
    MachOHeader header;
    memcpy(&header, ptr, sizeof(MachOHeader));
    ptr += sizeof(MachOHeader);
    LoadCommands lc;
    SegmentCommand64 sc;
    unsigned s = 0;
    for (int i = 0; i < header.number_of_load_commands; ++i){
        if (s == 0){
            memcpy(&lc, ptr, sizeof(LoadCommands));
            s += (lc.ncmds == LC_SEGMENT_64);
            ptr += s == 0 ? lc.szcmds : 0;
        } else if (s == 1){
            memcpy(&sc, ptr, sizeof(SegmentCommand64));
            if (!strcmp(sc.segname, segment)){
                return sc;
            }
            ptr += s == 1 ? sc.cmdsz: 0;
        }
    }
    return std::nullopt;
}

template <typename T>
inline std::optional<T> macho_find_cmd(char* ptr, unsigned cmd){
    MachOHeader header;
    memcpy(&header, ptr, sizeof(MachOHeader));
    LoadCommands lc;
    ptr += sizeof(MachOHeader);
    for (int i = 0; i < header.number_of_load_commands; ++i){
        memcpy(&lc, ptr, sizeof(LoadCommands));
        if (lc.szcmds == 0){
            std::cout << "Not found\n";
            break;
        }
        if (lc.ncmds == cmd){
            T res;
            memcpy(&res, ptr, sizeof(T));
            return res;
        };
        ptr += lc.szcmds;
    }
    return std::nullopt;
}


auto read_file(const char* path){
    std::vector<char> res;
    auto sz = std::filesystem::file_size(path);
    res.resize(sz);
    std::ifstream f(path, std::ios::binary);
    f.read(res.data(), sz);
    return res;
}

// LC_SYMTAB::symoff
#pragma pack(push,1)
struct nlist64 {
    unsigned n_strx{};
    unsigned char  n_type{};
    unsigned char  n_sect{};
    unsigned short n_desc{};
    unsigned long  n_value{};
};
#pragma pack(pop)

inline vm_region_basic_info_data_64_t get_page_protection(void* addr){
    mach_vm_address_t r_addr = (mach_vm_address_t)((uintptr_t)addr);
    mach_vm_size_t sz = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t name;
    auto res = mach_vm_region(
        mach_task_self(),
        &r_addr,
        &sz,
        VM_REGION_BASIC_INFO_COUNT_64,
        (vm_region_info_t)&info,
        &count,
        &name
    );
    return info;
}

inline void patch_sym_runtime(char* base, const char* s, size_t slide, void* new_address){
    auto got = macho_find_section(base, "__DATA_CONST", "__got");
    if (!got.has_value()) return;
    size_t entries = (got->sz / sizeof(void*));

    auto lc_symtab_seg = macho_find_cmd<SymtabCommand>(base, LC_SYMTAB);
    if (!lc_symtab_seg.has_value()) return;

    auto lc_dysymtab_seg = macho_find_cmd<DySymtabCommand>(base, LC_DYSYMTAB);
    if (!lc_dysymtab_seg.has_value()) return;

    auto linkedit = macho_find_segment_header(base, "__LINKEDIT");
    if (!linkedit.has_value()) return;

    auto text = macho_find_segment_header(base, "__TEXT");
    if (!text.has_value()) return;

    auto linkedit_runtime = base + (linkedit->vmaddr - text->vmaddr);
    // std::cout << "Base:" << (void*)base << ' ' << " Text:" << (void*)text->vmaddr << '\n';
    // auto slide = base - text->vmaddr;

    auto get_addr_linkedit = [&](size_t offset){
        return linkedit_runtime + (offset - linkedit->fileoff);
    };

    auto get_addr_seg = [&](size_t address){
        return slide + address;
    };

    auto dysym_begin = (unsigned*)(get_addr_linkedit(lc_dysymtab_seg->indirectsymoff));

    for (int i = 0; i < entries; ++i){
        auto s_index = *(dysym_begin + got->reserved1 + i);
        if (s_index & INDIRECT_SYMBOL_LOCAL || s_index & INDIRECT_SYMBOL_ABS) continue;
        auto cur_n64 = ((nlist64*)(get_addr_linkedit(lc_symtab_seg->symoff))) + s_index;
        auto cur_str = (get_addr_linkedit(lc_symtab_seg->stroff)) + cur_n64->n_strx;
        if (!strcmp(cur_str, s)){
#if LOG
            std::cout << "!!!" << cur_str << "!!!";
#endif
            auto got_val = ((void**)(get_addr_seg(got->addr))) + i;

            auto page_addr = (void*)((uintptr_t)got_val & 0xfffffffffffff000);
            auto info = get_page_protection(page_addr);
            if (info.protection & VM_PROT_WRITE)
                *got_val = new_address;
            else {
                if (info.max_protection & VM_PROT_WRITE){
                    mach_vm_address_t r_addr = (mach_vm_address_t)((uintptr_t)page_addr);
                    mach_vm_size_t sz = vm_page_size;
                    
                    auto task = mach_task_self();
                    auto kr = mach_vm_protect(task, r_addr, sz, false, VM_PROT_READ | VM_PROT_WRITE);
                    if (kr != KERN_SUCCESS){
                        std::cout << "mach_vm_protect(1):" << mach_error_string(kr) << '\n';
                        return;
                    }

                    auto pp = get_page_protection(page_addr);
                    if (!(pp.protection & (VM_PROT_READ)) || !(pp.protection & (VM_PROT_WRITE))){
                        std::cout << "Update page error";
                        return;
                    }
                
                    *got_val = new_address;

                    kr = mach_vm_protect(task, r_addr, sz, false, info.protection);
                    if (kr != KERN_SUCCESS){
                        std::cout << "mach_vm_protect(2):" << mach_error_string(kr) << '\n';
                        return;
                    }
                } else {
                    std::cout << "max_protection write disabled:" << ((info.max_protection & VM_PROT_READ) ? 'r': '-') <<
                        ((info.max_protection & VM_PROT_WRITE) ? 'w': '-') << ((info.max_protection & VM_PROT_EXECUTE) ? 'x': '-');
                }
            }
            break;
        }      
    }
}

struct FunctionInfo {
    const char* sym;
    void* addr;
};

inline void rebind(const std::initializer_list<FunctionInfo>& il){
    auto count = _dyld_image_count();
    for (int i = 0; i < count; ++i){
        auto f_path = _dyld_get_image_name(i);
        auto img_base = (char*)_dyld_get_image_header(i);
#if LOG        
        std::cout << f_path << ' ' << (void*)img_base << ' ';
#endif 
        for (auto& fi: il){
            patch_sym_runtime(img_base, fi.sym, _dyld_get_image_vmaddr_slide(i), fi.addr);
        }
    
#if LOG        
        std::cout << "\n";
#endif
    }
}
#endif
#endif
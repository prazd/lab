#if defined(__APPLE__)
#ifndef MACHO_HPP
#define MACHO_HPP
#include <chrono>
#include <thread>
#include <utility>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/ptrace.h>

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
inline T macho_find_cmd(const std::vector<char>& f, unsigned cmd){
    auto ptr = f.data();
    auto e = ptr + f.size();
    ptr += sizeof(MachOHeader);
    LoadCommands lc;
    while(ptr < e){
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
    std::abort();
}
#endif
#endif
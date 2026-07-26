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
#endif
#endif
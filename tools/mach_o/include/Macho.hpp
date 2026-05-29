#ifndef MACHO_HPP
#define MACHO_HPP
#include <chrono>
#include <thread>
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



inline Section64 find_section(const std::vector<char>& f, const char* segment, const char* section){
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
#endif
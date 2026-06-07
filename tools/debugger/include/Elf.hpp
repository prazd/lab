#if defined(__linux__)
#ifndef ELF_HPP
#define ELF_HPP
#include <iostream>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "Utils.hpp"

#pragma pack(push, 1)
struct Elf64Header {
    unsigned char e_ident[16];
    unsigned short e_type;
    unsigned short e_machine;
    unsigned e_version;
    unsigned long e_entry;
    unsigned long e_phoff;
    unsigned long e_shoff;
    unsigned e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum; 
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
    void print(){
        PRINT(e_type)
        PRINT(e_machine)
        PRINT(e_version)
        PRINT(e_entry)
        PRINT(e_phoff)
        PRINT(e_shoff)
        PRINT(e_flags)
        PRINT(e_ehsize)
        PRINT(e_phentsize)
        PRINT(e_phnum)
        PRINT(e_shentsize)
        PRINT(e_shnum)
        PRINT(e_shstrndx)
    }
};

struct ElfProgramHeader {
    unsigned p_type;
    unsigned p_flags;
    unsigned long  p_offset;
    unsigned long  p_vaddr;
    unsigned long  p_paddr;
    unsigned long  p_filesz;
    unsigned long  p_memsz;
    unsigned long  p_align;
    inline void print(){
        PRINT(p_type)
        PRINT(p_flags)
        PRINT(p_offset)
        PRINT(p_vaddr)
        PRINT(p_paddr)
        PRINT(p_filesz)
        PRINT(p_memsz)
        PRINT(p_align)
    }
};

struct ElfSectionHeader {
    unsigned sh_name;
    unsigned sh_type;
    unsigned long  sh_flags;
    unsigned long  sh_addr;
    unsigned long  sh_offset;
    unsigned long  sh_size;
    unsigned  sh_link;
    unsigned  sh_info;
    unsigned long  sh_addralign;
    unsigned long  sh_entsize;
    inline void print(){
        PRINT(sh_name)
        PRINT(sh_type)
        PRINT(sh_flags)
        PRINT(sh_addr)
        PRINT(sh_offset)
        PRINT(sh_size)
        PRINT(sh_link)
        PRINT(sh_info)
        PRINT(sh_addralign)
        PRINT(sh_entsize)
    }
};
#pragma pack(pop)

enum ElfProgramType {
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR,
    PT_TLS
};

inline void print_elf_header(const std::vector<char>& f) {
    parse_to<Elf64Header>(f.data()).print();
}

inline unsigned long elf_find_section_offset_by_name(const std::vector<char>& f, const char* name){
    auto elf_header = parse_to<Elf64Header>(f.data());
    auto it = f.data() + elf_header.e_shoff;
    auto shrtab_offset = parse_to<ElfSectionHeader>(it + elf_header.e_shentsize * elf_header.e_shstrndx).sh_offset;
    auto shrtab_ptr = f.data() + shrtab_offset;
    for (size_t i = 0; i < elf_header.e_shnum; ++i){
        auto s_header = parse_to<ElfSectionHeader>(it);
        if (!strcmp((shrtab_ptr + s_header.sh_name), name)){
            return s_header.sh_offset;
        }
        it += elf_header.e_shentsize;
    }
    return 0;
}

inline unsigned long elf_find_min_load_address(const std::vector<char>& f){
    auto elf_header = parse_to<Elf64Header>(f.data());
    auto it = f.data() + elf_header.e_phoff;
    for (size_t i = 0; i < elf_header.e_shnum; ++i){
        auto p_header = parse_to<ElfProgramHeader>(it);
        if ((ElfProgramType)p_header.p_type == PT_LOAD){
            return p_header.p_vaddr & 0xfffffffffffff000;
        }
        it += elf_header.e_phentsize;
    }
    return 0;
}

inline user_regs_struct linux_read_regs(size_t pid){
    user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1)
        std::cout << "linux_read_regs error\n";
    return regs;
}

inline void linux_write_regs(size_t pid, const user_regs_struct& regs){
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1)
        std::cout << "linux_write_regs error\n";
}

inline unsigned long elf_base_image_address(size_t pid, uintptr_t rip, unsigned long header_begin){
    rip &= 0xfffffffffffff000;
    for (;;){
        auto word = ptrace(PTRACE_PEEKDATA, pid, rip, NULL);
        if (word == header_begin){
            return rip;
        }
        rip-=0x1000;
    }
    return 0;
}

struct ProcMapEntry {
    struct Permissions {
        enum Flag: unsigned char {Read=0x1,Write=0x2,Execute=0x4,Private=0x8,Shared=0x10};
        unsigned char value{};
        void reset(){value = 0;}
        void set(unsigned char m, bool v){
            value |= v ? m : 0;
        }
        bool get(unsigned char m) const {
            return value & m;
        }
        void toggle(unsigned char m){
            value ^= m;
        }
    };
    std::pair<size_t, size_t> vma_range{};
    Permissions permissions{};
    size_t offset{};
    std::pair<size_t, size_t> device;
    size_t inode{};
    std::string path_name;

    void parse_permissions(std::string_view sv){
        permissions.reset();
        for (auto el: sv){
            switch(el){
                case 'r': {permissions.set(Permissions::Flag::Read,    true); break;}
                case 'w': {permissions.set(Permissions::Flag::Write,   true); break;}
                case 'x': {permissions.set(Permissions::Flag::Execute, true); break;}
                case 'p': {permissions.set(Permissions::Flag::Private, true); break;}
                case 's': {permissions.set(Permissions::Flag::Shared,  true); break;}
                default: break;
            }
        }
    }

    void parse_from_line(const std::string_view line){
        auto splited = split_str(line, ' ');
        vma_range = parse_hex_to_u_pair(splited[0], '-');
        parse_permissions(splited[1]);
        if (auto [ptr, ec] = std::from_chars(splited[2].data(), splited[2].data() + splited[2].size(), offset, 16); ec != std::errc{}){
            std::cout << "Offset:" << splited[2] << '\n';
            std::cout << "offset parse error\n";
            offset = 0;
        }
        device = parse_hex_to_u_pair(splited[3], ':');
         if (auto [ptr, ec] = std::from_chars(splited[4].data(), splited[4].data() + splited[4].size(), inode, 16); ec != std::errc{}){
            std::cout << "Inode:" << splited[4] << '\n';
            std::cout << "inode parse error\n";
            offset = 0;
        }
        path_name = std::string(splited[5]);
    }

    void print()const{
        std::cout << vma_range.first << '-' << vma_range.second  << ' ' <<
        (int)(permissions.value) << ' ' << 
        offset << ' ' << 
        device.first << ':' << device.second << ' ' << 
        inode << ' ' << path_name;
    }
};
#endif
#endif
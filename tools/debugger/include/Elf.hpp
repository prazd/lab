#if defined(__linux__)
#ifndef ELF_HPP
#define ELF_HPP
#include <iostream>
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
        std::cout << "Name:" << (shrtab_ptr + s_header.sh_name) << '\n';
        if (!strcmp((shrtab_ptr + s_header.sh_name), name)){
            return s_header.sh_offset;
        }
        it += elf_header.e_shentsize;
    }
    return 0;
}
#endif
#endif
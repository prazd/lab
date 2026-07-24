#include <iostream>
#include <vector>
#include <dlfcn.h>
#include <sys/mman.h>
#include <mach-o/dyld.h>
#include <fstream>
#include <filesystem>
#include "Macho.hpp"

void* operator new(size_t n){
    printf("malloc:%zu", n);
    return malloc(n);
}

auto read_file(const char* path){
    std::vector<char> res;
    auto sz = std::filesystem::file_size(path);
    res.resize(sz);
    std::ifstream f(path, std::ios::binary);
    f.read(res.data(), sz);
    return res;
}

#pragma pack(push,1)
struct nlist64 {
    unsigned n_strx{};
    unsigned char  n_type{};
    unsigned char  n_sect{};
    unsigned short n_desc{};
    unsigned long  n_value{};
};
#pragma pack(pop)

void patch_sym(const std::vector<char>& f, const char* s){
    auto got = macho_find_section(f, "__DATA_CONST", "__got");
    auto lc_symtab_seg = macho_find_cmd<SymtabCommand>(f, LC_SYMTAB);
    auto lc_dysymtab_seg = macho_find_cmd<DySymtabCommand>(f, LC_DYSYMTAB);
    
    // TODO:
    // auto dysym_begin = (unsigned*)(f.data() + lc_dysymtab_seg.offset + 4);
    // auto sym_begin = (unsigned*)(f.data() + lc_symtab_seg.offset + 4);
    // for (int i = 0; i < 100; ++i){
    //     auto cur = *(dysym_begin + got.reserved1);
    // }
}

int main(int argc, char* argv[]){
    // auto count = _dyld_image_count();
    // for (int i = 0; i < count; ++i)
    //     std::cout << _dyld_get_image_name(i) << ' ' << _dyld_get_image_header(i) << '\n';
    auto f = read_file(argv[1]);
    patch_sym(f, "_mmap");

    void* addr = mmap(NULL, 200, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    munmap(addr, 200);
}

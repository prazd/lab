#include <iostream>
#include <vector>
#include <dlfcn.h>
#include <sys/mman.h>
#include <mach-o/dyld.h>
#include <fstream>
#include <filesystem>
#include "Macho.hpp"

#if 0
void* operator new(size_t n){
    printf("malloc:%zu", n);
    return malloc(n);
}
#endif

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

using mmap_f = void*(*)(void* addr, size_t len, int prot, int flags, int fd, off_t off);
extern "C" void* my_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off){
    static mmap_f mmap_orig = (mmap_f)dlsym(RTLD_NEXT, "mmap");
    printf("mmap:%zu", len);
    return mmap_orig(addr, len, prot, flags, fd, off);
}

void patch_sym_runtime(char* base, const char* s){
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

    auto get_addr = [&](size_t offset){
        return linkedit_runtime + (offset - linkedit->fileoff);
    };

    auto dysym_begin = (unsigned*)(get_addr(lc_dysymtab_seg->indirectsymoff));

    for (int i = 0; i < entries; ++i){
        auto s_index = *(dysym_begin + got->reserved1 + i);
        // TODO: check
        if (s_index == INDIRECT_SYMBOL_LOCAL || s_index == INDIRECT_SYMBOL_ABS) continue;
        auto cur_n64 = ((nlist64*)(get_addr(lc_symtab_seg->symoff))) + s_index;
        auto cur_str = (get_addr(lc_symtab_seg->stroff)) + cur_n64->n_strx;
        if (!strcmp(cur_str, s)){
            std::cout << "!!!Str:" << cur_str << "!!!";
            auto got_val = ((void**)(get_addr(got->offset))) + i;
        // TODO: finish
        //     *got_val = (void*)my_mmap;
            break;
        }
    }
}

void patch_sym_file(char* base, const char* s){
    auto got = macho_find_section(base, "__DATA_CONST", "__got");
    if (!got.has_value()) return;
    size_t entries = (got->sz / sizeof(void*));

    auto lc_symtab_seg = macho_find_cmd<SymtabCommand>(base, LC_SYMTAB);
    if (!lc_symtab_seg.has_value()) return;

    auto lc_dysymtab_seg = macho_find_cmd<DySymtabCommand>(base, LC_DYSYMTAB);
    if (!lc_dysymtab_seg.has_value()) return;

    auto dysym_begin = (unsigned*)(base + lc_dysymtab_seg->indirectsymoff);

    for (int i = 0; i < entries; ++i){
       auto s_index = *(dysym_begin + got->reserved1 + i);
        auto cur_n64 = ((nlist64*)(base + lc_symtab_seg->symoff)) + s_index;
        auto cur_str = (base + lc_symtab_seg->stroff) + cur_n64->n_strx;
        std::cout << "Str:" << cur_str << '\n';
        if (!strcmp(cur_str, s)){
            auto got_val = ((void**)(base + got->offset)) + i;
            *got_val = (void*)my_mmap;
            break;
        }
    }
}

int main(int argc, char* argv[]){
#if 1
    auto count = _dyld_image_count();
    for (int i = 0; i < count; ++i){
        auto f_path = _dyld_get_image_name(i);
        auto img_base = (char*)_dyld_get_image_header(i);
        std::cout << f_path << ' ' << (void*)img_base << ' ';
        patch_sym_runtime(img_base, "_mmap");
        std::cout << "\n";
    }
#else    
    auto f = read_file(argv[1]);
    patch_sym_file(f.data(), "_mmap");
#endif
    void* addr = mmap(NULL, 200, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    munmap(addr, 200);
}
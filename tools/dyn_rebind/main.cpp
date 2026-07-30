#include <iostream>
#include <vector>
#include <dlfcn.h>
#include <sys/mman.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <fstream>
#include <filesystem>
#include "Macho.hpp"

#define LOG 0

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
    printf("MMAP:%zu", len);
    return mmap_orig(addr, len, prot, flags, fd, off);
}

vm_region_basic_info_data_64_t get_page_protection(void* addr){
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
    auto slide = base - text->vmaddr;

    auto get_addr_linkedit = [&](size_t offset){
        return linkedit_runtime + (offset - linkedit->fileoff);
    };

    auto get_addr_seg = [&](size_t address){
        return slide + address;
    };

    auto dysym_begin = (unsigned*)(get_addr_linkedit(lc_dysymtab_seg->indirectsymoff));

    for (int i = 0; i < entries; ++i){
        auto s_index = *(dysym_begin + got->reserved1 + i);
        if (s_index == INDIRECT_SYMBOL_LOCAL || s_index == INDIRECT_SYMBOL_ABS) continue;
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
                *got_val = (void*)my_mmap;
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
                
                    *got_val = (void*)my_mmap;

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
#if LOG        
        std::cout << "Str:" << cur_str << '\n';
#endif
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
#if LOG        
        std::cout << f_path << ' ' << (void*)img_base << ' ';
#endif        
        patch_sym_runtime(img_base, "_mmap");
#if LOG        
        std::cout << "\n";
#endif
    }
#else    
    auto f = read_file(argv[1]);
    patch_sym_file(f.data(), "_mmap");
#endif
    void* addr = mmap(NULL, 200, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    munmap(addr, 200);
}
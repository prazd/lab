#include <dlfcn.h>
#include <sys/mman.h>
#include <cstdio>
#include "Rebind.hpp"

using mmap_f = void*(*)(void* addr, size_t len, int prot, int flags, int fd, off_t off);
extern "C" void* my_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off){
    static mmap_f mmap_orig = (mmap_f)dlsym(RTLD_NEXT, "mmap");
    printf("MMAP:%zu", len);
    return mmap_orig(addr, len, prot, flags, fd, off);
}

void __attribute__((constructor)) init(){
    rebind({{"_mmap", (void*)my_mmap}});
}
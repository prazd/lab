#include <iostream>
#include <vector>
#include <dlfcn.h>
#include <sys/mman.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <fstream>
#include <filesystem>
#define LOG 1
#include "Rebind.hpp"
#define REBIND_CALL 0

#if 0
void* operator new(size_t n){
    printf("malloc:%zu", n);
    return malloc(n);
}
#endif

#if REBIND_CALL
using mmap_f = void*(*)(void* addr, size_t len, int prot, int flags, int fd, off_t off);
extern "C" void* my_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off){
    static mmap_f mmap_orig = (mmap_f)dlsym(RTLD_NEXT, "mmap");
    printf("MMAP:%zu", len);
    return mmap_orig(addr, len, prot, flags, fd, off);
}
#endif

int main(int argc, char* argv[]){
#if REBIND_CALL
    rebind({{"_mmap", (void*)my_mmap}});
#endif    
    void* addr = mmap(NULL, 200, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    munmap(addr, 200);
}
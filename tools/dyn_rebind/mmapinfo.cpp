#include <dlfcn.h>
#include <sys/mman.h>
#include <cstdio>

void __attribute__((constructor)) init(){
    printf("Load\n");
}
using mmap_f = void*(*)(void* addr, size_t len, int prot, int flags, int fd, off_t off);

extern "C" void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off){ 
    static mmap_f mmap_orig = (mmap_f)dlsym(RTLD_NEXT, "mmap");
    printf("mmap:%zu", len);
    return mmap_orig(addr, len, prot, flags, fd, off);
}
#include <dlfcn.h>
#include <sys/mman.h>
#include <cstdio>
#include <unistd.h>
#include "Rebind.hpp"
#include <array>


template <size_t n>
static auto light_itoa(std::string_view s, size_t v) {
    std::array<char, n> res;
    std::copy(std::begin(s), std::end(s), std::begin(res));
    size_t i = n - 2;
    do{
       auto z = v % 10;
        res[i--] = 48 + z;
        v /= 10;
    } while(v);
    return res;
}

using mmap_f = void*(*)(void* addr, size_t len, int prot, int flags, int fd, off_t off);
extern "C" void* my_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off){
    static mmap_f mmap_orig = (mmap_f)dlsym(RTLD_NEXT, "mmap");
    static size_t n = 0;
    auto mmap_sz = light_itoa<5 + 20 + 1>("MMAP:....................\n", len);
    auto num = light_itoa<5 + 20 + 1>("CALL:....................|", n++);
    std::array<char, std::size(mmap_sz) + std::size(num)> res;
    std::copy(std::begin(num), std::end(num), std::begin(res));
    std::copy(std::begin(mmap_sz), std::end(mmap_sz), std::begin(res) + std::size(num));
    write(1, res.data(), res.size());
    return mmap_orig(addr, len, prot, flags, fd, off);
}

void __attribute__((constructor)) init(){
    rebind({{"_mmap", (void*)my_mmap}});
}
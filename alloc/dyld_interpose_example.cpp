#include <cstdio>
#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <atomic>
#include <thread>

constexpr size_t ARENA_SIZE = 50'000'000;

#define DYLD_INTERPOSE(_repl, _orig) \
__attribute__((used)) static struct { \
    const void* repl; \
    const void* orig; \
} _interpose_##_orig \
__attribute__((section("__DATA,__interpose"))) = { \
    (const void*)(unsigned long)&_repl, \
    (const void*)(unsigned long)&_orig \
};

template <size_t arena_sz>
struct SimpleArenaAllocator{
    std::atomic_flag af = ATOMIC_FLAG_INIT;
    void lock(){
        while(af.test_and_set(std::memory_order_acquire)){
            static constexpr int max_spins = 1000;
            for (int spins = 0; spins < max_spins; ++spins) {
                if (!af.test_and_set(std::memory_order_acquire)) {
                    return;
                }
            }
            std::this_thread::yield();
        }
    }
    void unlock(){
        af.clear(std::memory_order_release);
    }
    unsigned char* begin{};
    unsigned char* current{};
    SimpleArenaAllocator(): begin((unsigned char*)malloc(arena_sz)), current(begin){}
    void* alloc(size_t sz){
        lock();
        if (current + sz >= begin + arena_sz) {
            return nullptr;
        }
        auto res = current;
        current += sz;
        //  x + (8 - (x % 8))
        current += reinterpret_cast<unsigned long long>(current) % 8 ? (8 - reinterpret_cast<unsigned long long>(current) % 8) : 0;
        unlock();
        return res;
    }
    bool belong(unsigned char* addr) {
        return (addr >= begin && addr <= begin + arena_sz);
    }
    ~SimpleArenaAllocator() {
        free(begin);
    }
};

static auto& get_arena() {
    static SimpleArenaAllocator<ARENA_SIZE> sa;
    return sa;
}

extern "C" void* sa_malloc(size_t n) {
    if (auto addr = get_arena().alloc(n)) {
        write(2, "alloc custom\n", 13);
        return addr;
    }
    write(2, "alloc default\n", 14);
    return malloc(n);
}

extern "C" void sa_free(void* addr) {
    if (get_arena().belong((unsigned char*)addr)){ 
        write(2, "free custom\n", 12);
        return;
    }
    write(2, "free default\n", 13);
    free(addr);
}

DYLD_INTERPOSE(sa_malloc, malloc)
DYLD_INTERPOSE(sa_free, free)

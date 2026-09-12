#include <cerrno>
#include <iostream>
#include <sys/mman.h>
#include <utility>
#include <cassert>
#include <random>
#include <array>

template <size_t sz>
struct Mmap{
    void* addr{};
    void* sp{};
    Mmap():
    addr(mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)), 
    sp((reinterpret_cast<unsigned char*>(addr) + (sz - 1))){
        sp = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(sp) - (reinterpret_cast<uintptr_t>(sp) & 0xf))); // align 16
    }
    ~Mmap(){munmap(addr, sz);}
};

template <size_t n, typename ... Args>
decltype(auto) run_with_heap_sp(auto f, Args&& ... args){
    Mmap<n> m;
    unsigned long prev_sp{};
    asm volatile("movq %%rsp, %0":"=r"(prev_sp));
    asm volatile("movq %0, %%rsp"::"r"(m.sp));
    if constexpr (std::is_same_v<decltype(f(std::forward<Args>(args)...)), void>){
        f(std::forward<Args>(args)...);
        asm volatile("movq %0, %%rsp"::"r"(prev_sp));
    } else {
        auto r = f(std::forward<Args>(args)...);
        asm volatile("movq %0, %%rsp"::"r"(prev_sp));
        return r;
    }
}

int get_rand(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned char> ud(0, 255);

    volatile unsigned char arr[1 << 23]{};

    arr[0] = ud(gen);
    return arr[0];
}

size_t fib_r(size_t n){
    static std::array<size_t, 1200'000> prev_res{};
    if (n == 0) return 0;
    if (n == 1) return 1;
    if (prev_res[n]) return prev_res[n];
    prev_res[n-1] = fib_r(n-1);
    prev_res[n-2] = fib_r(n-2);
    return prev_res[n-1] + prev_res[n-2];
}

size_t fib(size_t n){
    size_t a = 0, b = 1;
    if (n == 0) return 0;
    if (n == 1) return 1;
    while(n--) a = std::exchange(b, b + a);
    return a;
}

int main(){
    auto f1 = fib(1000'000);
    auto f2 = run_with_heap_sp<(1 << 26)>(fib_r, 1000'000);
    assert(f1 == f2);
    std::cout << "fib(wrapped):" << f1 << '\n';
    std::cout << "fib_r(wrapped):" << f2 << '\n';
    std::cout << "rand:" << run_with_heap_sp<(1 << 25)>(get_rand);
}
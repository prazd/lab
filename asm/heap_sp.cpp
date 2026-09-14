#include <cerrno>
#include <iostream>
#include <sys/mman.h>
#include <signal.h>
#include <utility>
#include <unistd.h>
#include <cassert>
#include <random>
#include <array>
#include <cstring>

template <size_t sz>
struct HStackGuard{
    #define SIG_LOG 1
    static constexpr size_t SIG_STACK_SZ = 1 << 23;
    static constexpr size_t PAGE_SZ = 4096;
    static inline unsigned char* sig_stack_base{};
    static inline unsigned char* sig_sp{};
    static inline unsigned char* guard_page_addr{};
    static inline unsigned char* addr{};
    static inline unsigned char* sp{};
    static inline void unmap(){
        munmap(addr, sz);
        munmap(sig_stack_base, SIG_STACK_SZ);
    }
    static inline void sig_handler(int sig, siginfo_t* info, void*){
#if SIG_LOG
        static size_t count = 0;
        static char log_str[] = "SIG HANDLE:____________________\n";
        size_t i = 30;
        size_t n = count;
        do {log_str[i--] = 48 + (n % 10); n /= 10;} while(n);
        write(1, log_str, 32);
        ++count;
#endif
        mprotect(guard_page_addr, PAGE_SZ,  PROT_READ | PROT_WRITE);
        guard_page_addr -= PAGE_SZ;
        if (guard_page_addr <= addr) { write(1, "guard_page_addr <= addr", 23); _exit(1);}
        mprotect(guard_page_addr, PAGE_SZ, PROT_NONE);
    }
    static inline void reset(){
        unmap();
        struct sigaction sa{0};
        sa.sa_sigaction = sig_handler;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        
        sig_stack_base = (unsigned char*)mmap(NULL, SIG_STACK_SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        stack_t ss{};
        ss.ss_sp = sig_stack_base;
        ss.ss_size =SIG_STACK_SZ;
        ss.ss_flags = 0;
        sigaltstack(&ss, nullptr);


        if (sigaction(SIGSEGV, &sa, nullptr) == -1) {std::cout << "sigaction error"; std::abort();}
        if (sigaction(SIGBUS, &sa, nullptr) == -1)  {std::cout << "sigaction error"; std::abort();}
        addr = (unsigned char*)mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        sp = addr + (sz - 1);
        sp = (unsigned char*)((reinterpret_cast<uintptr_t>(sp) - (reinterpret_cast<uintptr_t>(sp) & 0xf))); // align 16
        guard_page_addr = (addr + sz) - (PAGE_SZ * 2);
        mprotect(addr, sz - PAGE_SZ, PROT_NONE);
    }
};


template <size_t sz>
struct HStack{
    void* addr{};
    void* sp{};
    HStack():
    addr(mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)), 
    sp((reinterpret_cast<unsigned char*>(addr) + (sz - 1))){
        sp = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(sp) - (reinterpret_cast<uintptr_t>(sp) & 0xf))); // align 16
    }
    ~HStack(){munmap(addr, sz);}
};

template <size_t n, typename ... Args>
decltype(auto) run_with_heap_sp(auto f, Args&& ... args){
    HStack<n> hs;
    unsigned long prev_sp{};
    asm volatile("movq %%rsp, %0":"=r"(prev_sp));
    asm volatile("movq %0, %%rsp"::"r"(hs.sp));
    if constexpr (std::is_same_v<decltype(f(std::forward<Args>(args)...)), void>){
        f(std::forward<Args>(args)...);
        asm volatile("movq %0, %%rsp"::"r"(prev_sp));
    } else {
        auto r = f(std::forward<Args>(args)...);
        asm volatile("movq %0, %%rsp"::"r"(prev_sp));
        return r;
    }
}

template <size_t sz, typename ... Args>
decltype(auto) run_with_dyn_stack(auto f, Args&& ... args){
    HStackGuard<sz> hs;
    hs.reset();
    unsigned long prev_sp{};
    asm volatile("movq %%rsp, %0":"=r"(prev_sp));
    asm volatile("movq %0, %%rsp"::"r"(hs.sp));
    if constexpr (std::is_same_v<decltype(f(std::forward<Args>(args)...)), void>){
        f(std::forward<Args>(args)...);
        asm volatile("movq %0, %%rsp"::"r"(prev_sp));
    } else {
        auto r = f(std::forward<Args>(args)...);
        asm volatile("movq %0, %%rsp"::"r"(prev_sp));
        return r;
    }
    hs.unmap();
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

unsigned char dyn_test(){
    volatile unsigned char x[4096 * 12]{};
    x[0] = 0xff;
    return x[0];
}

int main(){
    auto f1 = fib(1000'000);
    auto f2 = run_with_heap_sp<(1 << 26)>(fib_r, 1000'000);
    assert(f1 == f2);
    std::cout << "fib(wrapped):" << f1 << '\n';
    std::cout << "fib_r(wrapped):" << f2 << '\n';
    std::cout << "rand:" << run_with_heap_sp<(1 << 25)>(get_rand) << '\n';
    run_with_dyn_stack<(1 << 25)>(dyn_test);
}
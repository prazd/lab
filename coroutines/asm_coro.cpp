#include <iostream>
#include <unordered_map>
#include <cstring>

#define YIELD \
    asm("movq %%rsp, %0" :   "=r"(Coro::f_s[Coro::current_coro].begin)); \
    asm("movq %%rbp, %0" :   "=r"(Coro::f_s[Coro::current_coro].end)); \
    asm("movq %%rax, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RAX]));\
    asm("movq %%rbx, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RBX]));\
    asm("movq %%rcx, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RCX]));\
    asm("movq %%rdx, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RDX]));\
    asm("movq %%rdi, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RDI]));\
    asm("movq %%rsi, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RSI]));\
    asm("movq %%rbp, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RBP]));\
    asm("movq %%rsp, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RSP]));\
    asm("movq %%r8,  %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R8]));\
    asm("movq %%r9,  %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R9]));\
    asm("movq %%r10, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R10]));\
    asm("movq %%r11, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R11]));\
    asm("movq %%r12, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R12]));\
    asm("movq %%r13, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R13]));\
    asm("movq %%r14, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R14]));\
    asm("movq %%r15, %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::R15]));\
    asm("movq 8(%%rsp), %0" :  "=r"(Coro::f_s[Coro::current_coro].regs[Coro::State::RIP]));\

struct Coro {
    struct State {
        enum Reg {RAX, RBX, RCX, RDX, RDI, RSI, RBP, RSP,
                R8, R9, R10, R11, R12, R13, R14, R15, RIP, RegsCount};
        unsigned long regs[RegsCount]{};
        unsigned char stack[0x2000]{};
        uintptr_t begin = 0;
        uintptr_t end = 0;
        size_t top = 0;
        void print_stack(){
            for (ssize_t i = top-1; i>=0; --i)
                std::cout << '[' << i << "]:" << (int)stack[i] << '\n';
        }
    };
    static inline void(*current_coro)() = nullptr;
    static inline std::unordered_map<void(*)(),  State> f_s;
    static void dump_stack(){
        memcpy(f_s[current_coro].stack, (void*)(f_s[current_coro].begin), f_s[current_coro].end-f_s[current_coro].begin);
        f_s[current_coro].top = f_s[current_coro].end-f_s[current_coro].begin;
    }
};

void foo(){
    int x = 10;
    YIELD
    Coro::dump_stack();
    Coro::f_s[Coro::current_coro].print_stack();
    int z = 20;
}

int main(){
    Coro::f_s[foo] = Coro::State{};
    foo();
}

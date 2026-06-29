#include <iostream>
#include <unordered_map>

#define YIELD asm("call *%0" :: "r"((uintptr_t)Coro::yield) : "memory");

struct Coro {
    struct State {
        enum Reg {RAX, RBX, RCX, RDX, RDI, RSI, RBP, RSP,
                R8, R9, R10, R11, R12, R13, R14, R15, RIP, RegsCount};
        unsigned long regs[RegsCount];
    };

    static inline void(*current_coro)() = nullptr;
    static inline std::unordered_map<void(*)(),  State> f_s;

    static void yield(){
        asm("movq %%rax, %0" :  "=r"(f_s[current_coro].regs[State::RAX]));
        asm("movq %%rbx, %0" :  "=r"(f_s[current_coro].regs[State::RBX]));
        asm("movq %%rcx, %0" :  "=r"(f_s[current_coro].regs[State::RCX]));
        asm("movq %%rdx, %0" :  "=r"(f_s[current_coro].regs[State::RDX]));
        asm("movq %%rdi, %0" :  "=r"(f_s[current_coro].regs[State::RDI]));
        asm("movq %%rsi, %0" :  "=r"(f_s[current_coro].regs[State::RSI]));
        asm("movq %%rbp, %0" :  "=r"(f_s[current_coro].regs[State::RBP]));
        asm("movq %%rsp, %0" :  "=r"(f_s[current_coro].regs[State::RSP]));
        asm("movq %%r8,  %0" :  "=r"(f_s[current_coro].regs[State::R8]));
        asm("movq %%r9,  %0" :  "=r"(f_s[current_coro].regs[State::R9]));
        asm("movq %%r10, %0" :  "=r"(f_s[current_coro].regs[State::R10]));
        asm("movq %%r11, %0" :  "=r"(f_s[current_coro].regs[State::R11]));
        asm("movq %%r12, %0" :  "=r"(f_s[current_coro].regs[State::R12]));
        asm("movq %%r13, %0" :  "=r"(f_s[current_coro].regs[State::R13]));
        asm("movq %%r14, %0" :  "=r"(f_s[current_coro].regs[State::R14]));
        asm("movq %%r15, %0" :  "=r"(f_s[current_coro].regs[State::R15]));
        asm("movq 8(%%rbp), %0" :  "=r"(f_s[current_coro].regs[State::RIP]));
    }
};

void foo(){
    int x = 10;
    YIELD;
    int z = 20;
}

int main(){
    Coro::f_s[foo] = Coro::State{};
    foo();
}

#include <iostream>
#include <unordered_map>
#include <cstring>
#include <iomanip>

#define YIELD \
    asm("movq %%rsp, %0" : "=r"(Coro::f_s.begin)); \
    asm("movq %%rbp, %0" : "=r"(Coro::f_s.end)); \
    asm("movq %%rax, %0" : "=r"(Coro::f_s.regs[Coro::State::RAX]));\
    asm("movq %%rbx, %0" : "=r"(Coro::f_s.regs[Coro::State::RBX]));\
    asm("movq %%rcx, %0" : "=r"(Coro::f_s.regs[Coro::State::RCX]));\
    asm("movq %%rdx, %0" : "=r"(Coro::f_s.regs[Coro::State::RDX]));\
    asm("movq %%rdi, %0" : "=r"(Coro::f_s.regs[Coro::State::RDI]));\
    asm("movq %%rsi, %0" : "=r"(Coro::f_s.regs[Coro::State::RSI]));\
    asm("movq %%rbp, %0" : "=r"(Coro::f_s.regs[Coro::State::RBP]));\
    asm("movq %%rsp, %0" : "=r"(Coro::f_s.regs[Coro::State::RSP]));\
    asm("movq %%r8,  %0" : "=r"(Coro::f_s.regs[Coro::State::R8]));\
    asm("movq %%r9,  %0" : "=r"(Coro::f_s.regs[Coro::State::R9]));\
    asm("movq %%r10, %0" : "=r"(Coro::f_s.regs[Coro::State::R10]));\
    asm("movq %%r11, %0" : "=r"(Coro::f_s.regs[Coro::State::R11]));\
    asm("movq %%r12, %0" : "=r"(Coro::f_s.regs[Coro::State::R12]));\
    asm("movq %%r13, %0" : "=r"(Coro::f_s.regs[Coro::State::R13]));\
    asm("movq %%r14, %0" : "=r"(Coro::f_s.regs[Coro::State::R14]));\
    asm("movq %%r15, %0" : "=r"(Coro::f_s.regs[Coro::State::R15]));\
    asm("call *%0" :: "r"((uintptr_t)Coro::save_rip) : "memory");\
    asm("leave");\
    asm("ret");\

#define RESTORE \
        asm("movq %0,%%rbx" :: "m"(Coro::f_s.regs[Coro::State::RBX]) : "rbx");\
        asm("movq %0,%%rcx" :: "m"(Coro::f_s.regs[Coro::State::RCX]) : "rcx");\
        asm("movq %0,%%rdx" :: "m"(Coro::f_s.regs[Coro::State::RDX]) : "rdx");\
        asm("movq %0,%%rdi" :: "m"(Coro::f_s.regs[Coro::State::RDI]) : "rdi");\
        asm("movq %0,%%rsi" :: "m"(Coro::f_s.regs[Coro::State::RSI]) : "rsi");\
        asm("movq %0,%%r8"  :: "m"(Coro::f_s.regs[Coro::State::R8])  : "r8");\
        asm("movq %0,%%r9"  :: "m"(Coro::f_s.regs[Coro::State::R9])  : "r9");\
        asm("movq %0,%%r10" :: "m"(Coro::f_s.regs[Coro::State::R10]) : "r10");\
        asm("movq %0,%%r11" :: "m"(Coro::f_s.regs[Coro::State::R11]) : "r11");\
        asm("movq %0,%%r12" :: "m"(Coro::f_s.regs[Coro::State::R12]) : "r12");\
        asm("movq %0,%%r13" :: "m"(Coro::f_s.regs[Coro::State::R13]) : "r13");\
        asm("movq %0,%%r14" :: "m"(Coro::f_s.regs[Coro::State::R14]) : "r14");\
        asm("movq %0,%%r15" :: "m"(Coro::f_s.regs[Coro::State::R15]) : "r15");\
        asm("call *%0" :: "r"((uintptr_t)Coro::restore_rip) : "memory");\

struct Coro {
    struct State {
        enum Reg {RAX, RBX, RCX, RDX, RDI, RSI, RBP, RSP,
                R8, R9, R10, R11, R12, R13, R14, R15, RIP, RegsCount};
        static constexpr std::string_view regs_str[] = {
            "RAX", "RBX", "RCX", "RDX", "RDI", "RSI", "RBP", "RSP",
            "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RIP"
        };             
        unsigned long regs[RegsCount]{};
        unsigned char stack[0x2000]{};
        uintptr_t begin = 0;
        uintptr_t end = 0;
        size_t top = 0;
        State(){}
        void print(){
            for (ssize_t i = top-1; i>=0; --i)
                std::cout << '[' << i << "]:" << (int)stack[i] << '\n';    
            for (size_t i = 0; i < RegsCount; ++i){
                std::cout << std::hex << regs_str[i] << ':' << regs[i] << '\n';
            }
            int x = 10;
        }
    };
    static inline State f_s = State{};

    static void restore_rip(){
        asm("movq %0,8(%%rsp)" :: "r"(Coro::f_s.regs[Coro::State::RIP]) : "memory");
    }
        
    // TODO: debug
    static void save_rip(){
        asm("movq 8(%%rsp), %0" :  "=r"(Coro::f_s.regs[Coro::State::RIP]));
        Coro::f_s.regs[Coro::State::RIP] += 2;
    }
    static void dump_stack(){
        memcpy(f_s.stack, (void*)(f_s.begin), f_s.end-f_s.begin);
        f_s.top = f_s.end-f_s.begin;
    }
};

void foo(bool first){
    if (!first){
        RESTORE
    }
    // int x = 10;
    // Coro::dump_stack();
    printf("1");
    YIELD
    printf("3");
    YIELD
    printf("5");
}

int main(){
    foo(true);
    printf("2");
    foo(false);
    printf("4");
    foo(false);
    printf("6");
}

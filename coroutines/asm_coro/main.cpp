#include <iostream>
#include <unordered_map>
#include <cstring>
#include <iomanip>
#include <vector>
#include <utility>
#include <cassert>

extern "C" void store_rip(void*);
extern "C" void load_rip(void*);

#define SAVE_BEFORE_CALL\
    asm("movq %%rsp, %0" : "=r"(Coro::tmp.rsp1));\

#define SAVE_AFTER_CALL\
    asm("movq %%rsp, %0" : "=r"(Coro::tmp.rsp2));\

#define RESTORE_AFTER_CALL\
    asm("movq %0,%%rbp" :: "m"(Coro::tmp.regs[Coro::State::RBP]) :  "rbp");\
    asm("movq %0,%%rsp" :: "m"(Coro::tmp.regs[Coro::State::RSP]) :  "rsp");\

#define PROLOGUE\
    asm("movq %%rbx, %0" : "=r"(Coro::tmp.regs[Coro::State::RBX]));\
    asm("movq %%r12, %0" : "=r"(Coro::tmp.regs[Coro::State::R12]));\
    asm("movq %%r13, %0" : "=r"(Coro::tmp.regs[Coro::State::R13]));\
    asm("movq %%r14, %0" : "=r"(Coro::tmp.regs[Coro::State::R14]));\
    asm("movq %%r15, %0" : "=r"(Coro::tmp.regs[Coro::State::R15]));\
    if (Coro::f_s->need_restore) {RESTORE}\

#define PROLOGUE_S\
    if (Coro::f_s->need_restore) {RESTORE_S}\

#define EPILOGUE\
    Coro::f_s->done = true;\
    asm("pop %rdi");\

#define SAVE_STACK\
    asm("movq %%rsp, %0" : "=r"(Coro::f_s->begin)); \
    asm("movq %%rbp, %0" : "=r"(Coro::f_s->end)); \
    memcpy(Coro::f_s->stack, (void*)(Coro::f_s->begin), Coro::f_s->end-Coro::f_s->begin);\
    Coro::f_s->top = Coro::f_s->end-Coro::f_s->begin;\

#define SAVE_REGS\
    asm("movq %%rax, %0" : "=r"(Coro::f_s->regs[Coro::State::RAX]));\
    asm("movq %%rbx, %0" : "=r"(Coro::f_s->regs[Coro::State::RBX]));\
    asm("movq %%rcx, %0" : "=r"(Coro::f_s->regs[Coro::State::RCX]));\
    asm("movq %%rdx, %0" : "=r"(Coro::f_s->regs[Coro::State::RDX]));\
    asm("movq %%rdi, %0" : "=r"(Coro::f_s->regs[Coro::State::RDI]));\
    asm("movq %%rsi, %0" : "=r"(Coro::f_s->regs[Coro::State::RSI]));\
    asm("movq %%r8,  %0" : "=r"(Coro::f_s->regs[Coro::State::R8]));\
    asm("movq %%r9,  %0" : "=r"(Coro::f_s->regs[Coro::State::R9]));\
    asm("movq %%r10, %0" : "=r"(Coro::f_s->regs[Coro::State::R10]));\
    asm("movq %%r11, %0" : "=r"(Coro::f_s->regs[Coro::State::R11]));\
    asm("movq %%r12, %0" : "=r"(Coro::f_s->regs[Coro::State::R12]));\
    asm("movq %%r13, %0" : "=r"(Coro::f_s->regs[Coro::State::R13]));\
    asm("movq %%r14, %0" : "=r"(Coro::f_s->regs[Coro::State::R14]));\
    asm("movq %%r15, %0" : "=r"(Coro::f_s->regs[Coro::State::R15]));\
    asm("movq %0,%%rbx"  :: "m"(Coro::tmp.regs[Coro::State::RBX])  : "rbx");\
    asm("movq %0,%%r12"  :: "m"(Coro::tmp.regs[Coro::State::R12])  : "r12");\
    asm("movq %0,%%r13"  :: "m"(Coro::tmp.regs[Coro::State::R13])  : "r13");\
    asm("movq %0,%%r14"  :: "m"(Coro::tmp.regs[Coro::State::R14])  : "r14");\
    asm("movq %0,%%r15"  :: "m"(Coro::tmp.regs[Coro::State::R15])  : "r15");\

#define YIELD \
    SAVE_STACK \
    SAVE_REGS \
    asm("movq %0, %%rdi" :: "r"((uintptr_t)&(Coro::f_s->regs[Coro::State::RIP])) : "rdi");\
    asm("call *%0" :: "r"((uintptr_t)store_rip) : "memory");\
    asm("leave");\
    asm("ret");\

#define YIELD_S\
    SAVE_REGS\
    asm("movq %0, %%rdi" :: "r"((uintptr_t)&(Coro::f_s->regs[Coro::State::RIP])) : "rdi");\
    asm("call *%0" :: "r"((uintptr_t)store_rip) : "memory");\
    asm("leave");\
    asm("ret");\

#define RESTORE_STACK\
        size_t rsp = 0;\
        asm("movq %%rsp, %0" : "=r"(rsp));\
        memcpy((void*)rsp, Coro::f_s->stack, Coro::f_s->top);\

#define RESTORE_REGS\
        asm("movq %0,%%rbx" :: "m"(Coro::f_s->regs[Coro::State::RBX]) : "rbx");\
        asm("movq %0,%%rcx" :: "m"(Coro::f_s->regs[Coro::State::RCX]) : "rcx");\
        asm("movq %0,%%rdx" :: "m"(Coro::f_s->regs[Coro::State::RDX]) : "rdx");\
        asm("movq %0,%%rdi" :: "m"(Coro::f_s->regs[Coro::State::RDI]) : "rdi");\
        asm("movq %0,%%rsi" :: "m"(Coro::f_s->regs[Coro::State::RSI]) : "rsi");\
        asm("movq %0,%%r8"  :: "m"(Coro::f_s->regs[Coro::State::R8])  : "r8");\
        asm("movq %0,%%r9"  :: "m"(Coro::f_s->regs[Coro::State::R9])  : "r9");\
        asm("movq %0,%%r10" :: "m"(Coro::f_s->regs[Coro::State::R10]) : "r10");\
        asm("movq %0,%%r11" :: "m"(Coro::f_s->regs[Coro::State::R11]) : "r11");\
        asm("movq %0,%%r12" :: "m"(Coro::f_s->regs[Coro::State::R12]) : "r12");\
        asm("movq %0,%%r13" :: "m"(Coro::f_s->regs[Coro::State::R13]) : "r13");\
        asm("movq %0,%%r14" :: "m"(Coro::f_s->regs[Coro::State::R14]) : "r14");\
        asm("movq %0,%%r15" :: "m"(Coro::f_s->regs[Coro::State::R15]) : "r15");\

#define RESTORE \
        RESTORE_STACK\
        RESTORE_REGS\
        asm("push %rdi");\
        asm("movq %0,%%rdi" :: "r"(&Coro::f_s->regs[Coro::State::RIP]) : "rdi");\
        asm("call *%0" :: "r"((uintptr_t)load_rip) : "memory");\

#define RESTORE_S\
        RESTORE_REGS\
        asm("push %rdi");\
        asm("movq %0,%%rdi" :: "r"(&Coro::f_s->regs[Coro::State::RIP]) : "rdi");\
        asm("call *%0" :: "r"((uintptr_t)load_rip) : "memory");\

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
        unsigned long rsp1{};
        unsigned long rsp2{};
        uintptr_t begin = 0;
        uintptr_t end = 0;
        size_t top = 0;
        bool need_restore = false;
        bool done = false;
        bool equal = false;
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
    
    // TODO: debug
    static void restore_rip(){
        asm("movq %0,8(%%rsp)" :: "r"(Coro::f_s->regs[Coro::State::RIP]) : "memory");
    }
    
    static void save_rip(){
        asm("movq 8(%%rsp), %0" : "=r"(Coro::f_s->regs[Coro::State::RIP]));
        Coro::f_s->regs[Coro::State::RIP] += 2;
    }

    // TODO: handle args
    static inline std::vector<State> states;
    static inline std::vector<void(*)()> functions;
    static inline State* f_s = nullptr;
    static inline State tmp = State{};
    static inline void add(void(*addr)()){
        states.push_back(State{});
        functions.push_back(addr);
    }
    static inline void run(){
        if (states.empty()) return;
        while(!states.empty()){
            bool done = false;
            for (size_t i = 0, e = states.size(); i < e; ++i){
                f_s = &states[i];
                if (!f_s->done) {
                    SAVE_BEFORE_CALL
                    functions[i]();
                    SAVE_AFTER_CALL
                    assert(Coro::tmp.rsp1 == Coro::tmp.rsp2);
                    if ((f_s && !f_s->need_restore)) f_s->need_restore = true;
                }
                if ((f_s && f_s->done)) done = true;
            }
            if (done) states.erase(std::remove_if(std::begin(states), std::end(states), [](auto& el){return el.done;}), std::end(states));
        }
    }
};

__attribute__((optimize("-fno-omit-frame-pointer")))
void foo_stackless(){
    PROLOGUE_S
    printf("Foo_s:1\n");
    YIELD_S
    printf("Foo_s:2\n");
    YIELD_S
    printf("Foo_s:3\n");
    EPILOGUE
}

__attribute__((optimize("-fno-omit-frame-pointer")))
void foo(){
    PROLOGUE
    int x = 2;
    printf("Foo:1\n");
    YIELD
    int y = 3;
    printf("Foo:2\n");
    YIELD
    int z = x + y;
    printf("Foo:%d\n", z);
    EPILOGUE
}

__attribute__((optimize("-fno-omit-frame-pointer")))
void bar(){
    PROLOGUE
    int x = 3;
    printf("Bar:1\n");
    YIELD
    int y = 7;
    printf("Bar:2\n");
    YIELD
    int z = x + y;
    printf("Bar:%d\n", z);
    EPILOGUE
}

int main(){
    // TODO: debug
    // Coro::add(foo);
    // Coro::add(foo);
    Coro::add(foo_stackless);
    // Coro::add(bar);
    Coro::run();
}

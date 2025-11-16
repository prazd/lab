#include <iostream>
#include <iomanip>
#include <cassert>


class RegistersStateX86 {
    #define LOAD(x)  asm("mov %%"#x", %0" : "=r"(regs[x]));
    
    unsigned long long regs[17]{};
    std::string_view regs_str[17]{
        "rax", "rbx", "rcx", "rdx", "rdi", "rsi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15", "rip"
    };
public:
    enum Register {
        rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp,
        r8,  r9,  r10, r11, r12, r13, r14, r15, rip
    };
    
    unsigned long long get(Register reg) const {
        return regs[reg];
    }

    void set(Register reg, unsigned long long value) {
        regs[reg] = value;
    }

    void print() {
        std::cout << std::hex;
        for (int i = 0; auto el: regs){
            std::cout << regs_str[i] << ": 0x" << regs[i] << '\n'; ++i;
        }
        std::cout << std::dec;
    }

    void update() {
        LOAD(rax)
        LOAD(rbx)
        LOAD(rcx)
        LOAD(rdx)
        LOAD(rdi)
        LOAD(rsi)
        LOAD(rbp)
        LOAD(rsp)
        LOAD(r8)
        LOAD(r9)
        LOAD(r10)
        LOAD(r11)
        LOAD(r12)
        LOAD(r13)
        LOAD(r14)
        LOAD(r15)
        asm("leaq (%%rip), %0" : "=r"(regs[rip]));
    }

    void call_void() {
        asm("call *%0" :: "r"(regs[rip]) : "memory");
    }

    template <typename FuncPtr, typename ... Args>
    auto call(Args&& ... args) {
        return ((FuncPtr)regs[rip])(std::forward<Args>(args)...);
    }

};

void foo(){
    std::cout << "Void call\n";
}

int sum(int a, int b){return a + b;}

int main() {
    RegistersStateX86 rs;
    rs.update();
    rs.print();
    rs.set(RegistersStateX86::rip, (unsigned long long)foo);
    rs.call_void();
    rs.set(RegistersStateX86::rip, (unsigned long long)sum);
    assert(rs.call<int(*)(int,int)>(10,20) == 30);
}

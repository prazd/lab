#include <iostream>
#include <iomanip>

class RegistersStateX86 {
    #define SAVE_REG(x) asm("mov %%"#x", %0" : "=r"(regs[x]));
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
    void print() {
        std::cout << std::hex;
        for (int i = 0; auto el: regs){
            std::cout << regs_str[i] << ": 0x" << regs[i] << '\n'; ++i;
        }
        std::cout << std::dec;
    }
    void update() {
        SAVE_REG(rax)
        SAVE_REG(rbx)
        SAVE_REG(rcx)
        SAVE_REG(rdx)
        SAVE_REG(rdi)
        SAVE_REG(rsi)
        SAVE_REG(rbp)
        SAVE_REG(rsp)
        SAVE_REG(r8)
        SAVE_REG(r9)
        SAVE_REG(r10)
        SAVE_REG(r11)
        SAVE_REG(r12)
        SAVE_REG(r13)
        SAVE_REG(r14)
        SAVE_REG(r15)
        asm("leaq (%%rip), %0" : "=r"(regs[rip]));
    }
};

int main() {
    RegistersStateX86 rs;
    rs.update();
    rs.print();
}

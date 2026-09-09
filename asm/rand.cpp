#include <iostream>
#include <optional>

class Rand {
    unsigned long regs[2]{};

    enum Instruction{RDRAND, RDSEED};

    template <Instruction i>
    std::optional<unsigned long> exec(){
        if       constexpr (i == RDRAND)  asm volatile("rdrand %%rax" :"=a"(regs[0])); 
        else if  constexpr (i == RDSEED)  asm volatile("rdseed %%rax" :"=a"(regs[0])); 

        asm volatile("setc %%bl" :"=b"(regs[1]));
        return regs[1] ? std::optional(regs[0]) : std::nullopt;    
    }

public:   
    std::optional<unsigned long> rdrand() {
        return exec<RDRAND>();
    }

    std::optional<unsigned long> rdseed() {
        return exec<RDSEED>();
    }
};

int main(){
    Rand r;
    
    auto rd = r.rdrand();
    if (!rd.has_value()) {std::cout << "RDRAND failed\n"; return -1;}
    std::cout << "RDRAND:" << rd.value() << '\n';
    
    auto rs = r.rdrand();
    if (!rs.has_value()) {std::cout << "RDSEED failed\n"; return -1;}
    std::cout << "RDSEED:" << rs.value() << '\n';
}

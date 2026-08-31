
#include <iostream>
#include <iomanip>
#include <cstring>
#include <array>

struct CPUID {
    unsigned rax{};
    unsigned rbx{};
    unsigned rcx{};
    unsigned rdx{};

    auto vendor() {
        std::array<char,13> res;
        asm volatile("cpuid" : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx): "a"(0));
        memcpy(res.data(), &rbx, 4);
        memcpy(res.data() + 4, &rdx, 4);
        memcpy(res.data() + 8, &rcx, 4);
        return res;
    
    }
    bool hypervisor(){
        asm volatile("cpuid" : "=c"(rcx): "a"(1) : "rbx","rdx");
        return rcx & 0x8000'0000;
    }
};

int main(){
    CPUID cpuid;
    auto d = cpuid.vendor();
    std::cout << "Vendor:" << d.data() << '\n';
    std::cout << "Hypervisor:" << (int)cpuid.hypervisor() << '\n';
    return 0;
}
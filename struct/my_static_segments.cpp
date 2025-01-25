#include <iostream>

constexpr int b = 30; // rodata
int z; // bss
int a = 10; // data segment

void func() {} // code segment

template <typename T>
void print(const T& val) {
    for(auto el: val){
        if ((void*)&a == el) 
            std::cout << "DataSegment:";
        else if ((void*)&b == el)
            std::cout << "RoData:";
         else if ((void*)&z == el) 
            std::cout << "BSS:";
        else if ((void*)&func == el) 
            std::cout << "CodeSegent:";
        std::cout << el << "\n";
    }
}

int main() {
    void* addr[4]{(void*)&a, (void*)&b, (void*)&z, (void*)&func};
    std::sort(addr, addr+4, [](auto a, auto b){return a > b;});
    print(addr);
}
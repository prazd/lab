#include <iostream>
#include <array>

#define DEBUG 0

template <typename T>
void debug_print(T& val){
    std::cout << "ArgHex:\n";
    auto arr = std::bit_cast<std::array<unsigned char, sizeof(T)>>(val);
    for (auto el: arr) std::cout << std::hex << (int)el << '\n';
}

extern "C" void print_e(){
    std::cout << "empty print!" << '\n';
}
extern "C" void print_i(int x){
#if DEBUG
    debug_print(x);
#endif    
    std::cout << std::dec << "Int value:" << x << '\n';
}
extern "C" void print_d(double x){
#if DEBUG
    debug_print(x);
#endif  
    std::cout << "Double value:" << x << '\n';
}
extern "C" void print_s(const char* x){
#if DEBUG
    debug_print(x);
#endif    
    std::cout << "Str value:" << x << '\n';
}

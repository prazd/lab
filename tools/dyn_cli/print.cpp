#include <iostream>
#include <array>

// /usr/local/Cellar/raylib/5.5/lib/libraylib.dylib

#define DEBUG 0
#define PRINT_ARG(arg) << #arg << arg << ' '

extern "C" constexpr size_t GVAR = 1024;

template <typename T>
void debug_print(T& val){
    std::cout << "ArgHex:\n";
    auto arr = std::bit_cast<std::array<unsigned char, sizeof(T)>>(val);
    for (auto el: arr) std::cout << std::hex << (int)el << '\n';
}

template <typename ... T>
void print_args(T ... args) {
    std::cout << "Args:\n";
    size_t i = 0;
    ((std::cout << "arg" << i++<< ':' << args << ' '),...);
}

extern "C" void print_e(){
    std::cout << "empty print!" << '\n';
}

#define P_FUNC()


// Int args
extern "C" void print_i(int i){
#if DEBUG
    debug_print(i);
#endif    
    std::cout << std::dec << "Int value:" << i << '\n';
}

extern "C" void print_i6(int i1, int i2, int i3, int i4, int i5, int i6){  
    print_args(i1,i2,i3,i4,i5,i6);
}

// Double/Float args
extern "C" void print_d(double x){
#if DEBUG
    debug_print(x);
#endif  
    std::cout << "Double value:" << x << '\n';
}

extern "C" void print_d8(double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8){
    print_args(d1,d2,d3,d4,d5,d6,d7,d8);
}

extern "C" void print_f8(float d1, float d2, float d3, float d4, float d5, float d6, float d7, float d8){
    print_args(d1,d2,d3,d4,d5,d6,d7,d8);
}

// print_if8 1 2 3 4 5 6 0.1f 0.2f 0.3f 0.4f 0.5f 0.6f 0.7f 0.8f
extern "C" void print_if8(int i1, int i2, int i3, int i4, int i5, int i6, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8){
    print_args(i1,i2,i3,i4,i5,i6,f1,f2,f3,f4,f5,f6,f7,f8);
}

// print_id8 1 2 3 4 5 6 0.1d 0.2d 0.3d 0.4d 0.5d 0.6d 0.7d 0.8d
extern "C" void print_id8(int i1, int i2, int i3, int i4, int i5, int i6, double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8){
    print_args(i1,i2,i3,i4,i5,i6,d1,d2,d3,d4,d5,d6,d7,d8);
}

// print_id8_2 1 0.1d 2 3 4 0.2d 0.3d 0.4d 5 0.5d 0.6d 0.7d 6 0.8d
extern "C" void print_id8_2(int i1, double d1, int i2, int i3, int i4, double d2, double d3, double d4, int i5, double d5, double d6, double d7, int i6, double d8){
    print_args(i1,i2,i3,i4,i5,i6,d1,d2,d3,d4,d5,d6,d7,d8);
}

extern "C" void print_f(double x){
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

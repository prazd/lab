#include <iostream>
#include <random>
#include <chrono>
#include <cassert>

#define VOLATILE_VERSION 1
#if VOLATILE_VERSION
#define ARR_T     volatile unsigned char
#define ARR_T_PTR volatile unsigned char*
#define SWAR_PTR  volatile unsigned long*
#else
#define ARR_T     unsigned char
#define ARR_T_PTR unsigned char*
#define SWAR_PTR  unsigned long*
#endif    

constexpr size_t COUNT = 100'000'000;

ARR_T ARR1_A[COUNT]{};
ARR_T ARR1_B[COUNT]{};
ARR_T SUM1[COUNT]{};

ARR_T ARR2_A[COUNT]{};
ARR_T ARR2_B[COUNT]{};
ARR_T SUM2[COUNT]{};

void sum1(ARR_T_PTR a, ARR_T_PTR b, ARR_T_PTR s){
    for (size_t i = 0; i < COUNT; ++i) 
        s[i] = a[i] + b[i];
}

void sum2(SWAR_PTR a, SWAR_PTR b, SWAR_PTR s){
    for (size_t i = 0; i < COUNT; i += sizeof(unsigned long)){
        *s = ((*a & 0x7f7f7f7f7f7f7f7f) + (*b & 0x7f7f7f7f7f7f7f7f)) ^
            ((*a ^ *b) & 0x8080808080808080);
        ++a;
        ++b;
        ++s;
    }
}

template <typename ... Args>
auto bench(auto f, Args&& ... args){
    auto begin = std::chrono::steady_clock::now();
    f(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
}

int main(){
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<unsigned char> ud(0x0, 0xff);
    std::generate(ARR1_A, ARR1_A + COUNT, [&]{return ud(mt);});
    std::generate(ARR1_B, ARR1_B + COUNT, [&]{return ud(mt);});
    std::copy(ARR1_A, ARR1_A + COUNT, ARR2_A);
    std::copy(ARR1_B, ARR1_B + COUNT, ARR2_B);
    
    std::cout << "Sum:" <<  bench(sum1, ARR1_A, ARR1_B, SUM1) << " ms\n";
    std::cout << "SWAR(SIMD Within A Register):" << bench(sum2, (SWAR_PTR)ARR2_A, (SWAR_PTR)ARR2_B, (SWAR_PTR)SUM2) << " ms\n";
    assert(std::equal(SUM1, SUM1 + COUNT, SUM2));
}
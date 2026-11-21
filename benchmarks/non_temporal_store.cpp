#include <iostream>
#include <chrono>
#include <random>
#include <iterator>
#include <immintrin.h>
#include <limits>
#include <memory>

template <std::integral T>
struct Rand {
    static inline std::random_device rd = std::random_device();
    static inline std::mt19937 mt = std::mt19937(rd());
    static inline std::uniform_int_distribution<T> dis = std::uniform_int_distribution<T>(0, std::numeric_limits<T>::max());
    static T gen(){return dis(mt);}
};

constexpr size_t SZ = 1 << 22;
alignas(32) int ARR1[SZ];
alignas(32) int ARR2[SZ];
alignas(32) int ARR3[SZ];
alignas(32) int ARR4[SZ];

void set_nts(int v, int* src, int* end){
    __m256i vi = _mm256_set1_epi32(v);
    auto ptr = (__m256i*)src;
    for (size_t i = 0, e = ((end-src) * 4)/32;i!=e;++i)
        _mm256_stream_si256(ptr + i, vi);
    _mm_sfence();
}

void set(int v, int* src, int* end){
    for (;src!=end;++src) *src = v;
}

void f1(int* src1, int* end1, int* src2, int* end2){
    size_t counter = 1;
    for (size_t i = 0, e = end1-src1; i != e; ++i){
        if (!(i % 1000)) set_nts(counter++, src2, end2);
        src1[i] = Rand<int>::gen();
    }
}

void f2(int* src1, int* end1, int* src2, int* end2){
    size_t counter = 1;
    for (size_t i = 0, e = end1-src1; i != e; ++i){
        if (!(i % 1000)) set(counter++, src2, end2);
        src1[i] = Rand<int>::gen();
    }
}

template <typename ... Args>
void bench(const char* str, auto f, Args&& ... args){
    auto begin = std::chrono::steady_clock::now();
    f(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    std::cout << str << ':' << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
}

// c++-15 -O3 -mavx2 -std=c++23  non_temporal_store.cpp
int main(){
    bench("NTS", f1, ARR1, ARR1 + SZ, ARR2, ARR2 + SZ);
    bench("Default", f2, ARR3, ARR3 + SZ, ARR4, ARR4 + SZ);
}
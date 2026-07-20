#include <iostream>
#include <immintrin.h>
#include <chrono>
#include <cstring>

constexpr size_t SZ = 100'000'000;

unsigned char A1d[SZ];
unsigned char A1s[SZ];
unsigned char B1d[SZ];
unsigned char B1s[SZ];
unsigned char C1d[SZ];
unsigned char C1s[SZ];

void __attribute__((constructor)) init(){
    std::generate(A1s, A1s + SZ, []{return rand() & 0xff;});
    std::generate(B1s, B1s + SZ, []{return rand() & 0xff;});
    std::generate(C1s, C1s + SZ, []{return rand() & 0xff;});
}

void f1(unsigned char* it, unsigned char* ite, unsigned char* it2){
    for (;it != ite;++it,++it2) *it2 = *it;
}

void f2(unsigned char* it, unsigned char* ite, unsigned char* it2){
    memcpy(it2, it, (ite - it) * sizeof(*it));
}

void f3(unsigned char* it, unsigned char* ite, unsigned char* it2){
    for (size_t i = 0, e = ite - it; i != e; i+=32){
        __m256i a = _mm256_loadu_si256((__m256i*)(it + i));
        _mm256_storeu_si256((__m256i*)(it2 + i), a);
    }
}

void bench(const char* text, auto f, unsigned char(&arr_s)[SZ],  unsigned char(&arr_d)[SZ]){
    auto begin = std::chrono::steady_clock::now();
    f(std::begin(arr_s), std::end(arr_s), std::begin(arr_d));
    auto end = std::chrono::steady_clock::now();
    std::cout << text << ':' << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
}


// c++-15 -mavx2 -O3 -std=c++23 copy_bench.cpp
int main(){
    bench("Simple:", f1, A1s, A1d);  
    bench("Memcpy:", f2, B1s, B1d);
    bench("AVX(256):", f3, C1s, C1d);
}
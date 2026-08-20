#include <iostream>
#include <immintrin.h>
#include <chrono>
#include <cstring>
#include <numeric>
#include <random>

constexpr size_t SZ = 100'000'000;

unsigned char A1d[SZ];
unsigned char A1s[SZ];
unsigned char B1d[SZ];
unsigned char B1s[SZ];
unsigned char C1d[SZ];
unsigned char C1s[SZ];
unsigned char D1d[SZ];
unsigned char D1s[SZ];

void __attribute__((constructor)) init(){
    std::generate(A1s, A1s + SZ, []{return rand() & 0xff;});
    std::generate(B1s, B1s + SZ, []{return rand() & 0xff;});
    std::generate(C1s, C1s + SZ, []{return rand() & 0xff;});
    std::generate(D1s, D1s + SZ, []{return rand() & 0xff;});
}

void f1(unsigned char* it, unsigned char* ite, unsigned char* it2){
    for (;it != ite;++it,++it2) *it2 = *it;
}

void f2(unsigned char* it, unsigned char* ite, unsigned char* it2){
    memcpy(it2, it, (ite - it) * sizeof(*it));
}

void f3(unsigned char* it, unsigned char* ite, unsigned char* it2){
    for (size_t i = 0, e = ite - it; i != e; i+=32)
        _mm256_storeu_si256((__m256i*)(it2 + i), _mm256_loadu_si256((__m256i*)(it + i)));
}

void f4(unsigned char* it, unsigned char* ite, unsigned char* it2){
    size_t sz = ite - it;
    volatile unsigned char tmp = 0;

    for (size_t i = 0; i < sz; i += 4096) // TLB priming
        tmp = *(it + i);

    for (size_t i = 0; i < sz; i += 4096)
        _mm_prefetch(it + i, _MM_HINT_NTA);

    for (size_t i = 0, e = ite - it; i != e; i+=128){
        _mm256_storeu_si256((__m256i*)(it2 + i), _mm256_loadu_si256((__m256i*)(it + i)));
        _mm256_storeu_si256((__m256i*)(it2 + i + 32), _mm256_loadu_si256((__m256i*)(it + i + 32)));
        _mm256_storeu_si256((__m256i*)(it2 + i + 64), _mm256_loadu_si256((__m256i*)(it + i + 64)));
        _mm256_storeu_si256((__m256i*)(it2 + i + 96), _mm256_loadu_si256((__m256i*)(it + i + 96)));
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
    std::random_device rd;
    std::mt19937 g(rd());
    void* addresses[5]{&&SIMPLE, &&MEMCPY, &&AVX, &&AVX_TLB_PRIMING, &&END};
    unsigned char order[5]{};
    std::iota(std::begin(order), std::end(order), 0);
    std::shuffle(order, order + std::size(order) - 1, g);
    size_t i = 0;

    goto *addresses[order[i++]];
SIMPLE:
    bench("Simple:", f1, A1s, A1d);  
    goto *addresses[order[i++]];

MEMCPY:    
    bench("Memcpy:", f2, B1s, B1d);
    goto *addresses[order[i++]];

AVX:    
    bench("AVX(256):", f3, C1s, C1d);
    goto *addresses[order[i++]];

AVX_TLB_PRIMING:
    bench("AVX(256) + TLB priming:", f4, D1s, D1d);
    goto *addresses[order[i++]];

END:
    return 0;
}
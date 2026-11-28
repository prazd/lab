#include <immintrin.h>
#include <random>
#include <algorithm>
#include <chrono>
#include <iostream>

template <float begin, float end>
struct RandF{
    static inline auto rd = std::random_device();
    static inline auto mt = std::mt19937(rd());
    static inline auto ud = std::uniform_real_distribution<float>(begin, end);
    static float gen(){
        return ud(mt);
    } 
};

constexpr size_t SZ = 1 << 27;
alignas(32) float ARR1[SZ];
alignas(32) float ARR2[SZ];

alignas(32) float RES1[SZ];
alignas(32) float RES2[SZ];

void __attribute__((constructor)) init(){
    std::generate(ARR1, ARR1 + SZ, []{return RandF<1.f, 5.f>::gen();});
    std::generate(ARR2, ARR2 + SZ, []{return RandF<1.f, 5.f>::gen();});
}

void f1(float* __restrict arr, float* __restrict res, size_t sz){
    for (size_t i = 0; i != sz; ++i){
        if (arr[i] > 1.5f){
            res[i] = arr[i] * 2.5f;
        } else {
            res[i] = arr[i] * 1.8f;
        }
    }
}

void f2(float* __restrict arr, float* __restrict res, size_t sz){ 
    __m256 threshhold = _mm256_set1_ps(1.5f);
    __m256 m1 = _mm256_set1_ps(2.5f);
    __m256 m2 = _mm256_set1_ps(1.8f);
    for (size_t i = 0; i != sz; i+=8){
        __m256 a_vec = _mm256_load_ps(arr + i);
        __m256 mask = _mm256_cmp_ps(a_vec, threshhold, _CMP_GT_OQ);

        __m256 r1 = _mm256_mul_ps(a_vec, m1);
        __m256 r2 = _mm256_mul_ps(a_vec, m2);

        __m256 r3 = _mm256_blendv_ps(r2, r1, mask);
        _mm256_store_ps(res + i, r3);
    }
}

template <typename ... Args>
void bench(const char* str, auto f, Args&& ... args){
    auto begin = std::chrono::steady_clock::now();
    f(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    std::cout << str << ':' << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
}

// c++-15 -std=c++23 -O3 -mavx  branchless_blend.cpp
int main(){
    bench("F default", f1, ARR1, RES1, SZ);
    bench("F intr", f2, ARR2, RES2, SZ);  
}

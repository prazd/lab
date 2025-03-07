#include <iostream>
#include <immintrin.h>
#include <cassert>
#include <chrono>
 
constexpr size_t sz = 1024 * 1024 * 100; // % 8 == 0
alignas(32) float x[sz];
alignas(32) float y[sz];
alignas(32) float r1[sz]{};
alignas(32) float r2[sz]{};

void seed() {
    std::generate(x, x + sz, []{return float(rand() % 100);});
    std::generate(y, y + sz, []{return float(rand() % 100);});
}

void sum(){
    for (size_t i = 0; i < sz; ++i)
        r1[i] = x[i] + y[i];
}

void sum_simd_256() {
    for (size_t i = 0; i < sz; i += 8) {
        __m256 vx = _mm256_load_ps(x + i);
        __m256 vy = _mm256_load_ps(y + i);
        __m256 vresult = _mm256_add_ps(vx, vy);
        _mm256_storeu_ps(r2 + i, vresult);
    }
}

int main() {
    seed();
    auto func = [](void(*f)(), std::string_view tag){
        auto start = std::chrono::high_resolution_clock::now();
        f();
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << tag << " " << "Milliseconds: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "\n";
    };
    func(sum, "Default");
    func(sum_simd_256, "SIMD256");
    assert(std::equal(std::begin(r1), std::end(r1), std::begin(r2), std::end(r2)));
}
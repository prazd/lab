#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

// g++ -O3 -march=native -funroll-loops -fno-exceptions -fno-rtti -std=c++2b inline_unrolling_bench.cpp

inline void f(std::atomic<unsigned long>& counter) {
    for (int j = 0; j < 1000'000; ++j) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

// if 1 - 20 cycles unroll
// if 0 - 16 cycles unroll + additional mov instruction
#define INLINE_VERSION 0

int main() {
    std::atomic<unsigned long> counter = 0;
    std::vector<std::thread> vec;
    vec.reserve(10);
    auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; ++i) {
        vec.emplace_back([&counter]{
#if INLINE_VERSION        
        f(counter);
#else
        for (int j = 0; j < 1000'000; ++j) {
           counter.fetch_add(1, std::memory_order_relaxed);
        }
#endif            
        });
    }
    for (auto& el: vec) el.join();
    auto end = std::chrono::steady_clock::now();
    std::cout << "Res:" << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
}
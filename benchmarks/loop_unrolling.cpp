#include <iostream>
#include <chrono>
#include <numeric>

// c++-14 -O0 -std=c++23 loop_unrolling.cpp
// c++-14 -O1 -std=c++23 loop_unrolling.cpp
// c++-14 -O2 -fno-unroll-loops -fno-tree-vectorize -std=c++23 loop_unrolling.cpp
constexpr size_t sz = 40'000'000;
int arr[sz]{};
int arr2[sz]{};

size_t get_sum_default() {
    size_t sum{};
    for (int i = 0; i < sz;++i) 
        sum += arr[i];
    return sum;
}

size_t get_sum_unrolling() {
    size_t sum{};
    for (int i = 0; i < sz;i+=4) 
        sum += arr2[i] + arr2[i + 1] + arr2[i + 2] + arr2[i + 3];
    return sum;
}

void bench(auto func, auto info)  {
    size_t r{};
    auto begin = std::chrono::steady_clock::now();
    r = func();
    auto end = std::chrono::steady_clock::now();
    std::cout << info << " Time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
}

int main() {
    std::iota(std::begin(arr), std::end(arr), 0);
    std::iota(std::begin(arr2), std::end(arr2), 0);

    bench(get_sum_default, "Default");
    bench(get_sum_unrolling, "Unrolling");
}
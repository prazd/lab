#include <immintrin.h>
#include <cstddef>
#include <chrono>
#include <numeric>
#include <iostream>
#include <span>

constexpr size_t height = 100'000;
constexpr size_t width = 1000;

int arr[height * width]{};
int arr2[height * width]{};

size_t get_sum(std::span<int> arr) {
    size_t sz = arr.size();
    size_t res{};
    for (int j = 0; j < width; ++j){
        for (int i = 0; i < height; ++i) {
            res += arr[(i * width) + j];
        }
    }
    return res;
}

size_t get_sum_prefetch(std::span<int> arr) {
    size_t sz = arr.size();
    size_t res{};
    constexpr size_t prefetch_dis = 64; // maybe experimental change
    for (int j = 0; j < width; ++j){
        for (int i = 0; i < height; ++i) {
            auto idx = ((i + prefetch_dis) * width + j);
            if (idx < sz) _mm_prefetch((arr.data() + idx), _MM_HINT_T0);
            res += arr[(i * width) + j];
        }
    }
    return res;
}

void bench(auto func, auto& arr, auto info) {
    auto begin = std::chrono::steady_clock::now();
    auto r = func({std::begin(arr), std::end(arr)});
    auto end = std::chrono::steady_clock::now();
    std::cout <<"Res:" << r << " " << info << " time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms\n";
}

int main() {
    std::iota(std::begin(arr), std::end(arr), 0);
    std::iota(std::begin(arr2), std::end(arr2), 0);

    bench(get_sum,arr, "Default");
    bench(get_sum_prefetch, arr2, "Prefetch");
}
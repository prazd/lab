#include <iostream>
#include <chrono>
#include <algorithm>

struct S1 {
    unsigned short f1: 6{};
    unsigned short f2: 4{};
    unsigned short f3: 3{};
    unsigned short f4: 3{};
};

struct S2 {
    unsigned char f1{};
    unsigned char f2{};
    unsigned char f3{};
    unsigned char f4{};
};
constexpr size_t sz = 10'000'000;

S1 arr1[sz];
S2 arr2[sz];

void benchmark(auto& arr,  auto msg) {
    auto begin = std::chrono::steady_clock::now();
    for (auto& el: arr){
        el.f1 = 1;
        el.f2 = 2;
        el.f3 = 3;
        el.f4 = 4;
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << msg << " :" << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << " ms\n";
}

int main() {
    benchmark(arr1, "S1(bitfields)");
    benchmark(arr2, "S2");
}
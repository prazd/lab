#include <iostream>
#include <numeric>
#include <array>
#include <algorithm>
#include <chrono>

// g++ -O3 -std=c++2b

#define USE_ATTRIBUTE 1

#if USE_ATTRIBUTE
__attribute__((hot))
#endif
auto hot1(){
    return rand() % 100;
}

#if USE_ATTRIBUTE
__attribute__((cold))
#endif 
auto cold1(){
    return rand() % 200;
}

#if USE_ATTRIBUTE
__attribute__((hot))
#endif 
auto hot2(){
    return rand() % 300;
}

#if USE_ATTRIBUTE
__attribute__((cold))
#endif
auto cold2() {
    return rand() % 400;
}

constexpr size_t count = 1'000'000;

int main() {
    size_t res = cold1();
    auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < count; ++i) {
        res += hot1();
        res += hot2(); 
    }
    auto end = std::chrono::steady_clock::now();
    res += cold2();
    std::cout << "Time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
}
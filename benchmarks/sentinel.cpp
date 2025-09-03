#include <iostream>
#include <chrono>
#include <numeric>

constexpr size_t sz = 1000;

int IN1[sz];
int OUT1[sz];

int IN2[sz + 1];
int OUT2[sz];

void normal() {
    for(int i = 0; i < sz; ++i)
        OUT1[i] = IN1[i] + 1;
}

void sentinel() {
    auto ptr1 = IN2;
    auto ptr2 = OUT2;
    while(*ptr1 != -1) *ptr2++ = *ptr1++ + 1;
}


void bench(auto func, auto info) {
    auto begin = std::chrono::steady_clock::now();
    func();
    auto end = std::chrono::steady_clock::now();
    std::cout << info << " " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() << "ms\n";
}

int main() {
    IN2[sz] = -1;
    std::iota(std::begin(IN1), std::end(IN1), 0);
    std::iota(std::begin(IN2), std::end(IN2)-1, 0);
    bench(normal, "Normal");
    bench(sentinel, "Sentinel");
}
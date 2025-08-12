#include <iostream>
#include <chrono>
#include <numeric>
#include <utility>

constexpr size_t sz = 40'000'000;

int A[sz]{};
int B[sz]{};

int A2[sz]{};
int B2[sz]{};

void sum() {
    for (int i = 2; i < sz; ++i)
        B[i] = (A[i-2] + A[i-1] + A[i])/3;
}

void sum_pipeline() {
    int a0 = A2[0];
    int a1 = A2[1];
    int a2 = A2[2];
    for (int i = 2; i < sz-1; ++i){
        B2[i] = (a0 + a1 + a2)/3;
        a0 = a1;
        a1 = a2;
        a2 = A2[i + 1];
    }
    B2[sz-1] = (a0 + a1 + a2)/3;
}

void bench(auto func, auto info) {
    auto now = std::chrono::steady_clock::now();
    func();
    auto end = std::chrono::steady_clock::now();
    std::cout << info <<" time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end-now).count() << "ms\n";
}

int main() {
    std::iota(std::begin(A), std::end(A), 0);
    std::iota(std::begin(B), std::end(B), 0);

    std::iota(std::begin(A2), std::end(A2), 0);
    std::iota(std::begin(B2), std::end(B2), 0);

    bench(sum, "Default ");
    bench(sum_pipeline, "Pipeline ");
}
#include <iostream>
#include <chrono>

constexpr size_t sz = 100'000'000;

int A[sz]{};
int B[sz]{};

size_t simple() {
    size_t count{};
    for (auto el: A){
        if (el == 1){
            ++count;
        }
    }
    return count;
}

size_t expect() {
    size_t count{};
    for (auto el: B){
        if (__builtin_expect(el == 1, 1)){
            ++count;
        }
    }
    return count;
}

void bench(auto func, auto info) {
    auto begin = std::chrono::steady_clock::now();
    auto res = func();
    auto end = std::chrono::steady_clock::now();
    std::cout << info << " " << res << " time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms\n";
}

int main() {
    std::generate(std::begin(A), std::end(A), []{static int counter = 0; return (counter++ % 10'000 == 0) ? 0 : 1;});
    std::generate(std::begin(B), std::end(B), []{static int counter = 0; return (counter++ % 10'000 == 0) ? 0 : 1;});

    bench(expect, "Builtin Expect");
    bench(simple, "Simple");   
}
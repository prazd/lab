#include <iostream>
#include <valarray>
#include <cassert>
#include <algorithm>

constexpr size_t sz = 200'000'000;

std::vector<int> x(sz);
std::vector<int> y(sz);

std::valarray<int> vx(sz);
std::valarray<int> vy(sz);

std::vector<int> sum(sz);
std::valarray<int> vsum(sz);

void init() {
    std::generate(x.begin(), x.end(), []{return rand() % 100;});
    std::generate(y.begin(), y.end(), []{return rand() % 100;});
    std::generate(std::begin(vx), std::end(vx), []{return rand() % 100;});
    std::generate(std::begin(vy), std::end(vy), []{return rand() % 100;});
}


void simple_sum(size_t count) {
    while(count--) {
        for (size_t i = 0; i < sz; ++i) {
            sum[i] = x[i] + y[i];
        }
    }
}

void valarray_sum(size_t count) {
    while(count--) {
        vsum = vx + vy;
    }
}

int main() {
    init();
    auto elapsed_call = [&](auto&& func, auto&&... args){
        auto start = std::chrono::steady_clock::now();
        func(args...);
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    auto sum_ms = elapsed_call(simple_sum, 1);
    std::cout << "Sum:" << sum_ms << "ms" << '\n';

    auto vsum_ms = elapsed_call(valarray_sum, 1);
    std::cout << "Vsum:" << vsum_ms << "ms" << '\n';
}
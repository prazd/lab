#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>
#include <new>

#define FALSE_SHARING 0

constexpr size_t count = 30'000'000;

struct S1 {
    std::atomic<int> a;
};

#if FALSE_SHARING
struct S2 {
    std::atomic<int> a;
};
#else
struct alignas(std::hardware_destructive_interference_size) S2 {
    std::atomic<int> a;
};
#endif

template <typename T>
struct Sharded {
    std::array<T, 4> arr;
};

void bench(auto&& func, auto&&... args) {
    auto start = std::chrono::high_resolution_clock::now();
    func(args...);
    auto end = std::chrono::high_resolution_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time:" << d.count() << "ms" << '\n';
}

int main() {
    Sharded<S1> s1;
    std::vector<std::thread> p1;

    Sharded<S2> s2;
    std::vector<std::thread> p2;
    
    auto f = [&](auto& p, auto& s){
        for (size_t i = 0; i < 4; ++i)
            p.emplace_back([&s, index = i]{for (size_t i = 0; i < count; ++i) s.arr[index].a++;});
        for (auto&& el: p) 
            el.join();
    };

    bench(f, p1, s1);
    bench(f, p2, s2);
}
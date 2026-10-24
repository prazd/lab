#include <iostream>
#include <algorithm>
#include <random>
#include <immintrin.h>
#include <numeric>
#include <array>

constexpr size_t SZ = 4096;

template <std::integral T = size_t, size_t n = SZ>
struct Rand {
    static inline std::random_device rd = std::random_device();
    static inline std::mt19937 mt = std::mt19937(rd());
    static inline std::uniform_int_distribution<T> dis = std::uniform_int_distribution<T>(0,SZ-1);
    static inline auto gen(){
        return dis(mt);
    }
};

struct S {
    alignas(64) unsigned char arr[SZ];
    void fill(){
        std::iota(std::begin(arr), std::end(arr), 0);
    }
    void clean_cache(){
        auto ptr = (unsigned char*)(arr);
        for (size_t i = 0, e = sizeof(arr); i != e; i+=64)
            _mm_clflush(ptr + i);
        _mm_mfence();
    }
    auto access_time(size_t i){
        unsigned long begin_l{};
        unsigned long begin_h{};
        unsigned long end_l{};
        unsigned long end_h{};
        asm volatile("lfence");
        asm volatile("rdtsc": "=a"(begin_l), "=d"(begin_h));
        volatile auto val = arr[i];
        asm volatile("rdtscp" : "=a"(end_l), "=d"(end_h));
        asm volatile("lfence");
        return (end_l | (end_h << 32)) - (begin_l | (begin_h << 32));
    }
    S(){fill(); clean_cache();}
};

int main(){
    S s;
    auto ri = Rand<>::gen();

    std::array<size_t, SZ/64> indexes{};
    std::iota(indexes.begin(), indexes.end(), 0);
    std::shuffle(indexes.begin(), indexes.end(), Rand<>::mt);

    std::array<size_t, SZ/64> res{};
    for (auto i : indexes){
        s.clean_cache();
        volatile auto val = s.arr[ri];
        res[i] = s.access_time(i * 64);
    }
    size_t min_v = std::numeric_limits<size_t>::max();
    size_t min_i = 0;
    for (size_t i = 0, e = res.size(); i != e; ++i){
        if (res[i] < min_v) {min_v = res[i]; min_i = i;}
    }
    std::cout << "Rand:" << ri << '\n';
    std::cout << "Min access time:" << (min_i * 64) << '-' << (min_i * 64)+64 << '\n';
}
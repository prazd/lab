#include <vector>
#include <algorithm>
#include <iostream>
#include <random>

template <std::integral T, size_t start, size_t end> 
struct RandInt {
    static inline std::mt19937 gen = std::mt19937{std::random_device{}()};
    static inline std::uniform_int_distribution dis = std::uniform_int_distribution<T>(start, end);
    static auto get() {
        return dis(gen);
    }
};

extern "C" void cppfunc(int n) {
    std::vector<int> vec(n);
    std::generate(vec.begin(), vec.end(), []{return RandInt<int, 0, 100>::get();});
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(std::cout, " "));
}

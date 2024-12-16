#include <iostream>
#include <algorithm>
#include <numeric>
#include <array>


template <uint64_t n>
constexpr uint64_t factorial() {
    std::array<uint64_t, n> arr{};
    std::iota(arr.begin(), arr.end(), 1);
    return std::accumulate(arr.begin(), arr.end(), static_cast<uint64_t>(1), std::multiplies<>{});
}

constexpr uint64_t factorial(uint64_t n) {
        uint64_t res = 1;
        for(uint64_t i = 1; i <= n;++i)
                res*=i;
        return res;
}


int main() {
    static_assert(factorial<20>() == factorial(20));
    constexpr auto res = factorial<20>();
    std::cout << res << std::endl;
}
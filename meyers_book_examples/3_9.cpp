#include <iostream>
#include <array>

// c++ 11
constexpr int pow11(int base, int exp) noexcept {
    return (exp == 0 ? 1 : base * pow11(base, exp - 1));
}


// c++ 14
constexpr int pow14(int base, int exp) noexcept {
    auto result = 1;
    for (int i = 0; i < exp; ++i) result *= base;
    return result;
}


int main() {
    std::array<int, pow11(2, 3)> arr{};
    static_assert(arr.size() == 8);

    std::array<int, pow14(2, 2)> arr2{};
    static_assert(arr2.size() == 4);
}

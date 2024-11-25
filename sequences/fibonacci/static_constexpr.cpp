#include <iostream>

template <size_t n>
struct Fibonacci {
    static constexpr size_t value = Fibonacci<n - 1>::value + Fibonacci<n - 2>::value;
};

template <>
struct Fibonacci<0> {
    static constexpr size_t value = 0;
};

template <>
struct Fibonacci<1> {
    static constexpr size_t value = 1; 
};

int main() {
    constexpr auto res = Fibonacci<30>::value;
    static_assert(res == 832040);
    std::cout << res << "\n";
}

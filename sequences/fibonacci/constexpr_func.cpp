#include <iostream>

template <size_t n>
constexpr size_t fibonacci() {
    size_t a = 0, b = 1;
    for(size_t i = 0; i < n ; ++i) {
        size_t tmp = a;
        a = b;
        b = tmp + b;
    }
    return a;
}

int main() {
    constexpr auto result = fibonacci<42>();
    static_assert(result == 267914296);
    std::cout << result << "\n";
}
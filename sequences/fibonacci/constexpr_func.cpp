#include <iostream>
#include <utility>


constexpr size_t fibonacci(size_t n) {
    size_t a = 0, b = 1;
    for(size_t i = 0;i<n;++i)
        a = std::exchange(b, a + b);
    return a;
}

int main() {
    constexpr auto result = fibonacci(42);
    static_assert(result == 267914296);
    std::cout << result << "\n";
}
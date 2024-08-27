#include <iostream>

template <typename T, size_t N>
constexpr size_t array_size(T(&)[N]) noexcept {
    return N;
}

int main() {
    int a[21]{};
    int b[array_size(a)]{};
    static_assert(sizeof(a)/sizeof(a[0]) == sizeof(b)/sizeof(b[0]));
}
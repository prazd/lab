#include <iostream>
#include <type_traits>
#include <limits>

template <typename T>
constexpr T reverse_bit(T n) {
    static_assert(std::is_integral_v<T>);
    using UnsignedT = std::make_unsigned_t<T>;
    UnsignedT res = 0;
    UnsignedT un = static_cast<UnsignedT>(n);
    int sz = std::numeric_limits<UnsignedT>::digits;
    for (int i = 0; i < sz; ++i) {
        res = (res << 1) | (un & 1);
        un >>= 1;
    }
    return static_cast<T>(res);
}

int main() {
    constexpr auto res = reverse_bit<unsigned char>(2);
    static_assert(res == 64);

    constexpr auto res2 = reverse_bit<unsigned char>(148);
    static_assert(res2 == 41);
}
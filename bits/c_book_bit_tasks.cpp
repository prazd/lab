#include <iostream>

// C programming language book (63/53)

template <typename T>
using UnsignedT = std::make_unsigned_t<T>;

template <std::integral T>
constexpr T getbits(T bits, int p, int n) {
    auto ubits = static_cast<UnsignedT<T>>(bits);
    return static_cast<T>((ubits >> (p+1-n)) & ~(~UnsignedT<T>{}<<n));
}

template <std::integral T>
constexpr T setbits(T x, int p, int n, T y) {
    auto ux = static_cast<UnsignedT<T>>(x);
    auto uy = static_cast<UnsignedT<T>>(y);
    auto mask = (((UnsignedT<T>{1} << n)-1) << (p + 1 - n));
    ux &= ~mask;
    return static_cast<T>(ux | ((((UnsignedT<T>{1} << n)-1) & uy) << (p + 1 - n)));
}


template <std::integral T>
constexpr T invert(T x, int p, int n) {
    auto ux = static_cast<UnsignedT<T>>(x);
    auto mask = (((UnsignedT<T>{1} << n)-1) << (p + 1 - n));
    return static_cast<T>(ux ^ mask);
}

template <std::integral T>
constexpr T rightrot(T x, int n) {
    constexpr int num_bits = sizeof(T) * CHAR_BIT;
    auto ux = static_cast<UnsignedT<T>>(x);
    n %= num_bits;
    return static_cast<T>((ux >> n) | (ux << (num_bits - n)));
}


int main() {
    static_assert(getbits(160, 7, 4) == 10);

    static_assert(setbits(245u, 3, 4, 138u) == 250);
    static_assert(setbits(0u, 1, 2, 15u) == 3);
    static_assert(setbits(15u, 3, 4, 10u) == 10);

    static_assert(invert(15u, 1, 2) == 12);
    static_assert(invert(10u, 3, 4) == 5);

    constexpr unsigned char x = 15;
    static_assert(rightrot(x, 4) == 240);

    constexpr unsigned char y = 240;
    static_assert(rightrot(y, 4) == 15);

    constexpr unsigned char z = 10;
    static_assert(rightrot(z, 2) == 130);
}
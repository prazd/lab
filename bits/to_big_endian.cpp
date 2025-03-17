#include <iostream>
#include <cassert>
#include <bit>
#include <array>

// g++ -std=c++2b to_big_endian.cpp 

template <std::integral T>
T to_big_endian(T t) {
    auto bytes = std::bit_cast<std::array<unsigned char, sizeof(T)>>(t);
    std::reverse(bytes.begin(), bytes.end());
    return std::bit_cast<T>(bytes);
}

int main() {
    int x = 0x12345678;
    auto y = to_big_endian(x);
    assert(y == 0x78563412);
    assert(y == std::byteswap(x)); // (c++23)
}
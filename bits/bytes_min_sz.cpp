#include <iostream>

template <std::integral T>
constexpr size_t bytes_min_sz(T var) {
    using UnsignedT = std::make_unsigned_t<T>;
    UnsignedT uvar = static_cast<UnsignedT>(var);
    size_t min_bytes = 1;
    size_t shift = (sizeof(var) - 1) * CHAR_BIT;
    while(shift > 0) {
        if (uvar >> shift){
            return shift / CHAR_BIT + 1;
        };
        shift-=CHAR_BIT;
    }
    return min_bytes;
}

template <std::integral T>
constexpr size_t bits_min_sz(T var) {
    using UnsignedT = std::make_unsigned_t<T>;
    UnsignedT uvar = static_cast<UnsignedT>(var);
    size_t res = 0;
    while(var) {
        var >>= 1;
        ++res;
    }
    return res;
}


int main() {
    static_assert(bytes_min_sz(std::numeric_limits<unsigned long>::max()) == 8);
    static_assert(bytes_min_sz(std::numeric_limits<unsigned int>::max()) == 4);
    static_assert(bytes_min_sz(std::numeric_limits<unsigned short>::max()) == 2);
    static_assert(bytes_min_sz(std::numeric_limits<unsigned char>::max()) == 1);

    static_assert(bytes_min_sz(256) == 2);
    static_assert(bytes_min_sz(65536) == 3);
    static_assert(bytes_min_sz(0) == 1);

    static_assert(bits_min_sz((unsigned)10) == std::bit_width((unsigned)10));
    static_assert(bits_min_sz(std::numeric_limits<unsigned int>::max()) == std::bit_width(std::numeric_limits<unsigned int>::max()));
}
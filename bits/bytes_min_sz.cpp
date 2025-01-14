#include <iostream>

template <std::integral T>
constexpr size_t min_sz(T var) {
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

int main() {
    static_assert(min_sz(std::numeric_limits<unsigned long>::max()) == 8);
    static_assert(min_sz(std::numeric_limits<unsigned int>::max()) == 4);
    static_assert(min_sz(std::numeric_limits<unsigned short>::max()) == 2);
    static_assert(min_sz(std::numeric_limits<unsigned char>::max()) == 1);

    static_assert(min_sz(256) == 2);
    static_assert(min_sz(65536) == 3);
    static_assert(min_sz(0) == 1);
}
#include <iostream>

template <std::integral T>
constexpr uint64_t bit_sz(T n) {
    uint64_t res = 0;
    using UnsignedT = std::make_unsigned_t<T>;
    UnsignedT un = static_cast<UnsignedT>(n);
    while(un) {
        un >>= 1;
        ++res;
    }
    return res;
}


int main() {
    static_assert(bit_sz(0) == 0);
    static_assert(bit_sz(1) == 1);
    static_assert(bit_sz(255) == 8);
    static_assert(bit_sz(256) == 9);
    static_assert(bit_sz(-1) == sizeof(int) * 8); // !
    static_assert(bit_sz(std::numeric_limits<int>::max()) == 31);
    static_assert(bit_sz(std::numeric_limits<int>::min()) == 32);
    static_assert(bit_sz(std::numeric_limits<uint64_t>::max()) == 64);
    static_assert(bit_sz(static_cast<int8_t>(127)) == 7);
    static_assert(bit_sz(static_cast<int8_t>(-1)) == 8);
}
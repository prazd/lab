#include <iostream>
#include <limits>
#include <climits>
#include <cassert>

template <std::integral T>
constexpr size_t hamming_weight_comptime(T val) {
    size_t res{};
    while(val) {
        val &= val - 1;
        ++res;
    }
    return res;
}

template <typename T>
size_t count_one(T& val) {
    size_t count = 0;
    auto ptr = reinterpret_cast<unsigned char*>(&val);
    for(int i = 0; i < sizeof(val)*CHAR_BIT; ++i) {
        ptr[i/CHAR_BIT] & (1 << i%CHAR_BIT) ? ++count : count;
    }
    return count;
}

template <typename T>
constexpr size_t count_one_comptime(T& val) {
    unsigned int arr[sizeof(val)];
    for(int i = 0; i < sizeof(val);++i) {
        arr[i] = (val >> i*CHAR_BIT) & 0xFF;
    }
    size_t count = 0;
    for(int i = 0; i < sizeof(val)*CHAR_BIT; ++i) {
        arr[i/CHAR_BIT] & (1 << i%CHAR_BIT) ? ++count : count;
    }
    return count;
}

int main() {
    constexpr unsigned int x = std::numeric_limits<unsigned int>::max();
    constexpr auto result = count_one_comptime(x);
    static_assert(result == hamming_weight_comptime(x));
    static_assert(2 == hamming_weight_comptime(10));
    static_assert(8 == hamming_weight_comptime(255));
    
    unsigned int y = std::numeric_limits<unsigned int>::max();
    auto result2 = count_one(y);
    assert(result2 == 32);
}
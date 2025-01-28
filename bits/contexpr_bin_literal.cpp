#include <iostream>
#include <climits>
#include <limits>
#include <array>

// Konstantin Vladimirov example:
template <char... Chars>
constexpr unsigned long long  operator""_bin() {
    static_assert(sizeof...(Chars) <= sizeof(unsigned long long) * CHAR_BIT);
    std::array<char, sizeof...(Chars)> arr{Chars...};
    unsigned long long res{};
    size_t shift = 0;
    for (auto el: arr) {
        switch(el) {
            case '0': {res = res * 2; break;}
            case '1': {res = res * 2 + 1; break;}
            default: {throw "Unexpected symbol";}
        }
    }
    return res;
}

int main() {
    constexpr auto el = 1010_bin;
    static_assert(el == 10);
    constexpr auto el2 = 11111111_bin;
    static_assert(el2 == 255);
    constexpr auto el3 = 1111111111111111111111111111111111111111111111111111111111111111_bin;
    static_assert(el3 == std::numeric_limits<unsigned long long>::max());
}
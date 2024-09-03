#include <iostream>
#include <array>

// В С++1 1 функции constexpr могут содержать не более одной выполнимой инструк­ ции - return.
//  Это выглядит более ограничивающим, чем является на самом деле, по­ скольку для повышения выразительности соnstехрr-функций можно использовать две хитрости.
//  Во-первых, можно применять условный оператор " ? : " вместо инструкции if-else, а во-вторых, вместо циклов можно использовать рекурсию.
//  Таким образом, функция pow может быть реализована следующим образом:

// c++ 11
constexpr int pow11(int base, int exp) noexcept {
    return (exp == 0 ? 1 : base * pow11(base, exp - 1));
}


// В С++ 14 ограничения на соnstехрr-функции существенно слабее, так что становится возможной следующая реализация:
// c++ 14
constexpr int pow14(int base, int exp) noexcept {
    auto result = 1;
    for (int i = 0; i < exp; ++i) result *= base;
    return result;
}


int main() {
    std::array<int, pow11(2, 3)> arr{};
    static_assert(arr.size() == 8);

    std::array<int, pow14(2, 2)> arr2{};
    static_assert(arr2.size() == 4);
}

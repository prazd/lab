#include <iostream>

enum E1: char{
    First = 1,
    Second = 2,
    Third = 4
};

enum class E2: char {
    First,
    Second,
    Third,
    Count
};

int main() {
    constexpr auto f = Third;
    static_assert(f & (First | Second | Third));
    static_assert((char)E2::Count == 3);
}
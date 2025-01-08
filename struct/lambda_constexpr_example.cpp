#include <iostream>

int main() {
    auto sum = [](auto a, auto b)constexpr{
        return a + b;
    };
    constexpr auto res = sum(10,20);
}
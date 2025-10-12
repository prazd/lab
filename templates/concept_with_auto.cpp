#include <iostream>

consteval int foo(std::integral auto val) {
    return 42;
}

int main() {
    constexpr std::integral auto res = foo(0);
}
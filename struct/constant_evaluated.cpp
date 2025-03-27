#include <type_traits>
#include <cassert>

constexpr int compute(int x) {
    if (std::is_constant_evaluated()) {
        return x * x;
    } else {
        return x + 1;
    }
}

int main() {
    constexpr int a = compute(5);
    static_assert(a == 25);
    int b = compute(5);
    assert(b == 6);
    return 0;
}
#include <iostream>

constexpr auto find_unique_el_sum(const auto& arr) {
    size_t s = 0;
    size_t result{};
    while(true) {
        size_t sum{};
        for (auto el: arr) sum += el>>s;
        if (!sum) break;
        size_t n = 1 & sum;
        result |= (n << s);
        ++s;
    }
    return result;
}

constexpr auto find_unique_el_xor(const auto& arr) {
    size_t res{};
    for (auto el: arr) res ^= el;
    return res;
}

int main() {
    constexpr unsigned arr[11]{1,2,2,1,4,4,42,5,9,9,5};
    static_assert(find_unique_el_xor(arr) == 42);
    static_assert(find_unique_el_sum(arr) == 42);
}
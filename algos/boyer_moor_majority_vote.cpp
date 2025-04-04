#include <iostream>
#include <type_traits>
#include <array>
#include <iterator>

// Boyer–Moore majority vote algorithm
template <std::forward_iterator It>
constexpr auto majority_element(It it, It ite) {
    typename std::indirectly_readable_traits<It>::value_type candidate{};
    size_t count = 0;
    for (;it != ite; ++it) {
        auto cur = *it;
        if (count == 0) candidate = cur;
        count += (cur == candidate) ? 1 : -1;
    }
    return candidate;
}

int main() {
    constexpr std::array<int, 5> x{1,1,1,2,2};
    static_assert(majority_element(x.begin(), x.end()) == 1);
}
#include <iostream>
#include <algorithm>
#include <array>

template <std::integral T>
struct Comparator {
    constexpr static bool cmp(T x, T y) {return x < y;}
    using cmp_func = bool(*)(T x, T y); 
    constexpr operator cmp_func() {  // or operator auto()
        return cmp;
    }
};

template <typename Ctr>
consteval auto get_sorted(const Ctr& arr) {
    auto res = arr;
    std::sort(std::begin(res), std::end(res), Comparator<typename Ctr::value_type>{});
    return res;
}

int main() {
    constexpr auto arr = get_sorted(std::array<int, 7>{30,10,2,34,45,11,123});
    static_assert(arr == std::array<int, 7>{2,10,11,30,34,45,123});
}

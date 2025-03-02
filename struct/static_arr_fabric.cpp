#include <array>
#include <algorithm>
#include <cassert>

template <typename T>
struct StaticArrFabric {
    template <size_t... n> 
    static consteval auto get_max_arrays() {
        std::array<int, sizeof...(n)> el{(n)...};
        std::sort(el.begin(), el.end());
        return el.back();
    }
    template <size_t... n>
    static constexpr auto m_tuple() {
        constexpr auto max = get_max_arrays<1,2,3>();
        return std::array<std::array<T, max>, sizeof...(n)>{};
    }
};

int main() {
    constexpr auto arr = StaticArrFabric<int>::get_max_arrays<1,2,3>();
    static_assert(arr == 3);
    auto [res1, res2, res3] = StaticArrFabric<int>::m_tuple<1,2,3>();
    assert(res1.size() == 3);
    assert(res2.size() == 3);
    assert(res3.size() == 3);
}
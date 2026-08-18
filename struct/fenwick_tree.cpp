#include <iostream>
#include <array>
#include <numeric>

template <std::integral T, size_t n>
struct FenwickTree {
    T arr[n]{};
    constexpr FenwickTree(std::random_access_iterator auto it, std::random_access_iterator auto ite){
        for (size_t i = 1; it != ite; ++it, ++i){
            auto count = i & -i;
            arr[i-1] = std::accumulate(it - (count - 1), it+1, T{});
        }
    }
    constexpr T prefix_sum(size_t i) const {
        T s = 0;
        while(i){
            s += arr[i-1];
            i -= i & -i;
        }
        return s;
    }
};

int main(){
    constexpr std::array<int, 8> arr{3,1,4,2,7,5,6,8};
    constexpr FenwickTree<int, 8> ft{arr.begin(), arr.end()};
    static_assert(ft.prefix_sum(1) == 3);
    static_assert(ft.prefix_sum(2) == 4);
    static_assert(ft.prefix_sum(3) == 8);
    static_assert(ft.prefix_sum(4) == 10);
    static_assert(ft.prefix_sum(5) == 17);
    static_assert(ft.prefix_sum(6) == 22);
    static_assert(ft.prefix_sum(7) == 28);
    static_assert(ft.prefix_sum(8) == 36);
}
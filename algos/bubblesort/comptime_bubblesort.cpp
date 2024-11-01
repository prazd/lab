#include <iostream>
#include <cassert>
#include <algorithm>
#include <iterator>
#include <array>

struct DefaultPredicat {
    constexpr bool operator()(const auto& a, const auto& b) const {
        return a > b;
    }
};

template <std::random_access_iterator T, typename Predicat = DefaultPredicat>
constexpr void bubble_sort(T begin, T end, Predicat predicat = DefaultPredicat{}) {
    for (auto i = begin; i != end; ++i) {
        bool swapped = false;
        for (auto j = begin; j != end - (i-begin)-1; ++j) {
            if (predicat(*j, *(j+1))) {
                std::swap(*j, *(j+1));
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

// comptime version
template <typename T, size_t sz>
constexpr decltype(auto) bubble_sort_comptime(T (&array)[sz]) {
    std::array<typename std::remove_const_t<T>, sz> result;
    std::copy(std::begin(array), std::end(array), result.begin());
    bubble_sort(result.begin(), result.end());
    return result;
}


int main() {
    constexpr int x2[5]{50,40,30,20,10};
    constexpr auto result = bubble_sort_comptime(x2);
    constexpr std::array<int, 5> arr{10,20,30,40,50};
    static_assert(std::equal(arr.begin(), arr.end(), result.begin()));
}
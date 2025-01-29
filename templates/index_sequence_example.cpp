#include <cstddef>
#include <array>
#include <initializer_list>
#include <utility>
#include <algorithm>
#include <iostream>
#include <iterator>

// Konstantin Vladimirov example:
template <typename T, size_t N, size_t... Ns>
constexpr std::array<T, N> make_array_impl(std::initializer_list<T> il, std::index_sequence<Ns...>) {
    return std::array<T, N>{*(il.begin() + Ns)...};
}

template <typename T, size_t N>
constexpr std::array<T, N> make_array(std::initializer_list<T> il) {
    return make_array_impl<T, N>(il, std::make_index_sequence<N>());
}

struct S {
    std::array<int, 5> arr;
    constexpr S(std::initializer_list<int> il): arr{make_array<int, 5>(il)}{}
};

int main() {
    constexpr S s{1,2,3,4,5};
    std::copy(s.arr.begin(), s.arr.end(), std::ostream_iterator<int>(std::cout, " "));
}
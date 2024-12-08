#include <iostream>
#include <cassert>
#include <type_traits>

template<typename, typename = void>
constexpr bool is_iterable = false;
 
template<typename T>
constexpr bool is_iterable<
    T,
    std::void_t<decltype(std::declval<T>().begin()),
                decltype(std::declval<T>().end())
    >
> = true;

template <typename T, std::enable_if_t<is_iterable<T>, bool>>
decltype(auto) begin(T& t) {
    return t.begin();
}

template <typename T, std::enable_if_t<is_iterable<T>, bool>>
decltype(auto) end(T& t) {
    return t.end();
}

template <typename T, size_t sz>
decltype(auto) begin(T (&t)[sz]) {
    return &t[0];
}

template <typename T, size_t sz>
decltype(auto) end(T (&t)[sz]) {
    return &t[sz];
}

int main() {
    int x[3]{1,2,3};
    
    auto xbegin = begin(x);
    assert(xbegin == x);

    auto xend = end(x);
    assert(xend == &x[3]);

    std::vector<int> y{1,2,3};
    auto ybegin = begin(y);
    assert(y.begin() == ybegin);

    auto yend = end(y);
    assert(y.end() == yend);
}
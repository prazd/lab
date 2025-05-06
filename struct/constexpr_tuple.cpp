#include <iostream>
template <typename ... T>
struct tuple;

template <typename Head, typename... Tail>
struct tuple<Head, Tail...>: tuple<Tail...>{
    Head head;
    consteval tuple(Head head, Tail... tail): tuple<Tail...>(std::forward<Tail>(tail)...), head(std::forward<Head>(head)){}
};

template<>
struct tuple<> {
};

template <size_t n, typename Head, typename ...Tail>
consteval decltype(auto) get(const tuple<Head, Tail...>& t) {
    if constexpr (n == 0) {
        return t.head;
    } else {
        return get<n-1>(static_cast<const tuple<Tail...>&>(t));
    }
}

template <typename T, typename Head, typename ...Tail>
consteval decltype(auto) get(const tuple<Head, Tail...>& t) {
    if constexpr (std::is_same_v<T, Head>) {
        return t.head;
    } else {
        return get<T>(static_cast<const tuple<Tail...>&>(t));
    }
}

int main() {
    constexpr auto x = tuple<int, float, double>(10, 20.f, 30.2);
    static_assert(get<int>(x) == get<0>(x));
    static_assert(get<float>(x) == get<1>(x));
    static_assert(get<double>(x) == get<2>(x));
}
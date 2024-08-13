#include <iostream>

template <typename... Types>
struct tuple;

template <typename Head, typename ...Tail>
struct tuple<Head, Tail...>: tuple<Tail...>{
    Head head;

    template <size_t n, typename... Types>
    friend decltype(auto) get(const tuple<Types...>& t);

public:
    tuple(Head h, Tail... tail)
        : tuple<Tail...>(std::forward<Tail>(tail)...), head(std::forward<Head>(h)){
            std::cout << "Head value:" << head << std::endl;
        }

};
template <>
struct tuple<> {};

template <size_t n, typename Head, typename... Tails>
decltype(auto) get(const tuple<Head, Tails...>& t) {
    if constexpr (n == 0){
        return t.head;
    } else {
        return get<n-1>(static_cast<const tuple<Tails...>&>(t));
    }
}

int main() {
    tuple<int, float, std::string> st(10, 20.f, "abc");
    std::cout << get<0>(st) << std::endl;
    std::cout << get<1>(st) << std::endl;
    std::cout << get<2>(st) << std::endl;
}
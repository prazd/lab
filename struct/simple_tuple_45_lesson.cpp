#include <iostream>

template <typename... Types>
struct tuple;

template <typename Head, typename ...Tail>
class tuple<Head, Tail...>{
    Head head;
    tuple<Tail...> tail;

    template <size_t n, typename... Types>
    friend decltype(auto) get(tuple<Types...>& t);
public:
    tuple(Head head, Tail ...tail)
        : head(std::forward<Head>(head)), tail(std::forward<Tail>(tail)...) {}

};
template <>
struct tuple<> {};

template <size_t n, typename... Types>
decltype(auto) get(tuple<Types...>& t) {
    if constexpr (n == 0){
        return t.head;
    } else {
        return get<n-1>(t.tail);
    }
}

int main() {
    tuple<int, float, std::string> st(10, 20.f, "123123");

    std::cout << get<0>(st) << std::endl;
    std::cout << get<1>(st) << std::endl;
    std::cout << get<2>(st) << std::endl;
}
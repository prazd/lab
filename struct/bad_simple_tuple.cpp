#include <iostream>

template <typename... Types>
struct tuple;

template <typename Head, typename ...Tail>
struct tuple<Head, Tail...>{
    Head head;
    tuple<Tail...> tail;

    tuple(Head head, Tail ...tail)
        : head(std::forward<Head>(head)), tail(std::forward<Tail>(tail)...) {}

    template <std::size_t n>
    decltype(auto) get() {
        if constexpr (n == 0) {
            return head;
        } else {
            return tail.template get<n-1>();
        }
    }
};

template <>
struct tuple<> {};

int main() {
    tuple<int, float, std::string> mt(10, 20.f, "123123");

    std::cout << mt.get<0>() << std::endl;
    std::cout << mt.get<1>() << std::endl;
    std::cout << mt.get<2>() << std::endl;

}
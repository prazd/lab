#include <iostream>
#include <array>

template <typename First, typename Second>
struct Pair {
    static constexpr std::array<std::string_view, 2> names = {
        "first", "second"
    };
    First first;
    Second second;
};

template <typename Object>
void visit(auto func, Object&& obj) {
    if constexpr (obj.names.size() == 0) {
    } else if constexpr (obj.names.size() == 1) {
        auto&& [m0] = std::forward<Object>(obj);
        func(obj.names[0], std::forward<decltype(m0)>(m0));
    } else if constexpr (obj.names.size() == 2) {
        auto&& [m0, m1] = std::forward<Object>(obj);
        func(obj.names[0], std::forward<decltype(m0)>(m0));
        func(obj.names[1], std::forward<decltype(m1)>(m1));
    }
}

int main() {
    Pair<int, float> p{10, 20.f};
    visit([](std::string_view sv, const auto& value){
        std::cout << sv << " : " << value << '\n';
    }, p);
}
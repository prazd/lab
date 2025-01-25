#include <iostream>
#include <variant>

template <typename... Ts>
struct overloaded: Ts...  {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

int main() {
    auto func = overloaded{
        [](int x){std::cout << "Int:" << x << "\n";},
        [](float x){std::cout << "Float:" << x << "\n";},
        [](double x){std::cout << "Double:" << x << "\n";},
    };

    std::variant<int, float, double> v{10};
    std::visit(func, v);
    v = 30.f;
    std::visit(func, v);
    v = 42.0;
    std::visit(func, v);
}
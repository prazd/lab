#include <iostream>
#include <experimental/scope>

// c++-14 -std=c++2b experimental_scope_exit.cpp
template <typename F>
auto make_defer(F&& f) {
    return std::experimental::fundamentals_v3::scope_exit<std::decay_t<F>>(std::forward<F>(f));
}

void process() {
    std::cout << "Begin" << "\n";
    auto cleanup = make_defer([]{std::cout << "Cleanup";});
    std::cout << "End" << "\n";
}

int main() {
    process();
}
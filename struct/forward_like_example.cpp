#include <iostream>
#include <vector>
#include <string>

// c++-14 -std=c++23 forward_like_example.cpp

struct Base {
    std::vector<std::string> data;
    Base(): data({"abc"}){}

    auto&& operator[](this auto&& self, size_t i) {
        return std::forward_like<decltype(self)>(self.data[i]);
    }
};

int main() {
    Base b;
    static_assert(std::is_lvalue_reference_v<decltype(b[0])>);
    static_assert(std::is_rvalue_reference_v<decltype(Base{}[0])>);
}
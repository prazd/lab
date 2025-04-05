#include <iostream>
#include <type_traits>

struct Classic {
    void print() const& {
        std::cout << "c_lvalue" << '\n';
    }
    void print() const&& {
        std::cout << "c_rvalue" << '\n';
    }
};

struct Modern {
    void print(this auto&& self) {
        if constexpr (std::is_lvalue_reference_v<decltype(self)>) {
            std::cout << "m_lvalue" << '\n';
        } else {
            std::cout << "m_rvalue" << '\n';
        }
    }
};

int main() {
    Classic c;
    c.print();
    Classic{}.print();

    Modern m;
    m.print();
    Modern{}.print();
}


#include <iostream>

struct Base{
    int x;
    int y;
    Base(int x, int y): x(x), y(y){}
    auto operator<=>(const Base& rhs) const = default;
};


int main() {
    Base a(10,10);
    Base b(10,10);
    Base c(11,10);
    std::cout << (a == b) << std::endl;
    std::cout << (c > a) << std::endl;
}

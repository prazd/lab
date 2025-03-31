#include <iostream>

struct Base {
    Base(){std::cout << "Ctr";}
    ~Base(){std::cout << "Dtr";}
};

int main() {
    char buffer[3];
    auto bptr = reinterpret_cast<Base*>(buffer);
    std::uninitialized_default_construct_n(bptr, 3);
    std::destroy_n(bptr, 3);
}
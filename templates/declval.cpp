#include <iostream>

struct Base {
    auto run() {
        return 10;
    }
private:
    Base() = default;    
};

int main() {
    decltype(std::declval<Base>().run()) x = 300;
    std::cout << typeid(x).name() << std::endl;
}
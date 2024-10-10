#include <iostream>

enum class Var: short {
    first, second
};

int main() {
    std::cout <<  typeid(std::underlying_type<Var>::type).name() << std::endl;
}
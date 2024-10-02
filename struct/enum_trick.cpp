#include <iostream>

enum Var: char{
    First = 1,
    Second = 2,
    Third = 4
};


int main() {
    auto f = Third;
    if (f & (First | Second | Third)) {
        std::cout << "ok" << std::endl;
    }
}
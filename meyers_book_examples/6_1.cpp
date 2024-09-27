#include <iostream>

decltype(auto) check_static() {
    static int x = 20;
    auto b = [=](){std::cout << x+30 << std::endl;};
    ++x;
    return b;
}

int main() {
   auto x = check_static();
   x();
   check_static();
   x();
}
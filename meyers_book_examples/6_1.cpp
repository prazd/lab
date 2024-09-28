#include <iostream>

struct Base{
    int value;
    Base(int value): value(value){}
    void run() {
        // В С++14 имеется лучший способ захвата члена-данных, заключающийся в использо­ вании обобщенного захвата лямбда-выражения
        auto l = [value = value]() {std::cout << value;};
        l();
    }
};

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

   //

   Base b(30);
   b.run();
}
#include <iostream>

int main() { 
    auto lambda = [](int x) { return x * x; };

    using LambdaType = decltype(lambda);
    
    class Derived : public LambdaType {
    public:
        using LambdaType::operator();
    };

    Derived d;
    
    std::cout << d(5) << std::endl; // Вывод: 25
    return 0;
}
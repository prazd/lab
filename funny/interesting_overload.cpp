#include <iostream>

struct S;

template <typename T> 
struct Wrapper{
    T val;
};

void foo(const int&){}
void foo(const Wrapper<S>&){}

int get() {
    return 1;
}

// тут компилятор имеет право скомпилировать и не скомпилировать:
// In the function selected by overload resolution can be determined without instantiating a class template definition,
// it is unspecified whether that instantiation actually takes place [temp.inst, 9]

// Гайдлайн понятен: просто не надейтесь на неполные типы
int main() {
    foo(get()); // error
    ::foo(get()); // error
}
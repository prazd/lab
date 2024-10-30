#include <iostream>

// С++23 Владимиров о денотации
// https://youtu.be/kJ6RvI9A1bU

// - Компилятор на самом деле вычисляет вещи сверху-вниз
// - Человек думает о процессе, запускающемся из точки снизу вверх

struct S {   
    int x{2};                // S, S::x
};

struct A {
    struct T {
        int x{1};            // A, A::T, A::T::x
    };
};

template <typename T>
struct B: A {                // <T>, B
    int f() {
        T t;                 // resolve T to A::T
        return t.x;
    }
};

int main() {
    B<S> b;                  // instantiate B<S>
    
    std::cout << b.f(); // 1
}










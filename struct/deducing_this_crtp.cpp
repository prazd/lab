#include <iostream>

struct Base {
    constexpr int foo(this auto&& self) {
        return self.bar();
    }  
};

struct Derived: Base {
    constexpr int bar() const {
        return 42;
    }
};

template <typename T>
concept Fooable = requires(T a)
{
    { a.foo() } -> std::same_as<int>;
};

template <Fooable T> 
constexpr int run(const T& r) {
    return r.foo();
}

int main() {
    static_assert(run(Derived{}) == 42);
}
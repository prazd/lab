struct Base {
    constexpr virtual int foo() const = 0;
};

struct Derived1: Base {
    constexpr int foo() const override {
        return 42;
    }
};

struct Derived2: Base {
    constexpr int foo() const override {
        return 255;
    }
};

consteval auto bar(const Base& b) {
    return b.foo();
}

int main() {
    static_assert(bar(Derived1{}) == 42);
    static_assert(bar(Derived2{}) == 255);
}
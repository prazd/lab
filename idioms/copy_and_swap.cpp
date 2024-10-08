#include <iostream>
#include <algorithm>

struct Base {
    int* x;
    Base(int val): x(new int(val)){}
    Base(const Base& b): x(new int(*b.x)){}
    Base(Base&& b) noexcept: x(b.x) {
        b.x = nullptr;
    }
    Base& operator=(Base b) {
        std::swap(b.x, x);
        return *this;
    }
    ~Base() {
        delete x;
    }
};

int main() {
    Base b(10);
    Base b2(20);
    Base b3 = b2;
    b = b2;
}
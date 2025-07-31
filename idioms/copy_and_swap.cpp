#include <iostream>
#include <algorithm>
#include <utility>
#include <cassert>

struct Base {
    int* x;
    Base(int val): x(new int(val)){}
    Base(const Base& b): x(new int(*b.x)){}
    Base(Base&& b) noexcept: x(std::exchange(b.x, nullptr)) {}
    Base& operator=(const Base& b) {
        Base tmp(b);
        std::swap(tmp.x, x);
        return *this;
    }
    Base& operator=(Base&& b) noexcept {
        Base tmp(std::move(b));
        std::swap(tmp.x, x);
        return *this;
    }
    /* 
        Or one operator= for copy/move assignment
                
        Base& operator=(Base b) {
            std::swap(tmp.x, x);
            return *this;
        }

    */
    ~Base() {
        delete x;
    }
};

int main() {
    Base b(10);
    Base b2(20);
    b = b2;
    assert(*b.x == *b2.x);
    Base b3(30);
    b3 = std::move(b2);
    assert(*b3.x == 20);
}
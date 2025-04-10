#include <iostream>

template <typename T>
class Stack {
    size_t cap;
    size_t sz;
    T* ptr;
public:    
    constexpr Stack(size_t cap): cap(cap), sz(0), ptr(new T[cap]{}){}
    constexpr Stack(): cap(4), sz(0), ptr(new T[cap]{}) {}
    constexpr void push(T val) {
        if (cap == sz) {
            Stack tmp(cap * 2);
            std::copy(ptr, ptr + cap, tmp.ptr);
            tmp.sz = sz;
            std::swap(ptr, tmp.ptr);
        }
        ptr[sz++] = val;
    }
    constexpr void pop() {
        if (!sz) throw std::underflow_error("pop error");
        --sz;
    }
    constexpr bool empty() const {
        return sz == 0;
    }
    constexpr T top() const {
        if (!sz) throw std::underflow_error("top error");
        return ptr[sz-1];
    }
    constexpr ~Stack() {delete[] ptr;}
};

consteval int foo() {
    Stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    s.push(4);
    s.push(5);
    s.pop();
    s.pop();
    s.pop();
    s.pop();
    return s.empty() ? 0 : s.top();
}

int main() {
    constexpr auto s = foo();
    static_assert(s == 1);
}
#include <iostream>
#include <stack>
#include <cassert>


template <typename T, size_t n>
struct StackStorage {
    using value_type = T;
    using reference  = T&;
    using const_reference = const T&;
    using size_type  = size_t;

    T buffer[n];
    size_t offset = 0;
    
    void push_back(T value) {
        buffer[offset++] = value;
    }
    T& back() {
        return buffer[offset-1];
    }
    void pop_back() {
        --offset;
    }

    bool empty() const {
        return offset == 0;
    }

    size_t size() const {
        return offset;
    }
};

template <typename T>
void print(const T& t) {
    auto ptr = (unsigned char*)&t;
    for (auto it = ptr + sizeof(t) - 1, ite = ptr - 1;it != ite;--it)
        std::cout << (void*)it << " " << (unsigned)(*it) << "\n";
}

int main() {
    std::stack<int, StackStorage<int, 30>> s;
    s.push(10);
    s.push(20);
    assert(s.top() == 20);
    s.pop();
    assert(s.top() == 10);
    s.push(30);
    assert(s.top() == 30);
    s.push(40);
    assert(s.top() == 40);
    s.pop();
    assert(s.top() == 30);
    s.pop();
    assert(!s.empty());
    assert(s.size() == 1);
}
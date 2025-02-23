#include <iostream>
#include <cassert>
#include <span>

template <typename T>
struct span {
    T* ptr;
    size_t elems;

    template <std::contiguous_iterator Iter>
    span(Iter it, Iter ite) noexcept : ptr(std::to_address(it)), elems(std::to_address(ite) - std::to_address(it)){}
    
    span(T* ptr, size_t elems) noexcept :  ptr(ptr), elems(elems){}

    span() noexcept : ptr(nullptr), elems(0) {}

    span(const span& s) noexcept = default;
    span(span&& s) noexcept = default;
    span& operator=(const span& s) noexcept = default;
    span& operator=(span&& s) noexcept = default;

    T& operator[](size_t index) const noexcept {
        return ptr[index];
    }

    bool empty() const noexcept {
        return elems == 0;
    }

    T& at(size_t index) const {
        if (index >= elems) {
            throw std::out_of_range("out of range");
        }
        return ptr[index];
    }

    [[nodiscard]] size_t size() const noexcept {
        return elems;
    }

    [[nodiscard]] T* data() const noexcept {
        return ptr;
    }

    T& front() const noexcept {
        return *ptr;
    }

    T& back() const noexcept {
        return ptr[elems-1];
    }

    T* begin() noexcept{
        return ptr;
    }

    T* end() noexcept {
        return ptr + elems;
    }

    const T* begin() const noexcept {
        return ptr;
    }

    const T* end() const noexcept {
        return ptr + elems;
    }

    bool operator==(const span<T>& rhs) const noexcept {
        return (rhs.elems == elems) && (std::equal(rhs.ptr, rhs.ptr + rhs.elems, ptr));
    }
};

int main() {
    int x[3]{1,2,3};
    span<int> s(std::begin(x), std::end(x));
    assert(s[0] == 1);
    assert(s[1] == 2);
    assert(s[2] == 3);
    s[0] = 10;
    assert(x[0] == 10);
    s.at(1) = 20;
    assert(x[1] == 20);
    s.at(0) = 30;
    assert(x[0] == 30);

    assert(!s.empty());
    assert(s.front() == 30);
    assert(s.back() == 3);

    span<int> s2(std::begin(x), std::end(x));
    assert(s == s2);
}
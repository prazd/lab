#include <iostream>

namespace lib {
    template <typename T>
    concept ValueType = std::is_arithmetic_v<T>;

    template <ValueType T>
    class Vec {
        static inline size_t default_cap = 4;
        size_t cap;
        size_t sz;
        T* arr;
    public:
        Vec(std::initializer_list<T> list): cap(list.size()), sz(0), arr(new T[cap]) {
            for (auto el: list) {
                arr[sz++] = el;
            }
        }
        Vec(const Vec<T>& rhs): cap(rhs.cap), sz(rhs.sz), arr(new T[cap]){
            std::copy(rhs.begin(), rhs.end(), arr);
        }

        Vec(const Vec<T>& rhs, size_t capacity): cap(capacity), sz(rhs.sz), arr(new T[cap]){
            std::copy(rhs.begin(), rhs.end(), arr);
        }
        
        Vec<T>& operator=(const Vec<T>& rhs) {
            if (&rhs == this) {
                return *this;
            }
            Vec<T> tmp(rhs);
            tmp.swap(*this);
            return *this;
        }
        Vec(Vec&& rhs) noexcept: cap(rhs.cap), sz(rhs.sz), arr(rhs.arr) {
            rhs.arr = nullptr;
            rhs.sz = 0;
            rhs.cap = 0;
        }
        Vec<T>& operator=(Vec&& rhs) noexcept {
            if (&rhs == this) {
                return *this;
            }
            Vec<T> tmp(rhs);
            tmp.swap(*this);
            return *this;
        }
        Vec(): cap(default_cap), sz(0), arr(new T[cap]){}
        void push(T el) {
            if (cap == sz) {
                Vec<T> tmp(*this, cap*2);
                tmp.swap(*this);
            }
            arr[sz++] = el;
        }
        void pop() {
            if (sz > 0) {
                sz--;
            }
        }
        void swap(Vec<T>& rhs) noexcept {
            std::swap(cap, rhs.cap);
            std::swap(arr, rhs.arr);
            std::swap(sz, rhs.sz);
        }
        T& operator[](size_t index) {
            return arr[index];
        }
        size_t capacity() const {
            return cap;
        }
        size_t size() const {
            return sz;
        }
        T* begin() const {
            return arr;
        }
        T* end() const {
            return &arr[sz];
        }
        const T* cbegin() const {
            return begin();
        }

        const T* cend() const {
            return end();
        }

        ~Vec() {
            delete[] arr;
        }
    };

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const lib::Vec<T>& v) {
        for(auto el: v) {
            os << el << " ";
        }
        os << "\n";
        return os;
    }
}



int main() {
    lib::Vec<int> v {20,2,3,4};
    v.push(20);
    std::cout << v << std::endl;
    
    lib::Vec<int> v2 = v;
    v2.push(100);
    std::cout << v2 << std::endl;


    lib::Vec<int> v3;
    v3 = v2;
    v3.push(140);
    std::cout << v3 << std::endl;

    lib::Vec<int> v4 = v3;
    lib::Vec<int> v5(std::move(v4));
    v5.push(200);
    std::cout << v5 << std::endl;

    lib::Vec<int> v6 = v5;
    lib::Vec<int> v7;
    v7 = std::move(v6);
    v7.push(222);
    std::cout << v7 << std::endl;
}

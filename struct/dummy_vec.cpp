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
        Vec(): cap(default_cap), sz(0), arr(new T[cap]){}
        void push(T el) {
            if (cap == sz) {
                auto tmp = new T[cap*2];
                // memcpy(tmp, arr, cap*sizeof(T));
                std::copy(arr, arr+cap, tmp);
                delete[] arr;
                cap*=2;
                std::swap(tmp, arr);
            }
            arr[sz++] = el;
        }
        void pop() {
            if (sz > 0) {
                sz--;
            }
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
}


int main() {
    lib::Vec<int> v {20,2,3,4};
    v.pop();
    v.push(42);
    for (auto el: v) {
        std::cout << el << std::endl;
    }
}

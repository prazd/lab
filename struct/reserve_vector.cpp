#include <iostream>

template <typename T>
class vector {
    T* arr;
    size_t sz;
    size_t cap;

    void reserve(size_t newcap) {
        if (newcap <= cap) {
            return;
        }
        // !! T* newarr = new T[newcap]; !!
        T* newarr = reinterpret_cast<T*>(new char[newcap * sizeof(T)]); // 1
        size_t index = 0;
        try {
            for (;index < sz; ++index) {
            //     newarr[index] = arr_[index]; // 2 ??? UB
                new(newarr+index) T(arr[index]); // placement new
            }
        } catch (...) {
            for (size_t nindex = 0; nindex < index; ++nindex) {
                (newarr + nindex)->~T();
            }
            delete[] reinterpret_cast<char*>(newarr);
            throw;
        }
     

        for (size_t index = 0; index < sz; ++index) {
            (arr+index)->~T();
        }
        delete[] reinterpret_cast<char*>(arr);

        arr = newarr;
        cap = newcap;
    }

    void push_back(const T& r) {
        if (sz == cap) {
            reserve(cap>0?cap*2:1);
        }
    }
};


int main() {

}
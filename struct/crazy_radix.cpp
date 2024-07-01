
#include <iostream>
#include <concepts>
#include <limits>

template<typename T>
concept UnsignedNumeric = std::is_arithmetic_v<T> && std::is_unsigned_v<T>;

template <UnsignedNumeric T>
struct R {
    T data;
//    bool updated;
    R(): data(0){}
    R(T d): data(d){}
    explicit operator T() {
        return data;
    }
};

template <UnsignedNumeric T>
struct MaxArray {
    static constexpr size_t lenght = std::numeric_limits<T>::max();
    static R<T> data[lenght];
    MaxArray(){
        for (T i = 0;i < lenght; i++) {
            data[i] = R<T>();
        }
    }
};
template <UnsignedNumeric T>
R<T> MaxArray<T>::data[MaxArray<T>::lenght];


template <UnsignedNumeric T, int N>
T* radix_sort(T array[N], MaxArray<T>* ma) {
    for(T i = 0; i < ma->lenght; i++) {
        ma->data[i] = R<T>();
    }
    for(T i = 0; i < N; i++) {
        ma->data[array[i]] = R(array[i]);
    }
    T* result_array = new T[N];
    T current_max_index{0};
    T result_index{0};
    while(current_max_index < ma->lenght) {
        T current_el = T(ma->data[current_max_index++]);
        if (current_el != 0) {
            result_array[result_index++] = current_el;
            continue;
        }
        if (result_index == N) {
            break;
        }
    }
    return result_array;
}

MaxArray<unsigned int> max_array;

int main() {
    unsigned int a[10]{90,80,70,60,123,20,30,86,13,49};
    auto* result = radix_sort<unsigned int, 10>(a, &max_array);
    for(int i=0;i<10;i++) {
        std::cout << result[i] << "  ";
    }
    delete[] result;
}
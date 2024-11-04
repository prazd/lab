#include <iostream>

template <typename T, size_t N>
T* copy_elements(T* arr[N]) {
    T* result = new T[N];
    auto next = result;
    for(size_t i = 0; i < N;++i) 
        result[i] = *(arr[i]);
    return result;
}

struct Base {
    int x;
    int y;
};

std::ostream& operator<<(std::ostream& os, const Base& b) {
    os << "x:" << b.x << " y:" << b.y;
    return os;
}

int main() {
    int x = 10;
    int y = 20;
    int z = 30;

    int* arr[3] = {&x, &y, &z};
    auto int_elems = copy_elements<int, 3>(arr);
    for(int i = 0;i < 3; ++i) {
        std::cout << "Int elems: " << int_elems[i] << std::endl;
    }
    delete[] int_elems;


    Base b1{10,20};
    Base b2{30,40};
    Base b3{50,60};

    Base* arr2[3] = {&b1, &b2, &b3};

    auto base_elems = copy_elements<Base, 3>(arr2);
    for(int i = 0;i < 3; ++i) {
        std::cout << "Base elems: " << base_elems[i] << std::endl;
    }

    delete[] base_elems;
}
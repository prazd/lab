#include <iostream>

struct S {
    static const int x = 100;
};

int z = S::x + 3; // x not ODR-used

int foo(const int* a) {return *a;}

const int S::x; // !!!
int b = foo(&S::x) + 5; // x ODR-used

int main(){
    std::cout << "S::x : " << S::x << "\n";
    std::cout << "Z:" << z << "\n";
    std::cout << "B:" << b << "\n";
}

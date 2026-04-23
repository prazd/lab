#include <iostream>

enum Types: uintptr_t {
    Int, Float, Double
};
constexpr uintptr_t TAG = 0x3;
constexpr uintptr_t MASK = ~TAG;

void handle(uintptr_t t){
    switch (static_cast<Types>(t & TAG)) {
        case Int:    {std::cout << "Int:"    <<  *reinterpret_cast<int*>(t & MASK)    << '\n';  break;}
        case Float:  {std::cout << "Float:"  <<  *reinterpret_cast<float*>(t & MASK)  << '\n';  break;}
        case Double: {std::cout << "Double:" <<  *reinterpret_cast<double*>(t & MASK) << '\n';  break;}
    }
}

int main(){
    static_assert(alignof(int)    == 4);
    static_assert(alignof(float)  == 4);
    static_assert(alignof(double) == 8);

    int x = 1;
    float y = 2.2f;
    double z = 3.3;

    uintptr_t tx = reinterpret_cast<uintptr_t>(&x) | Int;     // (uintptr_t)&x % 4 == 0
    uintptr_t ty = reinterpret_cast<uintptr_t>(&y) | Float;   // (uintptr_t)&y % 4 == 0
    uintptr_t tz = reinterpret_cast<uintptr_t>(&z) | Double;  // (uintptr_t)&z % 8 == 0
    
    handle(tx);
    handle(ty);
    handle(tz);
}
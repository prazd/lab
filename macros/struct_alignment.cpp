#include <iostream>

struct F {
    int x;
    long y;
};

#pragma pack(push, 1)
struct S {
    int x;
    long y;
};
#pragma pack(pop)

 struct T {
    int x;
    long y;
} __attribute__((aligned(32)));

int main() {
    std::cout << sizeof(F) << std::endl;
    std::cout << sizeof(S) << std::endl;
    std::cout << sizeof(T) << std::endl;
}
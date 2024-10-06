#include <iostream>

#if 1
void func() {
    std::cout << "Func1";
}
#endif

#if 0
void func() {
    std::cout << "Func2";
}
#endif


int main() {
    func();
}
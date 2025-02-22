#include <iostream>

struct Base {
    virtual void run() {std::cout << "BaseRun";}
};

struct Derived: Base {
    void run() override  {
        std::cout << "DerivedRun";
    }
};

int main() {
    Derived d;
    auto ptr = (Base*)&d;
    auto pVtable = (uint64_t**)ptr;
    std::cout << "First entry:" << (void*)pVtable[0][0] << "\n";
    using VoidFunc = void(*)(void*);
    auto method = (VoidFunc)(pVtable[0][0]);
    method(ptr);
}
#include <iostream>

struct Base {
    int x{10};
    virtual void print() {
        std::cout << "Base:" << x << "\n";
    }
    virtual ~Base() = default;
};

struct Derived: Base {
    void print() override {
        std::cout << "Derived:" << Base::x << "\n";
    }
};

int main() {
    alignas(alignof(Derived)) char buffer[sizeof(Derived)];
    Base* obj = new (buffer) Derived();
    obj->print();
    obj->~Base();

    new (buffer) Derived();
    
    // После new (buffer) Derived(); создаётся новый объект Derived, но компилятор может предполагать, что buffer всё ещё содержит старый объект.
    // std::launder сообщает компилятору: «Эта память теперь содержит новый объект, не полагайся на старые предположения».
    // std::launder нужен, потому что после placement new компилятор мог бы оптимизировать указатель, считая, что в buffer всё ещё старый объект.
    // Это важно, особенно если включена агрессивная оптимизация.

    auto* recovered = std::launder(reinterpret_cast<Derived*>(buffer));
    recovered->print();
    recovered->~Derived();
}
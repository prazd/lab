#include <iostream>

struct Iface {
    void work(int i = 42) {
        work_impl(i);
    }
protected:
    virtual void work_impl(int i) = 0;
    virtual ~Iface() = default;
};

struct Impl: Iface {
    void work_impl(int i) override {
        std::cout << "Impl:" << i << std::endl;
    }
};

void run(Iface* iface) {
    iface->work();
}

int main() {
    Impl impl;
    run(&impl);
}
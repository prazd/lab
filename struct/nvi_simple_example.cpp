#include <iostream>

struct Iface {
    void work_iface(int i = 42) {
        work(i);
    }
protected:
    virtual void work(int i) = 0;
    virtual ~Iface() = default;
};

struct Impl: Iface {
    void work(int i) override {
        std::cout << "Impl:" << i << std::endl;
    }
};

void run(Iface* iface) {
    iface->work_iface();
}

int main() {
    Impl impl;
    run(&impl);
}
#include <iostream>
#include <atomic>

class once_flag {
    std::atomic<bool> f = false;
public:
    bool cas() {
        bool expected = false;
        return f.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
    }
};

struct call_once {
    call_once(once_flag& flag, auto&& func) {
        bool expected = false;
        if (flag.cas()) {
            func();
        };
    }
};

once_flag x;

void foo() {
    call_once(x, []{
        std::cout << "Once";
    });
    std::cout << "Foo";
}

int main() {
    foo();
    foo();
}
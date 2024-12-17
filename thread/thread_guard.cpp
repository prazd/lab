#include <iostream>
#include <thread>
#include <chrono>


struct ScopedThreadGuard {
    std::thread t;
    explicit ScopedThreadGuard(std::thread t): t(std::move(t)) {
        if (t.joinable())
            throw std::logic_error("No thread");
    }
    ~ScopedThreadGuard(){t.join();}
    ScopedThreadGuard(const ScopedThreadGuard&) = delete;
    ScopedThreadGuard& operator=(const ScopedThreadGuard&) = delete;
};

struct ExceptionExample {
    void test() {
        int local_state = 0;
        ScopedThreadGuard g(std::thread([](){
            std::cout << "Start thread job" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            std::cout << "Finish thread job" << std::endl;
        }));
        auto el = [](){throw 1;};
        el();
    }
};



int main() {
    ExceptionExample ee;
    try {
        ee.test();
    } catch (int i) {
        std::cout << "Error:" << i << std::endl;
    }
}
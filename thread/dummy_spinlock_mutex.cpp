#include <iostream>
#include <atomic>
#include <thread>

class SpinlockMutex {
    std::atomic_flag af;
public:
    SpinlockMutex(): af(ATOMIC_FLAG_INIT) {}
    void lock(){
        while(af.test_and_set(std::memory_order_acquire)){
            static constexpr int max_spins = 1000;
            for (int spins = 0; spins < max_spins; ++spins) {
                if (!af.test_and_set(std::memory_order_acquire)) {
                    return;
                }
            }
            std::this_thread::yield();
        }
    }
    void unlock(){
        af.clear(std::memory_order_release);
    }
} mt;
std::vector<int> data;

int main() {
    std::thread t1([](){
        std::lock_guard lg(mt);
        for(size_t i = 0; i < 10;++i)
            data.push_back(i);
    });

    std::thread t2([](){
        std::lock_guard lg(mt);
        for(size_t i = 10; i < 20;++i)
            data.push_back(i);
    });

    t1.join();
    t2.join();
    for(auto el: data)
        std::cout << el << " ";
}
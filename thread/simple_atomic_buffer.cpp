#include <iostream>
#include <array>
#include <thread>

template <typename T, size_t n>
struct SimpleAtomicBuffer {
    std::array<T, n> buffer{};
    size_t sz = 0;
    std::atomic_flag af = ATOMIC_FLAG_INIT;
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
    void push(const T& value) {
        lock();
        buffer[sz] = value;
        sz = (sz + 1) % n;
        unlock();
    }
    void read(auto& arr) {
        lock();
        std::copy(std::begin(buffer), std::end(buffer), std::begin(arr));
        unlock();
    }
};

int main(){
    SimpleAtomicBuffer<int, 10> ab;
    ab.push(10);
    ab.push(20);
    ab.push(30);

    std::thread t1([](SimpleAtomicBuffer<int, 10>& ab){
        for (int i = 0; i < 100; ++i)
            ab.push(i);
    }, std::ref(ab));

    std::thread t2([](SimpleAtomicBuffer<int, 10>& ab){
        for (int i = 0; i < 100; ++i)
            ab.push(i);
    }, std::ref(ab));

    t1.join();
    t2.join();

    std::array<int, 10> res{};
    ab.read(res);
    for (auto el: res) std::cout << el << ' ';
}
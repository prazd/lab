#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <condition_variable>

class SafeCounter {
    std::atomic<int> counter{};
    bool kill{};
    std::chrono::milliseconds ms_wait{};
    std::chrono::steady_clock::time_point last_increment;
    std::condition_variable cv;
    std::mutex m;
    std::thread worker;
public:    
    SafeCounter(size_t ms): ms_wait(ms) {
        counter.store(0);
        worker = std::thread([this]{
            std::unique_lock lk(m);
            while(!kill) {
                auto next_reset = last_increment + ms_wait;
                while(!kill && cv.wait_until(lk, next_reset) != std::cv_status::timeout)
                    next_reset = last_increment + ms_wait;
                if (kill) {return;}
                if (std::chrono::steady_clock::now() >= last_increment + ms_wait){
                    counter.store(0);
                }
            }
        });
    }
    int get()const {
        return counter.load();
    }
    void increment() {
        ++counter;
        {
            std::lock_guard mt(m);
            last_increment = std::chrono::steady_clock::now();
        }
        cv.notify_one();
    }
    ~SafeCounter(){
        {
            std::lock_guard lg(m);
            kill = true;
        }
        cv.notify_one();
        if (worker.joinable()) worker.join();
    }
};

int main() {
    using namespace std::chrono_literals;
    SafeCounter counter(1000); // сброс через 1000 мс
    counter.increment();       // value = 1
    std::this_thread::sleep_for(500ms);
    counter.increment();       // value = 2, таймер сброса перезапускается
    std::this_thread::sleep_for(1500ms);
    std::cout << counter.get(); // value = 0
}
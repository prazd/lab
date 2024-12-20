#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

std::mutex mt;
std::condition_variable data_cond;
std::queue<int> x;
std::atomic<bool> val = true;

void producer() {
    for (size_t i = 0; i < 100; ++i) {
        {
            std::lock_guard lg(mt);
            for (size_t j = 0; j < 10; ++j) {     
                x.push(i + j);
            }
        }
        data_cond.notify_one();
    }
    val.store(false);
    data_cond.notify_one();
}

void consumer() {
    while (true) {
        std::unique_lock ul(mt);
        data_cond.wait(ul, []{ return !x.empty() || !val.load(); });

        if (!x.empty()) {
            auto d = x.front();
            x.pop();
            ul.unlock();
            std::cout << "Data:" << d << std::endl;
        } else if (!val.load()) {
            break;
        }
    }
}

int main() {
    std::thread p(producer);
    std::thread c(consumer);

    p.join();
    c.join();
    return 0;
}

#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <deque>
#include <queue>

template <typename T, typename Base = std::deque<T>>
class TQueue {
    mutable std::mutex mt;
    std::queue<T, Base> queue;
    std::condition_variable cv;
public:
    TQueue() = default;
    TQueue(const TQueue& rhs) {
        std::scoped_lock lock(mt, rhs.mt); 
        queue = rhs;
    }
    TQueue& operator=(const TQueue& rhs) {
        if (&rhs == this) {
            return *this;
        }
        {
            std::lock_guard lg(mt);
            queue = rhs;
        }
        return *this;
    }

    void push(T val) {
        std::lock_guard lg(mt);
        queue.push(val);
        cv.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock ul(mt);
        cv.wait(ul, [this]{return !queue.empty();});
        value = queue.front();
        queue.pop();
    }

    bool empty() const {
        std::lock_guard lg(mt);
        return queue.empty();
    }
};


int main() {
    TQueue<int> tq;
    std::thread t1([](TQueue<int>& t){
        for(size_t i = 0;i<30;++i){
            t.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    },std::ref(tq));

    std::thread t2([](TQueue<int>& t){
        int res;
        while(true){
            t.wait_and_pop(res);
            std::cout << "Get message:" << res << "\n";
        }
    },std::ref(tq));

    t2.detach();
    t1.join();
}
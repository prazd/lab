#include <iostream>
#include <thread>
#include <mutex>
#include <stack>
#include <optional>
#include <vector>

template <typename T, typename Base = std::deque<T>>
struct TStack {
    std::stack<T, Base> stack;
    mutable std::mutex mt;

    TStack() {}
    TStack(const TStack&) = delete;
    TStack& operator=(const TStack&) = delete;

    TStack(const std::initializer_list<T>& list) {
        for (auto el : list) {
            stack.push(el);
        }
    }

    void push(T val) {
        std::lock_guard lg(mt);
        stack.push(val);
    }

    std::optional<T> pop() {
        std::lock_guard lg(mt);
        if (stack.empty()) {
            return std::nullopt;
        }
        auto res = stack.top();
        stack.pop();
        return res;
    }

    size_t size() const {
        std::lock_guard lg(mt);
        return stack.size();
    }
};

int main() {
    TStack<int> s{1, 2};
    std::thread t1([](TStack<int>& s) {
        for (size_t i = 0; i < 10; ++i) {
            s.push(i);
        }
    }, std::ref(s));

    std::thread t2([](TStack<int>& s) {
        while (true) {
            auto val = s.pop();
            if (!val) {
                break;
            }
            std::cout << "Value: " << *val << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }, std::ref(s));

    t1.join();
    t2.join();
}

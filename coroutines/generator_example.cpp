#include <generator>
#include <iostream>

#include <coroutine>
#include <iostream>

template <typename T>
struct generator {
    struct promise_type {
        T current_value;

        generator get_return_object() { return generator{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        std::suspend_always yield_value(T value) noexcept {
            current_value = value;
            return {};
        }
        void unhandled_exception() { std::terminate(); }
    };

    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    explicit generator(handle_type h) : coro(h) {}
    ~generator() { if (coro) coro.destroy(); }

    T next() {
        coro.resume();
        return coro.done() ? T{} : coro.promise().current_value;
    }
};

generator<int> range(int start, int end) {
    for (int i = start; i < end; ++i) {
        co_yield i;
    }
}

std::generator<int> generate() {
   for (int i = 0; i < 10; ++i)
       co_yield i;
}


int main() {
  for (auto el: generate())
        std::cout << el;
  std::cout << "\n";  
  auto gen = range(1, 10);
    for (int i = gen.next(); i != 0; i = gen.next()) {
        std::cout << i;
    }
}
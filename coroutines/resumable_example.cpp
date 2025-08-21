#include <generator>
#include <iostream>
#include <utility>
#include <coroutine>
#include <iostream>

struct resumable {
    struct promise_type {
        using coro_handle = std::coroutine_handle<promise_type>;
        resumable get_return_object() { return coro_handle::from_promise(*this);}
        std::suspend_always initial_suspend() noexcept { return std::suspend_always(); }
        std::suspend_always final_suspend() noexcept { return std::suspend_always(); }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    using coro_handle = std::coroutine_handle<promise_type>;
    resumable(coro_handle handle): handle_(handle){}
    resumable(resumable&) = delete;
    resumable(resumable&& rhs): handle_(std::exchange(rhs.handle_, nullptr)){}

    bool resume() {
        if (!handle_.done())
            handle_.resume();
        return !handle_.done();
    }

    ~resumable() {
        if(handle_)
            handle_.destroy();
    }
private:
    coro_handle handle_;    
};

resumable foo() {
    std::cout << "Begin\n";
    co_await std::suspend_always();
    std::cout << "End\n";
}

int main() {
    auto r = foo();
    r.resume();
    std::cout << "Mid\n";
    r.resume();
}
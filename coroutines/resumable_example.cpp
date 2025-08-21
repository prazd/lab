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

#define WAIT(N) while(N--){std::cout << "Tick\n"; co_await std::suspend_always();}

resumable add() {
    std::cout << "Add\n";
    int z = 3;
    WAIT(z);
    std::cout << "Step\n";
    z = 3;
    WAIT(z);
    std::cout << "Step\n\n";
}

resumable load() {
    std::cout << "Load\n";
    int z = 3;
    WAIT(z);
    std::cout << "Step\n";
    z = 3;
    WAIT(z);
    std::cout << "Step\n";
    z = 3;
    WAIT(z);
    std::cout << "Step\n\n";
}

int main() {
    int t_cycles = 8;
    auto c_add = add();
    while(t_cycles--) c_add.resume();
    t_cycles = 12;
    auto c_load = load();
    while(t_cycles--) c_load.resume();
}
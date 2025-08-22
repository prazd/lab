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



struct CPU {
    const int& current_op;
    resumable execute() {
        int delay = 0;
        while(true) {
            switch(current_op) {
            case 1: {
                co_await std::suspend_always();
                std::cout << "1:Step 1\n"; 
                co_await std::suspend_always();
                co_await std::suspend_always(); 
                std::cout << "1:Step 2\n";
                co_await std::suspend_always();
                break;
            }
            case 2: {
                delay = 3;
                while(delay--) co_await std::suspend_always();
                std::cout << "2:Step 1" << '\n';
                delay = 3;
                co_await std::suspend_always();
                while(delay--) co_await std::suspend_always();
                std::cout << "2:Step 2" << '\n';
                co_await std::suspend_always();
                break;
            }
            default: {
                co_await std::suspend_always();
                std::cout << "3:Step 1\n";
                co_await std::suspend_always();
                co_await std::suspend_always();
                std::cout << "3:Step 2\n";
                co_await std::suspend_always();
                break;
            }
        }
    }
}
};

int get_t_cycles(int x) {
    switch(x){
        case 1: return 4;
        case 2: return 8;
        default: return 4;
    }
}

int main() {
    int count = 0;
    CPU cpu(count);
    auto e = cpu.execute();
    while(count++ < 3) {
        auto t_cycle = get_t_cycles(count);
        std::cout << "\n\nT cycle:" << t_cycle << "\n";
        while(t_cycle--){
            e.resume();
            std::cout << "Tick\n";
        }
    }
}
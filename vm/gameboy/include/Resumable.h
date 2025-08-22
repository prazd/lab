#ifndef RAM_H_RESUMABLE_H
#define RAM_H_RESUMABLE_H
#include <coroutine>
#include <iostream>
struct Resumable {
    struct promise_type{
        using coro_handle = std::coroutine_handle<promise_type>;
        Resumable get_return_object();
        std::suspend_always initial_suspend() noexcept;
        std::suspend_always final_suspend() noexcept;
        void return_void();
        void unhandled_exception();
    };
    using coro_handle = std::coroutine_handle<promise_type>;
    Resumable(coro_handle handle);
    Resumable(Resumable&) = delete;
    Resumable(Resumable&& rhs);
    bool resume();
    ~Resumable();
private:
    coro_handle handle_;
};
#endif //RAM_H_RESUMABLE_H

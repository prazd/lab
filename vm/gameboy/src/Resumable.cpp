#include "Resumable.h"

using coro_handle = std::coroutine_handle<Resumable::promise_type>;

using coro_handle = std::coroutine_handle<Resumable::promise_type>;
Resumable Resumable::promise_type::get_return_object() { return coro_handle::from_promise(*this);}
std::suspend_always Resumable::promise_type::initial_suspend() noexcept { return std::suspend_always(); }
std::suspend_always Resumable::promise_type::final_suspend() noexcept { return std::suspend_always(); }
void Resumable::promise_type::return_void() {}
void Resumable::promise_type::unhandled_exception() { std::terminate(); }

Resumable::Resumable(Resumable&& rhs): handle_(std::exchange(rhs.handle_, nullptr)) {}
Resumable::Resumable(coro_handle handle ): handle_(handle) {}

bool Resumable::resume() {
    if (!handle_.done())
        handle_.resume();
    return !handle_.done();
}

Resumable::~Resumable() {
    if (handle_)
        handle_.destroy();
}
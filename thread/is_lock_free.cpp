#include <atomic>
#include <iostream>
#include <cassert>

struct S {
    long x{};
};

struct S2 {
    int x{};
    char y{};
};

struct S3 {
    long x{};
    long y{};
    long z{};
};


int main() {
    static_assert(std::atomic<int>::is_always_lock_free);
    static_assert(std::atomic<S>::is_always_lock_free);
    static_assert(std::atomic<S2>::is_always_lock_free);
    static_assert(!(std::atomic<S3>::is_always_lock_free));

    assert(std::atomic<int>{}.is_lock_free());
    assert(std::atomic<S>{}.is_lock_free());
    assert((std::atomic<S2>{}.is_lock_free()));
    assert(!(std::atomic<S3>{}.is_lock_free()));
}
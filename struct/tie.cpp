#include <iostream>
#include <tuple>
#include <cassert>

auto tie(auto&... args) {
    return std::tuple<decltype(args)...>(args...);
}

int main() {
    int x;
    float y;
    tie(x,y) = std::make_tuple(10,20.f);
    assert(x == 10);
    assert(y == 20.f);
}
#include <future>
#include <iostream>
#include <thread>

auto sum(auto... vals) {
    return (vals + ...);
}

int main() {
    std::packaged_task<int(int,int)> task(sum<int,int>);
    auto f = task.get_future();
    std::thread t(std::move(task), 10, 20);
    t.join();
    std::cout << f.get() << std::endl;
}
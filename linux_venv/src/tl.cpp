#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

thread_local int count = 10;

std::mutex mt;

void local(size_t n) {
   while(count){   
        --count; 
        std::this_thread::yield();
   }
}

int main() {
    std::thread t1(local,1);
    std::thread t2(local,2);

    t1.join();
    t2.join();
}

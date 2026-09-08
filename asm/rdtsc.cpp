#include <iostream>
#include <chrono>
#include <thread>

const float FREQ = 2300000000.f; // 2.3 GHz
const size_t SLEEP_MS = 500; 

template <typename F, typename ... Args>
unsigned long bench(F f, Args&&... args){
    unsigned long begin_l{};
    unsigned long begin_h{};
    unsigned long end_l{};
    unsigned long end_h{};
   
    asm volatile("lfence");
    asm volatile("rdtsc": "=a"(begin_l), "=d"(begin_h));
    f(std::forward<Args>(args)...);
    asm volatile("rdtscp" : "=a"(end_l), "=d"(end_h));
    asm volatile("lfence");
    return (end_l | (end_h << 32)) - (begin_l | (begin_h << 32));
}

void test(size_t ms){
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main(){
   auto t = bench(test, SLEEP_MS);
   std::cout << "T:" << t << '\n'; 
   std::cout << "Sec:" << ((float)t / FREQ) << '\n';
}

#include <random>
#include <iostream>
#include <chrono>
#include <cassert>

constexpr size_t COUNT = 1000'000;

template <size_t begin, size_t end>
class Rand{
    static inline std::random_device rd;
    static inline std::mt19937 gen = std::mt19937(rd());
    static inline std::uniform_int_distribution<unsigned long> dis = std::uniform_int_distribution<unsigned long>(begin, end);
public:    
    static unsigned long get() {return dis(gen);}
};

class Rdrand {
    static inline unsigned long regs[2]{};
public:
    static inline size_t failed_count{};
    static unsigned long get() {
        asm volatile("rdrand %%rax" :"=a"(regs[0])); 
        asm volatile("setc %%bl" :"=b"(regs[1]));
        failed_count += !(regs[1]);
        return regs[0];
    }
};

void test_rand(){
    volatile unsigned long val{};
    for (size_t i = 0; i != COUNT; ++i) val = Rand<0,0xffffffffffffffff>::get();
}
void test_rdrand(){
    Rdrand::failed_count = 0;
    volatile unsigned long val{};
    for (size_t i = 0; i != COUNT; ++i) val = Rdrand::get();
    assert(!Rdrand::failed_count);
}


void bench(const char* text, auto f){
    auto begin = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    std::cout << text << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << " Ms\n";
}


int main(){
    bench("Rand:", test_rand);
    bench("Rdrand:", test_rdrand);
}
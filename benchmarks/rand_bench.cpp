#include <random>
#include <iostream>
#include <chrono>
#include <cassert>
#include <queue>

constexpr size_t COUNT = 1000'000;

template <size_t begin, size_t end>
struct Rand{
    static inline std::random_device rd;
    static inline std::mt19937 gen = std::mt19937(rd());
    static inline std::uniform_int_distribution<unsigned long> dis = std::uniform_int_distribution<unsigned long>(begin, end);
    static unsigned long dget(){return dis(gen);}
    static unsigned long rget() {return rd();}
};

struct Rdrand {
    static inline size_t failed_count{};
    static inline unsigned long get() {
        unsigned long rax = 0, rbx = 0;
        asm volatile("rdrand %%rax" :"=a"(rax)); 
        asm volatile("setc %%bl" :"=b"(rbx));
        failed_count += !(rbx);
        return rax;
    }
};

void test_rand_rget(){
    volatile unsigned long val{};
    for (size_t i = 0; i != COUNT; ++i) val = Rand<0,0xffffffffffffffff>::rget();
}

void test_rand_dget(){
    volatile unsigned long val{};
    for (size_t i = 0; i != COUNT; ++i) val = Rand<0,0xffffffffffffffff>::dget();
}

void test_rdrand(){
    Rdrand::failed_count = 0;
    volatile unsigned long val{};
    for (size_t i = 0; i != COUNT; ++i) val = Rdrand::get();
    assert(!Rdrand::failed_count);
}

auto bench(auto f){
    auto begin = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
}

struct Res {
    const char* text{};
    decltype(bench(test_rand_dget)) count{};
};
bool operator<(const Res& lhs, const Res& rhs){
    return lhs.count < rhs.count;
}
bool operator>(const Res& lhs, const Res& rhs){
    return !(operator<(lhs, rhs));
}

template <typename T, size_t N>
struct SimpleInplaceVec{
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using const_reference = const T&;

    alignas(T) unsigned char buffer[sizeof(T) * N];
    size_t cur = 0;

    void push_back(T val){new(std::launder(buffer + (cur * sizeof(T)))) T(val); ++cur;}
    void emplace(T&& val){new(std::launder(buffer + (cur * sizeof(T)))) T(std::forward<T>(val));++cur;}
    void pop_back(){reinterpret_cast<T*>(buffer + (cur * sizeof(T)))->~T(); --cur;}
    bool empty()const{return !cur;}
    const T& front()const{return *reinterpret_cast<const T*>(buffer);}
    T& front(){return *reinterpret_cast<T*>(buffer);}
    size_t size()const{return cur;}

    auto begin(){return  reinterpret_cast<T*>(buffer);}
    auto end(){return    reinterpret_cast<T*>(buffer) + cur;}
};

int main(){
    std::priority_queue<Res, SimpleInplaceVec<Res,3>, std::greater<Res>> rq;
    rq.push({"Rand(rget)", bench(test_rand_rget)});
    rq.push({"Rand(dget)", bench(test_rand_dget)});
    rq.push({"Rdrand", bench(test_rdrand)});
    while(!rq.empty()){auto v = rq.top(); rq.pop(); std::cout << v.text << ": " << v.count << " Ms\n";}
}
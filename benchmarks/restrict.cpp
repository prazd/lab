#include <iostream>
#include <chrono>
#include <numeric>

// GCC 15.1 -O2 

constexpr size_t sz = 20'000'000;

// vectorize
void add_s(int* __restrict a, int* __restrict b, int* __restrict c, size_t n) {
    for (int i = 0; i < n; ++i)
        a[i] = b[i] + c[i];
}

// default
void add(int* a, int* b, int* c, size_t n) {
    for (int i = 0; i < n; ++i)
        a[i] = b[i] + c[i];
}

// T1 data
int a[sz]{};
int b[sz]{};
int c[sz]{};

// T2 data
int as[sz]{};
int bs[sz]{};
int cs[sz]{};


void run_add(int* a, int* b, int* c, size_t n, void(*add_func)(int*,int*,int*,size_t), const char* t_name) {
    auto begin = std::chrono::steady_clock::now();
    add_func(a,b,c,n);
    auto end = std::chrono::steady_clock::now();
    std::cout << t_name<< " add: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms\n";
}

int main() {
    std::iota(std::begin(b), std::end(b), 0);
    std::iota(std::begin(c), std::end(c), 0);
    
    std::iota(std::begin(bs), std::end(bs), 0);
    std::iota(std::begin(cs), std::end(cs), 0);
    
    run_add(a, b, c, sz, add, "ADD");
    run_add(as, bs, cs, sz, add_s, "ADD_S ");
}
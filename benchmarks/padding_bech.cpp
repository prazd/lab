#include <iostream>

constexpr unsigned long long sz = 100'000'000;

struct S1 {
    char buffer[4];
    long x;
};

#pragma pack(push, 1)
struct S2 {
    char buffer[4];
    long x;
};
#pragma pack(pop)

void bench(auto& vec) {
    long sum = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i].x = rand() % 300;
        sum += vec[i].x;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time: " << d.count() << "ms\n";
}

int main() {
    static_assert(sizeof(S1) == 16);
    static_assert(sizeof(S2) == 12);

    std::vector<S1> s1_list(sz);
    std::vector<S2> s2_list(sz);

    std::cout << "S1:";
    bench(s1_list);
    std::cout << '\n';
    std::cout << "S2:";
    bench(s2_list);
}
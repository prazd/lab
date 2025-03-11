#include <iostream>

constexpr int count = 100'000'000;
constexpr int line_sz = 64;

template <size_t n> 
void benchmark_cache_size() {
    static constexpr auto sz = (n * 1024)/line_sz;
    static std::array<std::array<std::byte, line_sz>, sz> arr;
    static std::array<int, sz> ids;
    std::generate(ids.begin(), ids.end(), []{return rand() % sz;});
    
    auto start = std::chrono::system_clock::now();
    for (size_t i = 0; i < count; ++i)
        arr[ids[i % sz]][0] = static_cast<std::byte>(i);
    auto end = std::chrono::system_clock::now();
    
    std::cout << n << "KB: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "\n";
}

template <size_t... n>
void run_benches() {
    (benchmark_cache_size<n>(), ...);
}

int main() {
    srand(time(nullptr));
    run_benches<2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768>();
}
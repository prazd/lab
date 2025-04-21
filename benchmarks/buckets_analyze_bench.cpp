#include <iostream>
#include <chrono>
#include <unordered_map>

constexpr auto sz = 40;

std::array<int, sz> data;
std::unordered_map<int, int> m1;
std::unordered_map<int, int> m2;

void seed() {
    std::generate(std::begin(data), std::end(data), []{return rand() % 100;});
    m1.reserve(std::size(data));
    m2.reserve(std::pow(std::size(data), 2));
}

void analyze_collision(const auto& map) { 
    size_t bucket_count = map.bucket_count();
    size_t collision_buckets = 0;

    for (size_t i = 0; i < bucket_count; ++i) {
        size_t bsize = map.bucket_size(i);
        if (bsize > 1) ++collision_buckets;
    }
    std::cout << "Bucket count:" << bucket_count << '\n';
    std::cout << "Collision buckets:" << collision_buckets << '\n';
}

void bench(auto&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto d = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "\nTime:" << d.count() << '\n';
}

int main() {
    seed();

    bench([&]{for(auto el: data) m1[el] = rand() % 100;});
    analyze_collision(m1);
    
    bench([&]{for(auto el: data) m2[el] = rand() % 100;});
    analyze_collision(m2);
}
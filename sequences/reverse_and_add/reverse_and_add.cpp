#include <iostream>
#include <mutex>
#include <thread>
#include <map>

uint64_t reverse_num(uint64_t num) {
    uint64_t reverse = 0;
    uint64_t original = num;
    while(num) {
        reverse = (reverse * 10) + (num % 10);
        num /= 10;
    }
    return reverse;
}

bool is_num_palindrom(uint64_t num) {
    return reverse_num(num) == num;
}


std::vector<uint64_t> reverse_and_add_seq(uint64_t start, uint64_t deep) {
    std::vector<uint64_t> vec;
    uint64_t res = start;
    for(size_t i = 0; i < deep; ++i){
        auto reverse = reverse_num(res);
        auto sum = reverse + res;
        res = sum;
        if (is_num_palindrom(res)) {
            vec.push_back(res);
        }
    }
    return vec;
}

auto find_polindroms_in_seqs(uint64_t begin, uint64_t end, uint64_t deep) {
    std::map<uint64_t, std::vector<uint64_t>> result;
    std::vector<std::thread> pool;
    std::mutex mt;
    for(auto i = begin;i!=end;++i){
        pool.emplace_back([](uint64_t start, uint64_t deep, std::mutex& mt, std::map<uint64_t, std::vector<uint64_t>>& res_map){
            auto vec = reverse_and_add_seq(start, deep);
            std::lock_guard lg(mt);
            res_map[start] = vec;
        }, i, deep, std::ref(mt), std::ref(result));
    }
    for(auto& el: pool){
        el.join();
    }
    return result;
}

template <typename T>
void print(const T& ctr) {
    for(auto& el: ctr)
        std::cout << el << " ";
    std::cout << "\n";
}

int main() {
    auto res = find_polindroms_in_seqs(190, 200, 120);
    for(auto& [key, value]: res) {
        std::cout << "\n";
        std::cout << "Key:" << key << "\n";
        print(value);
        std::cout << "\n";
    }
}
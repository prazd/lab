#include <iostream>
#include <numeric>
#include <map>
#include <thread>

template <typename T>
std::vector<T> generate_vec(size_t n){
    std::vector<T> res(n);
    std::iota(res.begin(), res.end(), 0);
    return res;
}

size_t get_factorial(size_t n) {
    size_t res = 1;
    for(size_t i = 1; i <= n;++i)
        res*=i;
    return res;
}

void shift(auto& vec, size_t begin, size_t end) {
        auto x = vec[begin];
        for(size_t i = begin; i < end; ++i) {
            vec[i] = vec[(i+1)%end];
        }
        vec[end-1] = x;
}

void swap_last(auto& vec) {
    std::swap(vec[vec.size()-1],vec[vec.size()-2]);
}

auto fac_s_m(const std::vector<int>& elems) {
    auto local = elems;
    auto cpy = elems;
    int n = local.size();
    auto f = get_factorial(n);
    std::vector<std::vector<int>> res;

    // initial
    res.push_back(local);

    if (n == 2){
        auto next = res.back();
        swap_last(next);
        res.push_back(next);
        return res;
    }

    auto tt = local[local.size()-3];

    auto cc = n <= 3 ? 1 : (size_t)std::pow((double)(n-3), 2);

    for (auto j = 0; j < f; ++j) {
         auto nc = res.back();
         for (size_t k = 0; k < cc; ++k ){
            size_t b = 1;
            for (size_t i = 0; i < 3; ++i) {
                auto tmp2 = res.back();
                auto tmp3 = res.back();

                swap_last(tmp2);
                res.push_back(tmp2);

                shift(tmp3, tmp3.size()-3, tmp3.size());
                if (tmp3[tmp3.size()-3] == tt){
                    break;
                }
                res.push_back(tmp3);
            }{
                auto next = nc;
                shift(next, b, next.size());
                if (k + 1 != cc) {
                    res.push_back(next);
                }
                tt = next[next.size()-3];
                nc = next;
            }
         }

        auto tmp3 = cpy;
        shift(tmp3, 0, tmp3.size());
        if (tmp3.front() == local.front()){
                break;
        }
        res.push_back(tmp3);
        tt = tmp3[tmp3.size()-3];
        cpy = tmp3;
    }
    return res;
}

void test() {
    std::vector<int> next{0,2,3,4,1};
    shift(next, 1, next.size());
    for(auto el: next)
        std::cout << el << " ";
}

int main() {
    auto res = fac_s_m(std::vector<int>{0,1,2,3,4});
    std::cout << "Size: " << res.size() << "\n";
    for(auto& row: res){
        for(auto el: row)
            std::cout << el << " ";
        std::cout << "\n";
    }
}
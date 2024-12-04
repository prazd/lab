#include <iostream>
#include <numeric>
#include <map>
#include <chrono>
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

auto fac_s_m(const std::vector<int>& elems) {
    std::vector<int> local = elems;
    std::vector<int> cpy = elems;
    auto n = local.size();
    auto f = get_factorial(n);
    auto fm1 = get_factorial(n-1);

    std::vector<std::vector<int>> res;
    // initial 
    res.push_back(local);

    auto curr = local[0];
    auto m = 0;

    auto shift = [](auto& vec, size_t begin, size_t end){
        auto x = vec[begin];
        for(size_t i = begin; i < end; ++i) {
            vec[i] = vec[(i+1)%end];
        }
        vec[end-1] = x;
    };

    auto swap_last = [](auto& vec) {
        std::swap(*(vec.end()-1), *(vec.end()-2));
    };
    
    size_t i = 0;
    auto tt = local[1];

    while(1) {
    for (size_t k = 0; k < n; ++k ){
        for (size_t i = 0; i < 3; ++i) {
            std::vector<int> tmp2 = res.back();
            std::vector<int> tmp3 = res.back();

            swap_last(tmp2);
            res.push_back(tmp2);

            shift(tmp3, tmp3.size()-3, tmp3.size());
            if (tmp3[tmp3.size()-3] == tt){
                break;
            }
            res.push_back(tmp3);
        }
        auto next = res.back();
        shift(next, 1, next.size());
        res.push_back(next);
        tt = next[1];
    }    
       
        std::vector<int> tmp3 = cpy;
        shift(tmp3, 0, tmp3.size());
        if (tmp3[0] == local[0]){
                break;
        }
        res.push_back(tmp3);
        tt = tmp3[1];
        cpy = tmp3;
    }

    std::cout << "Sz:" << res.size() << std::endl;
   
   for(auto&& row: res){
        for(auto el: row){
            std::cout << el << " ";
        }
        std::cout << "\n";
    }
}

// todo: generalize
auto factorial_print(const std::vector<int>& els) {
    std::vector<int> vec = els;
    auto n = vec.size();
    auto f = get_factorial(n);
    auto fm1 = get_factorial(n-1);

    std::vector<std::vector<int>> res;

    std::cout << "Factorial(n):" << f << std::endl;

    for (size_t e = 0; e < n; ++e){
        std::vector<int> tmp = vec;
        tmp.erase(std::remove(tmp.begin(), tmp.end(), tmp[e]), tmp.end());
        for (size_t k = 0;k < fm1; ++k){
            std::vector<int> tmp2;
            tmp2.push_back(vec[e]);
            size_t m = 1;
            for (size_t i = 1; i < n; ++i){
               tmp2.push_back(tmp[((e+i)%(n-1)+m)%(n-1)]);
            }
            res.push_back(tmp2);
        }
       
        
    }

       // res.push_back(std::vector<int>{vec[e], tmp[((e)%(n-1)+m)%(n-1)], tmp[((e+1)%(n-1)+m)%(n-1)], tmp[((e+2)%(n-1)+m)%(n-1)]});
       // res.push_back(std::vector<int>{vec[e], tmp[((e)%(n-1)+m)%(n-1)], tmp[((e+2))%(n-1)+m)%(n-1)], tmp[((e+1)%(n-1)+m)%(n-1)]});
   
   std::cout << "Sz:" << res.size() << std::endl;
   
   for(auto&& row: res){
        for(auto el: row){
            std::cout << el << " ";
        }
        std::cout << "\n";
    }

    return res;
}

std::map<int, std::function<int(int, int)>> ops = {
    {0, [](int a, int b){return a + b;}},
    {1, [](int a, int b){return a * b;}},
    {2, [](int a, int b){return a - b;}},
    {3, [](int a, int b){return a / b;}},
};

std::map<int, std::string> ops_str = {
    {0, "+"},
    {1, "*"},
    {2, "-"},
    {3, "/"},
};

void calculate_print() {
    auto operation_ixs = factorial_print(generate_vec<int>(4));
    std::vector<int> nums{1,3,4,6};
    auto nums_vars = factorial_print(nums);

    // dummy
    for (auto& current_nums: nums_vars){
        for(auto& opi: operation_ixs){
            
            auto n1 = current_nums[0];
            auto n2 = current_nums[1];
            auto n3 = current_nums[2];
            auto n4 = current_nums[3];

            for (size_t i = 0; i < 4; ++i){
                std::cout << "------------" << "\n";
                auto m = ops[opi[i]](n1, n2);
                std::cout << n1 <<  ops_str[opi[i]] << n2 << "=" << m << "\n";
                auto m1 = ops[opi[(i+1)%4]](m, n3);
                std::cout << m <<  ops_str[opi[(i+1)%4]] << n3 << "=" << m1 << "\n";
                auto m2 = ops[opi[(i+2)%4]](m1, n4);
                std::cout << m1 <<  ops_str[opi[(i+2)%4]] << n4 << "=" << m2 << "\n";
                std::cout << "Res:" << m2 << "\n";
                std::cout << "------------" << "\n";
                // std::cout << "(("<< n1 << ops_str[opi[i]] << n2 <<")" << ops_str[opi[(i+1)%4]] << n3 << ")" << ops_str[opi[(i+2)%4]] << n4 << "=" << m2 << "\n";
            }
   
        }
    }
}


// 0 1 2 3 4
// 0 1 2 4 3
// 0 1 3 4 2
// 0 1 3 2 4
// 0 1 4 2 3
// 0 1 4 3 2


int main() {
    // calculate_print();
    fac_s_m(std::vector<int>{0,1,2,3,4});
}
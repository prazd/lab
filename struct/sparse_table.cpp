#include <iostream>
#include <cmath>

template <std::integral T, size_t n>
struct SparseTable {

    static consteval size_t log2(std::integral auto val){
        return std::floor(std::log2(val));
    }

    static constexpr size_t max2p(std::integral auto val){
        size_t count = 0;
        size_t res = 0;
        while(val){if (val & 1) res = count; ++count; val >>= 1;}
        return res;
    }

    T arr[log2(n) + 1][n]{};
    
    constexpr SparseTable(std::random_access_iterator auto it, std::random_access_iterator auto ite){
        for (size_t i = 0, e = log2(n) + 1; i != e; ++i){
            size_t curr_sz = 1 << i;
            for (int j = 0; j != n - curr_sz + 1; ++j){
                T min = !i ? *(it + j) : std::min(arr[i-1][j], arr[i-1][j+(curr_sz>>1)]);
                arr[i][j] = min;
            }
        }
    }
    constexpr T min(size_t begin ,size_t end) const {
        size_t k = max2p(end-begin+1);
        return std::min(arr[k][begin], arr[k][end-(1 << k) + 1]);
    }
    void print() const {
        for (auto& a: arr){
            for (auto& el: a){
                std::cout << el << ' ';
            }
            std::cout << '\n';
        }
    }
};

int main(){
    constexpr std::array<int, 8> arr{5,2,7,3,1,6,4,8};
    constexpr SparseTable<int, 8> st(arr.begin(), arr.end());
    static_assert(st.min(0,0) == 5);
    static_assert(st.min(0,1) == 2);
    static_assert(st.min(0,2) == 2);
    static_assert(st.min(0,3) == 2);
    static_assert(st.min(0,4) == 1);
    static_assert(st.min(5,7) == 4);
    static_assert(st.min(3,7) == 1);
    st.print();
}
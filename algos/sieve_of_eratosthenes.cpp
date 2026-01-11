#include <iostream>
#include <array>
#include <bitset>

template <typename T, size_t N>
consteval auto sieve_of_eratosthenes(){
    static_assert(N>2);
    std::bitset<N> is_prime_list{};
    size_t count = N-2;
    is_prime_list.set();
    is_prime_list.set(0,false);
    is_prime_list.set(1,false);
    for (int i = 2; i < N; ++i){
        if (!is_prime_list.test(i)) continue;
        for (int j = i * i; j < N; j+=i){
            if (is_prime_list.test(j)){
                --count;
                is_prime_list.set(j, false); 
            }
        }
    }
    return std::pair{count, is_prime_list};
}

template <typename T, size_t N>
consteval auto primes_list(){
    constexpr auto soe_res = sieve_of_eratosthenes<T,N>();
    std::array<T, soe_res.first> res{};
    size_t current = 0;
    for (int i = 0; i < N; ++i){
        if (soe_res.second.test(i)) res[current++] = i;
    }
    return res;
}

int main(){
    constexpr auto res = primes_list<int,10>();
    static_assert(res == std::array{2,3,5,7});
}

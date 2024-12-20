#include <iostream>

template <typename T>
constexpr size_t popcount(T value) {
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    static_assert(!std::is_same_v<T, bool>, "T must not be bool");
    
    using UnsignedT = std::make_unsigned_t<T>;
    UnsignedT uv = static_cast<UnsignedT>(value);
    size_t res;
    for(res = 0; uv; ++res)
        uv &= (uv - 1); // Hamming Weight
    return res;
}

int main(){
    static_assert(popcount(15) == __builtin_popcount(15));
}

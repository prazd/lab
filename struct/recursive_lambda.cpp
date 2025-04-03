// c++-14 -std=c++23 recursive_lambda.cpp
int main() {
    auto factorial = [](this auto&& self, int n) constexpr { // C++23 deducing this
        if (n <= 1) return n;
        return n * self(n - 1);
    }; 
    static_assert(factorial(5) == 120);
}
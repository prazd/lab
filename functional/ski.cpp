constexpr auto I = [](auto x){return x;};
constexpr auto K = [](auto x) {
    return [x](auto){return x;};
};

constexpr auto S = [](auto x){
    return [x](auto y){
        return [x,y](auto z) {
            return x(z)(y(z));
        };
    };
};

int main() {
    constexpr auto SKK = S(K)(K);
    static_assert(SKK(42) == 42);

    ///////

    constexpr auto W = [](auto f) {
        return [f](auto x) {
            return f(x)(x);
        };
    };

    constexpr auto add = [](auto x) {
        return [x](auto y) { return x + y; };
    };
    constexpr auto dup_add = W(add);
    static_assert(dup_add(5) == 10);

    ///////

    constexpr auto M = [](auto f) {
        return [f](auto x){
            return f(x)(x);
        };
    };

    constexpr auto twice = [](auto f) {
        return [f](auto x) {
            return [f, x](auto y) {
                return f(f(y)); 
            };
        };
    };

    constexpr auto inc = [](auto x) {return x + 1;};
    constexpr auto inc_twice = M(twice)(inc);
    static_assert(inc_twice(3) == 5);
}
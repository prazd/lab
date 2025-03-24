#include <iostream>
#include <cassert>
#include <charconv>
#include <cstdio>

template <std::forward_iterator It>
std::string rle_encoding(It it, It ite) {
    using ValueType = std::iter_value_t<It>;
    static_assert(std::is_same_v<ValueType, char>);
    auto dis = std::distance(it, ite);
    if (it == ite) return {};

    std::string res;
    res.reserve(std::distance(it, ite)*2);

    size_t counter = 1;
    ValueType curr = *it;
    ++it;


    for (;it!=ite;++it) {
        if (*it == curr){
            ++counter;
        } else {
            res += std::to_string(counter);
            res += curr;
            counter = 1;
            curr = *it;
        }
    }
    res += std::to_string(counter);
    res += curr;

    return res;
}

template <typename T>
concept Resizeble = requires(T t, size_t sz, typename T::value_type v) {
    { t.resize(sz, v) } -> std::same_as<void>;
};

template <Resizeble T>
T rle_decode(std::string_view s) {
    size_t reserve_sz{};
    T res;
    auto loop = [&](auto&& func){
        auto f = s.begin();
        for (auto it = s.begin() + 1, ite = s.end(); it != ite; ++it) {
            if (!std::isdigit(*it)){
                size_t sz;
                auto r = std::from_chars(f, it, sz);
                if (r.ec != std::errc{}) throw std::invalid_argument(s.data());
                func(sz, *it);
                f = std::next(it);
            }
        }
    };

    loop([&reserve_sz](size_t sz, auto){reserve_sz+=sz;});
    res.reserve(reserve_sz);

    loop([&res](size_t sz, auto el){
        // instead of push_back
        size_t old_size = res.size(); 
        res.resize(old_size + sz, el);
    });

    return res;
}


int main() {
    std::vector<char> value{'a','a','a','a','a','a','a','a','a','a','a','a','a', 'b', 'b', 'b'};
    assert(rle_encoding(value.begin(), value.end()) == "13a3b");
    assert(rle_decode<std::vector<char>>(rle_encoding(value.begin(), value.end())) == value);

    std::string svalue = "eeeeeeeeerrrrrrrrsssssssssxxxxxxx";
    assert(rle_encoding(svalue.begin(), svalue.end()) == "9e8r9s7x");
    assert(rle_decode<std::string>(rle_encoding(svalue.begin(), svalue.end())) == svalue);
}
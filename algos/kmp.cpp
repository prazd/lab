#include <iostream>
#include <vector>
#include <string_view>
#include <cassert>


std::vector<int> prefix_func(std::string_view s) {
    size_t sz = s.size();
    std::vector<int> pi(sz);
    pi[0] = 0;
    for (size_t i = 1; i < sz; ++i) {
        int j = pi[i - 1];
        while(j > 0 && s[i] != s[j]) j = pi[j-1];
        if (s[i] == s[j]) ++j;
        pi[i] = j;
    }
    return pi;
}


std::vector<int> kmp_search(std::string_view s, std::string_view sub) {
    auto pi = prefix_func(sub);
    auto s_sz = s.size();
    auto sub_sz = sub.size();
    int j = 0;
    std::vector<int> res;
    int k = 0;
    for (size_t i = 0; i < s_sz; ++i) {
        if (j > 0 && s[i] != sub[j]) {
            j = pi[j - 1];
        }
        if (s[i] == sub[j]) ++j;
        if (j == sub_sz) {
            res.push_back(i - j + 1);
            j = pi[j - 1];
        }
    }
    return res;
}

void print(const auto& vec){
    for (auto el: vec)
        std::cout << el << " ";
}

int main() {
    auto res = kmp_search("aabaaab", "aab");
    auto v = std::vector<int>{0,4};
    assert(res == v);

    res = kmp_search("abcabcabcd", "abcabcd");
    v = std::vector<int>{3};
    assert(res == v);
}
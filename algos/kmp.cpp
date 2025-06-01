#include <iostream>


// KMP example
std::vector<int> compute_prefix_function(const std::string& pattern) {
    int m = pattern.size();
    std::vector<int> pi(m, 0);
    int j = 0; 
    for (int i = 1; i < m; ++i) {
        while (j > 0 && pattern[i] != pattern[j])
            j = pi[j - 1];

        if (pattern[i] == pattern[j])
            j++;

        pi[i] = j;
    }
    return pi;
}

std::vector<int> kmp_search(const std::string& text, const std::string& pattern) {
    auto pi = compute_prefix_function(pattern);
    std::vector<int> res;
    int j = 0;
    auto text_size = text.size();
    auto pattern_size = pattern.size();
    for (int i = 0; i < text_size; ++i){
        while(j > 0 && text[i] != pattern[j])
            j = pi[j-1];
        if (text[i] == pattern[j])
            ++j;
        if (j == pattern_size){
            res.push_back(i - j + 1);
            j = pi[j - 1];
        }
    }
    return res;
}

int main() {
    auto res = kmp_search("abacab", "ab");
    for (auto el: res)
        std::cout << el << " ";
}
#include <iostream>

consteval size_t words_count(std::string_view str) {
    if (str.empty()) return 0;
    size_t res = *str.begin() == ' ' ? 0 : 1;
    for (auto it = str.begin() + 1, ite = str.end(); it !=ite;++it)
        res += (*it != ' ' && *std::prev(it) == ' ') ? 1 : 0;
    return res;
}

int main() {
    static_assert(words_count("Hello world!") == 2);
    static_assert(words_count("   Multiple   spaces    between   words   ") == 4);
}
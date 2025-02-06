#include <cassert>
#include <string>

int atoi(std::string s) {
    int res{};
    int tmp{};
    bool gz = true;
    bool update = false;
    auto imax = std::numeric_limits<int>::max();
    auto imin = std::numeric_limits<int>::min();
    auto ret_func = [&gz, &res]{return gz ? res : res * -1;};
    for (auto& el: s) {
        if (std::isalpha(el)) return ret_func();
        if (el == ' ') {if (update) return ret_func(); continue;}
        if (el == '.') return ret_func();
        if (el == '+') {if (update) return ret_func(); update = true; continue;}
        if (el == '-') {if (update) return ret_func(); update = true; gz = false; continue;};
        tmp = el - '0';
        if (!res) {
            res = tmp;
            update = true;
            continue;
        }
        if (imax / 10 < res || ((imax / 10 == res && tmp > 7)))
            return gz ? imax : imin;
        
        res *= 10;
        res += tmp;
        update = true;
    }
    return ret_func();
}

int main() {
    assert(atoi("42") == 42);
    assert(atoi("   -042") == -42);
    assert(atoi("1337c0d3") == 1337);
    assert(atoi("0-1") == 0);
    assert(atoi("words and 987") == 0);
}
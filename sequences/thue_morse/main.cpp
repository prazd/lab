#include <iostream>

std::string thue_morse_next(const std::string& val) {
    std::string res;
    res.reserve(val.size());
    for (char s : val) {
        res += (s == '0') ? '1' : '0';
    }
    return res;
}

void print_seq(size_t n) {
    std::string start = "0";
    for (size_t i = 0; i < n; ++i) {
        std::cout << start << "\n";
        start += thue_morse_next(start);
    }
}

int main() {
    print_seq(10);
}

#include <iostream>
#include <algorithm>
#include <string>
#include <cassert>

std::string addBinary(std::string as, std::string bs) {
    std::string res;
    size_t sz = std::max(as.size(), bs.size());
    res.reserve(sz + 1);
    bool carry = false;
    for (size_t i = 0; i < sz; ++i) {
        bool a = i < as.size() && as[as.size() - 1 - i] == '1';
        bool b = i < bs.size() && bs[bs.size() - 1 - i] == '1';

        bool next = a ^ b ^ carry;
        carry = (a && b) || (carry && (a || b));

        res.push_back(next ? '1' : '0');
    }
    if (carry) {
        res.push_back('1');
    }
    std::reverse(res.begin(), res.end());
    return res;
}

int main() {
    assert(addBinary("1010", "1011") == "10101");
    assert(addBinary("11", "1") == "100");
    assert(addBinary("11101010", "111001010") == "1010110100");
    assert(addBinary("1111011", "101011") == "10100110");
}
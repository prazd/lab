#include <iostream>
#include <bit>
#include <cassert>
#include <limits>
#include <sstream>
#include <algorithm>
#include <string>
#include <iomanip>

class Uint256 {
    size_t buffer[4]{};
public:
    Uint256() = default;
    Uint256(const Uint256& rhs) {
        std::copy(std::begin(rhs.buffer), std::end(rhs.buffer), buffer);
    }
    Uint256(size_t value) {
        buffer[0] = value;
        std::cout << std::bitset<64>(buffer[0]).to_string() << std::endl;
    }
    Uint256& operator+=(const Uint256& rhs) {
       bool carry = false;
       for (size_t i = 0; i < 4; ++i) {
            size_t sum = buffer[i] + rhs.buffer[i] + carry;
            carry = (sum < buffer[i]) || (carry && (sum == buffer[i]));
            buffer[i] = sum;
        }
        return *this;
    }
    std::string to_string() {
        std::string result;
        std::for_each(std::begin(buffer), std::end(buffer), [&result](const auto& el) {
            std::cout << std::bitset<sizeof(size_t)*CHAR_BIT>(el).to_string() << std::endl;
            result += std::bitset<sizeof(size_t)*CHAR_BIT>(el).to_string();
        });
        return result;
    }
};


int main() {
    Uint256 x(std::numeric_limits<size_t>::max());
    Uint256 y(std::numeric_limits<size_t>::max());
    x += y;
    std::cout << x.to_string() << std::endl;
    Uint256 z = x;
    std::cout << z.to_string() << std::endl;
    z += x;
    std::cout << z.to_string() << std::endl;
}
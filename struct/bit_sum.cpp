#include <iostream>
#include <bit>

void print_bin_sum(int x, int y) {
    constexpr auto sz = sizeof(x)*CHAR_BIT;
    std::bitset<sz> x_b(x);
    std::bitset<sz> y_b(y);
    std::bitset<sz> result(0);

    bool carry = false;
    for(unsigned long i = 0;i < sz; ++i) {
      bool x_b_v = x_b[i];
      bool y_b_v = y_b[i];

      result[i] = x_b_v ^ y_b_v ^ carry;
      carry = (x_b_v && y_b_v) || (x_b_v && carry) || (y_b_v && carry);
    }
    std::cout << "X:" << x_b << std::endl;
    std::cout << "Y:" << y_b << std::endl;
    std::cout << "R:" << result << std::endl;
    std::cout << result.to_ulong() << std::endl;
}


int main() {
    print_bin_sum(340,20);
}
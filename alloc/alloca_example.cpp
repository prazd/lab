#include <iostream>
#include <cstdlib>
#include <numeric>
#include <iterator>
#include <algorithm>
int main() {
    int x = 10;
    auto buffer = (char*)alloca(x);
    std::iota(buffer, buffer+10,0);
    std::copy(buffer, buffer + 10, std::ostream_iterator<int>(std::cout, " "));
}

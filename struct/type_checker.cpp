#include <iostream>

template <typename T>
void Check(T) = delete;

int main() {
    std::vector<int> vec = {1,2,3,4};
    std::vector<int>::size_type x;
    Check(x);
}
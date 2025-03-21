#include <iostream>

template <typename T>
void print_value_type(const T& t) {
    using ValueType = std::indirectly_readable_traits<T>::value_type;
    std::cout << "Value:" << typeid(ValueType).name() << '\n';
}

int main() {
    std::vector<int> x = {1,2,3,4};
    print_value_type(x);

    double y[3]{1,2,3};
    print_value_type(y);
}
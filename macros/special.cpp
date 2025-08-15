#include <iostream>

int main() {
    std::cout << "File:" << __FILE__ << '\n';
    std::cout << "Line:" << __LINE__ << '\n';
    std::cout << "Date:" << __DATE__ << '\n';
    std::cout << "Time:" << __TIME__ << '\n';
    std::cout << "Standart:" << __cplusplus << '\n';
}
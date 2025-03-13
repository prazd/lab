#include <iostream>
#include <string>

bool verify_password(std::string_view pass);

int main() {
    std::string val;
    std::cin >> val;
    if (verify_password(val))
        std::cout << "Correct!" << '\n';
    else 
        std::cout << "Wrong" << '\n';
}
#include <iostream>
#include <cctype>
#include <string>

std::string encode(const std::string_view& str, char shift) {
    std::string result;
    for (char el: str)
        if (std::isalpha(el)) {
            char base = std::islower(el) ? 'a' : 'A';
            result += (el - base + shift + 26) % 26 + base;
        } else {
            result += el;
        }
    return result;
}

std::string decode(const std::string_view& str, char shift) {
    return encode(str, -shift);
}

int main() {
    auto text = "TeXt TEXt";
    std::cout << "Text:" << text << "\n";
    auto encoded = encode(text, 10);
    std::cout << "Encoded:" << encoded << "\n";
    auto result = decode(encoded, 10);
    std::cout << "Result:" << result << "\n";
}
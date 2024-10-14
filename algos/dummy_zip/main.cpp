#include <iostream>
#include <fstream>
#include <bit>

unsigned char get_bin_data(const std::string& d) {
    unsigned char result = 0;
    for(auto el: d) {
        if (el == 'a') {
            result<<=1;
            result|=1;
        } else if (el == 'b') {
            result<<= 1;
        }
    }
    return result;
}

std::string unzip_data(const std::string& d) {
    std::string res;
    for(auto el: d) {
        if (el == '0') {
            res+="b";
        } else if (el == '1') {
            res+="a";
        }
    }
    return res;
}

void dzip(const char* val) {
    std::fstream file("test.txt", std::ios::out);
    if (!file) {
        std::abort();
    }
    file << get_bin_data(val);
    file.close();
}

std::string unzip() {
    const int bufferSize = 1; // Размер буфера
    char buffer[bufferSize];

    std::ifstream file("test.txt", std::ios::binary);

    if (!file) {
        std::abort();
    }

    file.read(buffer, bufferSize);

    if (!file) {
        std::abort();
    }
    file.close();
    return unzip_data(std::bitset<8>(buffer[0]).to_string());
}

int main() {
    auto data = "aaabbaab";
    dzip(data);
    if (unzip() == data) {
        std::cout << "OK!";
    }
}
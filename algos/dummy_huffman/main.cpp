#include <iostream>
#include <fstream>
#include <bit>
#include <algorithm>
#include <map>

struct Node {
    Node* left;
    Node* right;
    char  value;
    bool weight;
};

struct Tree {
    Node* root;
};

std::map<char, size_t> get_syms_count_map(const std::string& val) {
    std::map<char, size_t> mres;
    for (const auto& ch : val) ++mres[ch];
    std::for_each(mres.begin(), mres.end(), [](const auto& el){std::cout << el.first << " " << el.second << " ";});
    return mres;
}

constexpr const char* filename = "test.bin";

unsigned char to_bin(const std::string& d) {
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

std::string from_bin(const std::string& d) {
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
    std::fstream file(filename, std::ios::out);
    if (!file) {
        std::abort();
    }
    file << to_bin(val);
    file.close();
}

std::string dunzip() {
    const int bufferSize = 1;
    char buffer[bufferSize];

    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::abort();
    }

    file.read(buffer, bufferSize);

    if (!file) {
        std::abort();
    }
    file.close();
    return from_bin(std::bitset<8>(buffer[0]).to_string());
}

int main() {
    get_syms_count_map("abcaa");
    // std::cout << sizeof(Node) << std::endl;
    // auto data = "aaabbaab";
    // dzip(data);
    // if (dunzip() == data) {
    //     std::cout << "OK!";
    // }
}
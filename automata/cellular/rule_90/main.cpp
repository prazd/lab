#include <iostream>
#include <map>

// Rule90 : 01011010
//  1       1      1   ->    0
//  1       1      0   ->    1
//  1       0      1   ->    0
//  1       0      0   ->    1
//  0       1      1   ->    1
//  0       1      0   ->    0
//  0       0      1   ->    1
//  0       0      0   ->    0

std::unordered_map<std::string, char> r90 = {
    {"111", '0'},
    {"110", '1'},
    {"101", '0'},
    {"100", '1'},
    {"011", '1'},
    {"010", '0'},
    {"001", '1'},
    {"000", '0'},
};

void drawR90(size_t field, size_t itercount) {
    std::string gen;

    // init
    for(size_t i = 0; i < field; ++i) {
        if (i == field/2){
            gen += "1";
        } else {
            gen += "0";
        }
    }
    
    for(int i = 0; i < itercount; ++i) {
        for(auto& el: gen) {
            std::cout << (el == '1' ? "1" : ".");
        }
        std::cout << "\n";
        std::string tmp;
        for(int j = 0; j < field; ++j) {
            std::string key;
            key += gen[(j + gen.size() - 1) % gen.size()]; // left n
            key += gen[j];
            key += gen[(j + gen.size() + 1) % gen.size()]; // right n

            tmp += r90[key];
        }
        gen = std::move(tmp);
    }
}

int main() {
    drawR90(200, 300);
}
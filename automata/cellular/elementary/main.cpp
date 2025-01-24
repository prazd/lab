#include <iostream>
#include <unordered_map>
#include <bitset>
#include <algorithm>

// Rule 90  : 01011010
// Rule 30  : 00011110
// Rule 54  : 00111010
// Rule 110 : 00111010
std::unordered_map<std::string, char> mgen(unsigned char rule) {
    auto binval = std::bitset<8>(rule).to_string();
    return {
        {"111", binval[0]},
        {"110", binval[1]},
        {"101", binval[2]},
        {"100", binval[3]},
        {"011", binval[4]},
        {"010", binval[5]},
        {"001", binval[6]},
        {"000", binval[7]},
    };
}

template <typename CellMap>
void draw(CellMap&& cm, size_t field, size_t itercount, std::string onesym = "1", std::string zerosym = ".") {
    std::string gen(field, '0');
    std::string tmp(field, '0');
    gen[field/2] = '1';
    for(int i = 0; i < itercount; ++i) {
        for(auto& el: gen) {
            std::cout << (el == '1' ? onesym : zerosym);
        }
        std::cout << "\n";
        for(int j = 0; j < field; ++j) {
            std::string key;
            key += gen[(j + gen.size() - 1) % gen.size()]; // left n
            key += gen[j];
            key += gen[(j + gen.size() + 1) % gen.size()]; // right n
            tmp[j] = cm[key];
        }
	    std::swap(tmp, gen);
    }
}

int main() {
    draw(mgen(90), 250, 300, "0");
}
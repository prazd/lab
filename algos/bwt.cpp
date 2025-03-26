#include <iostream>
#include <cassert>

std::string bwt_encoding(std::string_view d) {
    std::string res;
    res.reserve(d.size());
    std::vector<std::string> vec(d.size());
    for (auto& el: vec) {el = d;};
    for (size_t i = 0; i < d.size(); ++i) {
        vec[i] = vec[i].substr(d.size()-i) + vec[i].substr(0,d.size()-i);
    }
    std::sort(vec.begin(), vec.end());
    for (auto& el: vec) {
        res.push_back(el[d.size()-1]);
    }
    return res;
}

std::string bwt_decoding(const std::string& d) {
    std::string tmp = d;
    std::vector<std::string> vec(d.size());
    for (size_t i = 0; i < d.size(); ++i) {
        for (size_t j = 0; j < d.size(); ++j) {
            vec[j] = d[j] + vec[j];
        }
        std::sort(vec.begin(), vec.end());
    }
    for (auto& el: vec){
        if ((el[d.size()-1]) == '$')
            return el;
    }
    return "";
}

int main() {
    assert(bwt_decoding(bwt_encoding("banana$")) == "banana$");
    assert(bwt_decoding(bwt_encoding("apple$")) == "apple$");
    assert(bwt_decoding(bwt_encoding("asdasdsdsdiwueiwueiwueiueiwqeweweeeeeeewwwwwwqqqq$")) == "asdasdsdsdiwueiwueiwueiueiwqeweweeeeeeewwwwwwqqqq$");
}
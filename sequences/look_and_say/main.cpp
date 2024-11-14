#include <iostream>

std::string conway_seq_next(std::string value) {
    std::string res;
    res.reserve(value.size() * 2);
    auto current = *value.begin();
    size_t count = 0;
    for(auto it = value.begin(), ite = value.end();it!=ite;++it) {
        if (*it == current) {
            ++count;
        } else {
            res += std::to_string(count);
            res += current;
            count = 1;
            current = *it;
        }
    }
    res += std::to_string(count);
    res += current;
    return res;
}

void conway_seq_deep(const std::string& init, size_t n) {
    std::string val = init;
    for (size_t i = 0;i<n;++i) {
        std::cout << val << "\n";
        val = conway_seq_next(val);
    }
}


// 1       
// 11      
// 21      
// 1211
// 111221
// 312211
// ...
int main() {
    conway_seq_deep("1", 30);
}
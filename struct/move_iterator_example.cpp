#include <iostream>
#include <cassert>
int main() {
    std::vector<std::string> vec{"abc", "def", "ghi"};
    std::vector<std::string> dest; 
    dest.assign(std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()));
    assert(vec.size() == 3);
    assert(dest.size() == 3);
    assert(dest[0] == "abc" && dest[1] == "def" && dest[2] == "ghi");
    assert(vec[0].empty() && vec[1].empty() && vec[2].empty());
}
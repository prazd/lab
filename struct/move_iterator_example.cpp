#include <iostream>
#include <cassert>
int main() {
    std::vector<std::string> vec{"abc", "def", "ghi"};
    std::vector<std::string> dest; 
    dest.assign(std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()));
    assert(vec.size() == 3 && dest.size() == 3);
    assert(dest[0] == "abc" && dest[1] == "def" && dest[2] == "ghi");
    assert(!std::any_of(vec.begin(), vec.end(), [](auto& el){return !el.empty();}));
}
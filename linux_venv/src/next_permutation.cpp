#include <iostream>


int main() {
    std::vector<int> x {1,2,3,4,5,6};
    for(auto el: x)
        std::cout << el << " ";
    std::cout << "\n";
    size_t count = 1;
    while(std::next_permutation(x.begin(), x.end())){
        for(auto el: x)
            std::cout << el << " ";
        std::cout << "\n";
        ++count;
    }
}
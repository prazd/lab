#include <iostream>
#include <algorithm>

template <typename T>
void print(T& t){
    for(auto el: t)
        std::cout << el << " ";
    std::cout << "\n";
}

template <typename Ctr>
bool nextPermutation(Ctr& nums) {
        if (nums.size() <= 1) {
            return false;
        }
        auto f = nums.rend();
        for(auto it = nums.rbegin() + 1, ite = nums.rend(); it != ite; ++it){
        if (*it < *(std::prev(it))){
                f = it;
                break;
            }
        } 
        if (f == nums.rend()) {
            std::sort(nums.begin(), nums.end());
            return false;
        }
        auto min = *f.base();
        auto gt_it = f.base();
        for(auto it = f.base(), ite = nums.end(); it != ite; ++it) {    
            if (*it > *f) {
                if (min < *f || (min > *f && *it <= min)) {
                    min = *it;
                    gt_it = it; 
                }
            }
        }
        std::swap(*f, *gt_it);
        std::sort(f.base(), nums.end());
        return true;
}


int main() {
    std::array<int,5> x{1,2,3,4,5};
    size_t count = 1;
    while(nextPermutation(x)){
        print(x);
        ++count;
    }
    std::cout << "Count:" << count << std::endl;
}
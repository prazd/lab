#include <iostream>
#include <optional>
#include <numeric>
#include <cassert>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iterator>

template <std::random_access_iterator Iter, typename U>
auto rec_bin_search(Iter begin, Iter end, U val) -> std::optional<Iter>{
    auto current = std::next(begin, std::distance(begin, end) / 2);
    if (*current == val) {
        return current;
    }
    if (begin == end - 1 || begin == end) {
        return {};
    }
    if (*current > val) {
        return rec_bin_search(begin, current, val);
    } else {
        return rec_bin_search(current, end, val);
    }
}


int main() {
    int arr[5]{1,2,3,4,5};
    auto res = rec_bin_search(std::begin(arr), std::end(arr), 3);  
    for (auto el: arr) {
        if (!rec_bin_search(std::begin(arr), std::end(arr), el)) {
            std::abort();
        }
    }
    
    if (rec_bin_search(std::begin(arr), std::end(arr), -10)) {
        std::abort();
    }

    if (rec_bin_search(std::begin(arr), std::end(arr), 10)) {
        std::abort();
    }
    std::vector<int> vec{10,20,30,40,50,60,70,80};    
    for(auto el: vec){
        if (!rec_bin_search(vec.begin(), vec.end(), el)) {
            std::abort();
        }
    }
}
#include <iostream>
#include <optional>
#include <numeric>
#include <cassert>
#include <thread>
#include <chrono>

template <typename T, typename U>
std::optional<bool> rec_bin_search(T begin, T end, U val) {
    auto current = std::midpoint(begin, end);
    if (*current == val) {
        return std::optional<bool>(true);
    }
    if (begin == end - 1) {
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
    std::for_each(std::begin(arr), std::end(arr),[&arr](const auto& el) {
        if (!rec_bin_search(std::begin(arr), std::end(arr), el)) {
            std::abort();
        }
    });

    if (rec_bin_search(std::begin(arr), std::end(arr), -10)) {
        std::abort();
    }

    if (rec_bin_search(std::begin(arr), std::end(arr), 10)) {
        std::abort();
    }

    // todo: fix vec midpoint
    
    // std::vector<int> vec{10,20,30,40,50,60,70,80};
    // auto mid = std::midpoint(std::begin(vec), std::end(vec));
    
    // std::for_each(vec.begin(), vec.end(),[&vec](const auto& el) {
    //     if (!rec_bin_search(vec.begin(), vec.end(), el)) {
    //         std::abort();
    //     }
    // });
    
}
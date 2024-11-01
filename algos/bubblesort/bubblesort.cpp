#include <iostream>
#include <cassert>
#include <algorithm>
#include <array>

struct DefaultPredicat {
    bool operator()(const auto& a, const auto& b) {
        return a > b;
    }
};

template <std::random_access_iterator T, typename Predicat = DefaultPredicat>
void bubble_sort(T begin, T end, Predicat predicat = DefaultPredicat{}) {
    for (auto i = begin; i != end; ++i) {
        bool swapped = false;
        for (auto j = begin; j != end - (i-begin)-1; ++j) {
            if (predicat(*j, *(j+1))) {
                std::swap(*j, *(j+1));
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
}


int main() {
    int x[4]{40,10,20,30};
    bubble_sort(x, x+4);
    std::array<int, 4> arr{10,20,30,40};
    assert(std::equal(arr.begin(), arr.end(), x));

    int x1[5]{30,10,4,5,11};
    bubble_sort(x1, x1+5, [](const auto a, const auto b){return a < b;});
    std::array<int, 5> arr2{30,11,10,5,4};
    assert(std::equal(arr2.begin(), arr2.end(), x1));

    int x2[5]{50,40,30,20,10};
    bubble_sort(x2, x2+5);
    std::array<int, 5> arr3{10,20,30,40,50};
    assert(std::equal(arr3.begin(), arr3.end(), x2));
}
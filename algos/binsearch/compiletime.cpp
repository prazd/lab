#include <iostream>
#include <optional>
#include <numeric>
#include <cassert>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <array>

template <typename T>
constexpr int rec_bin_search(const T& arr, int value, int begin = 0, int end = (sizeof(arr)/sizeof(arr[0]))) {
    if (begin >= end) {
        return -1;
    }
    const auto mid = begin + (end-begin)/2;
    if (arr[mid] == value){
        return mid;
    } else if(arr[mid] > value) {
        return rec_bin_search(arr, value, begin, mid);
    } else {
        return rec_bin_search(arr, value, mid+1, end);
    }
}

int main() {
    constexpr int arr[5]{1,2,3,4,5};

    constexpr auto res = rec_bin_search(arr, 1);
    static_assert(res == 0);

    constexpr auto res1 = rec_bin_search(arr, 2);
    static_assert(res1 == 1);

    constexpr auto res2 = rec_bin_search(arr, 3);
    static_assert(res2 == 2);

    constexpr auto res3 = rec_bin_search(arr, 4);
    static_assert(res3 == 3);

    constexpr auto res4 = rec_bin_search(arr, 5);
    static_assert(res4 == 4);

    constexpr std::array<int, 3> arr2{10,20,30};
    static_assert(sizeof(arr2) == 12);

    constexpr auto res_arr2 = rec_bin_search(arr2, 20);
    static_assert(res_arr2 == 1);

    constexpr auto res_arr2_2 = rec_bin_search(arr2, 10);
    static_assert(res_arr2_2 == 0);

    constexpr auto res_arr2_3 = rec_bin_search(arr2, 30);
    static_assert(res_arr2_3 == 2);
}
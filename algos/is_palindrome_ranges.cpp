#include <iostream>
#include <cassert>
#include <ranges>
#include <vector>
#include <algorithm>


template <typename Seq>
bool is_palindrome(const Seq& seq) {
    auto forward = seq | std::views::take(std::size(seq)/2);
    auto backward = seq | std::views::reverse | std::views::take(std::size(seq)/2);
    return std::ranges::equal(forward, backward);
}

int main() {
    std::vector<int> val{1,2,3,2,1};
    assert(is_palindrome(val));

    int x[3]{1,2,1};
    assert(is_palindrome(x));

    int x2[2]{1,2};
    assert(!is_palindrome(x2));
}
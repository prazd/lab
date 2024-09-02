#include <iostream>

// В максимально обобщенном коде предпочтительно использовать версии функций 
// begin, end, rbegiп и прочих, не являющиеся членами.
template <typename C>
void print(const C& container) {
    auto begin = std::begin(container);
    auto end = std::end(container);
    while(begin != end) {
        std::cout << *begin;
        ++begin;
    }
    std::cout << "\n";
}

int main() {
    std::vector<int> vec{1,2,3,4};
    print(vec);

    int arr[3]{10,20,30};
    print(arr);
}
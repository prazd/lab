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

// C++14
template <typename C, typename V>
void find_and_insert(C& container, const V& target_val, const V& insert_val){
    using std::cbegin;
    using std::cend;

    auto it = std::find(cbegin(container), cend(container), target_val);
    container.insert(it, insert_val);
}

int main() {
    std::vector<int> vec{1,2,3,4};
    print(vec);

    int arr[3]{10,20,30};
    print(arr);

    find_and_insert(vec, 3, 2);
    print(vec);
}
#include <iostream>
#include <list>
#include <future>

// 4.12 (Anthony Williams)
template <typename T>
std::list<T> sequantial_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    const T& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(), [pivot](auto& t){return t < pivot;});
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    auto new_lower(
        sequantial_quick_sort(std::move(lower_part))
    );
    auto new_higher(
        sequantial_quick_sort(std::move(input))
    );
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower);
    return result;
}

template <typename T>
std::list<T> sequantial_quick_sort_future(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    const T& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(), [pivot](auto& t){return t < pivot;});
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    std::future<std::list<T>> new_lower(
        std::async(&sequantial_quick_sort_future<T>, std::move(lower_part))); // ...

    auto new_higher(
        sequantial_quick_sort_future(std::move(input))
    );

    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

template <typename T>
void print(const T& ctr) {
    for(auto el: ctr)
        std::cout << el << " ";
    std::cout << "\n";
}

int main() {
    std::list<int> l(100);
    std::generate(l.begin(), l.end(), []{return rand()%100;}); // dummy random for test
    l = sequantial_quick_sort_future(std::move(l));
    print(l);
}
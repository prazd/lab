#include <iostream>
#include <vector>
#include <cassert>
#include <array>
#include <type_traits>

template <std::random_access_iterator It, std::random_access_iterator It2>
class Zip {
    It first;
    It2 second;
    size_t sz;

    class ZipIter {
        It first;
        It2 second;
    public:
        using it_value = std::remove_reference<decltype(*first)>::type;
        using it2_value = std::remove_reference<decltype(*second)>::type;

        using value_type = std::tuple<it_value, it2_value>;
        using reference = std::tuple<it_value&, it2_value&>;
        using pointer = std::tuple<it_value*, it2_value*>;
        using difference_type = std::tuple<std::ptrdiff_t, std::ptrdiff_t>;
        using iterator_category = std::forward_iterator_tag;

        ZipIter(It first, It2 second): first(first), second(second){}
        ZipIter& operator++() {
            ++first; ++second;
            return *this;
        }
        ZipIter operator++(int) {
            auto tmp = ZipIter{first, second};
            this->operator++();
            return tmp;
        }
        ZipIter& operator--() {
            --first; --second;
            return *this;
        }
        ZipIter operator--(int) {
            auto tmp = ZipIter{first, second};
            this->operator--();
            return tmp;
        }
        reference operator*() {
            return reference(*first, *second);
        }
        bool operator!=(const ZipIter& rhs) {
            return rhs.first != first || rhs.second != second;
        }
    };
public:
    Zip(It begin_first, It end_first,  It2 begin_second, It2 end_second): first(begin_first), second(begin_second), sz(std::min(std::distance(begin_first, end_first),std::distance(begin_second, end_second))){}
    auto begin() {
        return ZipIter(first, second);
    }
    auto end() {
        return ZipIter(first + sz, second + sz);
    }
};

int main() {
    std::vector<int> x{1,3,5};
    std::array<double, 4> y{2.1,4.2,6.3,8.4};
    Zip z(x.begin(), x.end(), y.begin(), y.end());
    for (const auto& el: z)
        std::cout << std::get<0>(el) << " " << std::get<1>(el) << '\n';

    
    std::get<0>((*z.begin())) = 10;
    assert(x[0] == 10);
    std::get<1>((*z.begin())) = 3.4;
    assert(y[0] == 3.4);
}
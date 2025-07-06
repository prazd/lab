#include <iostream>
#include <vector>
#include <type_traits>
#include <cassert>

template <std::random_access_iterator CtrIter>
struct ZigZag{
    CtrIter it;
    CtrIter ite;

    ZigZag(CtrIter it, CtrIter ite): it(it), ite(ite){}

    struct Iter {
        size_t sz{};
        size_t count{};
        CtrIter it{};
        CtrIter ite{};
        CtrIter current{};
        
        using value_type = typename std::iterator_traits<CtrIter>::value_type;
        using difference_type = typename std::iterator_traits<CtrIter>::difference_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::forward_iterator_tag;

        Iter(CtrIter it, CtrIter ite): it(it), ite(ite-1), current(it), sz(ite - it){}
        Iter(size_t count): count(count){}
        Iter operator++(int) {
            auto tmp = *this;
            this->operator++();
            return tmp;
        }
        void operator++(){
            if (count % 2 == 0) {++it; current = ite;}
            else{--ite; current = it;}
            ++count;
        }
        bool operator==(const Iter& rhs) const {
            return count == rhs.count;
        }
        auto& operator*() const {
            return *current;
        }
    };

    auto begin() {
        return Iter{it, ite};
    }
    auto end() {
        return Iter{ite - it};;
    }
};

int main() {
    std::vector<int> vec{1,2,3,4};
    auto z = ZigZag{vec.begin(), vec.end()};
    auto it = z.begin();
    assert(*(it++) == 1);
    assert(*(it++) == 4);
    assert(*(it++) == 2);
    assert(*(it++) == 3);
}
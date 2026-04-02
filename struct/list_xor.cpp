#include <iostream>
#include <cassert>
#include <array>
#include <algorithm>

template <typename T>
struct NodeXOR {
    T value{};
    NodeXOR* npx{};
    NodeXOR(){}
    NodeXOR(T&& value): value(std::forward<T>(value)){}
};

template <typename T>
class ListXOR {
    using NodePtr = NodeXOR<T>*;
    NodePtr first{};
    NodePtr last{};
    static NodePtr xaddress(NodePtr lhs, NodePtr rhs) {
        return (NodePtr)(((std::uintptr_t)lhs) ^ ((std::uintptr_t)rhs));  
    }

    void push(NodePtr addr, T&& value){
        auto el = new NodeXOR<T>(std::forward<T>(value));
        auto next = addr->npx;
        auto next_val = xaddress(xaddress(next->npx, addr), el);
        next->npx = next_val;
        el->npx = xaddress(addr, next);
        addr->npx = el;
    }

    class ListXORIter {
    protected:        
        mutable NodePtr prev;
        mutable NodePtr cur;
        mutable NodePtr next;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        ListXORIter(NodePtr prev, NodePtr cur): prev(prev), cur(cur), next(xaddress(cur->npx, prev)){}
        ListXORIter(NodePtr prev, NodePtr cur, NodePtr next): prev(prev), cur(cur), next(next){}

        bool operator!=(const ListXORIter& rhs) {
            return rhs.prev != prev || rhs.cur != cur;
        }

        bool operator==(const ListXORIter& rhs) {
            return !(this->operator!=(rhs));
        }

        const ListXORIter& operator++() const {
            auto next_address = xaddress(cur, next->npx);
            prev = cur;
            cur = next;
            next = next_address;
            return *this;
        }

        const ListXORIter& operator--() const {
            auto next_address = xaddress(cur, prev->npx);
            next = cur;
            cur = prev;
            prev = next_address;
            return *this;
        }
    };

    class ListXORIterForward: public ListXORIter {
    public:
        ListXORIterForward(NodePtr prev, NodePtr cur):  ListXORIter(prev, cur){}
        ListXORIterForward(NodePtr prev, NodePtr cur, NodePtr next): ListXORIter(prev, cur, next){}
        ListXORIterForward& operator++() {
            ListXORIter::operator++();
            return *this;
        }
        ListXORIterForward& operator--() {
            ListXORIter::operator--();
            return *this;
        }
        ListXORIter::reference operator*() const {
            return this->cur->value;
        }
    };


    class ListXORReverseIter: public ListXORIter {
    public:        
        ListXORReverseIter(NodePtr prev, NodePtr cur):  ListXORIter(prev, cur){}
        ListXORReverseIter(NodePtr prev, NodePtr cur, NodePtr next): ListXORIter(prev, cur, next){}
        ListXORReverseIter& operator++() {
            ListXORIter::operator--();
            return *this;
        }
        ListXORReverseIter& operator--() {
            ListXORIter::operator++();
            return *this;
        }
        T& operator*() const {
            return this->cur->value;
        }
    };

    class ListXORIterForwardConst: public ListXORIter {
    public:
        using reference = const T&;
        using value_type = T;
        using pointer = const T*;
        ListXORIterForwardConst(NodePtr prev, NodePtr cur):  ListXORIter(prev, cur){}
        ListXORIterForwardConst(NodePtr prev, NodePtr cur, NodePtr next): ListXORIter(prev, cur, next){}
        reference operator*() const {
            return this->cur->value;
        }
        const ListXORIterForwardConst& operator++() const {
            ListXORIter::operator++();
            return *this;
        }
        const ListXORIterForwardConst& operator--() const {
            ListXORIter::operator--();
            return *this;
        }
    };

    class ListXORReverseIterConst: public ListXORIter {
    public:
        using reference = const T&;
        using value_type = T;
        using pointer = const T*;
        ListXORReverseIterConst(NodePtr prev, NodePtr cur):  ListXORIter(prev, cur){}
        ListXORReverseIterConst(NodePtr prev, NodePtr cur, NodePtr next): ListXORIter(prev, cur, next){}
        reference operator*() const {
            return this->cur->value;
        }
        const ListXORReverseIterConst& operator--() const {
            ListXORIter::operator++();
            return *this;
        }
        const ListXORReverseIterConst& operator++() const {
            ListXORIter::operator--();
            return *this;
        }
    };

public:    
    ListXOR(): first(new NodeXOR<T>{}), last(new NodeXOR<T>{}){
        first->npx = last;
        last->npx = first;
    }

    void push_front(T&& value){
        push(first, std::forward<T>(value));
    }

    void push_back(T&& value){
        push(last, std::forward<T>(value));
    }

    auto begin() {
        return ListXORIterForward{first, first->npx};
    }

    auto end() {
        return ListXORIterForward{last->npx, last};
    }

    auto rbegin() {
        return ListXORReverseIter{xaddress(last, last->npx->npx), last->npx};
    }

    auto rend() {
        return ListXORReverseIter{nullptr, first, first->npx};
    }

    const auto begin() const {
        return ListXORIterForwardConst{first, first->npx};
    }

    const auto end() const {
        return ListXORIterForwardConst{last->npx, last};
    }

    const auto rbegin() const {
        return ListXORReverseIterConst{xaddress(last, last->npx->npx), last->npx};
    }

    const auto rend() const {
        return ListXORReverseIterConst{nullptr, first, first->npx};
    }

    ~ListXOR(){
        auto next = first->npx;
        auto current = first;
        while(current){
            delete current;
            auto cur = next;
            if (next) {
                auto next_npx = xaddress(next->npx, current);
                next = next_npx;        
            } 
            current = cur;
        }
    }
};


void print(const ListXOR<int>& list) {
    for (auto el: list) std::cout << el << " ";
    std::cout << '\n';
}

void print_r(const ListXOR<int>& list) {
    for (auto it = list.rbegin(), ite = list.rend(); it != ite; ++it) std::cout << *it << " ";
    std::cout << '\n';
}

int main(){
    ListXOR<int> list;
    list.push_back(10);
    list.push_back(30);
    list.push_back(40);
    list.push_front(112);

    auto expect = std::array<int, 4>{112,10,30,40};
    auto res = std::array<int, 4>{};
    std::copy(list.begin(), list.end(), res.begin());
    assert(std::equal(expect.begin(), expect.end(), res.begin(), res.end()));

    expect = std::array<int, 4>{40,30,10,112};
    res = std::array<int, 4>{};
    std::copy(list.rbegin(), list.rend(), res.begin());
    assert(std::equal(expect.begin(), expect.end(), res.begin(), res.end()));

    print(list);
    print_r(list);
}
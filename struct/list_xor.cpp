#include <iostream>

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
        return (NodePtr)(((unsigned long)lhs) ^ ((unsigned long)rhs));
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
        NodePtr prev;
        NodePtr cur;
        NodePtr next;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;
        ListXORIter(NodePtr prev, NodePtr cur): prev(prev), cur(cur), next(xaddress(cur->npx, prev)){}
        bool operator!=(const ListXORIter& rhs) {
            return rhs.prev != prev || rhs.cur != cur;
        }
        bool operator==(const ListXORIter& rhs) {
            return rhs.prev == prev && rhs.cur == cur;
        }

        ListXORIter& operator++() {
            auto next_address = xaddress(cur, next->npx);
            prev = cur;
            cur = next;
            next = next_address;
            return *this;
        }

      ListXORIter& operator--() {
            auto next_address = xaddress(cur, prev->npx);
            next = cur;
            cur = prev;
            prev = next_address;
            return *this;
        }

        reference operator*() {
            return cur->value;
        }
    };

    class ListXORReverseIter {
        NodePtr prev;
        NodePtr cur;
        NodePtr next;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using difference_type = std::ptrdiff_t;

        ListXORReverseIter(NodePtr prev, NodePtr cur): prev(prev), cur(cur), next(xaddress(cur->npx, prev)){}
        ListXORReverseIter(NodePtr prev, NodePtr cur, NodePtr next): prev(prev), cur(cur), next(next){}

        bool operator!=(const ListXORReverseIter& rhs) {
            return rhs.prev != prev || rhs.cur != cur || rhs.next != next;
        }

        bool operator==(const ListXORReverseIter& rhs) {
            return !(this->operator!=(rhs));
        }

        ListXORReverseIter& operator--() {
            auto next_address = xaddress(cur, next->npx);
            prev = cur;
            cur = next;
            next = next_address;
            return *this;
        }

        ListXORReverseIter& operator++() {
            auto next_address = xaddress(cur, prev->npx);
            next = cur;
            cur = prev;
            prev = next_address;
            return *this;
        }

        reference operator*() {
            return cur->value;
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
        return ListXORIter{first, first->npx};
    }

    auto end() {
        return ListXORIter{last->npx, last};
    }

    auto rbegin() {
        return ListXORReverseIter{xaddress(last, last->npx->npx), last->npx};
    }

    auto rend() {
        return ListXORReverseIter{nullptr, first, first->npx};
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

int main(){
    ListXOR<int> list;
    list.push_back(10);
    list.push_back(40);
    list.push_front(112);
    
    for (auto it = list.begin(), ite = list.end(); it != ite; ++it)
        std::cout << *it << ' ';

    std::cout << '\n';

    for (auto it = list.rbegin(), ite = list.rend(); it != ite; ++it)
        std::cout << *it << ' ';
}
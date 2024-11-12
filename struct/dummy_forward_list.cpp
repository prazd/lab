#include <iostream>
#include <memory>
#include <cassert>
#include <algorithm>

template <typename T>
class forward_list {
    
    template <typename U>
    struct Node {
        U value;
        std::unique_ptr<Node<U>> next;
        Node(U value): value(value), next(nullptr) {}
    };


    template <typename U>
    class Iter {
        Node<U>* next;
    public:
        Iter(Node<U>* next): next(next) {}
        Iter& operator++() {
            next = next->next.get();
            return *this;
        }
        Iter operator+(size_t rhs) {
            auto tmp = next;
            for(size_t i = 0;i < rhs;++i) {
                tmp = tmp->next.get();
            }
            return Iter(tmp);
        }
        bool operator!=(const Iter& rhs) {
            return next != rhs.next;
        }
        bool operator==(const Iter& rhs) {
            return next == rhs.next;
        }
        U& operator*() {
            return next->value;
        }
        Node<U>* get_next() {
            return next;
        }
    };

    Iter<T> add(std::unique_ptr<Node<T>>& n, T value) {
        auto node = std::make_unique<Node<T>>(value);
        node->next = std::move(n);
        n = std::move(node);
        return Iter<T>(n.get());
    }

    template <typename List>
    void copy_list(const List& rhs) {
        root = std::make_unique<Node<T>>(*(rhs.begin()));
        auto current = root.get();
        for (auto it = rhs.begin()+1, ite = rhs.end();it!=ite;++it) {
            current->next = std::make_unique<Node<T>>(*it);
            current = current->next.get();
        }
    }

    std::unique_ptr<Node<T>> root;

public:
    forward_list(){}
    forward_list(const std::initializer_list<T>& values) {
        copy_list<std::initializer_list<T>>(values);
    }

    forward_list(const forward_list<T>& rhs) {
        root.reset();
        copy_list<forward_list<T>>(rhs);
    }

    forward_list(forward_list&& rhs): root(std::move(rhs.root)){}

    forward_list<T>& operator=(const forward_list& rhs) {
        if (&rhs == this) {
            return *this;
        }
        root.reset();
        copy_list(rhs);
        return *this;
    }

    forward_list<T>& operator=(forward_list&& rhs) {
        if (&rhs == this) {
            return *this;
        }
        std::swap(rhs.root, this->root);
        rhs.root.reset();
        return *this;
    }

    size_t size() const {
        size_t result{};
        auto current = root.get();
        while(current) {
            current = current->next.get();
            ++result;
        }
        return result;
    }

    Iter<T> insert_after(Iter<T> pos, T ival) {
        auto& next = pos.get_next()->next;
        if (!next.get()) {
            return Iter<T>(nullptr);
        }
        return add(next, ival);
    }

    void push_front(T value) {
        add(root, value);
    }

    void pop_front() {
        root = std::move(root->next);
    }

    Iter<T> begin() const {
        return Iter<T>(root.get());
    }

    Iter<T> end() const {
        return Iter<T>(nullptr);
    }
};

int main() {
    forward_list<int> fl{1,2,3,4};
    fl.push_front(20);
    std::array<int, 5> arr{20,1,2,3,4};
    assert(std::equal(fl.begin(), fl.end(), arr.begin()));
    assert(fl.size() == 5);

    fl.pop_front();
    assert(std::equal(fl.begin(), fl.end(), arr.begin()+1));

    auto val = fl.insert_after(fl.begin()+2, 10);
    assert(val == (fl.begin()+3) && *val == 10);

    std::array<int, 5> arr2{1,2,3,10,4};
    assert(std::equal(fl.begin(), fl.end(), arr2.begin()));
    

    // copy ctr
    forward_list<int> fl1 (fl);
    assert(std::equal(fl1.begin(), fl1.end(), arr2.begin()));
    assert(fl1.size() == fl.size());

    // move ctr
    forward_list<int> fl1_1(std::move(fl1));
    assert(std::equal(fl1_1.begin(), fl1_1.end(), arr2.begin()));
    assert(fl1.size() == 0);

    // copy assignment
    forward_list<int> fl2{10,20,30,40};
    fl1 = fl2;
    std::array<int, 4> arr3{10,20,30,40};
    assert(std::equal(fl1.begin(), fl1.end(), arr3.begin()));
    assert(std::equal(fl2.begin(), fl2.end(), arr3.begin()));

    // move assignment
    std::array<int, 4> arr4{50,60,70,80};
    forward_list<int> fl3{50,60,70,80};
    forward_list<int> fl4;
    fl4 = std::move(fl3);
    assert(std::equal(fl4.begin(), fl4.end(), arr4.begin()));
    assert(fl3.size() == 0);
}
#include <iostream>
#include <memory>

template <typename T>
class Node {
    T value;
    size_t weight;
    std::unique_ptr<Node<T>> left;
    std::unique_ptr<Node<T>> right;
public:    
    Node(T value): value(value), weight(0), left(nullptr), right(nullptr){}
    void set_right(T value) {
        right = std::make_unique<Node<T>>(value);
    }
    void set_left(T value) {
        left = std::make_unique<Node<T>>(value);
    }
    T get_left_value() const {
        return left.get()->value;
    }
    T get_right_value() const {
        return right.get()->value;
    }
    T get_value() const {
        return value;
    }
    // for test
    Node<T>* get_left_ptr() const {
        return left.get();
    }
    Node<T>* get_right_ptr() const {
        return right.get();
    }
    Node<T>* get_ptr() const {
        return this;
    }
};

//   4
//  2  6
// n n  n n

template <typename T>
struct DefaultComparator {
    bool operator()(T x, T y) {
        return x >= y;
    }
};

template <typename T, typename Comparator = DefaultComparator<T>>
struct BinTree{
    std::unique_ptr<Node<T>> root;
    [[no_unique_address]] Comparator cmpr;
    BinTree(): root(nullptr){}
    BinTree(T value): root(std::make_unique<Node<T>>(value)) {}
    void insert(T value) {
        if (!root) {
            root = std::make_unique<Node<T>>(value);
            return;
        }
        auto curr = root.get();
        Node<T>* parrent = nullptr;
        while (curr) {
            parrent = curr;
            curr = cmpr(value, curr->get_value()) ? curr->get_right_ptr() : curr->get_left_ptr();
        }
        cmpr(value, parrent->get_value()) ? parrent->set_right(value) : parrent->set_left(value);
    }
    ~BinTree() {}
};

template <typename T>
struct PairValueComarator {
    bool operator()(const T& f, const T& s) {
        return f.second > s.second;
    }
};

using NodeType = std::pair<std::string, size_t>;

std::ostream& operator<<(std::ostream& os, const NodeType& nt) {
    os << nt.first << " " << nt.second;
    return os;
}

int main() {
    BinTree<NodeType, PairValueComarator<NodeType>> x;
    x.insert({"a", 4});
    x.insert({"b", 2});
    x.insert({"c", 6});
    x.insert({"d", 1});
    x.insert({"e", 3});
    x.insert({"f", 5});
    x.insert({"g", 7});

    std::cout << "               root: " << x.root->get_value() << std::endl;
    std::cout << "     Left: " << x.root->get_left_value() << "          Right:" <<  x.root->get_right_value()<< std::endl;
    std::cout << "Left:  " << x.root->get_left_ptr()->get_left_value()<< " Right:" <<  x.root->get_left_ptr()->get_right_value()<<  "Left:  " << x.root->get_right_ptr()->get_left_value() << " Right:" <<  x.root->get_right_ptr()->get_right_value() << std::endl;
}
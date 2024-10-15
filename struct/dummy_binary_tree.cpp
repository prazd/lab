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
struct BinTree{
    std::unique_ptr<Node<T>> root;
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
            if (value < curr->get_value()) {
                curr = curr->get_left_ptr();
            } else {
                curr = curr->get_right_ptr();
            }
        }
        if (value > parrent->get_value()) {
            parrent->set_right(value);
        } else {
            parrent->set_left(value);
        }
    }
    ~BinTree() {}
};

int main() {
    BinTree<int> x;
    x.insert(4);
    x.insert(2);
    x.insert(6);
    x.insert(1);
    x.insert(3);
    x.insert(5);
    x.insert(7);

    std::cout << "               root: " << x.root->get_value() << std::endl;
    std::cout << "     Left: " << x.root->get_left_value() << "          Right:" <<  x.root->get_right_value()<< std::endl;
    std::cout << "Left:  " << x.root->get_left_ptr()->get_left_value()<< " Right:" <<  x.root->get_left_ptr()->get_right_value()<<  "Left:  " << x.root->get_right_ptr()->get_left_value() << " Right:" <<  x.root->get_right_ptr()->get_left_value() << std::endl;
}
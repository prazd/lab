#include <iostream>
#include <memory>

template <typename T>
class Node {
    T val;
    std::unique_ptr<Node<T>> n;
public:
    Node(T val): val(val), n(nullptr){}
    T get_val() const {
        return val;
    }
    Node* next() const {
        return n.get();
    }
    void set_next(std::unique_ptr<Node<T>> next) {
        n = std::move(next);
    }
};

template <typename T>
class List {
    std::unique_ptr<Node<T>> head;
public:    
    List(T val): head(std::make_unique<Node<T>>(val)){}
    void push(T val) {
        Node<T>* curr = head.get();
        while(curr->next()) {
            curr = curr->next();
        }
        curr->set_next(std::make_unique<Node<T>>(val));
    }
    Node<T>* get(T val) const {
        Node<T>* curr = head.get();
        while (curr != nullptr) {
            if (curr->get_val() == val) {
                return curr;
            }
            curr = curr->next();
        }
        return nullptr;
    }

    bool empty() const {
        return head == nullptr;
    }
};

int main() {
    List<int> l(20);
    std::cout << l.get(20)->get_val() << std::endl;
}
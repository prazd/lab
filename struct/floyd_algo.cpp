#include <iostream>

template <typename T>
class Node {
    T val;
    Node<T>* n;
public:
    Node(T val): val(val), n(nullptr){}
    Node(T val, Node<T>* next): val(val), n(next){}
    T get_val() const {
        return val;
    }
    Node* next() const {
        return n;
    }
    void set_next(Node<T>* next) {
        n = next;
    }
};

template <typename T>
class List {
    Node<T>* head;
    size_t sz;
public:    
    List(T val): head(new Node<T>(val)), sz(1){}
    void push(T val) {
        Node<T>* curr = head;
        while(curr->next() != nullptr) {
            curr = curr->next();
        }
        curr->set_next(new Node<T>(val));
        ++sz;
    }
    void push(Node<T>* val) {
        Node<T>* curr = head;
        while(curr->next() != nullptr) {
            curr = curr->next();
        }
        curr->set_next(val);
        ++sz;
    }
    size_t size() const {
        return sz;
    }
    Node<T>* get(T val) const {
        Node<T>* curr = head;
        while(curr != nullptr) {
            if (curr->get_val() == val) {
                return curr;
            }
            curr = curr->next();
        }
        return nullptr;
    }

    Node<T>* get_head() const {
        return head;
    }

    bool empty() {
        return head == nullptr;
    }

    ~List() {
        Node<T>* curr = head;
        while(curr != nullptr) {
            auto n = curr->next();
            delete curr;
            curr = n;
        }
    }
};

template <typename T>
bool has_cycle(const List<T>& l) {
    auto slow = l.get_head();
    auto fast = l.get_head();
    while (fast != nullptr && fast->next() != nullptr) {
        slow = slow->next();
        fast = fast->next()->next();
        if (fast == slow) {
            return true;
        }
    }
    return false;
}

int main() {
    List<int> l(20);
    l.push(10);
    l.push(30);
    l.push(l.get_head());

    if (has_cycle(l)) {
        auto curr = l.get_head();
        while(curr != nullptr) {
            if (curr->next() == l.get_head()) {
                std::cout << "Fix cycle";
                curr->set_next(nullptr);
            }
            curr = curr->next();
        }
    }
}
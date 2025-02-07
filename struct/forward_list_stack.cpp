#include <iostream>
#include <cassert>

template <typename T>
struct Node {
    T val;
    std::unique_ptr<Node<T>> next;
    Node(): val(0), next(nullptr){}
    Node(T val): val(val), next(nullptr){}
};

template <typename T>
struct Stack {
    std::unique_ptr<Node<T>> head;
    Stack(): head(nullptr) {}
    Stack(T val): head(new Node<T>(val)){};
    bool empty() const {
        return head.get() == nullptr;
    }
    void push(T v) {
        auto next = std::unique_ptr<Node<T>>{new Node<T>{v}};
        auto tmp = std::move(head);
        next->next = std::move(tmp);
        head = std::move(next);
    }
    T pop() {
        if (empty()) {
            throw std::length_error("empty stack");
        }
        auto val = head->val;
        head = std::move(head->next);
        return val;
    }
};


int main() {
    Stack s{10};
    s.push(20);
    s.push(30);
    assert(!s.empty());
    assert(s.pop() == 30);
    assert(s.pop() == 20);
    assert(s.pop() == 10);
    assert(s.empty());
    s.push(300);
    assert(s.pop() == 300);
    assert(s.empty());
}
#include <iostream>
#include <cassert>

// Konstantin Vladimirov version: https://godbolt.org/z/7qhd5Y3ar (C++ (MIPT, ILab). Lecture 19)

template <typename T>
struct Node {
    T data;
    Node* left{};
    Node* right{};
    Node(T data): data(data){}
};

auto tree_get(auto root, auto... fields) {
    return (root ->* ... ->* fields);
}

auto right = &Node<int>::right;
auto left = &Node<int>::left;

int main() {
    Node<int> a(10), b(20), c(30), d(40), e(50);
    a.left = &b;
    a.right = &c;
    c.left = &d;
    c.right = &e;
    assert((tree_get(&a, left))->data == 20);
    assert((tree_get(&a, right))->data == 30);
    assert((tree_get(&a, right, right))->data == 50);
    assert((tree_get(&a, right, left))->data == 40);
}
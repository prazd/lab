#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>


template <typename T>
struct Node {
    std::vector<T> elems;
    std::vector<Node<T>> childs;

    Node() = default;
    Node(const std::vector<T>& elems): elems(elems) {}

    void add_elems(const T& elem) {
        elems.push_back(elem);
    }

    void add_child(Node<T>&& c){
        childs.push_back(std::move(c));
    }

    void print_elems() const {
        std::cout << "El:";
        for(const auto& el: elems)
            std::cout << el << " ";
        std::cout << "\n";
    }

    void print_child() const {
        for (const auto& c: childs)
            c.print_elems();
    }

};

template <typename T>
struct Tree {
    Node<T> root{};
    std::vector<T> root_elems;
    std::vector<std::vector<T>> res;

    Tree(const std::vector<T>& elems): root_elems(elems) {
        init_root(root_elems);
        std::cout << "Root:"<<"\n";
        root.print_child();
        std::cout << "\n";

        for(auto& c: root.childs) {
            generate(&c);
        }
    }

    void print() const {
        std::cout << "Size:" << res.size() << "\n";
        for(auto& row: res){
            for(auto& el: row)
                std::cout << el << " ";
            std::cout << "\n";
        }   
    }

private:
    void init_root(const std::vector<T>& elems) {
        for(auto el: elems) {
            root.add_child(std::vector<T>{el});
        }
    }

    void generate(Node<T>* node) {
        if (node->elems.size() == root_elems.size()){
            res.push_back(node->elems);
            return;
        }
        add_child(node);
        for(auto& c: node->childs){
            generate(&c);
        }
    }

    void add_child(Node<T>* node) {
        auto& nelems = node->elems;
        for(auto e: root_elems){
            auto fe = std::find(nelems.begin(), nelems.end(), e);
            if (fe != nelems.end())
                continue;
            auto nc = node->elems;
            nc.push_back(e);
            node->add_child(nc);
        }
    }
};


int main() {
    Tree<int> tree(std::vector<int>{1,2,3,4,5});
    tree.print();
}
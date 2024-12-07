#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

template <typename T>
struct Node {
    std::vector<T> elems;
    std::vector<Node<T>> childs;

    Node() = default;
    Node(std::vector<T>&& elems): elems(std::move(elems)) {}

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

//total heap usage: 285 allocs, 285 frees, 83,624 bytes allocated
//total heap usage: 156 allocs, 156 frees, 81,088 bytes allocated

size_t get_factorial(size_t n) {
    size_t res = 1;
    for(size_t i = 1; i <= n;++i)
        res*=i;
    return res;
}

template <typename T>
struct Tree {
    Node<T> root{};
    std::vector<T> root_elems;
    size_t cap;
    size_t sz;
    std::vector<std::vector<T>> res;

    Tree(const std::vector<T>& elems): root_elems(elems), cap(get_factorial(elems.size())), sz(0), res(cap) {
        init_root(root_elems);
        std::cout << "Root:"<<"\n";
        root.print_child();
        std::cout << "\n";

        for(auto& c: root.childs) {
            generate(&c);
        }
        assert(cap = sz);
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
            res[sz++] = node->elems;
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
            std::vector<T> nc (nelems.size()+1);
            std::copy(nelems.begin(), nelems.end(), nc.begin());
            nc.back() = e;
            node->add_child(std::move(nc));
        }
    }
};


int main() {
    Tree<int> tree(std::vector<int>{1,2,3,4});
    tree.print();
}
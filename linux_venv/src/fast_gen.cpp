#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>
#include <array>

template <typename T, size_t sz>
struct Node {
    size_t count;
    std::array<T, sz> elems;
    std::vector<Node<T, sz>> childs;

    Node() = default;
    Node(std::array<T, sz>&& elems, size_t count): count(count), elems(std::move(elems)) {}

    void add_child(std::array<T, sz>&& arr, size_t c){
        childs.emplace_back(std::move(arr), c);
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

template <typename T, size_t size>
struct Tree {
    Node<T, size> root{};
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
            root.add_child(std::array<T, size>{el}, 0);
        }
    }

    void generate(Node<T, size>* node) {
        if (node->count == root_elems.size()-1){
            res[sz++] = std::vector<T>(node->elems.begin(), node->elems.end());
            return;
        }
        add_child(node);
        for(auto& c: node->childs){
            generate(&c);
        }
    }

    void add_child(Node<T, size>* node) {
        auto& nelems = node->elems;
        auto count = node->count + 1;
        for(auto e: root_elems){
            auto fe = std::find(nelems.begin(), nelems.end(), e);
            if (fe != nelems.end())
                continue;
            std::array<T, size> nc(nelems);
            nc[count] = e;
            node->add_child(std::move(nc), count);
        }
    }
};


int main() {
    Tree<int, 5> tree(std::vector<int>{1,2,3,4});
    tree.print();
}
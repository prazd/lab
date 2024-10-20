#include <iostream>
#include <set>
#include <map>
#include <queue>
#include <vector>

struct Key{
    char sym;
    size_t value;
    Key(char sym, size_t value): sym{sym}, value{value}{}
    Key(const Key& k): sym(k.sym), value(k.value) {}
    bool operator==(const Key& k) const {
        return sym == k.sym && value == k.value;
    }
    bool operator<(const Key& k) const {
        return value <= k.value;
    }
};

struct Node {
    size_t value;
    bool sum_node;
    Key k;
    Node* left;
    Node* right;
    bool operator<(const Node& n) const {
        return n.value <= value;
    }
};

struct Tree {
    Node* root;

    void print_codes(const Node* node, std::map<char, std::string>& res_m, const std::string& prefix = "") const {
        if (!node) return;

        if (!node->sum_node) {
            std::cout << node->k.sym << ": " << prefix << "\n";
            res_m[node->k.sym] = prefix;
        }
        print_codes(node->left, res_m, prefix + "0");
        print_codes(node->right, res_m, prefix + "1");
    }
};

Tree create_tree(const std::set<Key>& s) {
    std::priority_queue<Node, std::vector<Node>, std::less<Node>> pq;
    std::for_each(s.begin(), s.end(), [&pq](const auto& el){pq.push(Node{.value = el.value, .sum_node = false, .k = el, .left = nullptr, .right = nullptr});});
    Tree res;
    std::cout << "S size" << s.size() << std::endl;
    std::cout << "PQ size" << pq.size() << std::endl;
    while (pq.size() > 1)
    {
        std::cout << "Iter" << std::endl;
        auto first_node = static_cast<Node>(pq.top());
        pq.pop();
        auto second_node = static_cast<Node>(pq.top());
        pq.pop();

        auto snode = Node{.value = first_node.value + second_node.value, .sum_node = true, .k = {0,0}, .left = new Node(first_node), .right = new Node(second_node)};
        pq.push(snode);
    }
    res.root = new Node(pq.top());
    return res;
}

std::set<Key> get_syms_count_map(const std::string& val) {
    std::set<Key> sres;
    std::map<char, size_t> mres;
    for (const auto& ch : val) {
        ++mres[ch];
    }
    std::for_each(mres.begin(), mres.end(),[&sres](const auto& p){sres.insert(Key(p.first, p.second));});
    return sres;
}

int main() {
    std::string str = "Try it out with your own content.";
    auto res = get_syms_count_map(str);
    std::for_each(res.begin(), res.end(), [](auto& p){ std::cout << p.sym << " " << p.value;});
    std::cout << "\n";
    auto tree = create_tree(res);
    std::map<char, std::string> mres;
    tree.print_codes(tree.root, mres);
    std::string strres;
    std::for_each(str.begin(), str.end(), [&mres, &strres](const auto sym){strres += mres[char(sym)];});
    
    std::cout << strres << std::endl;
}
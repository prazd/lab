#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <set>
#include <queue>
#include <vector>
#include <memory>
#include <stack>

constexpr const char* filename = "test.bin";

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
    std::unique_ptr<Key> k;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    Node(size_t value, bool sum_node, std::unique_ptr<Node> left, std::unique_ptr<Node> right, std::unique_ptr<Key> key)
            : value(value), sum_node(sum_node), left(std::move(left)), right(std::move(right)), k(std::move(key)) {}

    Node(size_t value, bool sum_node, Key* k = nullptr)
            : value(value), sum_node(sum_node), k(k ? std::make_unique<Key>(*k) : nullptr) {}
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&& rhs) noexcept
            : value(rhs.value), sum_node(rhs.sum_node), k(std::move(rhs.k)), left(std::move(rhs.left)), right(std::move(rhs.right)) {}

    bool operator>(const Node& n) const {
        return value > n.value;
    }
};

struct HuffmanTree {
    std::unique_ptr<Node> root;
    HuffmanTree(std::set<Key>& s) {
        auto compare = [](const std::unique_ptr<Node>& a, const std::unique_ptr<Node>& b) {
            return a->value >= b->value;
        };

        std::priority_queue<std::unique_ptr<Node>, std::vector<std::unique_ptr<Node>>, decltype(compare)> pq;

        for (const auto& el : s) {
            pq.push(std::make_unique<Node>(el.value, false, new Key(el.sym, el.value)));
        }

        while (pq.size() > 1) {
            auto left = std::move(const_cast<std::unique_ptr<Node>&>(pq.top()));
            pq.pop();

            auto right = std::move(const_cast<std::unique_ptr<Node>&>(pq.top()));
            pq.pop();

            auto k = std::make_unique<Key>(Key(0,0));

            auto new_node = std::make_unique<Node>(left->value + right->value, true, std::move(left), std::move(right), std::move(k));
            pq.push(std::move(new_node));
        }
        auto tmp = std::move(const_cast<std::unique_ptr<Node>&>(pq.top()));
        root = std::move(tmp);
        pq.pop();
    }

    std::map<char, std::string> construct_map_sym_bin() const {
        std::map<char, std::string> result;
        fill_sym_bin_map(root.get(), result);
        return result;
    }

    void fill_sym_bin_map(const Node* node, std::map<char, std::string>& res_m, const std::string& prefix = "") const {
        if (!node) return;
        if (!node->sum_node) {
            res_m[node->k->sym] = prefix;
        }
        fill_sym_bin_map(node->left.get(), res_m, prefix + "0");
        fill_sym_bin_map(node->right.get(), res_m, prefix + "1");
    }

    char find_node(const Node* node, std::map<char, std::string>& mres,std::string& current, std::stack<char>& s) {
        while(true) {
            auto s_top = s.top();
            current += s_top;
            s.pop();
            auto res = std::find_if(mres.begin(), mres.end(), [&current](auto& el){return el.second == current;});
            if (res != mres.end()) {
                return res->first;
            }
            node = s_top == '0' ? node->left.get() : node->right.get();
        }
    }
    void decode_result(std::map<char, std::string>& res_m, std::string& encoded_str) {
        std::stack<char> sym_stack;
        std::for_each(encoded_str.begin(), encoded_str.end(), [&sym_stack](auto& el){sym_stack.push(el);});

        std::string result;
        auto node = root.get();

        while(!sym_stack.empty()) {
            std::string current;
            auto symbol = find_node(node, res_m, current, sym_stack);
            result += symbol;
        }
        std::cout << "Decode result:" << result;
    }
};

std::set<Key> get_syms_count_map(const std::string& val) {
    std::set<Key> sres;
    std::map<char, size_t> mres;
    for (const auto& ch : val) {
        ++mres[ch];
    }
    std::for_each(mres.begin(), mres.end(),[&sres](const auto& p){sres.insert(Key(p.first, p.second));});
    return sres;
}

size_t get_sz(size_t sz) {
    double dsz = double(sz)/double(CHAR_BIT);
    if (dsz > static_cast<double>(static_cast<size_t>(dsz))) {
        return static_cast<size_t>(dsz) + 1;
    } else {
        return static_cast<size_t>(dsz);
    }
}

void save_bin(std::string str) {
    std::cout << "String size:" << str.size() << std::endl;;
    const size_t sz = get_sz(str.size());
    std::cout << "Buffer size:" << sz << std::endl;
    unsigned char buffer[sz];
    size_t count = 0;
    unsigned char l_res = 0;
    for(size_t i = 0; i < sz; ++i) {
        unsigned char l_res = 0;
        std::string tmp = str.substr(0, 8);
        size_t count = 0;
        std::for_each(tmp.begin(), tmp.end(), [&l_res, &count](const auto& val){
            if (val == '1') {
                l_res<<=1;
                l_res|=1;
            } else if (val == '0') {
                l_res<<= 1;
            }
            ++count;
        });
        if (8 - count > 0) {
            for(int i = 0;i < 8-count;++i) {
                l_res <<= 1;
            }
        }
        buffer[i] = l_res;
        if (!(i == sz - 1)) {
            str = str.substr(8, str.size());
        }
    }
    std::string res_str;
    std::for_each(buffer, buffer+sz, [&res_str](const auto& el){ res_str+=std::bitset<8>(el).to_string();});
    std::cout << "Result data:" << "\n" << res_str << std::endl;
    std::fstream file(filename, std::ios::out);
    if (!file) {
        std::abort();
    }
    file << buffer;
    file.close();
}


int main() {
    std::string str = "Try it out with your own content.";
    auto syms_count_map = get_syms_count_map(str);

    HuffmanTree tree(syms_count_map);
    auto mres = tree.construct_map_sym_bin();

    std::string result_string;
    std::for_each(str.begin(), str.end(), [&mres, &result_string](const auto sym){result_string += mres[char(sym)];});
    std::cout << "Result string:" << result_string << std::endl;

    std::string result_string_cp(result_string);
    std::reverse(result_string_cp.begin(), result_string_cp.end());

    tree.decode_result(mres, result_string_cp);
}

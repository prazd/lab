#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <map>
#include <set>
#include <queue>
#include <vector>
#include <memory>
#include <stack>
#include <cassert>

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
            std::cout << node->k->sym << ": " << prefix << "\n";
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
    std::string decode_result(std::map<char, std::string>& res_m, std::string encoded_str, size_t input_string_size) {
        std::stack<char> sym_stack;
        std::reverse(encoded_str.begin(), encoded_str.end());
        std::for_each(encoded_str.begin(), encoded_str.end(), [&sym_stack](auto& el){sym_stack.push(el);});

        std::string result;
        auto node = root.get();

        size_t counter = 0;
        while(counter != input_string_size) {
            std::string current;
            auto symbol = find_node(node, res_m, current, sym_stack);
            result += symbol;
            ++counter;
        }
        return result;
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
        std::string tmp;
        if (str.size() >= 8) {
            tmp = str.substr(0, 8);
        } else {
            tmp = str.substr(0, str.size());
        }
        
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


        if (count < 8) {
            size_t diff = 8 - count;
            for(size_t i = 0;i != diff;++i) {
                l_res<<=1;
            }
        }

        buffer[i] = l_res;

        if (!(i == sz - 1)) {
            str = str.substr(8, str.size());
        }
    }
    std::string res_str;
    std::for_each(buffer, buffer + sz, [&res_str](const auto& el){ res_str+=std::bitset<8>(el).to_string();});
    std::cout << "Encoded data:        " << res_str;

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::abort();
    }
    file.write(reinterpret_cast<const char*>(buffer), sizeof(buffer));
    file.close();
}

std::string read_bin() {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::abort();
    }
    const size_t file_size = static_cast<size_t>(std::filesystem::file_size(filename));
    char buffer[file_size];
    file.read(buffer, file_size);

    std::string result;
    std::for_each(buffer, buffer+file_size,[&result](const auto el){
        result += std::bitset<8>(el).to_string();
    });
    return result;
}


int main() {
    std::string input_str = "In computer science and information theory, a Huffman code is a particular type of optimal prefix code that is commonly used for lossless data compression. The process of finding or using such a code is Huffman coding, an algorithm developed by David A. Huffman while he was a Sc.D. student at MIT, and published in the 1952 paper \"A Method for the Construction of Minimum-Redundancy Codes\".";
    std::cout << "Input string:" << input_str << std::endl;
    std::cout << "Input string size:" << input_str.size() << std::endl;
    auto syms_count_map = get_syms_count_map(input_str);

    HuffmanTree tree(syms_count_map);
    auto mres = tree.construct_map_sym_bin();

    std::string bin_string;
    std::for_each(input_str.begin(), input_str.end(), [&mres, &bin_string](const auto sym){bin_string += mres[char(sym)];});
    
    std::cout << "Data to save:        " << bin_string << std::endl;
    save_bin(bin_string);
    std::cout << "\n" << std::endl;

    auto file_data = read_bin();
    std::cout << "Read data from file: " << file_data << std::endl;
    std::cout << "\n" << std::endl;

    auto result = tree.decode_result(mres, file_data, input_str.size());
    assert(result == input_str);
    std::cout << "Decode result:" << result;
}

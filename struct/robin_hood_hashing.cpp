#include <iostream>
#include <cassert>

template <typename Key, typename Value>
struct RHMap {
    size_t cap;
    size_t sz;
    
    struct Node {
        std::pair<Key, Value> p;
        size_t D;
    };

    std::vector<Node> nodes;

    RHMap(): cap(4), sz(0), nodes(cap){}
    static auto get_index(Key k, size_t c) {
        return std::hash<Key>{}(k) % c;
    }

    bool need_reordering() {
        return ((double)sz / (double)cap) * 100 >= 80;
    }

    void reorder() {
        auto new_nodes = std::vector<Node>(cap * 2);
        for (auto& el: nodes) {
            if (el.p.first != Key{}) {
                auto index = get_index(el.p.first, cap * 2);
                find_new_pos(el.p, index, new_nodes, cap * 2);
            }
        }
        cap *= 2;
        nodes = std::move(new_nodes);
    }

    void insert(std::pair<Key, Value> p) {
        if (need_reordering()) reorder();
        if (auto it = get(p.first)) {
            it->get() = p.second;
            return;
        }
        insert(p, nodes, cap);
        ++sz;
    }

    template <typename Arr>
    static void insert(const std::pair<Key, Value>& p, Arr& arr, size_t c) {
        auto index = get_index(p.first, c);
        find_new_pos(p, index, arr, c);
    }


    template <typename Arr>
    static void find_new_pos(const std::pair<Key, Value>& p, size_t index, Arr& arr, size_t c) {
        auto current = p;
        size_t d = 0;
        bool full_circle = false;
        for (auto i = index; !full_circle; i = (i + 1) % c, ++d) {
            if (arr[i].p.first == Key{}){
                arr[i] = {current, d};
                return; // new empty slot
            }
            if (d > arr[i].D) { // rob old el and old => current
                auto old_el = arr[i];
                arr[i] = {current, d};
                current = {old_el.p.first, old_el.p.second};
                d = old_el.D + 1; //
            }
            if ((i + 1) % c == index) full_circle = true;
        }
        throw std::overflow_error("RHHMap is full");
    }

    Value& operator[](Key k) {
        auto v = get(k);
        if (v.has_value()){
            return v.value();
        }
        if (need_reordering()) reorder();
        insert({k, Value{}});
        return get(k).value();
    }

    std::optional<std::reference_wrapper<Value>> get(Key k) {
        auto index = get_index(k, cap);
        size_t d = 0;
        bool full_circle = false;
        for (auto i = index; !full_circle; i = (i + 1) % cap, ++d) {
            if (nodes[i].p.first == k)
                return std::ref(nodes[i].p.second);
            if (nodes[i].p.first == Key{} || nodes[i].D < d)
                return {};
            if ((i + 1) % cap == index) full_circle = true;
        }
        return {};
    }

    void debug_print() {
        std::cout << "\n=================================\n\n";
        for(auto i = 0; i < cap; ++i){
            if (nodes[i].p.first == Key{})
                std::cout << i << ": Empty "<< "\n";
            else
                std::cout << i << ": { Key:" << nodes[i].p.first << ", Value:" << nodes[i].p.second << ", D:" << nodes[i].D <<"}\n";
        }

        std::cout << "\n=================================\n\n";
    }
};

int main() {
    RHMap<std::string, int> m;
    m.insert({"alex", 123});
    m.insert({"bax", 100});
    assert(m["alex"]== 123);
    assert(m["bax"]== 100);
    m.insert({"alex1", 101});
    m.insert({"alex2", 102});
    m.insert({"alex3", 103});
    m.insert({"alex4", 104});
    m.insert({"alex5", 105});
    assert(m["alex1"]== 101);
    assert(m["alex2"]== 102);
    assert(m["alex3"]== 103);
    assert(m["alex4"]== 104);
    assert(m["alex5"]== 105);
    assert(m["noname"] == 0);
    m.debug_print();
    m.insert({"peter", 111});
    assert(m["peter"]== 111);
    m.debug_print();
    m.insert({"alex", 333});
    assert(m["alex"] == 333);
    m.debug_print();
}
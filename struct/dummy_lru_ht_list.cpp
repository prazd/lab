#include <iostream>
#include <cassert>
#include <optional>
#include <unordered_map>
#include <tuple>

template <typename Key, typename Value, size_t cap = 3>
class LRU {
    struct Node{
        std::optional<std::pair<Key, Value>> data;
        Node* next = nullptr;
        Node* prev = nullptr;
    };
    struct List {
        Node* head;
        Node* tail;
        size_t sz = 0;
        List() {
            head = new Node;
            auto current = head;
            for (int i = 1; i < cap; ++i) {
                current->next = new Node;
                current->next->prev = current;
                current = current->next;
            }
            tail = head;
        }
        ~List() {
            while(head){
                auto next = head->next;
                delete head;
                head = next;
            }
        }

        Node* add_new(const Key& key, const Value& val) {
            if (!head->data.has_value()) {
                head->data = {key, val};
                ++sz;
                return head;
            }
            assert(cap != sz);
            tail = tail->next;
            tail->data = {key, val};
            ++sz;
            return tail;
        }

        std::tuple<Node*, Key> change_last(const Key& key, const Value& val) {
            assert(tail->data.has_value());
            auto prev_key = tail->data->first;
            tail->data = std::make_pair(key, val);
            return {tail, prev_key};
        }

        [[nodiscard]] size_t size() const{
            return sz;
        }

        void to_top(Node* current) {
            if (current == head)
                return;
            if (current == tail)
                tail = tail->prev;
            if (current->prev)
                current->prev->next = current->next;
            if (current->next)
                current->next->prev = current->prev;
            head->prev = current;
            current->next = head;
            current->prev = nullptr;
            head = current;
        }
    } list;
    std::unordered_map<Key, Node*> node_map;

public:
    void set_or_update(Key k, Value val) {
        auto n = node_map.find(k);
        if (n == node_map.end()){
            if (list.size() == cap) {
                auto [last_node, prev_key] = list.change_last(k, val);
                node_map.erase(prev_key);
                list.to_top(last_node);
                node_map[k] = last_node;
                return;
            }
            auto node_addr = list.add_new(k, val);
            node_map[k] = node_addr;
            list.to_top(node_addr);
            return;
        }
        n->second->data->second = val;
        list.to_top(n->second);
    }
    std::optional<Value> get(Key k) {
        auto n = node_map.find(k);
        if (n == node_map.end()) {
            return {};
        }
        list.to_top(n->second);
        return n->second->data.value().second;
    }
};


int main() {
    LRU<std::string, int> cache;

    cache.set_or_update("alex", 123);
    cache.set_or_update("bob", 222);
    assert(cache.get("bob").value() == 222);
    assert(cache.get("alex").value() == 123);


    cache.set_or_update("dan", 888);
    assert(cache.get("dan").value() == 888);

    cache.set_or_update("stan", 11111);
    assert(cache.get("stan").value() == 11111);

    cache.set_or_update("bobby", 7777);
    assert(cache.get("bobby").value() == 7777);

    cache.set_or_update("tedd", 333);
    assert(cache.get("tedd").value() == 333);
}
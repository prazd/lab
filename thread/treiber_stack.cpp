#include <iostream>
#include <cassert>
#include <thread>

template <typename T>
struct Node {
    T val;
    Node<T>* next;
    Node(): val(0), next(nullptr){}
    Node(T val): val(val), next(nullptr){}
};

template <typename T>
struct Stack {
    std::atomic<Node<T>*> head;
    Stack(): head(nullptr) {}
    Stack(T val){
        head.store(new Node<T>(val));
    };
    bool empty() const {
        return head.load() == nullptr;
    }
    void push(T v) {
        auto next = new Node<T>{v};
        for (;;) {
            auto tmp = head.load();
            next->next = tmp;
            if (head.compare_exchange_strong(tmp, next)){
                return;
            }
        }
    }
    T pop() {
        for(;;){
            if (empty()) {
                throw std::length_error("empty stack");
            }
            auto h = head.load();
            auto n = head.load()->next;
            if (head.compare_exchange_strong(h, n)){
                auto res = h->val;
                delete h;
                return res;
            }
        }
    }
    ~Stack() {
        auto h = head.load();
        while(h) {
            auto tmp = h->next;
            delete h;
            h = tmp;
        }
    }
};


int main() {
    Stack s{10};
    std::thread t1([&]{
        for (auto i = 1; i < 10; ++i) {
            s.push(i);
        }
    });
    std::thread t2([&]{
        for (auto i = 10; i < 20; ++i) {
            s.push(i);
        }
    });
    t1.join();
    t2.join();

    try {
        while(true) {
            std::cout << s.pop() << "\n";
        }
    } catch(std::length_error err) {
        std::cout << "Length err";
    }
}
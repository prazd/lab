#include <iostream>
#include <utility>
#include <cassert>

struct Pixel {
    unsigned char color{}; // 0-3
    unsigned char attrib{};
};

template <typename T, size_t sz>
class SimpleDeque {
    std::array<T, sz> queue{};
    size_t current = 0;
public:
    void push_back(const T& p) {
        queue[current++] = p;
    }
    void push_front(const T& p) {
        auto tmp = queue[0];
        for (int i = 1; i <= current; ++i){
            queue[i] = std::exchange(tmp, queue[i]);
        }
        queue[0] = p;
        ++current;
    }
    auto pop_front() {
        auto tmp = queue[0];
        for (int i = 1; i < sz; ++i){
            queue[i-1] = queue[i];
        }
        --current;
        return tmp;
    }

    auto pop_back() {
        return queue[--current];
    }

    size_t size() const {
        return current;
    }

    auto& front() {
        return queue[0];
    }
    bool empty() const {
        return !current;
    }
    auto& back() {
        return queue[current-1];
    }

    auto begin() {
        return queue.begin();
    }
    auto end() {
        return queue.begin() + current;
    }
};

int main(){
    SimpleDeque<Pixel, 8> pq;
    assert(pq.empty());
    pq.push_front({1, 1});
    assert(pq.front().color == 1);
    assert(pq.back().color == 1);
    pq.push_front({2, 4});
    pq.push_front({3, 4});
    pq.push_front({4, 4});
    pq.push_front({5, 4});
    assert(pq.size() == 5);
    assert(pq.front().color == 5);
    assert(pq.back().color == 1);
    pq.push_back({6, 4});
    assert(pq.front().color == 5);
    assert(pq.back().color == 6);
    pq.pop_back();
    assert(pq.back().color == 1);
    pq.pop_front();
    assert(pq.front().color == 4);
    pq.pop_front();
    assert(pq.front().color == 3);
    pq.pop_front();
    assert(pq.front().color == 2);
    pq.pop_front();
    assert(pq.front().color == 1);
    auto it = pq.begin();
    *it = Pixel{2,3};
    assert(pq.pop_back().color == 2);
    assert(pq.empty());
}
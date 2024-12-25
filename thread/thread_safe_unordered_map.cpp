#include <iostream>
#include <unordered_map>
#include <thread>
#include <shared_mutex>

template <typename K, typename V, typename Base = std::unordered_map<K, V>>
struct TSmap {
    mutable std::shared_mutex mt;
    Base um;
    TSmap() = default;
    
    TSmap(const TSmap& rhs) {
        std::shared_lock lg(rhs.mt);
        std::copy(rhs.um.begin(), rhs.um.end(), um.begin());
    }
    TSmap& operator=(const TSmap& rhs) {
        if (&rhs != this) {
            std::scoped_lock sl(rhs.mt, mt);
            um = rhs.um;
        }
        return *this;
    }

    TSmap(TSmap&& rhs) noexcept {
        std::unique_lock lock(rhs.mt);
        um = std::move(rhs.um);
    }

    TSmap& operator=(TSmap&& rhs) {
        if (&rhs != this) {
            std::scoped_lock sl(rhs.mt, mt);
            um = std::move(rhs.um);
            rhs.um.clear(); 
        }
        return *this;
    }

    V get(K key) const {
        std::shared_lock lock(mt);
        auto it = um.find(key);
        if (it != um.end()) {
            return it->second;
        }
        throw std::out_of_range("Key not found");
    }

    void set(K key, V value) {
        std::unique_lock lock(mt);
        um[key] = value;
    }

    void print() const {
        std::shared_lock lock(mt);
        for(auto& [key, value]: um) {
            std::cout << key << ":" << value << "\n";
        }
    }
};

int main() {
    TSmap<std::string, int> s;
    std::thread t1([&s](){
        for(int i = 0;i<10;++i) {
            s.set(std::to_string(i), i);
        }
    });

    std::thread t2([&s](){
        for(int i = 10;i<20;++i) {
            s.set(std::to_string(i), i);
        }
    });

    t1.join();
    t2.join();

    s.print();
}
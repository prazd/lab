#include <iostream>
#include <utility>
#include <cassert>
#include <forward_list>
#include <array>
#include <vector>

template <typename T, size_t pool_sz>
class SingleBlockPoolAllocator{
    static constexpr size_t to_power_of_two(unsigned val) {
        if (!(val & val - 1)) return val;
        size_t count = 0;
        while(val){val >>= 1; ++count;}
        return (1 << count);
    }
    static constexpr size_t block_sz = sizeof(T) < 8 ? 8 : to_power_of_two(sizeof(T));
    
    struct alignas(block_sz) Entry {
        Entry* next = nullptr;
    };
    static_assert(sizeof(Entry) == block_sz);

    unsigned char* begin{};
    unsigned char* end{};
    Entry* next_entry{};

    void init(){
        auto sz = pool_sz*block_sz;
        begin = (unsigned char*)malloc(sz);
        end = begin + sz;
        for (auto it = (Entry*)begin, ite = (Entry*)(end); it != ite; ++it){
            if (std::next(it) != ite){
                it->next = it + 1;
            }
        }
        next_entry = (Entry*)begin;
    }

public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template <typename U>
    struct rebind {
        using other = SingleBlockPoolAllocator<U, pool_sz>;
    };
    SingleBlockPoolAllocator(){init();}

    // TODO: ownership model for copy constructor/assignment
    template<typename U>
    bool operator==(const U& other) {
        return other.begin == begin;
    }

    T* allocate(size_t n) {
        assert(n == 1);
        if (!next_entry) throw std::bad_alloc();
        auto next = next_entry->next;
        auto entry = std::exchange(next_entry, next);
        return (T*)entry;
    }

    void deallocate(void* addr){
        auto addr_uc = (unsigned char*)addr;
        if (addr_uc < begin || addr_uc >= end) return;
        auto addr_e = (Entry*)addr;
        addr_e->next = next_entry;
        next_entry = addr_e;
    }

    void deallocate(void* addr, size_t n){
        assert(n == 1);
        auto addr_uc = (unsigned char*)addr;
        if (addr_uc < begin || addr_uc >= end) return;
        auto addr_e = (Entry*)addr;
        addr_e->next = next_entry;
        next_entry = addr_e;
    }

    template <typename U, typename ... Args>
    void construct(U* p, Args&& ... args){
        std::construct_at(p, std::forward<Args>(args)...);
    }

    template <typename U>
    void destruct(U* p){
        std::destroy_at(p);
    }
    ~SingleBlockPoolAllocator(){free(begin);}
};

struct S {
    int x{};
    float y{};
    std::vector<int> z{};
    S(int x, float y): x(x + 1), y(y + 1.f), z(x, x){}
};

bool operator==(const S& lhs, const S& rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
};

int main(){
    std::forward_list<S, SingleBlockPoolAllocator<S, 3>> fl;
    std::array<S, 3> expected{S{30,30.3f},S{20,20.2f},S{10,10.1f}};
    fl.push_front(S{10,10.1f});
    fl.push_front(S{20,20.2f});
    fl.push_front(S{30,30.3f});
    auto it = fl.begin();
    for (int i = 0; i < 3; ++i) assert(expected[i] == *it++);
}
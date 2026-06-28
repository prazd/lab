#include <iostream>
#include <utility>
#include <cassert>
#include <forward_list>
#include <array>
#include <vector>
#include <memory>

constexpr size_t BLOCK_SZ = 64;

struct MemoryResource {
    struct alignas(BLOCK_SZ) Entry {
        Entry* next = nullptr;
    };
    static_assert(sizeof(Entry) == BLOCK_SZ);
    Entry buffer[128];
    Entry* const end = buffer + 128;
    Entry* next_entry{};
};

template <typename T>
class SingleBlockPoolAllocator{
    using Entry = MemoryResource::Entry;
    std::shared_ptr<MemoryResource> mr{};
    
    void init(){
        mr = std::make_shared<MemoryResource>();
        for (auto it = mr.get()->buffer, ite = mr->end; it != ite; ++it){
            if (std::next(it) != ite){
                it->next = it + 1;
            }
        }
        mr->next_entry = mr.get()->buffer;
    }
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    auto get_memory_res() const {
        return mr;
    }

    template <typename U>
    struct rebind {
        using other = SingleBlockPoolAllocator<U>;
    };
    SingleBlockPoolAllocator(){init();}

    template <typename U>
    SingleBlockPoolAllocator(const SingleBlockPoolAllocator<U>& other): 
        mr(other.get_memory_res()){}

    template<typename U>
    bool operator==(const SingleBlockPoolAllocator<U>& other) {
        return std::is_same_v<U,T> && mr == other.mr;
    }

    T* allocate(size_t n) {
        assert(n == 1);
        if (!mr->next_entry) throw std::bad_alloc();
        auto next = mr->next_entry->next;
        auto entry = std::exchange(mr->next_entry, next);
        return (T*)entry;
    }

    void deallocate(void* addr){
        auto addr_uc = (unsigned char*)addr;
        if (addr_uc < (unsigned char*)mr.get()->buffer || addr_uc >= (unsigned char*)mr->end) return;
        auto addr_e = (Entry*)addr;
        addr_e->next = mr->next_entry;
        mr->next_entry = addr_e;
    }

    void deallocate(void* addr, size_t n){
        assert(n == 1);
        auto addr_uc = (unsigned char*)addr;
        if (addr_uc < (unsigned char*)mr.get()->buffer || addr_uc >= (unsigned char*)mr->end) return;
        auto addr_e = (Entry*)addr;
        addr_e->next = mr->next_entry;
        mr->next_entry = addr_e;
    }

    template <typename U, typename ... Args>
    void construct(U* p, Args&& ... args){
        std::construct_at(p, std::forward<Args>(args)...);
    }

    template <typename U>
    void destruct(U* p){
        std::destroy_at(p);
    }
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
    std::forward_list<S, SingleBlockPoolAllocator<S>> fl;
    std::array<S, 3> expected{S{30,30.3f},S{20,20.2f},S{10,10.1f}};
    fl.push_front(S{10,10.1f});
    fl.push_front(S{20,20.2f});
    fl.push_front(S{30,30.3f});
    auto it = fl.begin();
    for (int i = 0; i < 3; ++i) assert(expected[i] == *it++);

    std::forward_list<S, SingleBlockPoolAllocator<S>> fl2 = fl;
    fl2.push_front(S{40,40.4f});
    std::array<S, 4> expected2{S{40,40.4f},S{30,30.3f},S{20,20.2f},S{10,10.1f}};

    it = fl.begin();
    for (int i = 0; i < 3; ++i) assert(expected[i] == *it++);
    it = fl2.begin();
    for (int i = 0; i < 4; ++i) assert(expected2[i] == *it++);

    std::forward_list<S, SingleBlockPoolAllocator<S>> fl3;
    fl3 = fl2;
    it = fl3.begin();
    for (int i = 0; i < 4; ++i) assert(expected2[i] == *it++);
}
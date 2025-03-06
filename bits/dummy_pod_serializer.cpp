#include <iostream>
#include <cassert>

template <typename T>
struct PODSerializer {
    static T to_big_endian(const T& value) {
        if constexpr (std::endian::native == std::endian::big)  {
            return value;
        } else {
            auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
            std::reverse(bytes.begin(), bytes.end());
            return std::bit_cast<T>(bytes);
        }
    }

    static auto to_bytes(const T& v) {
        T be_v = to_big_endian(v);
        return std::bit_cast<std::array<std::byte, sizeof(T)>>(be_v);
    }

    static T from_bytes(std::array<std::byte, sizeof(T)> arr) {
        if constexpr (std::endian::native != std::endian::big) {
            std::reverse(arr.begin(), arr.end());
        }
        return std::bit_cast<T>(arr);
    }
};

#pragma pack(push, 1)
struct Packet { uint32_t id; float x, y, z; };
#pragma pack(pop)

int main() {
    auto bytes = PODSerializer<Packet>::to_bytes(Packet{1, 1.5f, -2.3f, 4.1f});
    auto p2 = PODSerializer<Packet>::from_bytes(bytes);
    assert(p2.id == 1);
    assert(p2.x == 1.5f);
    assert(p2.y == -2.3f);
    assert(p2.z == 4.1f);
}
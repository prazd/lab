#include <iostream>
#include <cassert>

namespace func {

    template <typename T>
    class Monadic {
        T data;
    public:
        Monadic(): data(T{}){};
        Monadic(T&& data): data{std::forward<T>(data)} {}
        Monadic<T>& set(T&& rhs) {
            data = std::forward<T>(rhs);
            return *this;
        }
        template <typename U, typename ...Args>
        decltype(auto) and_then(U func, Args&&... arg) {
            return func(*this, std::forward<Args>(arg)...);
        }
        const T& get_data() const {
            return data;
        }
        operator T() {
            return data;
        }
    };

    // Comptime
    template <typename T>
    class ComptimeMonadic {
            T data;
        public:
            constexpr ComptimeMonadic(T&& data): data{std::forward<T>(data)} {}
            template <typename U, typename ...Args>
            constexpr decltype(auto) and_then(U func, Args&&... arg) {
                return func(*this, std::forward<Args>(arg)...);
            }
            constexpr const T& get_data() const {
                return data;
            }
            constexpr operator T() const {
                return data;
            }
    };

    template <typename T>
    constexpr ComptimeMonadic<T> addc(const ComptimeMonadic<T>& x, T&& y) {
        return ComptimeMonadic<T>{x.get_data() + std::forward<T>(y)};
    }

    template <typename T>
    constexpr ComptimeMonadic<T> subc(const ComptimeMonadic<T>& x, T&& y) {
        return ComptimeMonadic<T>{x.get_data() - std::forward<T>(y)};
    }

    template <typename T>
    Monadic<T> add(const Monadic<T>& x, T&& y) {
        return Monadic<T>{x.get_data() + std::forward<T>(y)};
    }

    template <typename T>
    Monadic<T> sub(const Monadic<T>& x, T&& y) {
        return Monadic<T>{x.get_data() - std::forward<T>(y)};
    }

};

int main() {
    constexpr auto resc = func::ComptimeMonadic<int>{30}
    .and_then(func::addc<int>, 50)
    .and_then(func::subc<int>, 10)
    .and_then(func::addc<int>, func::ComptimeMonadic<int>{40});
    static_assert(resc.get_data() == 110);

    auto res = func::Monadic<int>{}
     .set(30)
     .and_then(func::add<int>, 50)
     .and_then(func::sub<int>, 10)
     .and_then(func::add<int>, func::Monadic<int>{40});
    
    assert(res.get_data() == 110);
}
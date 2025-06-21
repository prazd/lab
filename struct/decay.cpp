#include <type_traits>
#include <string>
template <typename T>
struct Decay {
private:    
    using U = std::remove_reference_t<T>;
public:
    using type = std::conditional_t<
                    std::is_array_v<U>,
                    std::add_pointer_t<std::remove_extent_t<U>>,
                    std::conditional_t<
                        std::is_function_v<U>,
                        std::add_pointer_t<U>,
                        std::remove_cv_t<U>
                     >
                >;
};

template <typename T>
using decay = Decay<T>::type;

int main() {
    static_assert(std::is_same<std::decay_t<int&&>, decay<int&&>>());
    static_assert(std::is_same<std::decay_t<void()>, decay<void()>>());
    static_assert(std::is_same<std::decay_t<const std::string&>, decay<const std::string&>>());
    static_assert(std::is_same<std::decay_t<int[10]>, decay<int[10]>>());   
}

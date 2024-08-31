#include <iostream>

// name address status
using UserInfo = std::tuple<std::string, std::string, size_t>;

// unscoped enum
enum UInfo {Name, Address, Status};

// enum class
enum class UserData{Name, Address, Status};

template <typename E>
constexpr auto toUType(E enumerator) noexcept {
    return static_cast<std::underlying_type_t<E>>(enumerator);
}

int main() {
    UserInfo a("alex", "123", 100);
    std::cout << std::get<Name>(a) << std::endl;

    std::cout << std::get<toUType(UserData::Name)>(a) << std::endl;


    /// BTW
    /// Кастование к enum class с использованием static_cast не проверяет, 
    /// попадает ли значение в допустимый диапазон перечисления, и это может привести к возникновению 
    /// переменных с недопустимыми значениями.
    /// Поэтому важно самостоятельно проверять валидность таких значений или использовать функции для безопасного преобразования.
    auto x = static_cast<UserData>(3);
    std::cout << typeid(x).name() << std::endl;

}
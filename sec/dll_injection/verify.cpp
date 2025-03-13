#include <string_view>
bool verify_password(std::string_view pass) {
    return pass == "123";
}
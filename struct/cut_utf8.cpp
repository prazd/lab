#include <string_view>
// UTF8 (1-4 bytes)
// 0xxxxxxx
// 110xxxxx 10xxxxxx
// 1110xxxx 10xxxxxx 10xxxxxx
// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
consteval std::string_view cut_utf_8(std::string_view str, int n) {
    size_t offset = 0;
    for (int i = 0; i < n; ++i) {
        auto cur = static_cast<unsigned char>(*(str.data() + offset));
        if (!(cur & 0x80))
            ++offset;
        else {
            if ((cur & 0xe0) == 0b1100'0000)      offset += 2;
            else if ((cur & 0xf0) == 0b1110'0000) offset += 3;
            else                                  offset += 4;
        }
    }
    return {str.data() + offset};
}

int main() {
    constexpr auto res = cut_utf_8("!!!ЮТФ8", 3);
    static_assert(res == std::string_view{"ЮТФ8"});
}
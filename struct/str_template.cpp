#include <iostream>
#include <algorithm>
#include <cassert>
#include <bitset>

template <size_t n, char find_sym>
class StrTemplate {
    char buffer[n]{};
    std::bitset<n> temp_pos{};

public:
    constexpr StrTemplate(std::string_view sv){
        std::copy(sv.begin(), sv.end(), buffer);
        buffer[n-1] = '\0';
        for (size_t i = 0; i < n; ++i)
            temp_pos[i] = (buffer[i] == find_sym);
    }

    constexpr void update(const std::initializer_list<std::string_view>& sv_list){
        size_t current_sv = 0;
        size_t sv_char_index = 0;
        for (size_t i = 0; i < n; ++i){
            if (current_sv == sv_list.size()) break;
            if (temp_pos[i]){
                buffer[i] = (*(sv_list.begin() + current_sv))[sv_char_index];
                bool sv_end = ((sv_char_index + 1) == (*(sv_list.begin() + current_sv)).size());
                current_sv += sv_end;
                sv_char_index = sv_end ? 0 : sv_char_index + 1;
            }
        }
    }

    constexpr void update_and_clear(const std::initializer_list<std::string_view>& sv_list){
        update(sv_list);
        clear_temp_chars();
    }

    constexpr void clear_temp_chars() {
        for (size_t i = 0; i < n; ++i)
            buffer[i] = (buffer[i] == find_sym) ? ' ' : buffer[i];
    }

    constexpr void reset() {
         for (size_t i = 0; i < n; ++i){
            if (temp_pos[i]) buffer[i] = find_sym;
        }
    }
    constexpr std::string_view to_sv() const {
        return std::string_view{buffer, n-1};
    }
    constexpr char* data(){return buffer;}
    constexpr char* begin(){return buffer;}
    constexpr char* end(){return buffer + n;}
};

consteval void test(){
    StrTemplate<12, '$'> st("Number: $$$");
    st.update({"123"});
    assert(std::string_view("Number: 123") == st.to_sv());

    st.update({"4","5","6"});
    assert(std::string_view("Number: 456") == st.to_sv());
    
    st.reset();
    assert(std::string_view("Number: $$$") == st.to_sv());

    st.update({"456"});
    assert(std::string_view("Number: 456") == st.to_sv());
    
    st.reset();
    st.update({"42"});
    st.clear_temp_chars();
    assert(std::string_view("Number: 42 ") == st.to_sv());

    st.update({"789"});
    assert(std::string_view("Number: 789") == st.to_sv());

    
    StrTemplate<16, '$'> st2("Number: $$$'$$$");
    st2.update({"123", "456"});
    assert(std::string_view("Number: 123'456") == st2.to_sv());

    st2.update({"4","5","6","7","8","9"});
    assert(std::string_view("Number: 456'789") == st2.to_sv());

    st2.reset();
    st2.update_and_clear({"12", "34"});
    auto expected = std::string_view{"Number: 123'4  "};
    assert(std::equal(st2.begin(), st2.end(), expected.begin()));
}

int main(){
    test();
}
#include <iostream>
#include <cassert>

template<typename T, size_t max_val>
struct MapNum {
    const T* const it_arr;
    size_t indexes[max_val + 1]{};
    constexpr MapNum(auto it_arr, auto it_end, auto it_i): it_arr(it_arr){
        for (size_t i = 0; it_arr != it_end; ++it_arr, ++i) indexes[*(it_i + i)] = i;
    }
    constexpr const auto& operator[](size_t index) const {
        return *(it_arr + (indexes[index]));
    }
};

enum Args {A=0x2,B=0x5,C=0x10,D=0x14,ArgsCount};
constexpr std::string_view args_str[ArgsCount] = {"A","B","C","D"};
constexpr size_t args[ArgsCount] = {A,B,C,D};

consteval void test() {
    MapNum<std::string_view, D> arg_map(args_str, args_str + ArgsCount, args);
    assert(arg_map[0x2] == std::string_view{"A"});
    assert(arg_map[0x5] == std::string_view{"B"});
    assert(arg_map[0x10] == std::string_view{"C"});
    assert(arg_map[0x14] == std::string_view{"D"});
}

int main(){
    test();
}
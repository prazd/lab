#ifndef MAPNUM_HPP
#define MAPNUM_HPP
template<typename T, size_t max_val>
struct MapNum {
    const T* const it_arr;
    size_t indexes[max_val + 1]{};
    constexpr MapNum(auto it_arr, auto it_end, auto it_i): it_arr(it_arr){
        for (size_t i = 0; it_arr != it_end; ++it_arr, ++i) {indexes[*(it_i + i)] = i;}
    }
    constexpr const auto& operator[](size_t index) const {
        return *(it_arr + (indexes[index]));
    }
};
#endif
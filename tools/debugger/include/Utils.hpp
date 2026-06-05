#ifndef FILES_HPP
#define FILES_HPP
#include <vector>
#include <filesystem>
#include <sstream>

#define PRINT(s)     std::cout << std::hex << #s << ":" << s << '\n';
#define PRINT_INT(s) std::cout << std::hex << #s << ":" << int(s) << '\n';

template <typename T, typename U>
T parse_to(U ptr){
    T res;
    memcpy(&res, ptr, sizeof(T));
    return res;
}

inline auto read_file(const char* filepath){
    std::vector<char> res;
    size_t f_size = std::filesystem::file_size(filepath);
    res.resize(f_size);
    auto f = std::ifstream(filepath, std::ios::binary);
    if (!f.is_open()) std::abort();
    f.read(res.data(), f_size);
    return res;
}
inline void print_hex(const char* it, size_t sz){
    std::cout << "\n==========================\n";
    std::stringstream val_ss;
    std::stringstream ascii_ss;
    std::string val_s;
    std::string ascii_s;
    size_t i = 0;
    for (auto e = it + sz; it != e; ++i, ++it){
        val_ss << std::hex << std::setw(2) << std::setfill('0')  <<  int((unsigned char)*it) << ' ';
        
        if (auto val = *it; val <= 32)ascii_ss << '.'; else ascii_ss << val;
        if ((i & 0xf) == 0xf){
            val_s = val_ss.str();
            std::cout << val_s << '|';
            ascii_s = ascii_ss.str();
            std::cout << ascii_s << '\n';
            ascii_ss.str("");
            val_ss.str("");
        }
    }
    std::cout << "\n==========================\n";
}
template <typename T, std::forward_iterator It>
inline constexpr auto read(It& it){
    T res{};
    memcpy(&res, it, sizeof(T));
    it+=sizeof(T);
    return res;
}

template <std::forward_iterator It>
inline std::string read_str(It& it){
    std::string res{};
    while(it) res += *it++;
    ++it;
    return res;
}
#endif
#ifndef FILES_HPP
#define FILES_HPP
#include <vector>
#include <filesystem>
#include <sstream>
#include <charconv>

#define PRINT(s)     std::cout << std::hex << #s << ":" << s << '\n';
#define PRINT_INT(s) std::cout << std::hex << #s << ":" << int(s) << '\n';
#define PRINT_ERRNO(s) std::cout << s << strerror(errno) << '\n';  
#define FATAL_ERRNO(s) PRINT_ERRNO(s) std::abort();

template <typename T, typename U>
T parse_to(U ptr){
    T res;
    memcpy(&res, ptr, sizeof(T));
    return res;
}

inline auto read_file(const std::string& str){
    std::vector<char> res;
    size_t f_size = std::filesystem::file_size(str.c_str());
    res.resize(f_size);
    auto f = std::ifstream(str.c_str(), std::ios::binary);
    if (!f.is_open()) std::abort();
    f.read(res.data(), f_size);
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

inline auto read_file_str(const char* filepath){
    std::vector<std::string> res;
    std::string tmp;
    auto f = std::ifstream(filepath);
    while(std::getline(f, tmp)){
        tmp += '\n';
        res.push_back(tmp);
        tmp.clear();
    }
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

std::vector<std::string_view> split_str(const std::string_view s, char delimiter){
    std::vector<std::string_view> res;
    auto s_pos = s.begin();
    for (size_t i = 0, e = s.size(); i != e; ++i){
        if (s[i] == delimiter || i + 1 == e){
            res.push_back(std::string_view{s_pos, s.begin() + i});
            while(s[i] == delimiter) ++i;
            s_pos = s.begin() + i;
        }
    }
    return res;
}

std::pair<size_t,size_t> parse_hex_to_u_pair(std::string_view sv, char delimiter){
    std::string_view f;
    std::string_view s;
    for (size_t i = 0, e = sv.size(); i != e; ++i){
        if (sv[i] == delimiter){
            f = std::string_view{sv.begin(), sv.begin() + i};
            s = std::string_view{sv.begin() + i+1, sv.end()};
            break;
        }
    }
    size_t f_u{};
    size_t s_u{};
    if (auto [ptr, ec] = std::from_chars(f.data(), f.data() + f.size(), f_u, 16); ec != std::errc{}){
        std::cout << "parse_hex_to_u_pair error\n";
        f_u = 0;
    }
    if (auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), s_u, 16); ec != std::errc{}){
        std::cout << "parse_hex_to_u_pair error\n";
        s_u = 0;
    }
    return std::pair(f_u, s_u);
}
#endif
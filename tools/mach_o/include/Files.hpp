#ifndef FILES_HPP
#define FILES_HPP
#include <vector>
#include <filesystem>
inline auto read_file(const char* filepath){
    std::vector<char> res;
    size_t f_size = std::filesystem::file_size(filepath);
    res.resize(f_size);
    auto f = std::ifstream(filepath, std::ios::binary);
    if (!f.is_open()) std::abort();
    f.read(res.data(), f_size);
    return res;
}
#endif
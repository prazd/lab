#ifndef NES_SHADERS_HPP
#define NES_SHADERS_HPP
#include <vector>
#include <string_view>
#include "Constants.hpp"
#include "raylib.h"
struct Shaders {
    struct Data {
        Shader s{};
        int resolutionLoc{};
        int timeLoc{};
    };
    size_t current = 0;
    std::vector<const char*> paths;
    std::vector<Data> s_data;
    Shaders(std::initializer_list<const char*> paths): paths(paths){
        Data d;
        s_data.reserve(paths.size());
        for (const auto& p: paths){
            d.s = LoadShader(0, p);
            d.timeLoc = GetShaderLocation(d.s, "iTime");
            d.resolutionLoc = GetShaderLocation(d.s, "iResolution");
            s_data.push_back(d);
        }
    }
    inline Data get() const {
        return s_data[current];
    }
    inline void handle(){
        if (IsKeyPressed(KEY_V))
            current = (current + 1) % s_data.size();
    }
};
#endif //NES_SHADERS_HPP

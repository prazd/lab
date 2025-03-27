#ifndef PACMAN_MAP_H
#define PACMAN_MAP_H
#include <array>
#include <string_view>
#include "raylib.h"
#include "Constants.h"
class Map {
public:
   static inline constexpr std::array<std::string_view, 15> raw = {
            "##############################",
            "####.......................###",
            "####.######.##.###.#######.###",
            "####.p#####.##.###.#####@..###",
            "####.######.##.###.#######.###",
            "####.######.##.###.#######.###",
            "####.######.##.###.#######.###",
            "####.###.##.##.###.##.####.###",
            "####.###.##.##.###.##.####.###",
            "####.###.##.##.###.##.####.###",
            "####.###.##.##.###.##.####.###",
            "####.###.##.##.###.##.####.###",
            "####.###.##.##.###.##.####.###",
            "####.......................###",
            "##############################"
   };

    template <char... sym>
    static inline constexpr size_t count() {
        size_t res{};
        for (auto&& row: raw){
            res += std::count_if(std::begin(row), std::end(row), [](auto el){return ((el == sym) || ...);});
        }
        return res;
    }

    template <typename Func>
    static constexpr void call(Func&& f) {
        size_t i{};
        size_t coords_i{};
        for (auto&& row: raw) {
            for(auto el: row) {
                f(i, coords_i, el);
                ++coords_i;
            }
        }
    }

};
#endif //PACMAN_MAP_H

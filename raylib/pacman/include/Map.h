#ifndef PACMAN_MAP_H
#define PACMAN_MAP_H
#include <array>
#include <string_view>
#include "raylib.h"
#include "Constants.h"
struct Map {
    static inline constexpr std::array<std::string_view, 15> source = {
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
        for (auto&& row: Map::source){
            res += std::count_if(std::begin(row), std::end(row), [](auto el){return ((el == sym) || ...);});
        }
        return res;
    }

    template <typename Func>
    static constexpr void call(Func&& f) {
        size_t i{};
        size_t coords_i{};
        for (auto&& row: Map::source) {
            for(auto el: row) {
                f(i, coords_i, el);
                ++coords_i;
            }
        }
    }

    static constexpr auto get_coords() {
        std::array<Vector2, Map::source.size() * Map::source[0].size()> res{};
        float x = 0.f;
        float y = 0.f;
        size_t i{};
        for (auto&& raw: source) {
            for (auto el: raw) {
                res[i++] = {x, y};
                x += gc::OFFSET;
            }
            x = 0;
            y += gc::OFFSET;
        }
        return res;
    }

};

struct Coords {
    static inline constexpr auto points = Map::get_coords();

    template <char s>
    static constexpr auto get_points_arr() {
        std::array<Vector2, Map::count<s>()> res{};
        Map::call([&res](auto& i, auto& coords_i, auto el){
            if (el == s) {
                res[i++] = points[coords_i];
            }
        });
        return res;
    }

    static constexpr Vector2 get_point(char p) {
        Vector2 res{};
        Map::call([&res, p](auto, auto coord_i, auto el){
            if (el == p) {
                res = Coords::points[coord_i];
            }
        });
        return res;
    }
};

struct Walls {
    static inline auto points = Coords::get_points_arr<'#'>();
};

struct CoinsMap{
    size_t collected;
    static inline std::array<std::pair<std::pair<float, float>, bool>, Map::count<'.'>()> coins{};
    CoinsMap();
    bool collect_all() const;
    void reset();
};

#endif //PACMAN_MAP_H

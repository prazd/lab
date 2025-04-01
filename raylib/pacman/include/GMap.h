#ifndef PACMAN_GMAP_H
#define PACMAN_GMAP_H
#include <array>
#include "raylib.h"
#include "Constants.h"
#include <optional>
#include <random>
#include <ranges>

enum Cells: char {Empty = '.', Wall = '#', PSpawn = 'p', GSpawn = '@'};

class GMap {
public:
    static inline std::array<std::array<char, 30>, 15> source;

    static inline void reset() {
        for (auto&& raw: source){
            for (auto& el: raw) {
                el = Cells::Wall;
            }
        }
    }

    template <char... sym>
    static inline size_t count() {
        size_t res{};
        for (auto&& row: source){
            res += std::count_if(std::begin(row), std::end(row), [](auto el){return ((el == sym) || ...);});
        }
        return res;
    }

    template <typename Func>
    static inline void call(Func&& f) {
        size_t i{};
        size_t coords_i{};
        for (auto&& row: source) {
            for(auto el: row) {
                f(i, coords_i, el);
                ++coords_i;
            }
        }
    }

    static inline auto get_coords() {
        std::array<Vector2, 30 * 15> res{};
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

struct GCoords {
    static inline auto points = GMap::get_coords();

    static constexpr Vector2 get_point(char p) {
        Vector2 res{};
        GMap::call([&res, p](auto, auto coord_i, auto el){
            if (el == p) {
                res = GCoords::points[coord_i];
            }
        });
        return res;
    }
};


struct GWalls {
    static inline std::array<std::optional<Vector2>, 30 * 15> points;
    static inline void update() {
        GMap::call([](auto& i, auto& coords_i, auto el){
            if (el == Cells::Wall) {
                GWalls::points[i++] = GCoords::points[coords_i];
            } else {
                GWalls::points[i++] = {};
            }
        });
    }
};

struct GCoinsMap{
    static inline size_t collected = 0;
    static inline std::array<std::optional<std::pair<std::pair<float, float>, bool>>, 30 * 15> coins{};
    static void seed();
    static bool collect_all();
    static void reset();
};

struct MapGenerator {
    static inline std::pair<int, int> directions[4] = {{2, 0}, {-2, 0}, {0,2}, {0,-2}};
    static inline auto rd = std::mt19937(std::random_device()());

    static inline void gs(int x, int y, int xstart = 0, int ystart = 0, int w = 30, int h = 15) {
        GMap::source[y][x] = Cells::Empty;
        std::shuffle(std::begin(directions), std::end(directions), rd);
        for (auto [dx, dy]: directions) {
            auto nx = dx + x, ny = dy + y;
            if (nx >= xstart && nx < w && ny >= ystart && ny < h && GMap::source[ny][nx] == Cells::Wall) {
                GMap::source[dy/2+y][dx/2 + x] = Cells::Empty;
                gs(nx, ny, xstart, ystart, w, h);
            }
        }
    }

    // todo: update
    static inline void gen() {
        gs( 4,2,4,2, 26, 14);
        for (auto &&raw: GMap::source) {
            auto f = std::find(std::begin(raw), std::end(raw), Cells::Empty);
            if (f != std::end(raw)){
                *f = Cells::PSpawn;
                break;
            }
        }

        for (auto& rit : std::ranges::reverse_view(GMap::source)) {
            auto f = std::find(std::begin(rit), std::end(rit), Cells::Empty);
            if (f != std::end(rit)) {
                *f = Cells::GSpawn;
                break;
            }
        }
    }
};
#endif //PACMAN_GMAP_H

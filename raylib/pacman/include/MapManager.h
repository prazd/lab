#ifndef PACMAN_MAPMANAGER_H
#define PACMAN_MAPMANAGER_H
#include "GMap.h"
#include "Map.h"
#include <algorithm>

enum Mode: char {Classic, Maze};

struct MapManager {
    static inline Mode current = Classic;

    static inline auto get_source() {
        switch (current) {
            case Classic: return Map::source;
            case Maze: return GMap::source;
        }
    }

    static inline auto count(auto s)  {
        switch (current) {
            case Classic: return Map::count<s>();
            case Maze: return GMap::count<s>();
        }
    }

    static inline auto call(auto f)  {
        switch (current) {
            case Classic: return Map::call(f);
            case Maze: return GMap::call(f);
        }
    }

    static inline auto get_coords()  {
        switch (current) {
            case Classic: return Map::get_coords();
            case Maze: return GMap::get_coords();
        }
    }

    static inline auto coord_point(char p)  {
        switch (current) {
            case Classic: return Coords::get_point(p);
            case Maze: return GCoords::get_point(p);
        }
    }

    static inline auto get_coords_raw()  {
        switch (current) {
            case Classic: return Coords::points;
            case Maze: return GCoords::points;
        }
    }

    static bool inline walls_any_of(auto&& func) {
        switch (current) {
            case Classic:{
                return std::any_of(std::begin(Walls::points), std::end(Walls::points), [func](auto& el){
                   auto [x,y] = el;
                   return func(x,y);
                });
            }
            case Maze:{
                return std::any_of(std::begin(GWalls::points), std::end(GWalls::points), [func](auto& el){
                    if (!el.has_value()) {
                        return false;
                    }
                    auto [x,y] = el.value();
                    return func(x,y);
                });
            }
        }
    }

    static void inline for_each_coins(auto&& func) {
        switch (current) {
            case Classic:{
                std::for_each(std::begin(CoinsMap::coins), std::end(CoinsMap::coins), [func](auto& el){
                    func(el);
                });
                break;
            }
            case Maze:{
                std::for_each(std::begin(GCoinsMap::coins), std::end(GCoinsMap::coins), [func](auto& el){
                    if (!el.has_value()){
                        return;
                    }
                    func(el.value());
                });
                break;
            }
        }
    }

    static void set_mode(Mode m) {
        switch (m) {
            case Maze: {
                GMap::reset();
                MapGenerator::gen();
                GWalls::update();
                GCoinsMap::reset();
                break;
            }
            default:
                CoinsMap::collected = 0;
                CoinsMap::reset();
                break;
        }
        current = m;
    }

    static inline auto& get_coins_collected() {
        switch (current) {
            case Classic:{
                return CoinsMap::collected;
            }
            case Maze:{
                return GCoinsMap::collected;
            }
        }
    }

};
#endif

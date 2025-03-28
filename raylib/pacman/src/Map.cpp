#include "Map.h"

CoinsMap::CoinsMap(): collected(0) {
    Map::call([](auto& i, auto coords_i, auto el){
        if (el == '.'){
            auto v = Coords::points[coords_i];
            coins[i++] = {{v.x+gc::COINS_OFFSET, v.y+gc::COINS_OFFSET}, true};
        }
    });
}

bool CoinsMap::collect_all() const {
    return collected == coins.size();
}

void CoinsMap::reset() {
    collected = 0;
    for (auto& el: coins) {
        auto& [p, active] = el;
        active = true;
    }
}




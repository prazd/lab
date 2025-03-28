#include "GMap.h"

GCoinsMap::GCoinsMap(): collected(0) {
    GMap::call([](auto& i, auto coords_i, auto el){
        if (el == Cells::Empty){
            auto v = GCoords::points[coords_i];
            coins[i++] = {{v.x+gc::COINS_OFFSET, v.y+gc::COINS_OFFSET}, true};
        } else {
            coins[i++] = {};
        }
    });
}

bool GCoinsMap::collect_all() const {
    return collected == coins.size();
}

void GCoinsMap::reset() {
//    collected = 0;
    GMap::call([](auto& i, auto coords_i, auto el){
        if (el == Cells::Empty){
            auto v = GCoords::points[coords_i];
            coins[i++] = {{v.x+gc::COINS_OFFSET, v.y+gc::COINS_OFFSET}, true};
        } else {
            coins[i++] = {};
        }
    });
    for (auto& el: coins) {
        if (el) {
            auto& [p, active] = el.value();
            active = true;
        }
    }
}
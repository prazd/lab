#include "Cheats.h"

void CheatCore::freeze_index(unsigned short index) {
    frozen_indexes.push_back(index);
}

void CheatCore::unfreeze_index(unsigned short index) {
    frozen_indexes.erase(std::remove(frozen_indexes.begin(), frozen_indexes.end(), index), frozen_indexes.end());
}

bool CheatCore::is_frozen(unsigned short index) {
    return std::any_of(frozen_indexes.begin(), frozen_indexes.end(), [index](auto el){return el == index;});
}

size_t CheatCore::size() {
    return frozen_indexes.size();
}

void CheatCore::clear() {
    frozen_indexes.clear();
}
#ifndef MAIN_CPP_CHEATS_H
#define MAIN_CPP_CHEATS_H
#include <vector>
struct CheatCore{
   static inline std::vector<unsigned short> frozen_indexes;
   static void freeze_index(unsigned short index);
   static void unfreeze_index(unsigned short index);
   static bool is_frozen(unsigned short index);
   static size_t size();
   static void clear();
};
#endif //MAIN_CPP_CHEATS_H
#ifndef PACMAN_RANDOM_H
#define PACMAN_RANDOM_H
#include <random>

template <typename T, int a = 0, int b = 0>
class Random{
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<T> dis = std::uniform_int_distribution<T>(a, b);
public:
    static int roll() {
        return dis(gen);
    }
    static int roll(int l, int r) {
        std::uniform_int_distribution<T> cdis = std::uniform_int_distribution<int>(l, r);
        return cdis(gen);
    }
};
#endif //PACMAN_RANDOM_H

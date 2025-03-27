#ifndef PACMAN_CONSTANTS_H
#define PACMAN_CONSTANTS_H
namespace gc {
    constexpr float WIDTH = 1200.f;
    constexpr float HEIGHT = 600.f;
    constexpr float OFFSET = 40.f;
    constexpr float R_PLAYER = 20.f;
    constexpr float P_CHECK_VIEW = 2.f;
    constexpr float PLAYER_VEL_X = 100.f;
    constexpr float PLAYER_VEL_Y = 100.f;
    constexpr float QUBE_SIDE = OFFSET - P_CHECK_VIEW;
    constexpr float FRAME_WIDTH = 40.f;
    constexpr float FRAME_HEIGHT = 40.f;
    constexpr float MONSTER_FRAME_HEIGHT = 40.f;
    constexpr float MONSTER_FRAME_WIDTH = 35.f;
    constexpr float FRAME_SZ = 40.f;
    constexpr float COINS_OFFSET = 15.f;
    constexpr float PLAYER_SPAWN_OFFSET_X = 21.f;
    constexpr float PLAYER_SPAWN_OFFSET_Y = 21.f;
    constexpr float MONSTER_VIEW_DISTANCE = 200.f;
}
#endif //PACMAN_CONSTANTS_H

#ifndef NES_GAMEGENIE_H
#define NES_GAMEGENIE_H
#include "Constants.hpp"
#if GAME_GENIE_MODE

//https://www.nesdev.org/wiki/Game_Genie
struct GameGenie {
    struct Entry {
        unsigned short address{};
        unsigned char value{};
        bool active = false;
        bool compare = false;
        unsigned char compare_value{};
    };
    Entry data[3]{};

    inline unsigned char index_by_address(unsigned short address) {
        if (address >= 0x8001 && address <= 0x8004)
            return 0;
        else if (address >= 0x8005 && address <= 0x8008)
            return 1;
        else
            return 2;
    }

    bool game_mode = false;
    bool started = false;

    inline int find_index(unsigned short address){
        int index = -1;
        for (int i = 0; i < 3; ++i){
            if (data[i].address == address && data[i].active){
                index = i;
                break;
            }
        }
        return index;
    };
    inline unsigned char get(int i, unsigned char original){
        if (data[i].compare && original != data[i].compare_value)
            return original;
        return data[i].value;
    };
    inline void handle(unsigned short address, unsigned char value){
        if (address == 0x8002 || address == 0x8006 || address == 0x800a) {
            auto i = index_by_address(address);
            data[i].address &= 0xff00;
            data[i].address |= value;
        } else if (address == 0x8001 || address == 0x8005 || address == 0x8009){
            auto i = index_by_address(address);
            data[i].address &= 0xff;
            data[i].address |= (0x8000 | (value << 8));
        } else if (address == 0x8004 || address == 0x8008 || address == 0x800c) {
            auto i = index_by_address(address);
            data[i].value = value;
        } else if (address == 0x8003 || address == 0x8007 || address == 0x800b){
            auto i = index_by_address(address);
            data[i].compare_value = value;
        } else if (address == 0x8000){
            if (game_mode) return;
            game_mode = value & 1;
            data[0].active =  !((value >> 4) & 1);
            data[0].compare = ((value >> 1) & 1);
            data[1].active = !((value >> 5) & 1);
            data[1].compare = ((value >> 2) & 1);
            data[2].active = !((value >> 6) & 1);
            data[2].compare = ((value >> 3) & 1);
        }
    };
};
#endif
#endif //NES_GAMEGENIE_H
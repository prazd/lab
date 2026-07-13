#ifndef NES_GAMEGENIE_HPP
#define NES_GAMEGENIE_HPP
#include "Constants.hpp"
#if GAME_GENIE_MODE
#include "Simple.hpp"

//https://www.nesdev.org/wiki/Game_Genie
struct GameGenie {
    struct Entry {
        unsigned short address{};
        unsigned char value{};
        bool active = false;
        bool compare = false;
        unsigned char compare_value{};
        std::string str;
        inline bool operator==(const Entry& rhs){
            return (address == rhs.address || (!str.empty() && !rhs.str.empty() && str == rhs.str));
        }
    };
    SimpleDeque<Entry,3> data{};

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
        for (int i = 0, e = data.size(); i != e; ++i){
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
        if (data.size() < 3) for (int i = 0; i < 3; ++i) data.push_back(Entry{});
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

    }
    // todo: finish
    enum Syms: unsigned char {
        A = 0x3, E, P, O, Z, X, L, U,
        G, K, I, S , T, V, Y, N, Unknown
    };

    static constexpr char syms_str[] = {
            'A', 'E', 'P', 'O', 'Z', 'X', 'L', 'U',
            'G', 'K', 'I', 'S' , 'T', 'V', 'Y', 'N'
    };

    static Syms get_syms_by_str(char str) {
        for (int i = 0, e = std::size(syms_str); i != e; ++i){
            if (str == syms_str[i]) return static_cast<Syms>(i + 3);
        }
        return Unknown;
    }

    // https://github.com/qalle2/nes-gg-disassembly/blob/main/prg.asm#L2692
    inline Entry custom_decode(auto& arr){
        static constexpr int code_keys[] = {3, 5, 2, 4, 1, 0, 7, 6};
        size_t sz = std::size(arr);
        unsigned char tmp = 0;
        bool carry = false;
//        ; Phase 1/2 of decoding: modify the letters of one code:
//        ; - if 0 (no letter), exit loop
//        ; - subtract 3 to get a 4-bit value
//        ; - shift right once
//        ; - copy least significant bit of previous letter to bit 3 of
//        ;   this letter
        for (auto& el: arr){
            el -= 3;
            el &= 0xf;
            carry = el & 1;
            el >>= 1;
            el |= tmp;
            tmp = 0;
            tmp |= (carry << 3);
        }
//        ; copy the bit that was shifted out from the last value,
//        ; to bit 3 of 1st value; thus, the values will have been
//        ; rotated instead of shifted (temp2 = LSB of previous letter)
        arr[0] = (arr[0] & 0x7) | tmp;

// ; Phase 2/2 of decoding: copy 8 nybbles from one semi-decoded
// ; code to 4 bytes in decoded_code according to code_key.
        unsigned char z[4]{};
        int nybble_offset = 0;
        tmp = 0;
        for (int i = 0; i < sz; i+=2){
            z[(i>>1)] = ((arr[code_keys[i]] << 4) | arr[code_keys[i+1]]);
        }
        z[0] &= 0x7f;

        return Entry{
            .address = (unsigned short)(0x8000 | ((z[0] << 8) | z[1])),
            .value = z[2],
            .active = true,
            .compare = sz == 8,
            .compare_value = z[3],
        };
    }
};
#endif
#endif //NES_GAMEGENIE_HPP
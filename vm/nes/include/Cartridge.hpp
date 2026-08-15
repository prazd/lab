#ifndef NES_CARTRIDGE_HPP
#define NES_CARTRIDGE_HPP
#include <iostream>
#include <fstream>
struct Cartridge {
    std::vector<unsigned char> buffer{};
    enum TimingMode: unsigned char{
        RP2C02, // NTSC NES
        RP2C07, // Licensed PAL NES
        MultipleRegion, //
        UA6538 // Dendy
    } timing_mode = RP2C02;

    enum Format: unsigned char {
        iNES, NES2_0
    } current_format = iNES;

    enum Flags: unsigned char {
        //  PRG-ROM size LSB
        Flag4 = 4,
        //  CHR-ROM size LSB
        Flag5 = 5,
        // Mapper, mirroring, battery, trainer
        Flag6 = 6,
        // Mapper, VS/Playchoice, NES 2.0
        Flag7 = 7,
        //  PRG-RAM size (rarely used extension)
        Flag8 = 8,
        // TV system (rarely used extension)
        Flag9 = 9,
        // TV system, PRG-RAM presence (unofficial, rarely used extension)
        Flag10 = 10,
        // CHR-RAM size
        Flag11 = 11,
        // CPU/PPU Timing
        Flag12 = 12,

        // TOOD: check
        Flag13 = 13,
        Flag14 = 14,
        Flag15 = 15
    };

    bool nes_eof_begin = false;
    bool is_battery = false;
    bool is_horizontal_arrangement = false;
    bool trainer_present = false;
    bool is_alternative_nametable = false;
    unsigned short mapper_num = 0;
    unsigned short submapper_num = 0;
    unsigned short prg_rom_sz = 0;
    unsigned short chr_rom_sz = 0;
    unsigned char prg_ram_sz = 0;
    bool is_prg_nvram = false;
    unsigned prg_nvram_sz = 0;
    unsigned char chr_ram_sz = 0;
    bool is_chr_nvram = false;
    unsigned chr_nvram_sz = 0;
    std::string rom_path{};
    std::string save_path = "/Users/prazd/Desktop/books/cpp/cpp/vm/nes/build/sav/";
    std::string selected_files_path = "/Users/prazd/Desktop/dev/";
    std::vector<std::string> select_files{};
    static constexpr size_t select_view_count = 10;
    int select_begin = 0;
    int select_end = select_view_count-1;

    bool selected = false;
    int selected_index = 0;

    inline void parse_header() {
        unsigned char nes_id = (buffer[Flag7] >> 2) & 0x3;
        nes_eof_begin = ((buffer[0] == 'N') && (buffer[1] == 'E') && (buffer[2] == 'S') && (buffer[3] == 0x1A));

        if (nes_id == 0b10 && nes_eof_begin)
            current_format = NES2_0;
        else if (nes_eof_begin)
            current_format = iNES;

        if (current_format == NES2_0 || current_format == iNES) {
            is_horizontal_arrangement = !((buffer[Flag6]) & 1);
            is_battery = (buffer[Flag6] >> 1) & 1;
            trainer_present = (buffer[Flag6] >> 2) & 1;
            is_alternative_nametable = (buffer[Flag6] >> 3) & 1;
        }

        switch (current_format) {
            case NES2_0: {
                mapper_num = ((buffer[Flag8] & 0xf) << 8) | (((buffer[Flag7] >> 4) & 0xf) << 4) | ((buffer[Flag6] >> 4) & 0xf);
                submapper_num = (buffer[Flag8] & 0xf0) >> 4;
                prg_rom_sz = (((buffer[Flag9]) & 0xf) << 8) | (buffer[Flag4]);
                chr_rom_sz = (((buffer[Flag9]) >> 4) & 0xf << 8) | (buffer[Flag5]);
                prg_ram_sz = (buffer[Flag10]) & 0xf;
                is_prg_nvram = ((buffer[Flag10]) >> 4) & 0xf;
                if (is_prg_nvram) prg_nvram_sz = (64 << (((buffer[Flag10]) >> 4) & 0xf));
                chr_ram_sz = (buffer[Flag11]) & 0xf;
                is_chr_nvram = ((buffer[Flag11]) >> 4) & 0xf;
                if (is_chr_nvram) chr_nvram_sz = 64 << (((buffer[Flag11]) >> 4) & 0xf);
                timing_mode = static_cast<TimingMode>(buffer[Flag12] & 0x3);
                break;
            }
            case iNES: {
                chr_rom_sz = buffer[Flag5];
                prg_rom_sz =  buffer[Flag4];
                mapper_num = (((buffer[Flag7] >> 4) & 0xf) << 4) | ((buffer[Flag6] >> 4) & 0xf);
                break;
            }
        }
    }

    void read(std::string_view rp){
        rom_path = rp;
        auto sz = std::filesystem::file_size(rom_path);
        buffer.resize(sz);
        auto fs = std::ifstream(rom_path, std::ios::binary);
        if (!fs.is_open()) fatal("ROM read error");
        fs.read(reinterpret_cast<char*>(buffer.data()),sz);
        parse_header();
    }

    std::string filename_from_path() const {
        auto it = rom_path.end() - 1;
        while(*std::prev(it) != '/') --it;
        return std::string(it, rom_path.end());
    }

    template <bool save>
    void update_game_state(char* buf){
        auto fname = filename_from_path();
        auto sz = fname.size();
        fname[sz-1] = 'v';
        fname[sz-2] = 'a';
        fname[sz-3] = 's';
        if constexpr (save){
            std::ofstream f(save_path + fname, std::ios::binary);
            if (!f.is_open()) {std::cout << "Save error\n"; return;}
            f.write(buf, 8192);
            f.close();
        } else {
            std::ifstream f(save_path + fname, std::ios::binary);
            if (!f.is_open()) {return;}
            f.read(buf, 8192);
            f.close();
        }
    }

    void update_select_files(){
        for (auto& entry: std::filesystem::directory_iterator(std::filesystem::path(selected_files_path))){
            auto str = entry.path().filename().string();
            if (str.contains(".nes")) select_files.push_back(str);
        }
    }
};
#endif
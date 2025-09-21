#include <iostream>
#include "raylib.h"
#include "PPU.h"
#include <queue>
#include <array>
#include "RAM.h"
#include "CPU.h"
#include "Joypad.h"
#include <filesystem>
#include <sstream>
#include <fstream>
#include <span>

#define DEBUG_MODE 1
constexpr size_t DEBUG_WIDTH = 1500;
constexpr size_t DEBUG_HEIGHT = 1500;

constexpr size_t LCD_WIDTH = 800;
constexpr size_t LCD_HEIGHT = 720;

constexpr size_t WIDTH = DEBUG_MODE ? DEBUG_WIDTH : LCD_WIDTH;
constexpr size_t HEIGHT = DEBUG_MODE ? DEBUG_HEIGHT: LCD_HEIGHT;

constexpr size_t DISPLAY_HEIGHT = 144;
constexpr size_t DISPLAY_WIDTH = 160;
constexpr double CPU_FREQUENCY = 4194304.0;
constexpr double CPU_CYCLE_TIME = 1/CPU_FREQUENCY;

using Tile = std::array<std::array<int, 8>, 8>;

struct DebugTools {
    static Tile get_tile_by_address(const RAM& ram, unsigned short address){
        std::array<unsigned char, 16> arr{};
        for (int i = 0; i < 16; ++i){
            arr[i] = ram.read_unsafe(address + i);
        }
        std::array<std::array<int, 8>, 8> colors{};
        std::array<int, 8> tmp{};
        size_t current{};
        for (size_t i = 0; i < 16; i+=2){
            auto l = arr[i];
            auto h = arr[i+1];
            for (int j = 0; j < 8; ++j) {
                auto lc = (l >> (7 - j)) & 1;
                auto hc = (h >> (7 - j)) & 1;
                tmp[j] = (hc << 1 | lc);
            }
            colors[current++] = tmp;
        }
        return colors;
    };

    template<size_t height, size_t width>
    static auto print_rects(Color c){
        std::array<Rectangle, height * width> res{};
        float x_pos = 0.0f;
        float y_pos = 0.0f;

        float step_x = LCD_WIDTH/width;
        float step_y = LCD_HEIGHT/height;
        int z = 0;
        for (int i = 0; i < height; ++i){
            for (int j = 0; j < width; ++j) {
                res[z++] = Rectangle{x_pos, y_pos, step_x, step_y};
                x_pos += step_x;
            }
            x_pos = 0;
            y_pos += step_y;
        }
        for (const auto& el: res){
            DrawRectangleLinesEx({el.x, el.y, el.width, el.height}, 1, c);
        }
    };

    static void print_segment(const RAM& ram, unsigned short address, int n) {
        std::cout << "\n=============================\n";
        for (int i = 0; i < n; ++i){
            std::cout << std::hex << address + i <<  ":" << (unsigned)ram.read_unsafe(address + i) << '\n';
        }
        std::cout << "\n=============================\n";
        std::cout << std::dec;
    }

    static void draw_n_tiles(const RAM& ram, unsigned short begin_address, Rectangle rec, int n_tiles) {
        std::stringstream ss;
        ss << "Address:" << std::hex << begin_address;
        auto str = ss.str();
        float x_begin = rec.x;
        float y_begin = rec.y;
        DrawRectangleLinesEx(rec, 1, BLACK);
        y_begin+=10.f;
        DrawText(str.c_str(), x_begin, y_begin, 30.f, BLACK);
        y_begin += 30.f;
        std::vector<Tile> vec{};
        for (int i = 0; i < n_tiles; ++i)
            vec.push_back(get_tile_by_address(ram, begin_address + (16 * i)));

        float xstep = (rec.width/20)/8;
        float ystep = (rec.height/20)/8;
        float x = x_begin;
        float y = y_begin;
        size_t count = 0;
        float current_x = x;
        float current_y = y;
        auto prev_x = x;
        for (auto& colors: vec) {
            for (auto&& row: colors){
                for (auto el: row) {
                    DrawRectangle(x, y, xstep, ystep, Pixel::get_pixel_color(el));
                    x+=xstep;
                }
                y += ystep;
                prev_x = std::exchange(x, current_x);
            }
            if ((prev_x + (xstep) >= rec.x + rec.width)){
                y = current_y = y + ystep;
                x = current_x = x_begin;
            } else {
                y = current_y;
                x = current_x = prev_x;
            }
        }
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct DebugWindow {
    using Buffer = std::array<std::array<Color, pwidth>, pheight>;
    Rectangle rec;
    Texture2D target;
    Buffer buffer{};
    std::string header;
    DebugWindow(float x, float y):
        rec({x, y, width, height}){

        Image img(GenImageColor(pwidth, pheight, WHITE));
        target = LoadTextureFromImage(img);
        UnloadImage(img);

        for (size_t i = 0; i < pheight; ++i) {
            for (size_t j = 0 ; j < pwidth; ++j) {
                buffer[i][j] = WHITE;
            }
        }
        UpdateTexture(target, buffer.data());
    }
    void update() {
        UpdateTexture(target, buffer.data());
    }
    void move_check() {
        if (CheckCollisionPointRec(GetMousePosition(), rec)){
            if (IsMouseButtonDown(0)) {
                rec.x = GetMousePosition().x - (width/2);
                rec.y = GetMousePosition().y - (height/2);
            }
        }
    }
    void draw(){
        move_check();
        DrawRectangleLinesEx(rec, 1, BLACK);
        DrawText(header.c_str(), rec.x, rec.y, 30.f, BLACK);
        DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{rec.x+1, rec.y+30, width-20, height-50}, {0, 0}, 0.0f, WHITE);
    }
    void draw_text(float font_sz) {
        move_check();
        DrawRectangleLinesEx(rec, 1, BLACK);
        DrawText(header.c_str(), rec.x, rec.y, font_sz, BLACK);
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height, size_t n_tiles>
struct TileWindow: public DebugWindow<pwidth, pheight, width, height>{
    using DW = DebugWindow<pwidth, pheight,width,height>;
    unsigned short base_address{};
    std::array<Tile, n_tiles> tiles;
    TileWindow(float x, float y, unsigned short base_address):
        DebugWindow<pwidth, pheight,width,height>(x, y),
        base_address(base_address){
    }
    void update_tiles(RAM& ram){
        std::stringstream ss;
        ss << "Tiles address:" << std::hex << base_address;
        auto str = ss.str();
        int x_begin = 0;
        int y_begin = 0;
        DW::header = ss.str();

        for (int i = 0; i < n_tiles; ++i)
            tiles[i] = DebugTools::get_tile_by_address(ram, base_address + (16 * i));

        int x = x_begin;
        int y = y_begin;
        size_t count = 0;
        int current_x = x;
        int current_y = y;
        auto prev_x = x;
        for (auto& colors: tiles) {
            for (auto&& row: colors){
                for (auto el: row) {
                    DW::buffer[y][x++] = Pixel::get_pixel_color(el);
                }
                ++y;
                prev_x = std::exchange(x, current_x);
            }
            if ((prev_x + 1 >= pwidth)){
                y = current_y = y;
                x = current_x = x_begin;
            } else {
                y = current_y;
                x = current_x = prev_x;
            }
        }
        DW::update();
    }
    void handle(RAM& ram){
        if (CheckCollisionPointRec(GetMousePosition(), DW::rec)){
            if (IsKeyDown(KEY_DOWN)){
                base_address += (20 * 16);
            } else if (IsKeyDown(KEY_UP)) {
                base_address -= (20 * 16);
            }
        }
        update_tiles(ram);
        DW::draw();
    }
};

template <size_t pwidth, size_t pheight, size_t width, size_t height>
struct RegWindows: public DebugWindow<pwidth, pheight, width, height> {
    using DW = DebugWindow<pwidth, pheight,width,height>;
    RegWindows(float x, float y): DW(x, y){}
    void handle(CPU& cpu){
        std::stringstream ss;
        ss << std::hex << " Registers:\n" <<
           " A:" << (unsigned)cpu.regs[CPU::A] << "\n" <<
           " F:" <<  (unsigned)cpu.regs[CPU::F] << "\n" <<
           " B:" <<  (unsigned)cpu.regs[CPU::B] << "\n" <<
           " C:" <<  (unsigned)cpu.regs[CPU::C] << "\n" <<
           " D:" <<  (unsigned)cpu.regs[CPU::D] << "\n" <<
           " E:" <<  (unsigned)cpu.regs[CPU::E] << "\n" <<
           " H:" <<  (unsigned)cpu.regs[CPU::H] << "\n" <<
           " L:" <<  (unsigned)cpu.regs[CPU::L] << "\n" <<
           " SP:" <<  cpu.sp << "\n" <<
           " PC:" <<  cpu.pc << "\n" <<
           " ZeroFlag:" << (unsigned)cpu.get_flag(CPU::Zf) << '\n' <<
           " NegFlag:" << (unsigned)cpu.get_flag(CPU::Nf) << '\n' <<
           " HalfFlag:" << (unsigned)cpu.get_flag(CPU::Hf) << '\n' <<
           " CarryFlag:" << (unsigned)cpu.get_flag(CPU::Cf) << '\n';
            DW::header = ss.str();
            DW::draw_text(24.f);
    }
};

struct GameBoyVM{
    CPU cpu;
    RAM ram;
    PPU ppu;
    Joypad joypad;
    std::string_view ram_path;

    RAM::MBC get_mbc_type(const auto& data) {
        switch (data[0x0147]) {
            case 0x01:
            case 0x02: // MBC1 + RAM
            case 0x03: // MBC1 + RAM + Battery
                {return RAM::MBC1;}
            // todo: finish other mbc types
            default:{
                return RAM::MBC0;
            }
        }
    }

    void try_to_use_save() {
        if (ram.rom[0x147] != 0x03) return;
        std::cout << "\nMBC1 with battery save\n";
        std::string spath(ram_path.substr(0, ram_path.size()-3));
        spath += ".sav";
        std::ifstream ifile(spath, std::ios::binary | std::ios::in);
        if (!ifile.is_open()){ std::cout << "Can't open save file:" << spath << '\n'; return;}
        ifile.read((char*)ram.cart_ram.data(), ram.cart_ram.size()); // todo: check
        ifile.close();
    }

    void main_loop_async(auto& cpu_r) {
        int current_t_cycle = 0;
        bool prev_lcdc_on = ram.mem[constants::LCDC_ADDRESS] & 0x80;
        int lcdc_on_count = 0;
        bool lcdc_init_finished = true;
        while(current_t_cycle++ < 70'224) {
                joypad.update_state(ram);
                cpu.timer.tick(ram);

#if 0
                std::cout << cpu.instruction_str[cpu.decode(ram.read(cpu.pc))] << " REG[A]:" << (int)(cpu.regs[CPU::A]) << '\n';
                std::cout << "TIMA:" << (int)ram.read(constants::TIMA_REGISTER) << '\n';
#endif

                if (ram.mem[constants::LCDC_ADDRESS] & 0x80) {
                    ram.mem[constants::STAT_ADDRESS] |= 0x80;
                    if (!prev_lcdc_on) {
                        lcdc_init_finished = false;
                        lcdc_on_count = 12;
                        prev_lcdc_on = true;
                        ppu.lyc_ly_compare(ram);
                    } else {
                       if (!lcdc_init_finished && lcdc_on_count-- == 0) {
                           lcdc_init_finished = true;
                           ppu.reset(ram);
                           ppu.set_start_mode();
                           ram.ppu_oam_usage = true;
                       }
                       if (lcdc_init_finished){
                             ppu.tick(ram);
                       }
                    }
                }
                cpu_r.resume();
                if (!(ram.mem[constants::LCDC_ADDRESS] & 0x80) && prev_lcdc_on) {
                    prev_lcdc_on = false;
                    ppu.reset(ram);
                }

                if (!cpu.halted && ram.dma_active) ram.dma_tick();
        }
    }

    int get_rom_size(const auto& data) {
        switch(data[0x0148]) {
            case 0x1: return 4;   // 64KB
            case 0x2: return 8;   // 128KB
            case 0x3: return 16;  // 256KB
            case 0x4: return 32;  // 512KB
            case 0x5: return 64;  // 1MB
            case 0x6: return 128; // 2MB
            case 0x7: return 256; // 4MB
            case 0x8: return 512; // 8MB
            case 0x52: return 72; // 1.1MB
            case 0x53: return 80; // 1.2MB
            case 0x54: return 96; // 1.5MB
        }
        return 2; // 32KB
    }

    int get_ram_size(const auto& data) {
        switch(data[0x0149]) {
            case 0x2: return 1; // 1 bank (8KiB)
            case 0x3: return 4; // 4 banks of 8KiB (32KiB)
            case 0x4: return 16; // 16 banks of 8KiB (128KiB)
            case 0x5: return 8; // 8 banks of 8KiB (64KiB)
        }
        // 0x01 - Unused
        // Listed in various unofficial docs as 2 KiB. However, a 2 KiB RAM chip was never used in a cartridge.
        // The source of this value is unknown.
        return 0; // No ram
    }

    void init_raw_rom(const std::string_view& path) {
        std::ifstream f{path, std::ios::binary | std::ios::in};
        if (!f.is_open()) {
            std::cout << "Cannot open file";
            std::abort();
        }
        auto sz = std::filesystem::file_size(path);
        std::vector<char> res(sz);
        f.seekg(0, std::ios::beg);
        f.read(res.data(), sz);
        f.close();
        ram.rom = std::move(res);
        std::cout << ram.rom.size();
    }

    explicit GameBoyVM(const std::string_view& path): ram_path(path) {
        init_raw_rom(path);
        // init rom
        if ((unsigned char)ram.rom[0x143] == 0xc0) {
            std::cout << "\nGameBoyColor unsupported\n";
            std::abort();
        }
        cpu.pc = 0x0100;
        cpu.sp = 0xFFFE;
        ram.rom_sz = get_rom_size(ram.rom);
        ram.mem[constants::JOYPAD] = 0xff;
        ram.mem[constants::LCDC_ADDRESS] = 0x91;
        ram.mem[constants::STAT_ADDRESS] = 0x80;
        ram.cart_ram.resize(get_ram_size(ram.rom) * constants::EXTERNAL_RAM_SZ);
        ram.current_mbc = get_mbc_type(ram.rom);
        ram.current_rom_bank_max = get_rom_size(ram.rom);
        try_to_use_save();
    }

    ~GameBoyVM(){
        if (ram.rom[0x147] == 0x03){
            std::string save_path(ram_path.substr(0, ram_path.size()-3));
            save_path += ".sav";
            std::ofstream save_file(save_path, std::ios::binary | std::ios::out);
            if (save_file.is_open()) {

                save_file.write((char*)ram.cart_ram.data(), ram.cart_ram.size());
                std::cout << "\nSaved succesfully\n";
            } else {
                std::cout << "!!!!!!!!!!! Save error !!!!!!!!!!!" << '\n';
            }
            save_file.close();
        } // MBC1 + RAM + Battery
    }
    
    void run() {
        // Gamboy Doctor
#if 1
        cpu.regs[CPU::A] = 0x01;
        cpu.regs[CPU::F] = 0xB0;
        cpu.regs[CPU::B] = 0x00;
        cpu.regs[CPU::C] = 0x13;
        cpu.regs[CPU::D] = 0x00;
        cpu.regs[CPU::E] = 0xD8;
        cpu.regs[CPU::H] = 0x01;
        cpu.regs[CPU::L] = 0x4D;
        cpu.sp = 0xfffe;
        cpu.pc = 0x0100;
#endif

        InitWindow(WIDTH, HEIGHT, "GameBoy");
        SetTargetFPS(60); // 59.7275 FPS

        // texture init
        Image img = GenImageColor(DISPLAY_WIDTH, DISPLAY_HEIGHT, WHITE);
        Texture2D target = LoadTextureFromImage(img);
        UnloadImage(img);
        std::array<std::array<Color, DISPLAY_WIDTH>, DISPLAY_HEIGHT> display_buffer{};

        // init display
        for (size_t i = 0; i < DISPLAY_HEIGHT; ++i) {
            for (size_t j = 0 ; j < DISPLAY_WIDTH; ++j) {
                display_buffer[i][j] = BLACK;
            }
        }
        UpdateTexture(target, display_buffer.data());

#if DEBUG_MODE
        TileWindow<160,144,500,500,300> tw(810,0, 0x8000);
        RegWindows<160,144,300,400> rw(810,500);
#endif

        auto cpu_r = cpu.instruction_async_loop(ram);
        while(!WindowShouldClose()){
            main_loop_async(cpu_r);
            UpdateTexture(target, ppu.read_buffer());
            BeginDrawing();
               ClearBackground(WHITE);
               DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{0, 0, LCD_WIDTH, LCD_HEIGHT}, {0, 0}, 0.0f, WHITE);
#if DEBUG_MODE
                tw.handle(ram);
                rw.handle(cpu);
#endif
// PRINT PIXELS
#if 0
//             DebugTools::print_rects<DISPLAY_HEIGHT, DISPLAY_WIDTH>(GREEN);
              DebugTools::print_rects<18, 20>(RED);
#endif
                DrawFPS(700, 10);
            EndDrawing();
        }
    }
};

// Done:
// /Users/prazd/Desktop/books/cpp/cpp/vm/gameboy/roms/cpu_instrs.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/basic.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/reg_read.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/halt_ime0_ei.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/div_write.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/manual-only/sprite_priority.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/madness/mgb_oam_dma_halt_sprites.gb (halt bug) - check
// /Users/prazd/Desktop/dev/gb-test-roms/instr_timing/instr_timing.gb
// /Users/prazd/Desktop/dev/gb-test-roms/mem_timing/mem_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/interrupts/ie_push.gb

// MBC-1
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_bank1.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_bank2.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_ramg.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/bits_mode.gb

// Failed:
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/ram_256kb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/ram_64kb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_16Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_1Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_2Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_4Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_512kb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_oam_ok_timing.gb

// PPU
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_1_2_timing-GS.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_0_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode0_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode3_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/stat_lyc_onoff.gb

// Failed:
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/vblank_stat_intr-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/stat_irq_blocking.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/lcdon_write_timing-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/hblank_ly_scx_timing-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode0_timing_sprites.gb - failed

// /Users/prazd/Desktop/dev/gb-test-roms/oam_bug/oam_bug.gb ?
// /Users/prazd/Desktop/dev/gb-test-roms/halt_bug.gb
// /Users/prazd/Desktop/dev/dmg-acid2/build/dmg-acid2.gb
// /Users/prazd/Desktop/dev/mealybug-tearoom-tests/m3_lcdc_bg_en_change.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/misc/ppu/vblank_stat_intr-C.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/sources-GS.gb

// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/div_timing.gb - failed

// /Users/prazd/Desktop/dev/scribbltests/lycscx/lycscx.gb
// /Users/prazd/Desktop/dev/scribbltests/fairylake/fairylake.gb
// /Users/prazd/Desktop/dev/scribbltests/scxly/scxly.gb

// Games:
// Tetris:
//     Source - https://github.com/alexsteb/tetris_disassembly/blob/master/main.asm
//              https://github.com/vinheim3/tetris-gb-disasm
//     ROM -    /Users/prazd/Desktop/dev/tetris.gb
// /Users/prazd/Desktop/dev/PokemonBlueVersion.gb - MBC3

int main() {
        GameBoyVM vm("/Users/prazd/Desktop/dev/ZeldaEn.gb");
        std::cout << "\nRAM sz:" << vm.get_ram_size(vm.ram.rom) << '\n';
        std::cout << "\nROM sz:" << vm.get_rom_size(vm.ram.rom) << '\n';
        vm.run();
}
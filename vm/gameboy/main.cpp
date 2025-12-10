#include <iostream>
#include <queue>
#include <array>
#include <filesystem>
#include <sstream>
#include <fstream>
#include "raylib.h"
#include "PPU.h"
#include "Memory.h"
#include "CPU.h"
#include "Joypad.h"
#include "APU.h"
#include "Debug.h"
#include <SDL.h>

#if CHEATS_ON
#include "Cheats.h"
#endif

struct GameBoyVM{
    CPU cpu;
    Memory mem;
    PPU ppu;
    Joypad joypad;
    std::string_view ram_path;
    bool is_valid_logo = false;
#if DEBUG_MODE
    bool cpu_running = true;
#endif
#if APU_ON
    APU apu;
#endif
    Memory::MBC get_mbc_type(const auto& data) {
        switch (data[0x0147]) {
            case 0x01: // MBC1
            case 0x02: // MBC1 + Memory
            case 0x03: // MBC1 + Memory + Battery
                {return Memory::MBC1;}

            case 0x5:  // MBC2
            case 0x6:  // MBC2+BATTERY
                {return Memory::MBC2;}

            case 0x0f: // MBC3+TIMER+BATTERY
            case 0x10: // MBC3+TIMER+RAM+BATTERY
            case 0x11: // MBC3
            case 0x12: // MBC3+RAM
            case 0x13: // MBC3+RAM+BATTERY
                {return Memory::MBC3;}

            case 0x19: // MBC5
            case 0x1A: // MBC5+RAM
            case 0x1B: // MBC5+RAM+BATTERY
            case 0x1C: // MBC5+RUMBLE
            case 0x1D: // MBC5+RUMBLE+RAM
            case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
                {return Memory::MBC5;}

            default:{
                {return Memory::MBC0;}
            }
        }
    }

    bool is_cartridge_with_save(){
        constexpr static int mbc_with_battery_types[] = {
                0x03, 0x6, 0x0f, 0x10,0x13, 0x1B, 0x1E
        };
        return std::any_of(std::begin(mbc_with_battery_types),
                           std::end(mbc_with_battery_types),
                           [cartridge_type= mem.rom[0x147]](auto& el){return el == cartridge_type;});
    }
    void try_to_use_save() {
        if (!is_cartridge_with_save()) return;
        std::cout << "\nMBC1 with battery save\n";
        std::string spath(ram_path.substr(0, ram_path.size()-3));
        spath += ".sav";
        std::ifstream ifile(spath, std::ios::binary | std::ios::in);
        if (!ifile.is_open()){ std::cout << "Can't open save file:" << spath << '\n'; return;}
        ifile.read((char*)mem.external_ram.data(), mem.external_ram.size()); // todo: check
        ifile.close();
    }

    void init_boot_rom() {
        std::copy(mem.rom.begin(), mem.rom.begin() + 256, mem.cartridge_backup.begin());
#if DEBUG_BOOTROM
        std::copy(constants::debug_boot_rom.begin(), constants::debug_boot_rom.end(), mem.rom.begin());
#else
        std::copy(constants::boot_rom.begin(), constants::boot_rom.end(), mem.rom.begin());
#endif
    }

    void load_from_backup(){
        std::copy(mem.cartridge_backup.begin(), mem.cartridge_backup.end(), mem.rom.begin());
    }


    void main_loop_async(auto& cpu_r, SDL_AudioDeviceID* dev) {
        int current_t_cycle = 0;
        bool prev_lcdc_on = mem.map[constants::LCDC_ADDRESS] & 0x80;
        bool prev_apu_on = mem.map[constants::NR52_ADDRESS] & 0x80;
        unsigned lcdc_on_count = 0;
        bool lcdc_init_finished = true;
        int prev_t_cycle = 0;

        // Finish full frame (can be delay if display off)
        // current_t_cycle++ < constants::T_CYCLES_PER_FRAME
        // TODO: debug
        while(!(prev_t_cycle == constants::T_CYCLES_PER_FRAME - 1 && ppu.internal_t_cycles == 0)) {
                prev_t_cycle = ppu.internal_t_cycles;
                joypad.update_state(mem);
                cpu.timer.tick(mem);
                ppu.tick(mem);
#if DEBUG_MODE
            if (cpu_running) cpu_r.resume();
#else
            cpu_r.resume();
#endif

#if BOOTROM_INIT

                if (!is_valid_logo && mem.map[constants::R_BANK] & 1){
                    is_valid_logo = true;
                    load_from_backup();
                    cpu.pc = 0x100;
                }
#endif
                if ((mem.map[constants::LCDC_ADDRESS] & 0x80) && !prev_lcdc_on) {
                    prev_lcdc_on = true;
                    ppu.reset(mem);
                }
#if APU_ON
            if ((mem.map[constants::NR52_ADDRESS] & 0x80)) {
                if (!prev_apu_on){
                    apu.frame_sequencer_step = 0;
                }
                apu.tick(mem);
            }
            apu.frame_seq_tick(mem);
            prev_apu_on = mem.map[constants::NR52_ADDRESS] & 0x80;
            if (apu.count_sample == 128){
                SDL_QueueAudio(*dev, apu.buffer, 512);
                apu.count_sample = 0;
            }
#endif
            if (!cpu.halted && mem.dma_active) mem.dma_tick();
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
        // Listed in various unofficial docs as 2 KiB. However, a 2 KiB Memory chip was never used in a cartridge.
        // The source of this value is unknown.
        return 1; // No mem
    }

    void init_raw_rom(std::string_view path) {
        std::ifstream f{path, std::ios::binary | std::ios::in};
        if (!f.is_open()) {
            std::cout << "Cannot open file";
            std::abort();
        }
        auto sz = std::filesystem::file_size(path);
        std::vector<char> res;
        res.resize(sz < 32000 ? 32000 : sz); // min 32Kb
        f.seekg(0, std::ios::beg);
        f.read(res.data(), sz);
        f.close();
        mem.rom = std::move(res);
        std::cout << mem.rom.size();
    }

    // https://gbdev.io/pandocs/Power_Up_Sequence.html#register-initialization
    void boot() {
        cpu.regs[CPU::A] = 0x01;
        cpu.regs[CPU::F] = 0xB0;
        cpu.regs[CPU::B] = 0x00;
        cpu.regs[CPU::C] = 0x13;
        cpu.regs[CPU::D] = 0x00;
        cpu.regs[CPU::E] = 0xD8;
        cpu.regs[CPU::H] = 0x01;
        cpu.regs[CPU::L] = 0x4D;
#if BOOTROM_INIT
        cpu.pc = 0x0000;
#else
        cpu.pc = 0x100;
#endif
        cpu.sp = 0xFFFE;

        mem.map[constants::JOYPAD] = 0xcf;
        mem.map[constants::SC_ADDRESS] = 0x7e;
        mem.map[constants::DIV_REGISTER] = 0xAB; // --
        cpu.timer.div_internal = (0xAB << 8);
        mem.map[constants::TAC_REGISTER] = 0xF8;
        mem.map[constants::IF_ADDRESS] = 0xE1;

        mem.map[constants::NR10_ADDRESS] = 0x80;
        mem.map[constants::NR11_ADDRESS] = 0xBF;
        mem.map[constants::NR12_ADDRESS] = 0xF3;
        mem.map[constants::NR13_ADDRESS] = 0xFF;
        mem.map[constants::NR14_ADDRESS] = 0xBF;

        mem.map[constants::NR21_ADDRESS] = 0x3F;
        mem.map[constants::NR22_ADDRESS] = 0x00;
        mem.map[constants::NR23_ADDRESS] = 0xFF;
        mem.map[constants::NR24_ADDRESS] = 0xBF;

        mem.map[constants::NR30_ADDRESS] = 0x7F;
        mem.map[constants::NR31_ADDRESS] = 0xFF;
        mem.map[constants::NR32_ADDRESS] = 0x9F;
        mem.map[constants::NR33_ADDRESS] = 0xFF;
        mem.map[constants::NR34_ADDRESS] = 0xBF;

        mem.map[constants::NR41_ADDRESS] = 0xFF;
        mem.map[constants::NR42_ADDRESS] = 0x00;
        mem.map[constants::NR43_ADDRESS] = 0x00;
        mem.map[constants::NR44_ADDRESS] = 0xBF;

        mem.map[constants::NR50_ADDRESS] = 0x77;
        mem.map[constants::NR51_ADDRESS] = 0xF3;
        mem.map[constants::NR52_ADDRESS] = 0xF1;

        mem.map[constants::STAT_ADDRESS] = 0x85; // --
        mem.map[constants::DMA] = 0xFF;
        mem.map[constants::BGP] = 0xFC;
#if BOOTROM_INIT
        // LCDC on in bootrom logic
        init_boot_rom();
#else
        mem.map[constants::LCDC_ADDRESS] = 0x91;
#endif
    }

    explicit GameBoyVM(const std::string_view path): ram_path(path) {
        init_raw_rom(path);
        // init rom
        if ((unsigned char)mem.rom[0x143] == 0xc0) {
            std::cout << "\nGameBoyColor unsupported\n";
            std::abort();
        }

        mem.rom_sz = get_rom_size(mem.rom);
        mem.max_rom_bank_value = mem.rom_sz - 1;
        mem.current_mbc = get_mbc_type(mem.rom);
        mem.ram_sz = get_ram_size(mem.rom);
        boot();
        mem.max_ram_bank_value = mem.ram_sz - 1;
        if (mem.current_mbc == Memory::MBC2)
            mem.external_ram.resize(512);
        else
            mem.external_ram.resize(get_ram_size(mem.rom) * constants::EXTERNAL_RAM_SZ);

        if (mem.current_mbc == Memory::MBC3)
            mem.external_ram_enabled = true;
        mem.current_rom_bank_max = get_rom_size(mem.rom);
        try_to_use_save();
    }

    void restart() {
        *this = GameBoyVM{this->ram_path};
    }

    ~GameBoyVM(){
        if (!is_cartridge_with_save()) return;
        std::string save_path(ram_path.substr(0, ram_path.size()-3));
        save_path += ".sav";
        std::ofstream save_file(save_path, std::ios::binary | std::ios::out);
        if (save_file.is_open()) {
            save_file.write((char*)mem.external_ram.data(), mem.external_ram.size());
            std::cout << "\nSaved succesfully\n";
        } else {
            std::cout << "!!!!!!!!!!! Save error !!!!!!!!!!!" << '\n';
        }
        save_file.close();
    }

    void run() {
        InitWindow(constants::WIDTH, constants::HEIGHT, "GameBoy");
        SetTargetFPS(60); // 59.7275 FPS
        SetExitKey(0);

        // texture init
#if DEBUG_MODE
        RenderTexture2D base = LoadRenderTexture(constants::WIDTH, constants::HEIGHT);
#endif
        Image img = GenImageColor(constants::DISPLAY_WIDTH, constants::DISPLAY_HEIGHT, WHITE);
        Texture2D target = LoadTextureFromImage(img);
        UnloadImage(img);
        std::array<std::array<Color, constants::DISPLAY_WIDTH>, constants::DISPLAY_HEIGHT> display_buffer{};

        // init display
        for (size_t i = 0; i < constants::DISPLAY_HEIGHT; ++i) {
            for (size_t j = 0 ; j < constants::DISPLAY_WIDTH; ++j) {
                display_buffer[i][j] = BLACK;
            }
        }
        UpdateTexture(target, display_buffer.data());

#if APU_ON
        SDL_Init(SDL_INIT_AUDIO);
        SDL_AudioSpec want {};
        want.freq = 44100;
        want.format = AUDIO_S16LSB;
        want.channels = 2;
        want.samples = 512;

        SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);

        if (!dev) {
            std::cerr << "Audio device error: " << SDL_GetError() << "\n";
            std::abort();
        }

        SDL_PauseAudioDevice(dev, 0);
#endif

#if DEBUG_MODE
        // tile view modes
        TileWindow<160,144,500,500,300> tw(810,0, 0x8000);
        HexViewer<500,500> hv(810,0);
        BgTileMapWindow<500,500> bgtm(810,0);
#if CHEATS_ON
        CheatsWin<500,500> cheats(810,0);
#endif

        SpriteWindowText<0,0,600,200> swt(10, 750);
        SpriteWindow<80, 64, 300, 400> sw(820, 517);

        PPURegistersText<0,0,200,150> ppurt(1320, 0);
        RegWindow<0,0,300,465> rw(1150, 517);
//        JoypadWindow<0,0,300,250> jw(820, 730);

#if APU_ON
        APURegistersText<0,0,200,150> apurt(1320, 150);
        APUChansActive<300,300> apuc(1320, 150);
        enum APUViewMode {
            ApuRegs,
            ApuChans,
            ApuViewModesCount
        };
        int current_apu_view_mode = 0;
        auto apu_view_mode_handle = [&]{
            switch (current_apu_view_mode) {
                case ApuRegs: {apurt.handle(mem); break;}
                case ApuChans: {apuc.handle(); break;}
            }
        };
#endif
        enum TileViewMode{
            TileWindow,
            BgTileMapWindow,
            HexViewer,
#if CHEATS_ON
            Cheats,
#endif
            ViewModesCount
        };
        unsigned char current_tile_view_mode = 0;
        auto tile_view_mode_handle = [&]{
            switch (current_tile_view_mode) {
                case TileWindow:       {tw.handle(mem); break;}
                case BgTileMapWindow:  {bgtm.handle(mem, ppu.last_scx, ppu.last_scy); break;}
                case HexViewer:        {hv.handle(mem); break;}
#if CHEATS_ON
                case Cheats:           {cheats.handle(mem); break;}
#endif
            }
        };

        RectsPrint<160,144, constants::LCD_WIDTH, constants::LCD_HEIGHT> pp(0, 0);
        RectsPrint<20 ,18, constants::LCD_WIDTH, constants::LCD_HEIGHT> tp(0, 0);

        bool print_pixels = false;
        bool print_tiles = false;
        enum FPS {
            SuperLow = 5,
            Low = 15,
            Mid = 30,
            High = 60
        };
        FPS fps_states[] = {SuperLow,Low, Mid, High};

        unsigned char current_fps_state = std::size(fps_states) - 1;
        unsigned char current_speed = 1;
        unsigned current_freq[] = {44100, 44100 * 2, 44100 * 3};
#endif

        auto cpu_r = cpu.instruction_async_loop(mem);
        while(!WindowShouldClose()){
#if DEBUG_MODE
                for (int i = 0; i < current_speed; ++i)
#if APU_ON
                    main_loop_async(cpu_r, &dev);
#else
                    main_loop_async(cpu_r, nullptr);
#endif

//                auto end = std::chrono::steady_clock::now();
//                std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms\n";

                UpdateTexture(target, ppu.read_buffer(mem));

                if (print_pixels) {pp.update(mem, 0, 0);}
                if (print_tiles)  {tp.update(mem, 0, print_pixels ? 50 : 0);}
#if APU_ON
                if (current_apu_view_mode == ApuChans) apuc.update(mem);
#endif

                BeginTextureMode(base);
                    ClearBackground(BLANK);
                    DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height},
                               Rectangle{0, 0, constants::LCD_WIDTH, constants::LCD_HEIGHT}, {0, 0}, 0.0f, WHITE);
                    tile_view_mode_handle();

                    sw.handle(mem);
                    rw.handle(mem, cpu);
                    swt.handle(sw);
//                    jw.handle(mem, joypad);
                    ppurt.handle(mem, ppu.last_scx, ppu.last_scy);
                    if (print_pixels) pp.handle(GREEN, 0, 0);
                    if (print_tiles)  tp.handle(RED, 0, print_pixels ? 50 : 0);
#if APU_ON
                apu_view_mode_handle();
#endif
                EndTextureMode();
#else



#if APU_ON
            main_loop_async(cpu_r, dev);
#else
            main_loop_async(cpu_r, nullptr);
#endif

            UpdateTexture(target, ppu.read_buffer(mem));
#endif

            BeginDrawing();
               ClearBackground(WHITE);
#if DEBUG_MODE
            DrawTextureRec(base.texture, {0, 0, (float)base.texture.width, -(float)base.texture.height}, {0, 0}, WHITE);
#else
               DrawTexturePro(target, {0, 0, (float)target.width, (float)target.height}, Rectangle{0, 0, constants::LCD_WIDTH, constants::LCD_HEIGHT}, {0, 0}, 0.0f, WHITE);
#endif

#if DEBUG_MODE

#endif
// PRINT PIXELS
#if DEBUG_MODE
            if (IsKeyPressed(KEY_M))
                  current_tile_view_mode = (current_tile_view_mode + 1) % ViewModesCount;

            if (CheckCollisionPointRec(GetMousePosition(), {0,0,constants::LCD_WIDTH,constants::LCD_HEIGHT})) {
                if (IsKeyPressed(KEY_ONE))
                    ppu.bg_change_color1 = !ppu.bg_change_color1;
                if (IsKeyPressed(KEY_TWO))
                    ppu.bg_change_color2 = !ppu.bg_change_color2;
                if (IsKeyPressed(KEY_THREE))
                    ppu.bg_change_color3 = !ppu.bg_change_color3;
                if (IsKeyPressed(KEY_FOUR))
                    ppu.bg_change_color4 = !ppu.bg_change_color4;
                if (IsKeyPressed(KEY_FIVE))
                    ppu.sprite_change_color = !ppu.sprite_change_color;

                if (IsKeyPressed(KEY_F)) {
                    current_fps_state = ((current_fps_state + 1) % std::size(fps_states));
                    SetTargetFPS(fps_states[current_fps_state]);
                }
                if (IsKeyPressed(KEY_T))
                    print_tiles = !print_tiles;
                if (IsKeyPressed(KEY_P))
                    print_pixels = !print_pixels;
                if (IsKeyPressed(KEY_S))
                    cpu_running = !cpu_running;

                if (IsKeyPressed(KEY_ZERO))
                    restart();
                if (IsKeyPressed(KEY_X)){
                    // todo: debug
                    current_speed = std::max(1, ((current_speed + 1) % 4));

                }
            }

#if APU_ON
              if (IsKeyPressed(KEY_A))
                  current_apu_view_mode = (current_apu_view_mode + 1) % ApuViewModesCount;
#endif

#endif
                DrawFPS(700, 10);
            EndDrawing();
        }
#if APU_ON
        SDL_CloseAudioDevice(dev);
        SDL_Quit();
#endif
    }
};

// Done:
// /Users/prazd/Desktop/books/cpp/cpp/vm/gameboy/roms/cpu_instrs.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/basic.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/reg_read.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/halt_ime0_ei.gb

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
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/ram_256kb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/ram_64kb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_16Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_1Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_2Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_4Mb.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/emulator-only/mbc1/rom_512kb.gb

// Failed:
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_oam_ok_timing.gb

// Timer:
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/div_write.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim00.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim01.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim10.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim11.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim00_div_trigger.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim01_div_trigger.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim10_div_trigger.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tim11_div_trigger.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tma_write_reloading.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tima_write_reloading.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/tima_reload.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/timer/rapid_toggle.gb


// PPU
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_1_2_timing-GS.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_0_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode0_timing.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode3_timing.gb
// /Users/prazd/Desktop/dev/dmg-acid2/build/dmg-acid2.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/vblank_stat_intr-GS.gb
// /Users/prazd/Desktop/dev/scribbltests/lycscx/lycscx.gb
// /Users/prazd/Desktop/dev/scribbltests/fairylake/fairylake.gb +-
// /Users/prazd/Desktop/dev/scribbltests/scxly/scxly.gb

// /Users/prazd/Desktop/dev/mealybug-tearoom-tests/m2_win_en_toggle.gb

// APU
// /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/01-registers.gb
// /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/02-len ctr.gb
// /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/03-trigger.gb
// /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/04-sweep.gb +-
// /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/06-overflow on trigger.gb
// /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/07-len sweep period sync.gb

// Failed:
//  - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/stat_lyc_onoff.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/stat_irq_blocking.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/lcdon_write_timing-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/hblank_ly_scx_timing-GS.gb - failed
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/ppu/intr_2_mode0_timing_sprites.gb - failed !!!!!!!!!

// /Users/prazd/Desktop/dev/gb-test-roms/oam_bug/oam_bug.gb ?
// /Users/prazd/Desktop/dev/gb-test-roms/halt_bug.gb
// /Users/prazd/Desktop/dev/mealybug-tearoom-tests/m3_lcdc_bg_en_change.gb
// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/misc/ppu/vblank_stat_intr-C.gb
// /Users/prazd/Desktop/dev/mts-20211031-1956-1ae00bf/acceptance/oam_dma/sources-GS.gb

// /Users/prazd/Desktop/dev/mts-20211031-2031-86d1acf/acceptance/div_timing.gb - failed

// Games:
// Tetris:
//     Source - https://github.com/alexsteb/tetris_disassembly/blob/master/main.asm
//              https://github.com/vinheim3/tetris-gb-disasm
//     ROM -    /Users/prazd/Desktop/dev/tetris.gb
// /Users/prazd/Desktop/dev/PokemonBlueVersion.gb - MBC3


// - mealybug-tearoom-tests:
// /Users/prazd/Desktop/dev/mealybug-tearoom-tests/m3_bgp_change.gb
// /Users/prazd/Desktop/dev/mealybug-tearoom-tests/m3_bgp_change_sprites.gb
// /Users/prazd/Desktop/dev/mealybug-tearoom-tests/m3_lcdc_bg_en_change.gb

int main() {
        //    /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/05-sweep details.gb
        //    /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/08-len ctr during power.gb - debug
        //    /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/09-wave read while on.gb - debug
        //    /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/10-wave trigger while on.gb
        //    /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/11-regs after power.gb
        //    /Users/prazd/Desktop/dev/gb-test-roms/dmg_sound/rom_singles/12-wave write while on.gb
        // /Users/prazd/Desktop/dev/gb-square-test/square_test.gb
        GameBoyVM vm("/Users/prazd/Desktop/dev/ZeldaEn.gb");
        std::cout << "\nRAM sz:" << vm.get_ram_size(vm.mem.rom) << '\n';
        std::cout << "\nROM sz:" << vm.get_rom_size(vm.mem.rom) << '\n';
        vm.run();
}
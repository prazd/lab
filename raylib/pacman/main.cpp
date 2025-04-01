#include "raylib.h"
#include <array>
#include <sstream>
#include "Animation.h"
#include "Constants.h"
#include "MapManager.h"
#include "Unit.h"

constexpr auto pacman_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/pacman/img/pacman.png";
Texture2D atlas;

void load_sprites() {
    auto img = LoadImage(pacman_img_path);
    atlas = LoadTextureFromImage(img);
    UnloadImage(img);
}

void draw_map() {
    MapManager::call([](auto, auto coord_i, auto el) {
        auto [x,y] = MapManager::get_coords_raw()[coord_i];
        if (el == Cells::Empty || el == Cells::PSpawn) DrawRectangleLinesEx((Rectangle){x, y, gc::OFFSET, gc::OFFSET}, 1, BLUE);
        else if (el == Cells::GSpawn) DrawRectangleLinesEx((Rectangle){x, y, gc::OFFSET, gc::OFFSET}, 1, RED);
    });
}

int main() {
    InitWindow(gc::WIDTH, gc::HEIGHT, "Pacman");
    load_sprites();
    SetTargetFPS(60);
    bool running = true;
    bool win = false;

    PacMan p;

    std::array<Ghost, 2> ms;
    std::stringstream score_text;

    auto monster_spawn_vec = MapManager::coord_point(Cells::GSpawn);

    auto stream_updater = [](std::stringstream& ss, auto&&... data) {
        ss.str("");
        (ss << ... << data);
    };
    MapManager::set_mode(Classic);
    CoinsMap::seed();
    auto restart = [&]{
        MapManager::set_mode(Classic);
        p = PacMan{};
        ms = std::array<Ghost, 2>{};
        monster_spawn_vec = MapManager::coord_point(Cells::GSpawn);
        stream_updater(score_text, "Score:0");
        running = true;
        win = false;
    };

    restart();

    auto target = LoadRenderTexture(gc::WIDTH, gc::HEIGHT);
     auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(BLACK);
            if (!running) {
                stream_updater(score_text, "Game Over. Scores ", MapManager::get_coins_collected(), ". Press R to restart");
                if (!win) DrawText(score_text.str().data(), gc::WIDTH/4, gc::HEIGHT/2, 30, WHITE);
                else DrawText("YOU WIN. Press R to restart", gc::WIDTH/4, gc::HEIGHT/2, 30, WHITE);

                EndTextureMode();
                if (IsKeyPressed(KEY_R)) {
                    restart();
                }
                return;
            }
            draw_map();
            auto dt = GetFrameTime();
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
                p.update(Direction::up);
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
                p.update(Direction::down);
            if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
                p.update(Direction::left);
            if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
                p.update(Direction::right);

            p.move(dt);
#ifdef DRAW_LINE
            DrawRectangleLinesEx((Rectangle){p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, QUBE_SIDE, QUBE_SIDE}, 1, BLUE);
#endif
            switch(p.dir) {
                case Direction::up:
                case Direction::down:
                {
                    DrawTexturePro(atlas, p.rect, {p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {0, gc::FRAME_SZ}, 90.f, WHITE);
                    break;
                }
                default:
                {
                    DrawTexturePro(atlas, p.rect, {p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {0, 0}, 0.f, WHITE);
                    break;
                }
            }


            MapManager::for_each_coins([&](auto& el){
                auto& [coord, active] = el;
                if (active) {
                    if (CheckCollisionRecs({p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {coord.first, coord.second, 10, 10})){
                        active = false;
                        ++MapManager::get_coins_collected();
                        return;
                    }
                    DrawTexturePro(atlas, {80, 22, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {coord.first, coord.second, 10, 10}, {0, 0}, 0.f, WHITE);
                }
            });

            for (auto& m: ms) {
                m.move(dt, p);
                if (CheckCollisionRecs({p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {m.pos.x-gc::R_PLAYER, m.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT})){
                    running = false;
                    win = false;
                }
                DrawTexturePro(atlas, m.rect, {m.pos.x-gc::R_PLAYER, m.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {2, 5}, 0.f, WHITE);
            }


            if (CheckCollisionRecs({p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {monster_spawn_vec.x, monster_spawn_vec.y, 10,10})){
//                running = false;
//                win = true;
                  restart();
            }

            stream_updater(score_text, "Score:", MapManager::get_coins_collected());
            DrawText(score_text.str().data(), 0, 0, 20, WHITE);

        EndTextureMode();
    };

    while(!WindowShouldClose()){
        render();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(1000, 10);
        EndDrawing();
    }

    UnloadTexture(atlas);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
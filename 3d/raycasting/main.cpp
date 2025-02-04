#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <cctype>

constexpr int WIDTH = 120;
constexpr int HEIGHT = 40;

int mapHeight = 16;
int mapWidth = 16;

std::wstring map;

void initmap() {
    map += L"################";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"################";
}

std::string get_input() {
    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    int ret = poll(fds, 1, 1);
    if (ret == -1) {
        std::cerr << "Ошибка выполнения poll()" << std::endl;
        return "";
    } else if (ret == 0) {
        return "";
    }
    if (fds[0].revents & POLLIN) {
        char buffer[2];
        std::cin.read(buffer, 1);
        buffer[1] = '\0';
        return {buffer};
    }
    return "";
}


int main() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    initmap();

    wchar_t  screen[WIDTH * HEIGHT];
    screen[WIDTH * HEIGHT - 1] = '\0';

    float player_x = 8.0f;
    float player_y = 8.0f;
    float player_a = 0.0f;

    float player_fov = 3.14f / 4.f;
    float depth = 16.0f;

    std::cout << "\x1B[2J\x1B[H"; // full flush
    while(true) {
        // auto res = get_input();
        for (auto x = 0; x < WIDTH; ++x) {
            // for each column, calc the projected ray angle into world space
            auto ray_angle = (player_a - player_fov/2.f) + (float(x)/(float)WIDTH) * player_fov;
            float wall_distance = 0;
            bool hitwall = false;
            float eye_x = std::sin(ray_angle);
            float eye_y = std::sin(ray_angle);

            while(!hitwall && wall_distance < depth) {
                wall_distance += 0.1f;
                int test_x = (player_x + eye_x * wall_distance);
                int test_y = (player_y + eye_y * wall_distance);
                if (test_x < 0 || test_x >= WIDTH || test_y < 0 || test_y >= HEIGHT) {
                    hitwall = true;
                    wall_distance = depth;
                } else {
                    if (map[test_y * WIDTH + test_x] == '#') {
                        hitwall = true;
                    }
                }
            }

            int ceiling = (float)(HEIGHT/2) - HEIGHT/((float)wall_distance);
            int floor = HEIGHT - ceiling;
            for(int y = 0; y < HEIGHT; ++y) {
                if (y < ceiling) {
                    screen[y*WIDTH + x] = ' ';
                } else if (y > ceiling && y <= floor){
                    screen[y*WIDTH + x] = '#';
                } else {
                    screen[y*WIDTH + x] = ' ';
                }
            }
        }
        std::cout << "\x1B[2J";
        std::wcout << screen << "\n";
        std::cout << std::flush;
    }

}
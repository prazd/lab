#include "raylib.h"
#include <random>

constexpr float R = 2;

template <int begin, int end>
class RandomCube {
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<int> dis = std::uniform_int_distribution<int>(begin, end);
public:
    static int roll() {
        return dis(gen);
    }
};

struct Point {
    float x;
    float y;
    float r;
};

struct NextPointGenerator {
    Point p1{600, 10, R};
    Point p2{300, 400, R};
    Point p3{900, 400, R};
    NextPointGenerator() = default;
    Point generate(const Point& p) {
        auto num = RandomCube<1,6>::roll();
        Point* current;
        switch(num){
            case 1:
            case 2: {current = &p1;break;}
            case 3:
            case 4: {current = &p2;break;}
            case 5:
            case 6: {current = &p3;break;}
        }
        return Point{std::midpoint(current->x, p.x), std::midpoint(current->y, p.y), R};
    }
};

int main(void)
{
    InitWindow(1200, 600, "Sierpinski");
    NextPointGenerator np;
    std::vector<Point> points{np.p1,np.p2,np.p3, Point{700, 300, R}};

    RenderTexture2D target = LoadRenderTexture(1200, 600);
    BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        for (auto& p : points)
            DrawCircle(p.x, p.y, p.r, RED);
    EndTextureMode();

    while (!WindowShouldClose())
    {
        points.emplace_back(np.generate(points.back()));
        BeginTextureMode(target);
            auto next = points.back();
            DrawCircle(next.x, next.y, next.r, RED);
        EndTextureMode();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }
    
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

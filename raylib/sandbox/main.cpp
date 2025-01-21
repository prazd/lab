#include "raylib.h"
#include <math.h>

#define PARTICLE_COUNT 1000

typedef struct Particle {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    Color color;
} Particle;

void InitParticles(Particle particles[], Vector2 origin) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        particles[i].position = origin;
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(100, 300) / 100.0f;
        particles[i].velocity = (Vector2){ cos(angle) * speed, sin(angle) * speed };
        particles[i].lifetime = GetRandomValue(10, 50) / 10.0f;
        particles[i].color = (Color){ (unsigned char)GetRandomValue(200, 255), (unsigned char)GetRandomValue(50, 150), 0, 255 };
    }
}

void UpdateParticles(Particle particles[], float deltaTime) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        particles[i].position.x += particles[i].velocity.x;
        particles[i].position.y += particles[i].velocity.y;
        particles[i].lifetime -= deltaTime;
        particles[i].color.a = (unsigned char)(particles[i].lifetime * 255); // Плавное исчезновение
    }
}

void DrawParticles(Particle particles[]) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (particles[i].lifetime > 0) {
            DrawCircleV(particles[i].position, 2, particles[i].color);
        }
    }
}

int main() {
    InitWindow(800, 600, "Particle Explosion");

    Particle particles[PARTICLE_COUNT];
    Vector2 origin = { 400, 300 };
    InitParticles(particles, origin);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Обновление частиц
        UpdateParticles(particles, deltaTime);

        // Рендеринг
        BeginDrawing();
        ClearBackground(WHITE);

        DrawParticles(particles);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <algorithm>

#define SAMPLE_RATE   40'000
#define BUFFER_SIZE   750
#define CHANNELS      1     // моно
#define SAMPLE_SIZE   4

static float phase_square = 0.0f;
static float phase_noise = 0.0f;
static unsigned short lfsr = 0xACE1u;

static float next_noise_sample() {
    unsigned bit  = ((lfsr >> 0) ^ (lfsr >> 1)) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);
    return (lfsr & 1) ? 0.3f : -0.3f;
}

static float next_square_sample(float freq, float duty) {
    float period = (float)SAMPLE_RATE / freq;
    phase_square += 1.0f;
    if (phase_square >= period) phase_square -= period;
    return (phase_square / period < duty) ? 0.5f : -0.5f;
}

static void generate_audio(short *buffer, int samples) {
    std::generate(buffer, buffer + samples, []{return 100;});
    for (int i = 0; i < samples; i++) {
        
        float sq = next_square_sample(300.0f, 0.5f); // 440 Hz, 50% duty
        float ns = next_square_sample(300.0f, 0.5f); // 440 Hz, 50% duty
        // float ns = next_noise_sample();
        float mix = (ns + sq)/2;
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;
        buffer[i] = (short)(mix * 32767);
    }
}

int main(void) {
    InitWindow(400, 200, "Raylib realtime sound (Game Boy style)");
    InitAudioDevice();

    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    PlayAudioStream(stream);

    short buffer[BUFFER_SIZE];
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsAudioStreamProcessed(stream)) {
            generate_audio(buffer, BUFFER_SIZE);
            UpdateAudioStream(stream, buffer, BUFFER_SIZE);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Streaming audio... (Square + Noise)", 40, 80, 20, DARKGRAY);
        EndDrawing();
    }

    StopAudioStream(stream);
    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

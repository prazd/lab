#pragma once
#include <cstdint>

struct MetalContext;

extern "C" {
    MetalContext* metal_create_window_and_renderer(int buffer_w, int buffer_h, int window_w, int window_h);
    void metal_present(MetalContext* ctx, const uint32_t* pixels);
    void metal_start_main_loop(MetalContext* ctx);
    uint32_t* metal_get_framebuffer(MetalContext* ctx);
}

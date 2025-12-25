#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import "metal_api.h"
#include <atomic>
#import <QuartzCore/QuartzCore.h> 

extern std::atomic<bool> new_frame_ready;

struct MetalContext {
    NSWindow* window;
    CAMetalLayer* layer;
    id<MTLDevice> device;
    id<MTLCommandQueue> queue;
    id<MTLTexture> fb;
    id<MTLRenderPipelineState> pso;
    int buffer_w, buffer_h;
    int window_w, window_h;

    uint32_t* framebuffer;
    CVDisplayLinkRef displayLink;
};

@interface MetalContextWrapper : NSObject <NSWindowDelegate>
@property (nonatomic, assign) MetalContext* ctx;
- (void)draw;
@end

static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* context) {
    MetalContextWrapper* wrapper = (__bridge MetalContextWrapper*)context;
    [wrapper draw];
    return kCVReturnSuccess;
}

@implementation MetalContextWrapper
- (void)draw {
    if (new_frame_ready.exchange(false)) {
        [self.ctx->fb replaceRegion:MTLRegionMake2D(0,0,self.ctx->buffer_w,self.ctx->buffer_h)
                    mipmapLevel:0
                      withBytes:self.ctx->framebuffer
                    bytesPerRow:self.ctx->buffer_w * 4];
    }

    id<CAMetalDrawable> drawable = [self.ctx->layer nextDrawable];
    if (!drawable) return;

    MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
    rp.colorAttachments[0].texture = drawable.texture;
    rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;  // как мы обсуждали раньше
    rp.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLCommandBuffer> cb = [self.ctx->queue commandBuffer];
    id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
    [enc setRenderPipelineState:self.ctx->pso];
    [enc setFragmentTexture:self.ctx->fb atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [enc endEncoding];

    [cb presentDrawable:drawable];
    [cb commit];
}
- (void)windowWillClose:(NSNotification*)notification {
    if (self.ctx->displayLink) {
        CVDisplayLinkStop(self.ctx->displayLink);
        CVDisplayLinkRelease(self.ctx->displayLink);
        self.ctx->displayLink = NULL;
    }
    [NSApp terminate:nil];
}
@end

static id<MTLRenderPipelineState> create_pipeline(id<MTLDevice> device, id<MTLLibrary> lib) {
    MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
    desc.vertexFunction = [lib newFunctionWithName:@"vertex_main"];
    desc.fragmentFunction = [lib newFunctionWithName:@"fragment_main"];
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    NSError* err = nil;
    return [device newRenderPipelineStateWithDescriptor:desc error:&err];
}

MetalContext* metal_create_window_and_renderer(int buffer_w, int buffer_h, int window_w, int window_h) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect r = NSMakeRect(0, 0, window_w, window_h);
    NSWindow* win = [[NSWindow alloc]
        initWithContentRect:r
                  styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    [win center];
    [win makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    CAMetalLayer* layer = [CAMetalLayer layer];
    win.contentView.wantsLayer = YES;
    win.contentView.layer = layer;
    

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> queue = [device newCommandQueue];

    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

    MTLTextureDescriptor* td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                   width:buffer_w height:buffer_h mipmapped:NO];
    id<MTLTexture> fb = [device newTextureWithDescriptor:td];

    NSError* error = nil;
    NSURL *url = [NSURL fileURLWithPath:@"/Users/prazd/Desktop/books/cpp/cpp/metal/metal_api/Shaders.metallib"];
    id<MTLLibrary> lib = [device newLibraryWithURL:url error:&error];
    id<MTLRenderPipelineState> pso = create_pipeline(device, lib);

    MetalContext* ctx = new MetalContext
    {win, layer, device, queue, fb, pso, 
    buffer_w, buffer_h, window_w, window_h, 
    new uint32_t[buffer_w*buffer_h], NULL};
    return ctx;
}

uint32_t* metal_get_framebuffer(MetalContext* ctx) {
    return ctx->framebuffer;
}

void metal_start_main_loop(MetalContext* ctx) {
    MetalContextWrapper* wrapper = [MetalContextWrapper new];
    wrapper.ctx = ctx;
    ctx->window.delegate = (id<NSWindowDelegate>)wrapper;
    CVDisplayLinkRef dl;
    CVReturn err = CVDisplayLinkCreateWithActiveCGDisplays(&dl);
    if (err != kCVReturnSuccess) {
        NSLog(@"Failed to create CVDisplayLink: %d", err);
        return;
    }
    CVDisplayLinkSetOutputCallback(dl, displayLinkCallback, (__bridge void*)wrapper);
    ctx->displayLink = dl;
    CVDisplayLinkStart(dl);

    [NSApp run];
}

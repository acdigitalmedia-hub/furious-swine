#include "sdl_runtime.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

// ============================================================================
// BUILT-IN SOFTWARE RASTERIZER + NATIVE X11 FALLBACK (IF SDL2 IS NOT INSTALLED)
// ============================================================================
struct SoftTexture {
    int w = 0, h = 0;
    bool linear = false;
    bool whiteRGB = false;  // every texel is white: only alpha matters (font atlas)
    std::vector<uint32_t> pixels;
};

struct SoftRenderer {
    int w = 1280, h = 720;
    uint8_t drawR = 0, drawG = 0, drawB = 0, drawA = 255;
    std::vector<uint32_t> fb;
};

// Minimal X11 dynamic bindings for fallback windowing when SDL2 is absent
struct X11Fallback {
    void* lib = nullptr;
    void* display = nullptr;
    unsigned long window = 0;
    void* gc = nullptr;
    unsigned long wmDelete = 0;
    bool active = false;

    using fn_XOpenDisplay = void* (*)(const char*);
    using fn_XCloseDisplay = int (*)(void*);
    using fn_XDefaultScreen = int (*)(void*);
    using fn_XRootWindow = unsigned long (*)(void*, int);
    using fn_XBlackPixel = unsigned long (*)(void*, int);
    using fn_XCreateSimpleWindow = unsigned long (*)(void*, unsigned long, int, int,
                                                     unsigned int, unsigned int,
                                                     unsigned int, unsigned long, unsigned long);
    using fn_XStoreName = int (*)(void*, unsigned long, const char*);
    using fn_XSelectInput = int (*)(void*, unsigned long, long);
    using fn_XMapWindow = int (*)(void*, unsigned long);
    using fn_XCreateGC = void* (*)(void*, unsigned long, unsigned long, void*);
    using fn_XPending = int (*)(void*);
    using fn_XNextEvent = int (*)(void*, void*);
    using fn_XLookupKeysym = unsigned long (*)(void*, int);
    using fn_XInternAtom = unsigned long (*)(void*, const char*, int);
    using fn_XSetWMProtocols = int (*)(void*, unsigned long, unsigned long*, int);
    using fn_XFlush = int (*)(void*);
    using fn_XPutImage = int (*)(void*, unsigned long, void*, void*, int, int, int, int,
                                 unsigned int, unsigned int);
    using fn_XDefaultVisual = void* (*)(void*, int);
    using fn_XDefaultDepth = int (*)(void*, int);
    using fn_XCreateImage = void* (*)(void*, void*, unsigned int, int, int, char*,
                                      unsigned int, unsigned int, int, int);

    fn_XOpenDisplay pXOpenDisplay = nullptr;
    fn_XCloseDisplay pXCloseDisplay = nullptr;
    fn_XDefaultScreen pXDefaultScreen = nullptr;
    fn_XRootWindow pXRootWindow = nullptr;
    fn_XBlackPixel pXBlackPixel = nullptr;
    fn_XCreateSimpleWindow pXCreateSimpleWindow = nullptr;
    fn_XStoreName pXStoreName = nullptr;
    fn_XSelectInput pXSelectInput = nullptr;
    fn_XMapWindow pXMapWindow = nullptr;
    fn_XCreateGC pXCreateGC = nullptr;
    fn_XPending pXPending = nullptr;
    fn_XNextEvent pXNextEvent = nullptr;
    fn_XLookupKeysym pXLookupKeysym = nullptr;
    fn_XInternAtom pXInternAtom = nullptr;
    fn_XSetWMProtocols pXSetWMProtocols = nullptr;
    fn_XFlush pXFlush = nullptr;
    fn_XPutImage pXPutImage = nullptr;
    fn_XDefaultVisual pXDefaultVisual = nullptr;
    fn_XDefaultDepth pXDefaultDepth = nullptr;
    fn_XCreateImage pXCreateImage = nullptr;
    void* ximage = nullptr;

    bool init(const char* title, int w, int h, char* fbData) {
        const char* envDrv = std::getenv("SDL_VIDEODRIVER");
        if (envDrv && std::string(envDrv) == "dummy") {
            active = true;
            return true; // Headless self-test mode
        }
        lib = dlopen("libX11.so.6", RTLD_LAZY);
        if (!lib) lib = dlopen("libX11.so", RTLD_LAZY);
        if (!lib) return false;

        pXOpenDisplay = reinterpret_cast<fn_XOpenDisplay>(dlsym(lib, "XOpenDisplay"));
        pXCloseDisplay = reinterpret_cast<fn_XCloseDisplay>(dlsym(lib, "XCloseDisplay"));
        pXDefaultScreen = reinterpret_cast<fn_XDefaultScreen>(dlsym(lib, "XDefaultScreen"));
        pXRootWindow = reinterpret_cast<fn_XRootWindow>(dlsym(lib, "XRootWindow"));
        pXBlackPixel = reinterpret_cast<fn_XBlackPixel>(dlsym(lib, "XBlackPixel"));
        pXCreateSimpleWindow = reinterpret_cast<fn_XCreateSimpleWindow>(dlsym(lib, "XCreateSimpleWindow"));
        pXStoreName = reinterpret_cast<fn_XStoreName>(dlsym(lib, "XStoreName"));
        pXSelectInput = reinterpret_cast<fn_XSelectInput>(dlsym(lib, "XSelectInput"));
        pXMapWindow = reinterpret_cast<fn_XMapWindow>(dlsym(lib, "XMapWindow"));
        pXCreateGC = reinterpret_cast<fn_XCreateGC>(dlsym(lib, "XCreateGC"));
        pXPending = reinterpret_cast<fn_XPending>(dlsym(lib, "XPending"));
        pXNextEvent = reinterpret_cast<fn_XNextEvent>(dlsym(lib, "XNextEvent"));
        pXLookupKeysym = reinterpret_cast<fn_XLookupKeysym>(dlsym(lib, "XLookupKeysym"));
        pXInternAtom = reinterpret_cast<fn_XInternAtom>(dlsym(lib, "XInternAtom"));
        pXSetWMProtocols = reinterpret_cast<fn_XSetWMProtocols>(dlsym(lib, "XSetWMProtocols"));
        pXFlush = reinterpret_cast<fn_XFlush>(dlsym(lib, "XFlush"));
        pXPutImage = reinterpret_cast<fn_XPutImage>(dlsym(lib, "XPutImage"));
        pXDefaultVisual = reinterpret_cast<fn_XDefaultVisual>(dlsym(lib, "XDefaultVisual"));
        pXDefaultDepth = reinterpret_cast<fn_XDefaultDepth>(dlsym(lib, "XDefaultDepth"));
        pXCreateImage = reinterpret_cast<fn_XCreateImage>(dlsym(lib, "XCreateImage"));

        if (!pXOpenDisplay || !pXCreateSimpleWindow || !pXPutImage || !pXCreateImage) return false;
        display = pXOpenDisplay(nullptr);
        if (!display) return false;

        int screen = pXDefaultScreen(display);
        unsigned long root = pXRootWindow(display, screen);
        unsigned long black = pXBlackPixel(display, screen);
        window = pXCreateSimpleWindow(display, root, 40, 40, w, h, 1, black, black);
        pXStoreName(display, window, title);
        // ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
        pXSelectInput(display, window, (1L << 15) | (1L << 0) | (1L << 2) | (1L << 3) | (1L << 6));
        wmDelete = pXInternAtom(display, "WM_DELETE_WINDOW", 0);
        pXSetWMProtocols(display, window, &wmDelete, 1);
        pXMapWindow(display, window);
        gc = pXCreateGC(display, window, 0, nullptr);
        void* vis = pXDefaultVisual(display, screen);
        int depth = pXDefaultDepth(display, screen);
        ximage = pXCreateImage(display, vis, depth, 2 /*ZPixmap*/, 0, fbData, w, h, 32, w * 4);
        active = true;
        return true;
    }

    void present(int w, int h) {
        if (display && window && gc && ximage) {
            pXPutImage(display, window, gc, ximage, 0, 0, 0, 0, w, h);
            pXFlush(display);
        }
    }

    bool pollEvent(SDL_Event* ev) {
        if (!display || !pXPending) return false;
        while (pXPending(display) > 0) {
            alignas(8) unsigned char xev[192]{};
            pXNextEvent(display, xev);
            std::memset(ev, 0, sizeof(*ev));
            int type = *reinterpret_cast<int*>(xev);
            if (type == 2) { // KeyPress
                unsigned long ks = pXLookupKeysym(xev, 0);
                ev->type = SDL_KEYDOWN;
                if (ks == 0xFF1B) ev->key.keysym.sym = SDLK_ESCAPE;
                else if (ks == 0xFF08) ev->key.keysym.sym = SDLK_BACKSPACE;
                else if (ks == 0xFF09) ev->key.keysym.sym = SDLK_TAB;
                else if (ks == 0xFF0D || ks == 0xFF8D) ev->key.keysym.sym = SDLK_RETURN;
                else if (ks == 0xFF51) ev->key.keysym.sym = SDLK_LEFT;
                else if (ks == 0xFF53) ev->key.keysym.sym = SDLK_RIGHT;
                else if (ks == 0xFFC2) ev->key.keysym.sym = SDLK_F5;
                else if (ks == 0xFFC8) ev->key.keysym.sym = SDLK_F11;
                else if (ks == 0xFFFF) ev->key.keysym.sym = SDLK_DELETE;
                else ev->key.keysym.sym = static_cast<int32_t>(ks & 0xFF);
                return true;
            } else if (type == 4 || type == 5) { // ButtonPress / ButtonRelease
                int x = *reinterpret_cast<int*>(xev + 64);
                int y = *reinterpret_cast<int*>(xev + 68);
                unsigned int btn = *reinterpret_cast<unsigned int*>(xev + 84);
                ev->type = (type == 4) ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
                ev->button.button = (btn == 1) ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT;
                ev->button.x = x;
                ev->button.y = y;
                return true;
            } else if (type == 6) { // MotionNotify
                int x = *reinterpret_cast<int*>(xev + 64);
                int y = *reinterpret_cast<int*>(xev + 68);
                ev->type = SDL_MOUSEMOTION;
                ev->motion.x = x;
                ev->motion.y = y;
                return true;
            } else if (type == 33) { // ClientMessage
                ev->type = SDL_QUIT;
                return true;
            }
        }
        return false;
    }

    void shutdown() {
        if (display && pXCloseDisplay) pXCloseDisplay(display);
        display = nullptr;
        if (lib) dlclose(lib);
        lib = nullptr;
    }
};

// Dynamic SDL2 Function Table
struct DynamicSDL2 {
    void* lib = nullptr;
    bool loaded = false;
    SoftRenderer softRen;
    X11Fallback x11;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

    int (*pSDL_Init)(uint32_t) = nullptr;
    void (*pSDL_Quit)(void) = nullptr;
    const char* (*pSDL_GetError)(void) = nullptr;
    SDL_Window* (*pSDL_CreateWindow)(const char*, int, int, int, int, uint32_t) = nullptr;
    void (*pSDL_DestroyWindow)(SDL_Window*) = nullptr;
    int (*pSDL_SetWindowFullscreen)(SDL_Window*, uint32_t) = nullptr;
    int (*pSDL_RenderSetLogicalSize)(SDL_Renderer*, int, int) = nullptr;
    SDL_Renderer* (*pSDL_CreateRenderer)(SDL_Window*, int, uint32_t) = nullptr;
    void (*pSDL_DestroyRenderer)(SDL_Renderer*) = nullptr;
    int (*pSDL_SetRenderDrawBlendMode)(SDL_Renderer*, int) = nullptr;
    int (*pSDL_SetRenderDrawColor)(SDL_Renderer*, uint8_t, uint8_t, uint8_t, uint8_t) = nullptr;
    int (*pSDL_RenderClear)(SDL_Renderer*) = nullptr;
    void (*pSDL_RenderPresent)(SDL_Renderer*) = nullptr;
    int (*pSDL_RenderFillRectF)(SDL_Renderer*, const SDL_FRect*) = nullptr;
    int (*pSDL_RenderGeometry)(SDL_Renderer*, SDL_Texture*, const SDL_Vertex*, int, const int*, int) = nullptr;
    int (*pSDL_RenderCopyExF)(SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_FRect*, const double, const SDL_FPoint*, const int) = nullptr;
    SDL_Surface* (*pSDL_CreateRGBSurfaceWithFormat)(uint32_t, int, int, int, uint32_t) = nullptr;
    void (*pSDL_FreeSurface)(SDL_Surface*) = nullptr;
    SDL_Texture* (*pSDL_CreateTextureFromSurface)(SDL_Renderer*, SDL_Surface*) = nullptr;
    void (*pSDL_DestroyTexture)(SDL_Texture*) = nullptr;
    int (*pSDL_SetTextureBlendMode)(SDL_Texture*, int) = nullptr;
    int (*pSDL_SetTextureScaleMode)(SDL_Texture*, int) = nullptr;
    SDL_RWops* (*pSDL_RWFromFile)(const char*, const char*) = nullptr;
    SDL_Surface* (*pSDL_LoadBMP_RW)(SDL_RWops*, int) = nullptr;
    int (*pSDL_SaveBMP_RW)(SDL_Surface*, SDL_RWops*, int) = nullptr;
    int (*pSDL_RenderReadPixels)(SDL_Renderer*, const SDL_Rect*, uint32_t, void*, int) = nullptr;
    SDL_AudioSpec* (*pSDL_LoadWAV_RW)(SDL_RWops*, int, SDL_AudioSpec*, uint8_t**, uint32_t*) = nullptr;
    void (*pSDL_FreeWAV)(uint8_t*) = nullptr;
    uint32_t (*pSDL_OpenAudioDevice)(const char*, int, const SDL_AudioSpec*, SDL_AudioSpec*, int) = nullptr;
    void (*pSDL_CloseAudioDevice)(uint32_t) = nullptr;
    int (*pSDL_QueueAudio)(uint32_t, const void*, uint32_t) = nullptr;
    void (*pSDL_ClearQueuedAudio)(uint32_t) = nullptr;
    void (*pSDL_PauseAudioDevice)(uint32_t, int) = nullptr;
    void (*pSDL_LockAudioDevice)(uint32_t) = nullptr;
    void (*pSDL_UnlockAudioDevice)(uint32_t) = nullptr;
    int (*pSDL_PollEvent)(SDL_Event*) = nullptr;
    uint32_t (*pSDL_GetTicks)(void) = nullptr;
    void (*pSDL_Delay)(uint32_t) = nullptr;

    void tryLoad() {
        if (std::getenv("SNOUT_FORCE_X11_FALLBACK")) return;
        const char* candidates[] = {
            "libSDL2-2.0.so.0",
            "libSDL2-2.0.so",
            "libSDL2.so",
            "/usr/lib64/libSDL2-2.0.so.0",
            "/usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0",
            "/usr/lib/libSDL2-2.0.so.0",
            "/lib64/libSDL2-2.0.so.0"
        };
        for (const char* name : candidates) {
            lib = dlopen(name, RTLD_LAZY);
            if (lib) break;
        }
        if (!lib) return;

        #define BIND_SDL(fn) p##fn = reinterpret_cast<decltype(p##fn)>(dlsym(lib, #fn))
        BIND_SDL(SDL_Init);
        BIND_SDL(SDL_Quit);
        BIND_SDL(SDL_GetError);
        BIND_SDL(SDL_CreateWindow);
        BIND_SDL(SDL_DestroyWindow);
        BIND_SDL(SDL_SetWindowFullscreen);
        BIND_SDL(SDL_RenderSetLogicalSize);
        BIND_SDL(SDL_CreateRenderer);
        BIND_SDL(SDL_DestroyRenderer);
        BIND_SDL(SDL_SetRenderDrawBlendMode);
        BIND_SDL(SDL_SetRenderDrawColor);
        BIND_SDL(SDL_RenderClear);
        BIND_SDL(SDL_RenderPresent);
        BIND_SDL(SDL_RenderFillRectF);
        BIND_SDL(SDL_RenderGeometry);
        BIND_SDL(SDL_RenderCopyExF);
        BIND_SDL(SDL_CreateRGBSurfaceWithFormat);
        BIND_SDL(SDL_FreeSurface);
        BIND_SDL(SDL_CreateTextureFromSurface);
        BIND_SDL(SDL_DestroyTexture);
        BIND_SDL(SDL_SetTextureBlendMode);
        BIND_SDL(SDL_SetTextureScaleMode);  // optional (SDL >= 2.0.12)
        BIND_SDL(SDL_RWFromFile);
        BIND_SDL(SDL_LoadBMP_RW);
        BIND_SDL(SDL_SaveBMP_RW);
        BIND_SDL(SDL_RenderReadPixels);
        BIND_SDL(SDL_LoadWAV_RW);
        BIND_SDL(SDL_FreeWAV);
        BIND_SDL(SDL_OpenAudioDevice);
        BIND_SDL(SDL_CloseAudioDevice);
        BIND_SDL(SDL_QueueAudio);
        BIND_SDL(SDL_ClearQueuedAudio);
        BIND_SDL(SDL_PauseAudioDevice);
        BIND_SDL(SDL_LockAudioDevice);
        BIND_SDL(SDL_UnlockAudioDevice);
        BIND_SDL(SDL_PollEvent);
        BIND_SDL(SDL_GetTicks);
        BIND_SDL(SDL_Delay);
        #undef BIND_SDL

        if (pSDL_Init && pSDL_CreateWindow && pSDL_CreateRenderer && pSDL_RenderGeometry) {
            loaded = true;
        }
    }
};

static DynamicSDL2 gSDL;

bool sdlRuntimeLoaded() { return gSDL.loaded; }

// Blend helper for built-in software renderer
static inline void blendPixel(uint32_t& dst, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (a == 0) return;
    if (a == 255) {
        dst = (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) |
              (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
        return;
    }
    uint32_t invA = 255 - a;
    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8) & 0xFF;
    uint32_t db = dst & 0xFF;
    uint32_t nr = (r * a + dr * invA) >> 8;
    uint32_t ng = (g * a + dg * invA) >> 8;
    uint32_t nb = (b * a + db * invA) >> 8;
    dst = (0xFFu << 24) | (nr << 16) | (ng << 8) | nb;
}

// Samples a soft texture at texel-space (u, v); out = {a, r, g, b}.
static void sampleTexel(const SoftTexture& t, float u, float v, float out[4]) {
    auto fetch = [&](int x, int y) {
        x = std::clamp(x, 0, t.w - 1);
        y = std::clamp(y, 0, t.h - 1);
        return t.pixels[y * t.w + x];
    };
    auto channels = [](uint32_t p, float c[4]) {
        c[0] = static_cast<float>((p >> 24) & 0xFF);
        c[1] = static_cast<float>((p >> 16) & 0xFF);
        c[2] = static_cast<float>((p >> 8) & 0xFF);
        c[3] = static_cast<float>(p & 0xFF);
    };
    if (!t.linear) {
        channels(fetch(static_cast<int>(std::floor(u)), static_cast<int>(std::floor(v))), out);
        return;
    }
    float fx = u - 0.5f, fy = v - 0.5f;
    int x0 = static_cast<int>(std::floor(fx)), y0 = static_cast<int>(std::floor(fy));
    float tx = fx - x0, ty = fy - y0;
    float c00[4], c10[4], c01[4], c11[4];
    channels(fetch(x0, y0), c00);
    channels(fetch(x0 + 1, y0), c10);
    channels(fetch(x0, y0 + 1), c01);
    channels(fetch(x0 + 1, y0 + 1), c11);
    for (int i = 0; i < 4; ++i) {
        float top = c00[i] + (c10[i] - c00[i]) * tx;
        float bot = c01[i] + (c11[i] - c01[i]) * tx;
        out[i] = top + (bot - top) * ty;
    }
}

// Alpha-only bilinear/nearest sample for white textures.
static float sampleAlpha(const SoftTexture& t, float u, float v) {
    auto a = [&](int x, int y) {
        x = std::clamp(x, 0, t.w - 1);
        y = std::clamp(y, 0, t.h - 1);
        return static_cast<float>(t.pixels[y * t.w + x] >> 24);
    };
    if (!t.linear) return a(static_cast<int>(std::floor(u)), static_cast<int>(std::floor(v)));
    float fx = u - 0.5f, fy = v - 0.5f;
    int x0 = static_cast<int>(std::floor(fx)), y0 = static_cast<int>(std::floor(fy));
    float tx = fx - x0, ty = fy - y0;
    float top = a(x0, y0) + (a(x0 + 1, y0) - a(x0, y0)) * tx;
    float bot = a(x0, y0 + 1) + (a(x0 + 1, y0 + 1) - a(x0, y0 + 1)) * tx;
    return top + (bot - top) * ty;
}

int SDL_Init(uint32_t flags) {
    gSDL.tryLoad();
    if (gSDL.loaded) return gSDL.pSDL_Init(flags);
    gSDL.softRen.fb.assign(1280 * 720, 0xFF1C2438u);
    return 0;
}

void SDL_Quit(void) {
    if (gSDL.loaded) { gSDL.pSDL_Quit(); return; }
    gSDL.x11.shutdown();
}

const char* SDL_GetError(void) {
    if (gSDL.loaded) return gSDL.pSDL_GetError();
    return "X11/Software Fallback Error";
}

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags) {
    if (gSDL.loaded) return gSDL.pSDL_CreateWindow(title, x, y, w, h, flags);
    gSDL.softRen.w = w;
    gSDL.softRen.h = h;
    gSDL.softRen.fb.assign(w * h, 0xFF1C2438u);
    if (!gSDL.x11.init(title, w, h, reinterpret_cast<char*>(gSDL.softRen.fb.data()))) {
        return nullptr;
    }
    return reinterpret_cast<SDL_Window*>(&gSDL.x11);
}

void SDL_DestroyWindow(SDL_Window* window) {
    if (gSDL.loaded && window) gSDL.pSDL_DestroyWindow(window);
}

int SDL_SetWindowFullscreen(SDL_Window* window, uint32_t flags) {
    if (gSDL.loaded && window && gSDL.pSDL_SetWindowFullscreen) {
        return gSDL.pSDL_SetWindowFullscreen(window, flags);
    }
    return 0;
}

int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h) {
    if (gSDL.loaded && renderer && gSDL.pSDL_RenderSetLogicalSize) {
        return gSDL.pSDL_RenderSetLogicalSize(renderer, w, h);
    }
    return 0;
}

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags) {
    if (gSDL.loaded) return gSDL.pSDL_CreateRenderer(window, index, flags);
    return reinterpret_cast<SDL_Renderer*>(&gSDL.softRen);
}

void SDL_DestroyRenderer(SDL_Renderer* renderer) {
    if (gSDL.loaded && renderer) gSDL.pSDL_DestroyRenderer(renderer);
}

int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode) {
    if (gSDL.loaded) return gSDL.pSDL_SetRenderDrawBlendMode(renderer, blendMode);
    return 0;
}

int SDL_SetRenderDrawColor(SDL_Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (gSDL.loaded) return gSDL.pSDL_SetRenderDrawColor(renderer, r, g, b, a);
    gSDL.softRen.drawR = r;
    gSDL.softRen.drawG = g;
    gSDL.softRen.drawB = b;
    gSDL.softRen.drawA = a;
    return 0;
}

int SDL_RenderClear(SDL_Renderer* renderer) {
    if (gSDL.loaded) return gSDL.pSDL_RenderClear(renderer);
    uint32_t col = (0xFFu << 24) | (static_cast<uint32_t>(gSDL.softRen.drawR) << 16) |
                   (static_cast<uint32_t>(gSDL.softRen.drawG) << 8) | gSDL.softRen.drawB;
    std::fill(gSDL.softRen.fb.begin(), gSDL.softRen.fb.end(), col);
    return 0;
}

void SDL_RenderPresent(SDL_Renderer* renderer) {
    if (gSDL.loaded) { gSDL.pSDL_RenderPresent(renderer); return; }
    gSDL.x11.present(gSDL.softRen.w, gSDL.softRen.h);
}

int SDL_RenderFillRectF(SDL_Renderer* renderer, const SDL_FRect* rect) {
    if (gSDL.loaded) return gSDL.pSDL_RenderFillRectF(renderer, rect);
    if (!rect) return 0;
    int x0 = std::max(0, static_cast<int>(rect->x));
    int y0 = std::max(0, static_cast<int>(rect->y));
    int x1 = std::min(gSDL.softRen.w, static_cast<int>(rect->x + rect->w));
    int y1 = std::min(gSDL.softRen.h, static_cast<int>(rect->y + rect->h));
    for (int y = y0; y < y1; ++y) {
        uint32_t* row = &gSDL.softRen.fb[y * gSDL.softRen.w];
        for (int x = x0; x < x1; ++x) {
            blendPixel(row[x], gSDL.softRen.drawR, gSDL.softRen.drawG,
                       gSDL.softRen.drawB, gSDL.softRen.drawA);
        }
    }
    return 0;
}

int SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture,
                       const SDL_Vertex* vertices, int num_vertices,
                       const int* indices, int num_indices) {
    if (gSDL.loaded) {
        return gSDL.pSDL_RenderGeometry(renderer, texture, vertices, num_vertices, indices, num_indices);
    }
    const SoftTexture* tex = reinterpret_cast<const SoftTexture*>(texture);
    auto rasterTri = [&](const SDL_Vertex& v0, const SDL_Vertex& v1, const SDL_Vertex& v2) {
        float minX = std::min({v0.position.x, v1.position.x, v2.position.x});
        float maxX = std::max({v0.position.x, v1.position.x, v2.position.x});
        float minY = std::min({v0.position.y, v1.position.y, v2.position.y});
        float maxY = std::max({v0.position.y, v1.position.y, v2.position.y});
        int x0 = std::max(0, static_cast<int>(std::floor(minX)));
        int x1 = std::min(gSDL.softRen.w - 1, static_cast<int>(std::ceil(maxX)));
        int y0 = std::max(0, static_cast<int>(std::floor(minY)));
        int y1 = std::min(gSDL.softRen.h - 1, static_cast<int>(std::ceil(maxY)));
        if (x0 > x1 || y0 > y1) return;
        float area = (v1.position.x - v0.position.x) * (v2.position.y - v0.position.y) -
                     (v1.position.y - v0.position.y) * (v2.position.x - v0.position.x);
        if (std::abs(area) < 1e-4f) return;
        float invArea = 1.0f / area;
        // Barycentric weights are linear in x: w = A * px + B(py).
        const float A0 = (v1.position.y - v2.position.y) * invArea;
        const float A1 = (v2.position.y - v0.position.y) * invArea;
        const bool flat = !tex &&
            std::memcmp(&v0.color, &v1.color, sizeof(SDL_Color)) == 0 &&
            std::memcmp(&v0.color, &v2.color, sizeof(SDL_Color)) == 0;
        const float eps = 1e-4f;
        for (int y = y0; y <= y1; ++y) {
            float py = y + 0.5f;
            float B0 = ((v1.position.x) * (v2.position.y - py) - (v1.position.y - py) * (v2.position.x)) * invArea;
            float B1 = ((v2.position.x) * (v0.position.y - py) - (v2.position.y - py) * (v0.position.x)) * invArea;
            float A2 = -(A0 + A1), B2 = 1.0f - B0 - B1;
            // Intersect the three half-planes to get this row's span.
            float lo = x0 + 0.5f, hi = x1 + 0.5f;
            bool empty = false;
            for (int e = 0; e < 3 && !empty; ++e) {
                float A = e == 0 ? A0 : (e == 1 ? A1 : A2);
                float B = e == 0 ? B0 : (e == 1 ? B1 : B2);
                if (A > 1e-9f) lo = std::max(lo, (-eps - B) / A);
                else if (A < -1e-9f) hi = std::min(hi, (-eps - B) / A);
                else if (B < -eps) empty = true;
            }
            if (empty || lo > hi) continue;
            int sx = std::max(x0, static_cast<int>(std::ceil(lo - 0.5f)));
            int ex = std::min(x1, static_cast<int>(std::floor(hi - 0.5f)));
            uint32_t* row = &gSDL.softRen.fb[y * gSDL.softRen.w];
            if (flat) {
                for (int x = sx; x <= ex; ++x) {
                    blendPixel(row[x], v0.color.r, v0.color.g, v0.color.b, v0.color.a);
                }
                continue;
            }
            for (int x = sx; x <= ex; ++x) {
                float px = x + 0.5f;
                float w0 = A0 * px + B0, w1 = A1 * px + B1, w2 = 1.0f - w0 - w1;
                float r = v0.color.r * w0 + v1.color.r * w1 + v2.color.r * w2;
                float g = v0.color.g * w0 + v1.color.g * w1 + v2.color.g * w2;
                float b = v0.color.b * w0 + v1.color.b * w1 + v2.color.b * w2;
                float a = v0.color.a * w0 + v1.color.a * w1 + v2.color.a * w2;
                if (tex && tex->whiteRGB) {
                    float u = (v0.tex_coord.x * w0 + v1.tex_coord.x * w1 + v2.tex_coord.x * w2) * tex->w;
                    float v = (v0.tex_coord.y * w0 + v1.tex_coord.y * w1 + v2.tex_coord.y * w2) * tex->h;
                    float ta = sampleAlpha(*tex, u, v);
                    if (ta < 0.5f) continue;  // transparent texel: nothing to blend
                    a *= ta / 255.0f;
                } else if (tex) {
                    // Texel modulated by the interpolated vertex colour, as SDL does.
                    float u = (v0.tex_coord.x * w0 + v1.tex_coord.x * w1 + v2.tex_coord.x * w2) * tex->w;
                    float v = (v0.tex_coord.y * w0 + v1.tex_coord.y * w1 + v2.tex_coord.y * w2) * tex->h;
                    float t[4];
                    sampleTexel(*tex, u, v, t);
                    a *= t[0] / 255.0f;
                    r *= t[1] / 255.0f;
                    g *= t[2] / 255.0f;
                    b *= t[3] / 255.0f;
                }
                blendPixel(row[x], static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                           static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                           static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f)),
                           static_cast<uint8_t>(std::clamp(a, 0.0f, 255.0f)));
            }
        }
    };
    if (indices && num_indices >= 3) {
        for (int i = 0; i + 2 < num_indices; i += 3) {
            rasterTri(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]);
        }
    } else {
        for (int i = 0; i + 2 < num_vertices; i += 3) {
            rasterTri(vertices[i], vertices[i + 1], vertices[i + 2]);
        }
    }
    return 0;
}

int SDL_RenderCopyExF(SDL_Renderer* renderer, SDL_Texture* texture,
                      const SDL_Rect* srcrect, const SDL_FRect* dstrect,
                      const double angle, const SDL_FPoint* center, const int flip) {
    if (gSDL.loaded) {
        return gSDL.pSDL_RenderCopyExF(renderer, texture, srcrect, dstrect, angle, center, flip);
    }
    if (!texture || !dstrect) return 0;
    auto* st = reinterpret_cast<SoftTexture*>(texture);
    float cx = dstrect->x + dstrect->w * 0.5f;
    float cy = dstrect->y + dstrect->h * 0.5f;
    float rad = static_cast<float>(-angle * 3.14159265358979323846 / 180.0);
    float c = std::cos(rad), s = std::sin(rad);
    float maxR = 0.55f * std::sqrt(dstrect->w * dstrect->w + dstrect->h * dstrect->h);
    int x0 = std::max(0, static_cast<int>(cx - maxR));
    int x1 = std::min(gSDL.softRen.w - 1, static_cast<int>(cx + maxR));
    int y0 = std::max(0, static_cast<int>(cy - maxR));
    int y1 = std::min(gSDL.softRen.h - 1, static_cast<int>(cy + maxR));
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            float dx = (x + 0.5f) - cx;
            float dy = (y + 0.5f) - cy;
            float lx = dx * c - dy * s + dstrect->w * 0.5f;
            float ly = dx * s + dy * c + dstrect->h * 0.5f;
            if (lx >= 0.0f && lx < dstrect->w && ly >= 0.0f && ly < dstrect->h) {
                int tx = std::min(st->w - 1, static_cast<int>((lx / dstrect->w) * st->w));
                int ty = std::min(st->h - 1, static_cast<int>((ly / dstrect->h) * st->h));
                uint32_t px = st->pixels[ty * st->w + tx];
                uint8_t a = (px >> 24) & 0xFF;
                uint8_t r = (px >> 16) & 0xFF;
                uint8_t g = (px >> 8) & 0xFF;
                uint8_t b = px & 0xFF;
                blendPixel(gSDL.softRen.fb[y * gSDL.softRen.w + x], r, g, b, a);
            }
        }
    }
    return 0;
}

int SDL_RenderCopyF(SDL_Renderer* renderer, SDL_Texture* texture,
                    const SDL_Rect* srcrect, const SDL_FRect* dstrect) {
    return SDL_RenderCopyExF(renderer, texture, srcrect, dstrect, 0.0, nullptr, SDL_FLIP_NONE);
}

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(uint32_t flags, int width, int height,
                                            int depth, uint32_t format) {
    if (gSDL.loaded) return gSDL.pSDL_CreateRGBSurfaceWithFormat(flags, width, height, depth, format);
    auto* surf = new SDL_Surface{};
    surf->w = width;
    surf->h = height;
    surf->pitch = width * 4;
    surf->pixels = new uint32_t[width * height]();
    return surf;
}

void SDL_FreeSurface(SDL_Surface* surface) {
    if (gSDL.loaded) { gSDL.pSDL_FreeSurface(surface); return; }
    if (!surface) return;
    delete[] static_cast<uint32_t*>(surface->pixels);
    delete surface;
}

SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
    if (gSDL.loaded) return gSDL.pSDL_CreateTextureFromSurface(renderer, surface);
    if (!surface) return nullptr;
    auto* st = new SoftTexture{};
    st->w = surface->w;
    st->h = surface->h;
    const uint32_t* src = static_cast<const uint32_t*>(surface->pixels);
    st->pixels.assign(src, src + st->w * st->h);
    st->whiteRGB = std::all_of(st->pixels.begin(), st->pixels.end(),
                               [](uint32_t p) { return (p & 0x00FFFFFFu) == 0x00FFFFFFu; });
    return reinterpret_cast<SDL_Texture*>(st);
}

void SDL_DestroyTexture(SDL_Texture* texture) {
    if (gSDL.loaded) { gSDL.pSDL_DestroyTexture(texture); return; }
    delete reinterpret_cast<SoftTexture*>(texture);
}

int SDL_SetTextureBlendMode(SDL_Texture* texture, int blendMode) {
    if (gSDL.loaded) return gSDL.pSDL_SetTextureBlendMode(texture, blendMode);
    return 0;
}

int SDL_SetTextureScaleMode(SDL_Texture* texture, int scaleMode) {
    if (gSDL.loaded) {
        return gSDL.pSDL_SetTextureScaleMode ? gSDL.pSDL_SetTextureScaleMode(texture, scaleMode) : -1;
    }
    if (texture) reinterpret_cast<SoftTexture*>(texture)->linear = (scaleMode != 0);
    return 0;
}

SDL_RWops* SDL_RWFromFile(const char* file, const char* mode) {
    if (gSDL.loaded) return gSDL.pSDL_RWFromFile(file, mode);
    return reinterpret_cast<SDL_RWops*>(const_cast<char*>(file));
}

int SDL_SaveBMP_RW(SDL_Surface* surface, SDL_RWops* dst, int freedst) {
    if (gSDL.loaded) return gSDL.pSDL_SaveBMP_RW(surface, dst, freedst);
    if (!surface || !dst) return -1;
    const char* path = reinterpret_cast<const char*>(dst);
    std::ofstream out(path, std::ios::binary);
    if (!out.good()) return -1;
    uint32_t w = surface->w, h = surface->h;
    uint32_t dataBytes = w * h * 4;
    uint32_t fileSize = 54 + dataBytes;
    uint32_t offset = 54, headerSize = 40;
    uint16_t planes = 1, bpp = 32;
    uint32_t comp = 0, ppm = 2835, zero = 0;
    out.write("BM", 2);
    out.write(reinterpret_cast<const char*>(&fileSize), 4);
    out.write(reinterpret_cast<const char*>(&zero), 4);
    out.write(reinterpret_cast<const char*>(&offset), 4);
    out.write(reinterpret_cast<const char*>(&headerSize), 4);
    out.write(reinterpret_cast<const char*>(&w), 4);
    int32_t negH = -static_cast<int32_t>(h); // Top-down 32-bit ARGB BMP
    out.write(reinterpret_cast<const char*>(&negH), 4);
    out.write(reinterpret_cast<const char*>(&planes), 2);
    out.write(reinterpret_cast<const char*>(&bpp), 2);
    out.write(reinterpret_cast<const char*>(&comp), 4);
    out.write(reinterpret_cast<const char*>(&dataBytes), 4);
    out.write(reinterpret_cast<const char*>(&ppm), 4);
    out.write(reinterpret_cast<const char*>(&ppm), 4);
    out.write(reinterpret_cast<const char*>(&zero), 4);
    out.write(reinterpret_cast<const char*>(&zero), 4);
    out.write(reinterpret_cast<const char*>(surface->pixels), dataBytes);
    return 0;
}

SDL_Surface* SDL_LoadBMP_RW(SDL_RWops* src, int freesrc) {
    if (gSDL.loaded) return gSDL.pSDL_LoadBMP_RW(src, freesrc);
    if (!src) return nullptr;
    const char* path = reinterpret_cast<const char*>(src);
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return nullptr;
    char sig[2];
    in.read(sig, 2);
    if (sig[0] != 'B' || sig[1] != 'M') return nullptr;
    uint32_t fileSize = 0, reserved = 0, dataOffset = 0, hdrSize = 0;
    int32_t w = 0, h = 0;
    uint16_t planes = 0, bpp = 0;
    in.read(reinterpret_cast<char*>(&fileSize), 4);
    in.read(reinterpret_cast<char*>(&reserved), 4);
    in.read(reinterpret_cast<char*>(&dataOffset), 4);
    in.read(reinterpret_cast<char*>(&hdrSize), 4);
    in.read(reinterpret_cast<char*>(&w), 4);
    in.read(reinterpret_cast<char*>(&h), 4);
    in.read(reinterpret_cast<char*>(&planes), 2);
    in.read(reinterpret_cast<char*>(&bpp), 2);
    if (w <= 0 || std::abs(h) <= 0 || bpp != 32) return nullptr;
    int absH = std::abs(h);
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, absH, 32, SDL_PIXELFORMAT_ARGB8888);
    in.seekg(dataOffset, std::ios::beg);
    uint32_t* dstPx = static_cast<uint32_t*>(surf->pixels);
    if (h < 0) {
        in.read(reinterpret_cast<char*>(dstPx), w * absH * 4);
    } else {
        for (int row = absH - 1; row >= 0; --row) {
            in.read(reinterpret_cast<char*>(dstPx + row * w), w * 4);
        }
    }
    return surf;
}

int SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect,
                         uint32_t format, void* pixels, int pitch) {
    if (gSDL.loaded) return gSDL.pSDL_RenderReadPixels(renderer, rect, format, pixels, pitch);
    std::memcpy(pixels, gSDL.softRen.fb.data(), gSDL.softRen.w * gSDL.softRen.h * 4);
    return 0;
}

SDL_AudioSpec* SDL_LoadWAV_RW(SDL_RWops* src, int freesrc, SDL_AudioSpec* spec,
                              uint8_t** audio_buf, uint32_t* audio_len) {
    if (gSDL.loaded) return gSDL.pSDL_LoadWAV_RW(src, freesrc, spec, audio_buf, audio_len);
    if (!src || !spec || !audio_buf || !audio_len) return nullptr;
    const char* path = reinterpret_cast<const char*>(src);
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.good()) return nullptr;
    auto sz = in.tellg();
    if (sz <= 44) return nullptr;
    *audio_len = static_cast<uint32_t>(sz) - 44;
    *audio_buf = new uint8_t[*audio_len];
    in.seekg(44, std::ios::beg);
    in.read(reinterpret_cast<char*>(*audio_buf), *audio_len);
    spec->freq = 44100;
    spec->channels = 1;
    return spec;
}

void SDL_FreeWAV(uint8_t* audio_buf) {
    if (gSDL.loaded) { gSDL.pSDL_FreeWAV(audio_buf); return; }
    delete[] audio_buf;
}

uint32_t SDL_OpenAudioDevice(const char* device, int iscapture,
                             const SDL_AudioSpec* desired, SDL_AudioSpec* obtained,
                             int allowed_changes) {
    if (gSDL.loaded) return gSDL.pSDL_OpenAudioDevice(device, iscapture, desired, obtained, allowed_changes);
    return 1;
}

void SDL_CloseAudioDevice(uint32_t dev) {
    if (gSDL.loaded) gSDL.pSDL_CloseAudioDevice(dev);
}

[[maybe_unused]] static int SDL_QueueAudio(uint32_t dev, const void* data, uint32_t len) {
    if (gSDL.loaded) return gSDL.pSDL_QueueAudio(dev, data, len);
    return 0;
}

[[maybe_unused]] static void SDL_ClearQueuedAudio(uint32_t dev) {
    if (gSDL.loaded) gSDL.pSDL_ClearQueuedAudio(dev);
}

void SDL_PauseAudioDevice(uint32_t dev, int pause_on) {
    if (gSDL.loaded) gSDL.pSDL_PauseAudioDevice(dev, pause_on);
}

void SDL_LockAudioDevice(uint32_t dev) {
    if (gSDL.loaded && gSDL.pSDL_LockAudioDevice) gSDL.pSDL_LockAudioDevice(dev);
}

void SDL_UnlockAudioDevice(uint32_t dev) {
    if (gSDL.loaded && gSDL.pSDL_UnlockAudioDevice) gSDL.pSDL_UnlockAudioDevice(dev);
}

int SDL_PollEvent(SDL_Event* event) {
    if (gSDL.loaded) return gSDL.pSDL_PollEvent(event);
    return gSDL.x11.pollEvent(event) ? 1 : 0;
}

uint32_t SDL_GetTicks(void) {
    if (gSDL.loaded) return gSDL.pSDL_GetTicks();
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - gSDL.startTime).count());
}

void SDL_Delay(uint32_t ms) {
    if (gSDL.loaded) { gSDL.pSDL_Delay(ms); return; }
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

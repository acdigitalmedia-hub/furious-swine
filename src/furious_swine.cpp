// ============================================================================
// SNOUT & SIEGE: RISE OF THE SWINE (furious_swine.cpp)
// A C++17 2D Physics Artillery Game for Linux (Zero -lSDL2 Linker Dependency)
//
// Why this compiles out-of-the-box on any Linux distro:
//   - Compiles with standard glibc flags only:
//       g++ -std=c++17 -O2 src/furious_swine.cpp -o furious_swine -ldl -lm
//   - At runtime, dynamically loads `libSDL2-2.0.so.0` via dlopen() from
//     /usr/lib64, /usr/lib, or /usr/lib/x86_64-linux-gnu (no SDL2-devel needed).
//   - If SDL2 runtime is not installed at all, automatically falls back to
//     native X11 (`libX11.so.6`) with a built-in 32-bit ARGB software renderer!
// ============================================================================

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
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

// ============================================================================
// PORTABLE SDL2 ABI TYPES & DYNAMIC RUNTIME DISPATCHER (SDL2 + X11 FALLBACK)
// ============================================================================
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_RWops;

struct SDL_Rect { int x, y, w, h; };
struct SDL_FRect { float x, y, w, h; };
struct SDL_Point { int x, y; };
struct SDL_FPoint { float x, y; };
struct SDL_Color { uint8_t r, g, b, a; };
struct SDL_Vertex { SDL_FPoint position; SDL_Color color; SDL_FPoint tex_coord; };

struct SDL_Surface {
    uint32_t flags;
    void* format;
    int w, h;
    int pitch;
    void* pixels;
    void* userdata;
    int locked;
    void* list_blitmap;
    SDL_Rect clip_rect;
    void* map;
    int refcount;
};

struct SDL_Keysym {
    int32_t scancode;
    int32_t sym;
    uint16_t mod;
    uint32_t unused;
};

struct SDL_KeyboardEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym keysym;
};

struct SDL_MouseMotionEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint32_t state;
    int32_t x;
    int32_t y;
    int32_t xrel;
    int32_t yrel;
};

struct SDL_MouseButtonEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint8_t button;
    uint8_t state;
    uint8_t clicks;
    uint8_t padding1;
    int32_t x;
    int32_t y;
};

union SDL_Event {
    uint32_t type;
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    uint8_t padding[56];
};

struct SDL_AudioSpec {
    int freq;
    uint16_t format;
    uint8_t channels;
    uint8_t silence;
    uint16_t samples;
    uint16_t padding;
    uint32_t size;
    void (*callback)(void* userdata, uint8_t* stream, int len);
    void* userdata;
};

enum {
    SDL_INIT_AUDIO = 0x00000010u,
    SDL_INIT_VIDEO = 0x00000020u,
    SDL_WINDOWPOS_CENTERED = 0x2FFF0000u,
    SDL_WINDOW_SHOWN = 0x00000004u,
    SDL_WINDOW_RESIZABLE = 0x00000020u,
    SDL_WINDOW_FULLSCREEN_DESKTOP = 0x00001001u,
    SDL_RENDERER_SOFTWARE = 0x00000001u,
    SDL_RENDERER_ACCELERATED = 0x00000002u,
    SDL_RENDERER_PRESENTVSYNC = 0x00000004u,
    SDL_BLENDMODE_NONE = 0x00000000u,
    SDL_BLENDMODE_BLEND = 0x00000001u,
    SDL_FLIP_NONE = 0x00000000,
    SDL_PIXELFORMAT_ARGB8888 = 0x16362004u,
    SDL_QUIT = 0x100u,
    SDL_KEYDOWN = 0x300u,
    SDL_KEYUP = 0x301u,
    SDL_MOUSEMOTION = 0x400u,
    SDL_MOUSEBUTTONDOWN = 0x401u,
    SDL_MOUSEBUTTONUP = 0x402u,
    SDL_BUTTON_LEFT = 1,
    SDL_BUTTON_RIGHT = 3,
    SDLK_BACKSPACE = 8,
    SDLK_TAB = 9,
    SDLK_RETURN = 13,
    SDLK_ESCAPE = 27,
    SDLK_SPACE = ' ',
    SDLK_r = 'r',
    SDLK_s = 's',
    SDLK_n = 'n',
    SDLK_p = 'p',
    SDLK_m = 'm',
    SDLK_b = 'b',
    SDLK_v = 'v',
    SDLK_1 = '1',
    SDLK_2 = '2',
    SDLK_3 = '3',
    SDLK_4 = '4',
    SDLK_5 = '5',
    SDLK_f = 'f',
    SDLK_a = 'a',
    SDLK_d = 'd',
    SDLK_c = 'c',
    SDLK_l = 'l',
    SDLK_t = 't',
    SDLK_g = 'g',
    SDLK_DELETE = 127,
    SDLK_RIGHT = (1 << 30) | 79,
    SDLK_LEFT = (1 << 30) | 80,
    SDLK_F5 = (1 << 30) | 62,
    SDLK_F11 = (1 << 30) | 68
};

// ============================================================================
// BUILT-IN SOFTWARE RASTERIZER + NATIVE X11 FALLBACK (IF SDL2 IS NOT INSTALLED)
// ============================================================================
struct SoftTexture {
    int w = 0, h = 0;
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

static int SDL_Init(uint32_t flags) {
    gSDL.tryLoad();
    if (gSDL.loaded) return gSDL.pSDL_Init(flags);
    gSDL.softRen.fb.assign(1280 * 720, 0xFF1C2438u);
    return 0;
}

static void SDL_Quit(void) {
    if (gSDL.loaded) { gSDL.pSDL_Quit(); return; }
    gSDL.x11.shutdown();
}

static const char* SDL_GetError(void) {
    if (gSDL.loaded) return gSDL.pSDL_GetError();
    return "X11/Software Fallback Error";
}

static SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags) {
    if (gSDL.loaded) return gSDL.pSDL_CreateWindow(title, x, y, w, h, flags);
    gSDL.softRen.w = w;
    gSDL.softRen.h = h;
    gSDL.softRen.fb.assign(w * h, 0xFF1C2438u);
    if (!gSDL.x11.init(title, w, h, reinterpret_cast<char*>(gSDL.softRen.fb.data()))) {
        return nullptr;
    }
    return reinterpret_cast<SDL_Window*>(&gSDL.x11);
}

static void SDL_DestroyWindow(SDL_Window* window) {
    if (gSDL.loaded && window) gSDL.pSDL_DestroyWindow(window);
}

static int SDL_SetWindowFullscreen(SDL_Window* window, uint32_t flags) {
    if (gSDL.loaded && window && gSDL.pSDL_SetWindowFullscreen) {
        return gSDL.pSDL_SetWindowFullscreen(window, flags);
    }
    return 0;
}

static int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h) {
    if (gSDL.loaded && renderer && gSDL.pSDL_RenderSetLogicalSize) {
        return gSDL.pSDL_RenderSetLogicalSize(renderer, w, h);
    }
    return 0;
}

static SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags) {
    if (gSDL.loaded) return gSDL.pSDL_CreateRenderer(window, index, flags);
    return reinterpret_cast<SDL_Renderer*>(&gSDL.softRen);
}

static void SDL_DestroyRenderer(SDL_Renderer* renderer) {
    if (gSDL.loaded && renderer) gSDL.pSDL_DestroyRenderer(renderer);
}

static int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode) {
    if (gSDL.loaded) return gSDL.pSDL_SetRenderDrawBlendMode(renderer, blendMode);
    return 0;
}

static int SDL_SetRenderDrawColor(SDL_Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (gSDL.loaded) return gSDL.pSDL_SetRenderDrawColor(renderer, r, g, b, a);
    gSDL.softRen.drawR = r;
    gSDL.softRen.drawG = g;
    gSDL.softRen.drawB = b;
    gSDL.softRen.drawA = a;
    return 0;
}

static int SDL_RenderClear(SDL_Renderer* renderer) {
    if (gSDL.loaded) return gSDL.pSDL_RenderClear(renderer);
    uint32_t col = (0xFFu << 24) | (static_cast<uint32_t>(gSDL.softRen.drawR) << 16) |
                   (static_cast<uint32_t>(gSDL.softRen.drawG) << 8) | gSDL.softRen.drawB;
    std::fill(gSDL.softRen.fb.begin(), gSDL.softRen.fb.end(), col);
    return 0;
}

static void SDL_RenderPresent(SDL_Renderer* renderer) {
    if (gSDL.loaded) { gSDL.pSDL_RenderPresent(renderer); return; }
    gSDL.x11.present(gSDL.softRen.w, gSDL.softRen.h);
}

static int SDL_RenderFillRectF(SDL_Renderer* renderer, const SDL_FRect* rect) {
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

static int SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture,
                              const SDL_Vertex* vertices, int num_vertices,
                              const int* indices, int num_indices) {
    if (gSDL.loaded) {
        return gSDL.pSDL_RenderGeometry(renderer, texture, vertices, num_vertices, indices, num_indices);
    }
    auto rasterTri = [&](const SDL_Vertex& v0, const SDL_Vertex& v1, const SDL_Vertex& v2) {
        float minX = std::min({v0.position.x, v1.position.x, v2.position.x});
        float maxX = std::max({v0.position.x, v1.position.x, v2.position.x});
        float minY = std::min({v0.position.y, v1.position.y, v2.position.y});
        float maxY = std::max({v0.position.y, v1.position.y, v2.position.y});
        int x0 = std::max(0, static_cast<int>( std::floor(minX)));
        int x1 = std::min(gSDL.softRen.w - 1, static_cast<int>(std::ceil(maxX)));
        int y0 = std::max(0, static_cast<int>(std::floor(minY)));
        int y1 = std::min(gSDL.softRen.h - 1, static_cast<int>(std::ceil(maxY)));
        float area = (v1.position.x - v0.position.x) * (v2.position.y - v0.position.y) -
                     (v1.position.y - v0.position.y) * (v2.position.x - v0.position.x);
        if (std::abs(area) < 1e-4f) return;
        float invArea = 1.0f / area;
        for (int y = y0; y <= y1; ++y) {
            float py = y + 0.5f;
            for (int x = x0; x <= x1; ++x) {
                float px = x + 0.5f;
                float w0 = ((v1.position.x - px) * (v2.position.y - py) -
                            (v1.position.y - py) * (v2.position.x - px)) * invArea;
                float w1 = ((v2.position.x - px) * (v0.position.y - py) -
                            (v2.position.y - py) * (v0.position.x - px)) * invArea;
                float w2 = 1.0f - w0 - w1;
                if (w0 >= -1e-4f && w1 >= -1e-4f && w2 >= -1e-4f) {
                    uint8_t r = static_cast<uint8_t>(v0.color.r * w0 + v1.color.r * w1 + v2.color.r * w2);
                    uint8_t g = static_cast<uint8_t>(v0.color.g * w0 + v1.color.g * w1 + v2.color.g * w2);
                    uint8_t b = static_cast<uint8_t>(v0.color.b * w0 + v1.color.b * w1 + v2.color.b * w2);
                    uint8_t a = static_cast<uint8_t>(v0.color.a * w0 + v1.color.a * w1 + v2.color.a * w2);
                    blendPixel(gSDL.softRen.fb[y * gSDL.softRen.w + x], r, g, b, a);
                }
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

static int SDL_RenderCopyExF(SDL_Renderer* renderer, SDL_Texture* texture,
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

static int SDL_RenderCopyF(SDL_Renderer* renderer, SDL_Texture* texture,
                           const SDL_Rect* srcrect, const SDL_FRect* dstrect) {
    return SDL_RenderCopyExF(renderer, texture, srcrect, dstrect, 0.0, nullptr, SDL_FLIP_NONE);
}

static SDL_Surface* SDL_CreateRGBSurfaceWithFormat(uint32_t flags, int width, int height,
                                                   int depth, uint32_t format) {
    if (gSDL.loaded) return gSDL.pSDL_CreateRGBSurfaceWithFormat(flags, width, height, depth, format);
    auto* surf = new SDL_Surface{};
    surf->w = width;
    surf->h = height;
    surf->pitch = width * 4;
    surf->pixels = new uint32_t[width * height]();
    return surf;
}

static void SDL_FreeSurface(SDL_Surface* surface) {
    if (gSDL.loaded) { gSDL.pSDL_FreeSurface(surface); return; }
    if (!surface) return;
    delete[] static_cast<uint32_t*>(surface->pixels);
    delete surface;
}

static SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
    if (gSDL.loaded) return gSDL.pSDL_CreateTextureFromSurface(renderer, surface);
    if (!surface) return nullptr;
    auto* st = new SoftTexture{};
    st->w = surface->w;
    st->h = surface->h;
    const uint32_t* src = static_cast<const uint32_t*>(surface->pixels);
    st->pixels.assign(src, src + st->w * st->h);
    return reinterpret_cast<SDL_Texture*>(st);
}

static void SDL_DestroyTexture(SDL_Texture* texture) {
    if (gSDL.loaded) { gSDL.pSDL_DestroyTexture(texture); return; }
    delete reinterpret_cast<SoftTexture*>(texture);
}

static int SDL_SetTextureBlendMode(SDL_Texture* texture, int blendMode) {
    if (gSDL.loaded) return gSDL.pSDL_SetTextureBlendMode(texture, blendMode);
    return 0;
}

static SDL_RWops* SDL_RWFromFile(const char* file, const char* mode) {
    if (gSDL.loaded) return gSDL.pSDL_RWFromFile(file, mode);
    return reinterpret_cast<SDL_RWops*>(const_cast<char*>(file));
}

static int SDL_SaveBMP_RW(SDL_Surface* surface, SDL_RWops* dst, int freedst) {
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

static SDL_Surface* SDL_LoadBMP_RW(SDL_RWops* src, int freesrc) {
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

static int SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect,
                                uint32_t format, void* pixels, int pitch) {
    if (gSDL.loaded) return gSDL.pSDL_RenderReadPixels(renderer, rect, format, pixels, pitch);
    std::memcpy(pixels, gSDL.softRen.fb.data(), gSDL.softRen.w * gSDL.softRen.h * 4);
    return 0;
}

static SDL_AudioSpec* SDL_LoadWAV_RW(SDL_RWops* src, int freesrc, SDL_AudioSpec* spec,
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

static void SDL_FreeWAV(uint8_t* audio_buf) {
    if (gSDL.loaded) { gSDL.pSDL_FreeWAV(audio_buf); return; }
    delete[] audio_buf;
}

static uint32_t SDL_OpenAudioDevice(const char* device, int iscapture,
                                    const SDL_AudioSpec* desired, SDL_AudioSpec* obtained,
                                    int allowed_changes) {
    if (gSDL.loaded) return gSDL.pSDL_OpenAudioDevice(device, iscapture, desired, obtained, allowed_changes);
    return 1;
}

static void SDL_CloseAudioDevice(uint32_t dev) {
    if (gSDL.loaded) gSDL.pSDL_CloseAudioDevice(dev);
}

[[maybe_unused]] static int SDL_QueueAudio(uint32_t dev, const void* data, uint32_t len) {
    if (gSDL.loaded) return gSDL.pSDL_QueueAudio(dev, data, len);
    return 0;
}

[[maybe_unused]] static void SDL_ClearQueuedAudio(uint32_t dev) {
    if (gSDL.loaded) gSDL.pSDL_ClearQueuedAudio(dev);
}

static void SDL_PauseAudioDevice(uint32_t dev, int pause_on) {
    if (gSDL.loaded) gSDL.pSDL_PauseAudioDevice(dev, pause_on);
}

static void SDL_LockAudioDevice(uint32_t dev) {
    if (gSDL.loaded && gSDL.pSDL_LockAudioDevice) gSDL.pSDL_LockAudioDevice(dev);
}

static void SDL_UnlockAudioDevice(uint32_t dev) {
    if (gSDL.loaded && gSDL.pSDL_UnlockAudioDevice) gSDL.pSDL_UnlockAudioDevice(dev);
}

static int SDL_PollEvent(SDL_Event* event) {
    if (gSDL.loaded) return gSDL.pSDL_PollEvent(event);
    return gSDL.x11.pollEvent(event) ? 1 : 0;
}

static uint32_t SDL_GetTicks(void) {
    if (gSDL.loaded) return gSDL.pSDL_GetTicks();
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - gSDL.startTime).count());
}

static void SDL_Delay(uint32_t ms) {
    if (gSDL.loaded) { gSDL.pSDL_Delay(ms); return; }
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ============================================================================
// CONSTANTS & MATH UTILITIES
// ============================================================================
static constexpr int WINDOW_W = 1280;
static constexpr int WINDOW_H = 720;
static constexpr float GROUND_Y = 630.0f;
static constexpr float SLING_X = 215.0f;
static constexpr float SLING_Y = 505.0f;
static constexpr float MAX_PULL = 115.0f;
static constexpr float LAUNCH_POWER = 10.9f;
static constexpr float GRAVITY = 880.0f;
static constexpr float PI = 3.14159265358979323846f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator-() const { return {-x, -y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }
    Vec2 normalized() const {
        float l = length();
        return (l > 1e-5f) ? Vec2(x / l, y / l) : Vec2(0.0f, 0.0f);
    }
};

static inline float cross(const Vec2& a, const Vec2& b) {
    return a.x * b.y - a.y * b.x;
}
static inline Vec2 cross(float w, const Vec2& r) {
    return Vec2(-w * r.y, w * r.x);
}
static inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

// ============================================================================
// GAME ENUMS & DATA STRUCTURES
// ============================================================================
enum class PigType {
    TROTTER  = 0, // Balanced clay-red origami pig -> Ability: Anvil Slam (vertical smash)
    TURBO    = 1, // Aerodynamic ochre boar        -> Ability: Supersonic Dash (2.6x speed)
    SPLIT    = 2, // Jade-green geometric piglet   -> Ability: Scatter Squadron (3-way split)
    BOMB     = 3, // Obsidian iron-plated hog      -> Ability: Seismic Detonation (shockwave)
    GLIDER   = 4, // Sky-blue origami aeronaut     -> Ability: Aeronaut Glide & Bomb Drop
    GRAVITON = 5, // Cosmic violet tesla boar      -> Ability: Graviton Vortex (implosion pull)
    DRILL    = 6  // Russet copper drill hog       -> Ability: Supersonic Bore & Foundation Burrowing
};

struct AerialBomb {
    Vec2 pos{};
    Vec2 vel{};
    float radius = 9.0f;
    float blastRadius = 95.0f;
    float damage = 120.0f;
    bool active = true;
    float lifeTimer = 0.0f;
};

enum class MaterialTier {
    TWIG       = 0,  // Tier 1: Wicker / Reed scaffolding (45 HP, light, shatters easily)
    TIMBER     = 1,  // Tier 2: Seasoned Oak / Walnut beams (125 HP, medium, cracks)
    STONE      = 2,  // Tier 3: Carved Slate / Basalt blocks (260 HP, heavy, armor-grade)
    ICE        = 3,  // Tier 4: Glacial Frost Ice (38 HP, ultra-slick friction, brittle crystalline shatter)
    TNT        = 4,  // Tier 5: Volatile Gunpowder Keg (35 HP, explosive chain-reaction detonation)
    TRAMPOLINE = 5,  // Tier 6: High-Tension Trampoline Deflector (high elastic ricochet rebound)
    MAGMA_VENT = 6   // Tier 7: Geothermal Magma Vent (indestructible/anchor basalt geyser)
};

enum class BirdClass {
    TOUCAN = 0,  // Sentry Toucan (78 HP, 5,000 pts) - Geometric prism beak
    OWL    = 1,  // Warden Owl (140 HP, 7,500 pts)   - Monolith armored owl
    HERON  = 2,  // Archduke Heron (210 HP, 10,000 pts) - Crowned royal totem bird
    FALCON = 3,  // Commander Falcon (150 HP + 120 Shield HP, 12,500 pts) - Directional Tower Shield
    CONDOR = 4   // Baron Von Condor (240 HP + 70 Balloon HP, 15,000 pts) - Suspended Imperial Zeppelin
};

struct PigBody {
    PigType type = PigType::TROTTER;
    Vec2 pos{};
    Vec2 vel{};
    float radius = 20.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float mass = 2.5f;
    float invMass = 0.4f;
    float restitution = 0.38f;
    bool active = false;
    bool launched = false;
    bool abilityUsed = false;
    bool exploded = false;
    float fuseTimer = -1.0f;
    float restTimer = 0.0f;
    float lifeTimer = 0.0f;
    std::vector<Vec2> trail;
    float vortexTimer = -1.0f;
    float drillTimer = -1.0f;
};

struct BlockBody {
    MaterialTier mat = MaterialTier::TWIG;
    Vec2 pos{};
    Vec2 vel{};
    float w = 40.0f;
    float h = 40.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float mass = 2.0f;
    float invMass = 0.5f;
    float inertia = 100.0f;
    float invInertia = 0.01f;
    float restitution = 0.22f;
    float friction = 0.55f;
    float hp = 45.0f;
    float maxHp = 45.0f;
    bool destroyed = false;
    bool hasGroundPath = false;
    bool stablySupported = false;
    float tntFuse = -1.0f;
    bool ignited = false;
};

struct BirdTarget {
    BirdClass kind = BirdClass::TOUCAN;
    Vec2 pos{};
    Vec2 vel{};
    float radius = 22.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float mass = 1.6f;
    float invMass = 0.625f;
    float hp = 60.0f;
    float maxHp = 60.0f;
    bool defeated = false;
    float blinkTimer = 0.0f;
    bool hasHelmet = false;
    float helmetHp = 0.0f;
    float maxHelmetHp = 0.0f;
    bool hasShield = false;
    float shieldHp = 0.0f;
    float maxShieldHp = 0.0f;
    bool inZeppelin = false;
    float balloonHp = 0.0f;
    float maxBalloonHp = 0.0f;
    bool balloonPopped = false;
};

struct PendulumBody {
    Vec2 anchor{};
    float length = 135.0f;
    float angle = 0.0f;      // current radians (0 = hanging straight down)
    float angVel = 0.0f;     // angular velocity rad/sec
    float radius = 24.0f;    // wrecking ball radius
    float mass = 4.5f;       // heavy granite / iron sphere
    bool spiked = true;

    Vec2 getBallPos() const {
        return anchor + Vec2(std::sin(angle) * length, std::cos(angle) * length);
    }
    Vec2 getBallVel() const {
        return Vec2(std::cos(angle) * length * angVel, -std::sin(angle) * length * angVel);
    }
};

enum class ParticleKind {
    GENERIC = 0,
    WOOD_SPLINTER,    // Elongated tumbling wicker / wood sliver
    TIMBER_CHUNK,     // Angular tumbling wood block chip
    STONE_SHARD,      // Heavy angular stone pebble
    FEATHER,          // Fluttering, oscillating feather with barb vanes
    SMOKE_PUFF,       // Expanding, soft alpha circular dust puff
    SPARK_GLINT,      // Star/cross glint
    RING,             // Shockwave ring
    ICE_CRYSTAL,      // Crystalline frost diamond shard
    HELMET_DEBRIS,    // Tumbling iron cauldron helmet piece
    EMBER,            // Fiery rising glowing volcanic ember
    GOLD_SPARKLE,     // Twinkling radiant golden truffle / achievement star particle
    SHIELD_DEBRIS,    // Tumbling reinforced tower shield fragment
    BALLOON_SCRAP,    // Floating shredded zeppelin canvas scrap
    COSMIC_MOTE       // Swirling violet/cyan gravitational vortex mote
};

struct Particle {
    Vec2 pos{};
    Vec2 vel{};
    float size = 5.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float life = 1.0f;
    float maxLife = 1.0f;
    SDL_Color color{255, 255, 255, 255};
    ParticleKind kind = ParticleKind::GENERIC;
    bool isRing = false;
    float ringRadius = 10.0f;
    float ringMaxRadius = 180.0f;
    float wobbleFreq = 9.0f;
    float wobblePhase = 0.0f;
};

struct FloatingScore {
    Vec2 pos{};
    std::string text;
    SDL_Color color{255, 240, 150, 255};
    float life = 1.2f;
    float maxLife = 1.2f;
};

struct LightningBolt {
    std::vector<Vec2> points;
    float life = 0.0f;
    float maxLife = 0.24f;
    float width = 3.5f;
    SDL_Color color{235, 248, 255, 255};
};

struct StormCloud {
    Vec2 pos{};
    float speed = 14.0f;
    float width = 160.0f;
    float height = 55.0f;
};

// Feature Pack D: Secret Golden Truffle Collectible
struct GoldenTruffle {
    Vec2 pos{};
    float radius = 14.0f;
    bool exists = false;
    bool active = false;
    bool collected = false;
    float pulseTime = 0.0f;
};

// Feature Pack D: Royal Swine Achievement System
enum class AchievementId {
    FIRST_BLOOD = 0,       // "First Strike"
    MARKSMAN,              // "Long-Distance Swine"
    CAULDRON_CRACKER,      // "Cauldron Cracker"
    SEISMIC_ANNIHILATION,  // "Seismic Annihilation"
    ANVIL_EXECUTIONER,     // "Anvil Executioner"
    POWDER_MAESTRO,        // "Powder Keg Maestro"
    ACROBATIC_HOG,         // "Acrobatic Swine"
    TRUFFLE_SNIFFER,       // "Truffle Sniffer"
    TRUFFLE_CONNOISSEUR,   // "Truffle Connoisseur"
    VAULT_CONQUEROR,       // "Vault Conqueror"
    VORTEX_MAESTRO,        // "Event Horizon" (Atticus Graviton Vortex)
    DEEP_BORE,             // "Deep Bore" (Rusty Drill Hog)
    COUNT
};

struct AchievementDef {
    AchievementId id;
    std::string key;
    std::string title;
    std::string desc;
    int targetCount = 1;
};

struct AchievementToast {
    std::string title;
    std::string desc;
    float timer = 0.0f;
    float slideY = -80.0f;
    bool active = false;
};

struct LevelSpec {
    std::string name;
    std::string subtitle;
    std::vector<PigType> pigQueue;
    std::vector<BlockBody> blocks;
    std::vector<BirdTarget> birds;
    std::vector<PendulumBody> pendulums;
    int star2Threshold = 20000;
    int star3Threshold = 30000;
    float wind = 0.0f;
    bool hasStorm = false;
    bool hasTruffle = false;
    Vec2 trufflePos{0.0f, 0.0f};
};

// ============================================================================
// BITMAP FONT & GEOMETRIC DRAWING PRIMITIVES
// ============================================================================
static const uint8_t FONT_5X7[96][7] = {
    {0,0,0,0,0,0,0}, // ' '
    {4,4,4,4,0,0,4}, // '!'
    {10,10,10,0,0,0,0}, // '"'
    {10,31,10,10,31,10,0}, // '#'
    {4,15,20,14,5,30,4}, // '$'
    {24,25,2,4,8,19,3}, // '%'
    {12,18,20,8,21,18,13}, // '&'
    {4,4,8,0,0,0,0}, // '\''
    {2,4,8,8,8,4,2}, // '('
    {8,4,2,2,2,4,8}, // ')'
    {0,4,21,14,21,4,0}, // '*'
    {0,4,4,31,4,4,0}, // '+'
    {0,0,0,0,4,4,8}, // ','
    {0,0,0,31,0,0,0}, // '-'
    {0,0,0,0,0,12,12}, // '.'
    {1,2,2,4,8,8,16}, // '/'
    {14,17,19,21,25,17,14}, // '0'
    {4,12,4,4,4,4,14}, // '1'
    {14,17,1,6,8,16,31}, // '2'
    {31,2,4,6,1,17,14}, // '3'
    {2,6,10,18,31,2,2}, // '4'
    {31,16,30,1,1,17,14}, // '5'
    {6,8,16,30,17,17,14}, // '6'
    {31,1,2,4,8,8,8}, // '7'
    {14,17,17,14,17,17,14}, // '8'
    {14,17,17,15,1,2,12}, // '9'
    {0,12,12,0,12,12,0}, // ':'
    {0,12,12,0,12,4,8}, // ';'
    {2,4,8,16,8,4,2}, // '<'
    {0,0,31,0,31,0,0}, // '='
    {8,4,2,1,2,4,8}, // '>'
    {14,17,1,2,4,0,4}, // '?'
    {14,17,23,21,23,16,14}, // '@'
    {14,17,17,31,17,17,17}, // 'A'
    {30,17,17,30,17,17,30}, // 'B'
    {14,17,16,16,16,17,14}, // 'C'
    {30,17,17,17,17,17,30}, // 'D'
    {31,16,16,30,16,16,31}, // 'E'
    {31,16,16,30,16,16,16}, // 'F'
    {14,17,16,23,17,17,15}, // 'G'
    {17,17,17,31,17,17,17}, // 'H'
    {14,4,4,4,4,4,14}, // 'I'
    {7,2,2,2,2,18,12}, // 'J'
    {17,18,20,24,20,18,17}, // 'K'
    {16,16,16,16,16,16,31}, // 'L'
    {17,27,21,21,17,17,17}, // 'M'
    {17,25,21,19,17,17,17}, // 'N'
    {14,17,17,17,17,17,14}, // 'O'
    {30,17,17,30,16,16,16}, // 'P'
    {14,17,17,17,21,18,13}, // 'Q'
    {30,17,17,30,20,18,17}, // 'R'
    {15,16,16,14,1,1,30}, // 'S'
    {31,4,4,4,4,4,4}, // 'T'
    {17,17,17,17,17,17,14}, // 'U'
    {17,17,17,17,17,10,4}, // 'V'
    {17,17,17,21,21,27,17}, // 'W'
    {17,17,10,4,10,17,17}, // 'X'
    {17,17,10,4,4,4,4}, // 'Y'
    {31,1,2,4,8,16,31}, // 'Z'
    {14,8,8,8,8,8,14}, // '['
    {16,8,8,4,2,2,1}, // '\\'
    {14,2,2,2,2,2,14}, // ']'
    {4,10,17,0,0,0,0}, // '^'
    {0,0,0,0,0,0,31}, // '_'
    {8,4,2,0,0,0,0}, // '`'
    {0,0,14,1,15,17,15}, // 'a'
    {16,16,30,17,17,17,30}, // 'b'
    {0,0,14,16,16,17,14}, // 'c'
    {1,1,15,17,17,17,15}, // 'd'
    {0,0,14,17,31,16,14}, // 'e'
    {6,8,8,28,8,8,8}, // 'f'
    {0,0,15,17,15,1,14}, // 'g'
    {16,16,30,17,17,17,17}, // 'h'
    {4,0,12,4,4,4,14}, // 'i'
    {2,0,6,2,2,18,12}, // 'j'
    {16,16,18,20,24,20,18}, // 'k'
    {12,4,4,4,4,4,14}, // 'l'
    {0,0,26,21,21,17,17}, // 'm'
    {0,0,30,17,17,17,17}, // 'n'
    {0,0,14,17,17,17,14}, // 'o'
    {0,0,30,17,30,16,16}, // 'p'
    {0,0,15,17,15,1,1}, // 'q'
    {0,0,22,25,16,16,16}, // 'r'
    {0,0,15,16,14,1,30}, // 's'
    {8,8,28,8,8,9,6}, // 't'
    {0,0,17,17,17,19,13}, // 'u'
    {0,0,17,17,17,10,4}, // 'v'
    {0,0,17,17,21,21,10}, // 'w'
    {0,0,17,10,4,10,17}, // 'x'
    {0,0,17,17,15,1,14}, // 'y'
    {0,0,31,2,4,8,31}, // 'z'
    {2,4,4,8,4,4,2}, // '{'
    {4,4,4,4,4,4,4}, // '|'
    {8,4,4,2,4,4,8}, // '}'
    {0,0,8,21,2,0,0}, // '~'
    {0,0,0,0,0,0,0}
};

static std::string formatNumber(int n) {
    std::string s = std::to_string(n);
    int insertPos = static_cast<int>(s.length()) - 3;
    while (insertPos > 0) {
        s.insert(insertPos, ",");
        insertPos -= 3;
    }
    return s;
}

static void drawText(SDL_Renderer* ren, const std::string& text, float x, float y,
                     float scale, SDL_Color col, bool center = false, bool shadow = true) {
    float charW = 6.0f * scale;
    float startX = center ? (x - (text.size() * charW) * 0.5f) : x;

    auto renderGlyphs = [&](float ox, float oy, SDL_Color c) {
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char ch = static_cast<unsigned char>(text[i]);
            if (ch < 32 || ch >= 128) ch = '?';
            const uint8_t* glyph = FONT_5X7[ch - 32];
            float gx = startX + i * charW + ox;
            for (int row = 0; row < 7; ++row) {
                uint8_t bits = glyph[row];
                for (int colBit = 0; colBit < 5; ++colBit) {
                    if (bits & (1 << (4 - colBit))) {
                        SDL_FRect r{gx + colBit * scale, y + row * scale + oy, scale, scale};
                        SDL_RenderFillRectF(ren, &r);
                    }
                }
            }
        }
    };

    if (shadow && col.a > 35) {
        SDL_Color shadowCol{8, 10, 16, static_cast<uint8_t>(col.a * 0.85f)};
        renderGlyphs(std::max(1.0f, scale * 0.6f), std::max(1.0f, scale * 0.6f), shadowCol);
    }
    renderGlyphs(0.0f, 0.0f, col);
}


static void drawTriangle(SDL_Renderer* ren, Vec2 a, Vec2 b, Vec2 c, SDL_Color col) {
    SDL_Vertex v[3] = {
        {{a.x, a.y}, col, {0, 0}},
        {{b.x, b.y}, col, {0, 0}},
        {{c.x, c.y}, col, {0, 0}}
    };
    SDL_RenderGeometry(ren, nullptr, v, 3, nullptr, 0);
}

static void drawConvexPoly(SDL_Renderer* ren, const std::vector<Vec2>& pts, SDL_Color col) {
    if (pts.size() < 3) return;
    std::vector<SDL_Vertex> verts(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        verts[i] = {{pts[i].x, pts[i].y}, col, {0.0f, 0.0f}};
    }
    std::vector<int> indices;
    indices.reserve((pts.size() - 2) * 3);
    for (size_t i = 1; i + 1 < pts.size(); ++i) {
        indices.push_back(0);
        indices.push_back(static_cast<int>(i));
        indices.push_back(static_cast<int>(i + 1));
    }
    SDL_RenderGeometry(ren, nullptr, verts.data(), static_cast<int>(verts.size()),
                       indices.data(), static_cast<int>(indices.size()));
}

static void drawThickLine(SDL_Renderer* ren, Vec2 a, Vec2 b, float thickness, SDL_Color col) {
    Vec2 dir = (b - a).normalized();
    Vec2 n(-dir.y * thickness * 0.5f, dir.x * thickness * 0.5f);
    std::vector<Vec2> quad = {a + n, b + n, b - n, a - n};
    drawConvexPoly(ren, quad, col);
}

static void drawNGon(SDL_Renderer* ren, Vec2 center, float radius, int sides,
                     float rotRad, SDL_Color col) {
    std::vector<Vec2> pts(sides);
    for (int i = 0; i < sides; ++i) {
        float a = rotRad + (2.0f * PI * i) / sides;
        pts[i] = {center.x + std::cos(a) * radius, center.y + std::sin(a) * radius};
    }
    drawConvexPoly(ren, pts, col);
}

static void drawStarIcon(SDL_Renderer* ren, Vec2 center, float outerR, float innerR,
                         SDL_Color fillCol, SDL_Color outlineCol) {
    for (int i = 0; i < 5; ++i) {
        float a0 = -PI * 0.5f + i * (2.0f * PI / 5.0f);
        float a1 = a0 + (PI / 5.0f);
        float a2 = a0 - (PI / 5.0f);
        Vec2 pOuter(center.x + std::cos(a0) * outerR, center.y + std::sin(a0) * outerR);
        Vec2 pRight(center.x + std::cos(a1) * innerR, center.y + std::sin(a1) * innerR);
        Vec2 pLeft(center.x + std::cos(a2) * innerR, center.y + std::sin(a2) * innerR);
        drawTriangle(ren, center, pLeft, pOuter, fillCol);
        drawTriangle(ren, center, pOuter, pRight, outlineCol);
    }
}

// ============================================================================
// HIGH-RESOLUTION MULTI-FACETED ORIGAMI CHARACTER MODELS (PIGS & TOTEM BIRDS)
// ============================================================================
static void renderOrigamiPigGeometry(SDL_Renderer* ren, PigType type, Vec2 pos,
                                     float r, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    auto rot = [&](float lx, float ly) -> Vec2 {
        return {pos.x + (lx * c - ly * s) * r, pos.y + (lx * s + ly * c) * r};
    };
    auto tri = [&](float x0, float y0, float x1, float y1, float x2, float y2, SDL_Color col) {
        drawTriangle(ren, rot(x0, y0), rot(x1, y1), rot(x2, y2), col);
    };
    auto quad = [&](float x0, float y0, float x1, float y1,
                    float x2, float y2, float x3, float y3, SDL_Color col) {
        drawConvexPoly(ren, {rot(x0, y0), rot(x1, y1), rot(x2, y2), rot(x3, y3)}, col);
    };
    auto crease = [&](float x0, float y0, float x1, float y1, SDL_Color col, float w = 1.4f) {
        drawThickLine(ren, rot(x0, y0), rot(x1, y1), w, col);
    };

    // Palette per Pig Class
    SDL_Color cCrown, cLight, cMid, cDark, cDeep, cRim, cSnoutL, cSnoutD, cEarIn;
    if (type == PigType::TROTTER) {
        // Trotter: Crimson Terracotta Vanguard Boar
        cCrown  = {244, 138, 112, 255};
        cLight  = {226, 114, 88, 255};
        cMid    = {198, 86, 62, 255};
        cDark   = {162, 62, 44, 255};
        cDeep   = {124, 42, 30, 255};
        cRim    = {32, 20, 18, 255};
        cSnoutL = {252, 176, 152, 255};
        cSnoutD = {218, 128, 106, 255};
        cEarIn  = {248, 162, 146, 255};
    } else if (type == PigType::TURBO) {
        // Bramble: Golden Amber Supersonic Aviator Boar
        cCrown  = {255, 214, 96, 255};
        cLight  = {242, 182, 58, 255};
        cMid    = {214, 146, 34, 255};
        cDark   = {176, 112, 20, 255};
        cDeep   = {136, 80, 12, 255};
        cRim    = {34, 24, 14, 255};
        cSnoutL = {255, 226, 138, 255};
        cSnoutD = {224, 174, 82, 255};
        cEarIn  = {245, 132, 68, 255};
    } else if (type == PigType::SPLIT) {
        // Clover: Jade-Emerald Shinobi / Ranger Piglet
        cCrown  = {122, 224, 186, 255};
        cLight  = {88, 192, 154, 255};
        cMid    = {62, 158, 124, 255};
        cDark   = {42, 122, 94, 255};
        cDeep   = {28, 88, 68, 255};
        cRim    = {18, 32, 28, 255};
        cSnoutL = {166, 238, 208, 255};
        cSnoutD = {114, 194, 162, 255};
        cEarIn  = {182, 245, 218, 255};
    } else if (type == PigType::GLIDER) {
        // Gale: Sky-Blue / Cerulean Origami Aeronaut Swine
        cCrown  = {140, 215, 255, 255};
        cLight  = {100, 190, 248, 255};
        cMid    = {64, 158, 224, 255};
        cDark   = {36, 120, 185, 255};
        cDeep   = {22, 80, 138, 255};
        cRim    = {18, 30, 48, 255};
        cSnoutL = {185, 235, 255, 255};
        cSnoutD = {130, 205, 245, 255};
        cEarIn  = {215, 245, 255, 255};
    } else if (type == PigType::GRAVITON) {
        // Atticus: Cosmic Violet / Deep Indigo Tesla Boar
        cCrown  = {175, 120, 245, 255};
        cLight  = {140, 85, 215, 255};
        cMid    = {105, 50, 180, 255};
        cDark   = {75, 25, 140, 255};
        cDeep   = {45, 12, 95, 255};
        cRim    = {22, 10, 42, 255};
        cSnoutL = {210, 170, 255, 255};
        cSnoutD = {160, 110, 230, 255};
        cEarIn  = {240, 190, 255, 255};
    } else if (type == PigType::DRILL) {
        // Rusty: Industrial Russet Copper / Bronze Drill Hog
        cCrown  = {235, 165, 80, 255};
        cLight  = {205, 130, 45, 255};
        cMid    = {165, 95, 25, 255};
        cDark   = {125, 65, 15, 255};
        cDeep   = {85, 40, 10, 255};
        cRim    = {35, 20, 10, 255};
        cSnoutL = {245, 205, 150, 255};
        cSnoutD = {200, 155, 95, 255};
        cEarIn  = {255, 180, 120, 255};
    } else {
        // Vulcan: Obsidian Dreadnought Demolition Hog
        cCrown  = {112, 120, 138, 255};
        cLight  = {86, 94, 110, 255};
        cMid    = {64, 70, 84, 255};
        cDark   = {44, 48, 58, 255};
        cDeep   = {28, 32, 40, 255};
        cRim    = {18, 18, 22, 255};
        cSnoutL = {148, 156, 172, 255};
        cSnoutD = {104, 112, 126, 255};
        cEarIn  = {238, 106, 48, 255};
    }

    // 1. Rear Curled Origami Tail & Trailing Accessories
    if (type == PigType::TURBO) {
        // Crimson Aviator Scarf fluttering behind Bramble
        tri(-0.85f, 0.15f, -1.48f, -0.05f, -1.25f, 0.32f, {214, 48, 42, 255});
        tri(-0.85f, 0.22f, -1.38f, 0.38f,  -1.10f, 0.55f, {168, 32, 28, 255});
    } else if (type == PigType::SPLIT) {
        // Ninja headband tails behind Clover
        tri(-0.78f, -0.35f, -1.42f, -0.58f, -1.22f, -0.25f, {224, 58, 52, 255});
        tri(-0.78f, -0.28f, -1.35f, -0.15f, -1.12f, 0.05f,  {178, 38, 34, 255});
    } else if (type == PigType::GLIDER) {
        // Origami Aero-Glider Tail Rudder
        tri(-0.85f, -0.10f, -1.55f, -0.42f, -1.25f, 0.15f, {185, 225, 255, 255});
        tri(-0.85f, 0.05f, -1.45f, 0.38f, -1.15f, 0.50f, {120, 180, 230, 255});
    } else if (type == PigType::GRAVITON) {
        // Dual Tesla Coils emitting crackling energy
        tri(-0.45f, -0.85f, -0.45f, -1.35f, -0.25f, -0.85f, {175, 120, 255, 255});
        drawNGon(ren, rot(-0.35f, -1.35f), r * 0.16f, 6, angle, {80, 240, 255, 255});
        tri(0.25f, -0.85f, 0.45f, -1.35f, 0.45f, -0.85f, {175, 120, 255, 255});
        drawNGon(ren, rot(0.45f, -1.35f), r * 0.16f, 6, angle, {80, 240, 255, 255});
    } else if (type == PigType::DRILL) {
        // Heavy cog gear & mechanical drive socket
        drawNGon(ren, rot(-0.85f, 0.05f), r * 0.24f, 6, angle * 3.0f, {160, 165, 175, 255});
        drawNGon(ren, rot(-0.85f, 0.05f), r * 0.12f, 6, angle * 3.0f, {45, 45, 55, 255});
    } else {
        // Geometric curled origami pig tail
        tri(-0.90f, 0.05f, -1.22f, -0.18f, -1.08f, 0.18f, cDark);
        tri(-1.22f, -0.18f, -1.34f, -0.02f, -1.08f, 0.18f, cMid);
    }

    // 2. Outer Dark Woodblock Silhouette Rim (12-sided sculpted hull)
    if (type == PigType::TURBO) {
        std::vector<Vec2> rimHull = {
            rot(1.24f, 0.02f), rot(0.72f, -0.68f), rot(-0.08f, -0.98f),
            rot(-0.84f, -0.78f), rot(-1.12f, 0.0f), rot(-0.84f, 0.78f),
            rot(-0.08f, 0.94f), rot(0.75f, 0.68f)
        };
        drawConvexPoly(ren, rimHull, cRim);
    } else {
        drawNGon(ren, pos, r * 1.05f, 12, angle + PI / 12.0f, cRim);
    }

    // 3. Multi-Faceted Origami Cranium & Cheek Planes (10 interlocking paper facets)
    // Top crown facet
    quad(-0.48f, -0.88f, 0.48f, -0.88f, 0.68f, -0.42f, -0.58f, -0.42f, cCrown);
    // Upper-left sunlit temple facet
    tri(-0.58f, -0.42f, -0.48f, -0.88f, -0.92f, -0.32f, cLight);
    // Upper-right forward brow facet
    tri(0.48f, -0.88f, 0.92f, -0.32f, 0.68f, -0.42f, cLight);
    // Central bridge facet
    quad(-0.58f, -0.42f, 0.68f, -0.42f, 0.78f, 0.18f, -0.62f, 0.18f, cMid);
    // Left rear cheek facet
    quad(-0.92f, -0.32f, -0.58f, -0.42f, -0.62f, 0.18f, -0.94f, 0.25f, cMid);
    // Right forward cheek facet
    quad(0.68f, -0.42f, 0.92f, -0.32f, 0.96f, 0.25f, 0.78f, 0.18f, cLight);
    // Lower jaw & chin shadow facets
    quad(-0.94f, 0.25f, -0.62f, 0.18f, -0.42f, 0.86f, -0.76f, 0.68f, cDark);
    quad(-0.62f, 0.18f, 0.78f, 0.18f, 0.52f, 0.88f, -0.42f, 0.86f, cDark);
    quad(0.78f, 0.18f, 0.96f, 0.25f, 0.76f, 0.70f, 0.52f, 0.88f, cDeep);

    // Subtle origami paper-fold crease lines across the cranium
    crease(-0.58f, -0.42f, 0.68f, -0.42f, cCrown, 1.2f);
    crease(-0.58f, -0.42f, -0.62f, 0.18f, cDark, 1.2f);
    crease(-0.62f, 0.18f, 0.78f, 0.18f, cDeep, 1.2f);

    // 4. Sculpted 3-Facet Origami Ears (Outer shell, Inner folded recess, Shadow tip)
    // Left ear
    tri(-0.68f, -0.72f, -0.28f, -1.28f, -0.10f, -0.78f, cRim);
    tri(-0.62f, -0.74f, -0.28f, -1.20f, -0.15f, -0.78f, cMid);
    tri(-0.48f, -0.78f, -0.28f, -1.08f, -0.22f, -0.78f, cEarIn);
    // Right ear
    tri(0.12f, -0.78f, 0.48f, -1.28f, 0.74f, -0.70f, cRim);
    tri(0.17f, -0.78f, 0.46f, -1.20f, 0.68f, -0.72f, cDark);
    tri(0.24f, -0.78f, 0.45f, -1.08f, 0.56f, -0.75f, cEarIn);

    // 5. Class-Specific Headgear & Signature Details
    if (type == PigType::TROTTER) {
        // Vanguard Boar Crimson-Gold Battle Crest Band across brow + origami ear-notch
        quad(-0.62f, -0.58f, 0.72f, -0.58f, 0.78f, -0.40f, -0.66f, -0.40f, {138, 34, 28, 255});
        tri(-0.05f, -0.64f, 0.15f, -0.64f, 0.05f, -0.36f, {245, 202, 82, 255});
        // Gleaming 2-facet curved origami boar tusks jutting from lower jaw
        tri(-0.08f, 0.42f, 0.06f, 0.82f, 0.18f, 0.44f, {252, 248, 235, 255});
        tri(0.06f, 0.82f, 0.18f, 0.44f, 0.24f, 0.72f, {210, 202, 185, 255});
        tri(0.55f, 0.42f, 0.74f, 0.80f, 0.82f, 0.40f, {252, 248, 235, 255});
    } else if (type == PigType::TURBO) {
        // Bramble's Twin Brass-and-Cyan Supersonic Aviator Goggles on Forehead
        quad(-0.74f, -0.64f, 0.80f, -0.64f, 0.84f, -0.48f, -0.78f, -0.48f, {72, 46, 24, 255});
        // Left goggle rim & lens
        drawNGon(ren, rot(-0.22f, -0.58f), r * 0.24f, 8, angle, {218, 172, 68, 255});
        drawNGon(ren, rot(-0.22f, -0.58f), r * 0.17f, 8, angle, {72, 215, 245, 255});
        tri(-0.30f, -0.66f, -0.14f, -0.66f, -0.26f, -0.52f, {220, 252, 255, 240});
        // Right goggle rim & lens
        drawNGon(ren, rot(0.32f, -0.58f), r * 0.24f, 8, angle, {218, 172, 68, 255});
        drawNGon(ren, rot(0.32f, -0.58f), r * 0.17f, 8, angle, {72, 215, 245, 255});
        tri(0.24f, -0.66f, 0.40f, -0.66f, 0.28f, -0.52f, {220, 252, 255, 240});
    } else if (type == PigType::SPLIT) {
        // Clover's Crimson Shinobi Headband with Silver Metal Forehead Plate & Clover Emblem
        quad(-0.68f, -0.62f, 0.74f, -0.62f, 0.78f, -0.42f, -0.72f, -0.42f, {204, 46, 42, 255});
        quad(-0.18f, -0.64f, 0.34f, -0.64f, 0.34f, -0.40f, -0.18f, -0.40f, {215, 226, 238, 255});
        drawNGon(ren, rot(0.08f, -0.52f), r * 0.08f, 4, angle, {42, 168, 118, 255});
        // Rosy origami cheek blushes
        drawNGon(ren, rot(-0.38f, 0.24f), r * 0.13f, 6, angle, {245, 148, 156, 200});
    } else if (type == PigType::GLIDER) {
        // Gale's Folded Origami Glider Wings (swept aerofoils) & Brass Aviator Goggles
        tri(-0.35f, -0.45f, -0.95f, -1.35f, 0.15f, -0.85f, {210, 240, 255, 255});
        tri(-0.35f, -0.45f, -1.15f, -1.15f, -0.95f, -1.35f, {150, 200, 245, 255});
        crease(-0.35f, -0.45f, -0.95f, -1.35f, {255, 255, 255, 220}, 1.5f);
        quad(-0.70f, -0.62f, 0.76f, -0.62f, 0.80f, -0.46f, -0.74f, -0.46f, {62, 50, 42, 255});
        drawNGon(ren, rot(-0.20f, -0.56f), r * 0.22f, 8, angle, {228, 185, 75, 255});
        drawNGon(ren, rot(-0.20f, -0.56f), r * 0.16f, 8, angle, {140, 235, 255, 255});
        tri(-0.28f, -0.63f, -0.12f, -0.63f, -0.24f, -0.50f, {255, 255, 255, 240});
        drawNGon(ren, rot(0.34f, -0.56f), r * 0.22f, 8, angle, {228, 185, 75, 255});
        drawNGon(ren, rot(0.34f, -0.56f), r * 0.16f, 8, angle, {140, 235, 255, 255});
        tri(0.26f, -0.63f, 0.42f, -0.63f, 0.30f, -0.50f, {255, 255, 255, 240});
    } else if (type == PigType::GRAVITON) {
        // Atticus's Twin Electric Tesla Coils & Cosmic Monocle / Diadem
        quad(-0.65f, -0.62f, 0.70f, -0.62f, 0.75f, -0.46f, -0.68f, -0.46f, {45, 30, 75, 255});
        // Left & right copper tesla coils
        crease(-0.35f, -0.62f, -0.40f, -1.25f, {220, 140, 60, 255}, 2.5f);
        drawNGon(ren, rot(-0.40f, -1.25f), r * 0.12f, 6, angle, {120, 230, 255, 255});
        drawNGon(ren, rot(-0.40f, -1.25f), r * 0.06f, 6, angle, {255, 255, 255, 255});
        crease(0.35f, -0.62f, 0.40f, -1.25f, {220, 140, 60, 255}, 2.5f);
        drawNGon(ren, rot(0.40f, -1.25f), r * 0.12f, 6, angle, {120, 230, 255, 255});
        drawNGon(ren, rot(0.40f, -1.25f), r * 0.06f, 6, angle, {255, 255, 255, 255});
        // Cosmic glowing forehead crest gem
        drawNGon(ren, rot(0.0f, -0.54f), r * 0.14f, 4, angle, {240, 100, 255, 255});
        drawNGon(ren, rot(0.0f, -0.54f), r * 0.07f, 4, angle, {255, 255, 255, 255});
    } else if (type == PigType::DRILL) {
        // Rusty's Heavy Industrial Miner Helmet with Brass Headlamp
        quad(-0.72f, -0.68f, 0.76f, -0.68f, 0.80f, -0.44f, -0.76f, -0.44f, {65, 45, 30, 255});
        // Miner hard-hat dome
        quad(-0.62f, -1.15f, 0.62f, -1.15f, 0.74f, -0.68f, -0.74f, -0.68f, {195, 120, 40, 255});
        tri(-0.62f, -1.15f, 0.0f, -1.35f, 0.62f, -1.15f, {225, 145, 55, 255});
        // Center brass carbide miner headlamp
        drawNGon(ren, rot(0.0f, -0.78f), r * 0.18f, 8, angle, {235, 185, 60, 255});
        drawNGon(ren, rot(0.0f, -0.78f), r * 0.12f, 8, angle, {255, 250, 180, 255});
    } else {
        // Vulcan's Riveted Steel Brow Plate & Multi-Layer Burning Bomb Fuse
        quad(-0.58f, -0.82f, 0.58f, -0.82f, 0.68f, -0.46f, -0.64f, -0.46f, {42, 46, 56, 255});
        drawNGon(ren, rot(-0.38f, -0.64f), r * 0.06f, 6, angle, {235, 162, 68, 255});
        drawNGon(ren, rot(0.0f,   -0.64f), r * 0.06f, 6, angle, {235, 162, 68, 255});
        drawNGon(ren, rot(0.38f,  -0.64f), r * 0.06f, 6, angle, {235, 162, 68, 255});
        // Fuse cord & 3-layer sparkler flame
        crease(0.0f, -0.85f, -0.08f, -1.22f, {168, 138, 96, 255}, 3.2f);
        tri(-0.34f, -1.16f, -0.08f, -1.62f, 0.20f, -1.16f, {255, 88, 28, 255});
        tri(-0.24f, -1.18f, -0.08f, -1.48f, 0.10f, -1.18f, {255, 182, 42, 255});
        tri(-0.14f, -1.20f, -0.08f, -1.36f, 0.02f, -1.20f, {255, 250, 195, 255});
        // Heavy iron war-tusks
        tri(-0.10f, 0.44f, 0.04f, 0.86f, 0.18f, 0.44f, {225, 230, 238, 255});
        tri(0.56f, 0.42f, 0.76f, 0.84f, 0.84f, 0.40f, {225, 230, 238, 255});
    }

    // 6. High-Resolution Expressive Eyes (Outline + Sclera + Iris + Pupil + Specular Catchlight + Brow)
    auto drawDetailedEye = [&](float ex, float ey, float er, float browTilt) {
        drawNGon(ren, rot(ex, ey), r * (er + 0.04f), 8, angle, cRim);
        drawNGon(ren, rot(ex, ey), r * er, 8, angle, {255, 252, 242, 255});
        SDL_Color irisCol = (type == PigType::BOMB)     ? SDL_Color{235, 92, 34, 255} :
                            (type == PigType::SPLIT)    ? SDL_Color{42, 138, 96, 255} :
                            (type == PigType::GLIDER)   ? SDL_Color{28, 140, 230, 255} :
                            (type == PigType::GRAVITON) ? SDL_Color{210, 80, 255, 255} :
                            (type == PigType::DRILL)    ? SDL_Color{240, 160, 40, 255} :
                                                          SDL_Color{92, 54, 32, 255};
        drawNGon(ren, rot(ex + 0.03f, ey), r * (er * 0.62f), 8, angle, irisCol);
        drawNGon(ren, rot(ex + 0.04f, ey), r * (er * 0.38f), 8, angle, {18, 18, 24, 255});
        // Bright white specular catchlight dot
        drawNGon(ren, rot(ex - 0.03f, ey - 0.05f), r * (er * 0.24f), 6, angle, {255, 255, 255, 255});
        // Origami furrowed eyebrow plate
        quad(ex - er * 1.15f, ey - er * 1.05f - browTilt,
             ex + er * 1.15f, ey - er * 1.05f + browTilt,
             ex + er * 1.10f, ey - er * 0.62f + browTilt,
             ex - er * 1.10f, ey - er * 0.62f - browTilt, cDeep);
    };
    drawDetailedEye(-0.24f, -0.18f, 0.20f, 0.05f);
    drawDetailedEye(0.46f,  -0.22f, 0.18f, -0.05f);

    // 7. Dimensional 3D Origami Snout (Outer bezel, top lit facet, lower shadow facet, 2 sculpted nostrils)
    std::vector<Vec2> snoutRim = {
        rot(0.02f, -0.22f), rot(0.84f, -0.22f), rot(0.98f, 0.12f),
        rot(0.84f, 0.48f),  rot(0.02f, 0.48f),  rot(-0.12f, 0.12f)
    };
    drawConvexPoly(ren, snoutRim, cRim);
    // Upper sunlit snout plane
    quad(0.06f, -0.17f, 0.80f, -0.17f, 0.91f, 0.12f, -0.05f, 0.12f, cSnoutL);
    // Lower shaded snout plane
    quad(-0.05f, 0.12f, 0.91f, 0.12f, 0.80f, 0.42f, 0.06f, 0.42f, cSnoutD);
    // Center snout ridge crease
    crease(-0.04f, 0.12f, 0.90f, 0.12f, {255, 240, 225, 140}, 1.3f);
    // Sculpted diamond nostrils with inner depth
    drawNGon(ren, rot(0.28f, 0.11f), r * 0.13f, 6, angle, cDeep);
    drawNGon(ren, rot(0.30f, 0.12f), r * 0.08f, 6, angle, {24, 16, 16, 255});
    drawNGon(ren, rot(0.64f, 0.11f), r * 0.13f, 6, angle, cDeep);
    drawNGon(ren, rot(0.66f, 0.12f), r * 0.08f, 6, angle, {24, 16, 16, 255});

    if (type == PigType::DRILL) {
        // Forward spiral titanium drill bore extending past snout
        tri(0.40f, -0.08f, 1.45f, 0.12f, 0.40f, 0.32f, {210, 215, 225, 255});
        tri(0.40f, 0.32f, 1.45f, 0.12f, 0.60f, 0.45f, {170, 175, 185, 255});
        crease(0.45f, 0.05f, 1.30f, 0.12f, {245, 195, 60, 255}, 2.4f);
        crease(0.45f, 0.22f, 1.30f, 0.12f, {245, 195, 60, 255}, 2.4f);
        drawNGon(ren, rot(1.45f, 0.12f), r * 0.14f, 4, angle, {255, 220, 80, 255});
    }
}

static void renderTotemBirdGeometry(SDL_Renderer* ren, BirdClass kind, Vec2 pos,
                                    float r, float angle, float hpRatio,
                                    bool hasHelmet = false, float helmetHpRatio = 1.0f,
                                    bool hasShield = false, float shieldHpRatio = 1.0f,
                                    bool inZeppelin = false, float balloonHpRatio = 1.0f) {
    float c = std::cos(angle), s = std::sin(angle);
    auto rot = [&](float lx, float ly) -> Vec2 {
        return {pos.x + (lx * c - ly * s) * r, pos.y + (lx * s + ly * c) * r};
    };
    auto tri = [&](float x0, float y0, float x1, float y1, float x2, float y2, SDL_Color col) {
        drawTriangle(ren, rot(x0, y0), rot(x1, y1), rot(x2, y2), col);
    };
    auto quad = [&](float x0, float y0, float x1, float y1,
                    float x2, float y2, float x3, float y3, SDL_Color col) {
        drawConvexPoly(ren, {rot(x0, y0), rot(x1, y1), rot(x2, y2), rot(x3, y3)}, col);
    };
    auto crease = [&](float x0, float y0, float x1, float y1, SDL_Color col, float w = 1.4f) {
        drawThickLine(ren, rot(x0, y0), rot(x1, y1), w, col);
    };

    SDL_Color cRim{22, 20, 26, 255};
    SDL_Color cTop, cMid, cDark, cWing1, cWing2, cBreast, cCrest, cBeak1, cBeak2;

    if (kind == BirdClass::TOUCAN) {
        // Sentry Toucan: Tropical Emerald-Teal & Rainbow Prism Beak
        cTop    = {58, 168, 178, 255};
        cMid    = {38, 132, 146, 255};
        cDark   = {24, 92, 106, 255};
        cWing1  = {28, 104, 122, 255};
        cWing2  = {18, 72, 88, 255};
        cBreast = {252, 206, 78, 255};
        cCrest  = {255, 196, 64, 255};
        cBeak1  = {255, 148, 48, 255};
        cBeak2  = {224, 68, 52, 255};
    } else if (kind == BirdClass::OWL) {
        // Warden Owl: Royal Indigo, Slate Armor & Silver Feather Chevrons
        cTop    = {112, 118, 168, 255};
        cMid    = {82, 88, 136, 255};
        cDark   = {56, 60, 102, 255};
        cWing1  = {64, 68, 114, 255};
        cWing2  = {44, 48, 84, 255};
        cBreast = {212, 218, 238, 255};
        cCrest  = {192, 202, 235, 255};
        cBeak1  = {238, 192, 78, 255};
        cBeak2  = {188, 142, 44, 255};
    } else if (kind == BirdClass::FALCON) {
        // Commander Falcon: Steel Peregrine Slate & Heavy Directional Tower Shield
        cTop    = {64, 84, 104, 255};
        cMid    = {44, 62, 80, 255};
        cDark   = {28, 42, 56, 255};
        cWing1  = {36, 52, 68, 255};
        cWing2  = {22, 34, 46, 255};
        cBreast = {230, 235, 245, 255};
        cCrest  = {210, 160, 45, 255};
        cBeak1  = {245, 190, 40, 255};
        cBeak2  = {195, 140, 25, 255};
    } else if (kind == BirdClass::CONDOR) {
        // Baron Von Condor: Jet-Black Mantle, Velvet Maroon Cowl & Steampunk Zeppelin
        cTop    = {45, 45, 52, 255};
        cMid    = {30, 30, 36, 255};
        cDark   = {18, 18, 22, 255};
        cWing1  = {115, 30, 36, 255};
        cWing2  = {80, 20, 24, 255};
        cBreast = {235, 230, 220, 255};
        cCrest  = {245, 240, 235, 255}; // Grand snow-white neck ruff
        cBeak1  = {235, 175, 50, 255};
        cBeak2  = {185, 125, 30, 255};
    } else {
        // Archduke Heron: Imperial Ivory, Royal Crimson Mantle & Golden Crown
        cTop    = {248, 244, 232, 255};
        cMid    = {224, 216, 198, 255};
        cDark   = {188, 178, 158, 255};
        cWing1  = {178, 38, 48, 255};  // Royal crimson cape/wings
        cWing2  = {132, 24, 32, 255};
        cBreast = {245, 204, 72, 255};
        cCrest  = {255, 212, 58, 255};
        cBeak1  = {238, 108, 64, 255};
        cBeak2  = {186, 64, 42, 255};
    }

    // 1. Splayed Origami Tail Feathers & Golden Talons at Base
    tri(-0.55f, 0.82f, -0.78f, 1.16f, -0.25f, 1.04f, cWing2);
    tri(0.25f,  1.04f, 0.78f,  1.16f, 0.55f,  0.82f, cWing2);
    // Left & right 2-toe origami talons gripping the perch
    tri(-0.48f, 0.94f, -0.58f, 1.12f, -0.34f, 1.06f, {235, 178, 54, 255});
    tri(-0.36f, 0.94f, -0.34f, 1.12f, -0.20f, 1.04f, {212, 152, 38, 255});
    tri(0.20f,  1.04f, 0.34f,  1.12f, 0.36f,  0.94f, {212, 152, 38, 255});
    tri(0.34f,  1.06f, 0.58f,  1.12f, 0.48f,  0.94f, {235, 178, 54, 255});

    // 2. Sculpted 10-Point Totem Silhouette Outline
    std::vector<Vec2> outline = {
        rot(-0.68f, -1.08f), rot(0.68f, -1.08f), rot(0.98f, -0.35f),
        rot(0.92f, 0.52f),   rot(0.68f, 1.04f),  rot(-0.68f, 1.04f),
        rot(-0.92f, 0.52f),  rot(-0.98f, -0.35f)
    };
    drawConvexPoly(ren, outline, cRim);

    // 3. Multi-Faceted Origami Torso & Head Planes
    quad(-0.60f, -0.98f, 0.60f, -0.98f, 0.78f, -0.32f, -0.78f, -0.32f, cTop);
    quad(-0.78f, -0.32f, 0.78f, -0.32f, 0.74f, 0.42f,  -0.74f, 0.42f,  cMid);
    quad(-0.74f, 0.42f,  0.74f, 0.42f,  0.58f, 0.95f,  -0.58f, 0.95f,  cDark);

    // 4. Layered 3-Tier Origami Folded Wings (Shoulder, Primary, Secondary Feathers)
    // Left Wing
    tri(-0.76f, -0.42f, -0.96f, 0.18f, -0.52f, 0.22f, cWing1);
    tri(-0.92f, 0.18f,  -0.86f, 0.82f, -0.44f, 0.62f, cWing2);
    tri(-0.76f, -0.10f, -0.88f, 0.52f, -0.48f, 0.42f, cWing1);
    // Right Wing
    tri(0.76f, -0.42f, 0.52f, 0.22f, 0.96f, 0.18f, cWing1);
    tri(0.92f, 0.18f,  0.44f, 0.62f, 0.86f, 0.82f, cWing2);
    tri(0.76f, -0.10f, 0.48f, 0.42f, 0.88f, 0.52f, cWing1);

    // 5. Geometric Chest Plastron & Feather Chevrons
    tri(-0.52f, 0.04f, 0.52f, 0.04f, 0.0f, 0.84f, cBreast);
    if (kind == BirdClass::OWL) {
        // Silver parliament owl chest chevrons
        tri(-0.34f, 0.24f, 0.34f, 0.24f, 0.0f, 0.54f, cDark);
        tri(-0.22f, 0.46f, 0.22f, 0.46f, 0.0f, 0.72f, cDark);
    } else if (kind == BirdClass::HERON) {
        // Archduke's Golden Royal Gorget & Crimson Ruby Medallion
        quad(-0.48f, 0.02f, 0.48f, 0.02f, 0.34f, 0.26f, -0.34f, 0.26f, {245, 192, 52, 255});
        drawNGon(ren, rot(0.0f, 0.18f), r * 0.15f, 6, angle, {214, 32, 48, 255});
        drawNGon(ren, rot(-0.02f, 0.16f), r * 0.06f, 4, angle, {255, 195, 205, 255});
    } else {
        // Toucan tropical crimson chest bib border
        tri(-0.36f, 0.08f, 0.36f, 0.08f, 0.0f, 0.46f, {228, 76, 54, 255});
    }

    // 6. Class-Specific Crown / Horns / Crest
    if (kind == BirdClass::HERON) {
        // 5-Spire Imperial Gold Crown with Jewel Facets
        quad(-0.54f, -1.04f, 0.54f, -1.04f, 0.60f, -0.84f, -0.60f, -0.84f, {218, 156, 28, 255});
        tri(-0.58f, -0.98f, -0.44f, -1.56f, -0.20f, -0.98f, cCrest);
        tri(-0.28f, -0.98f, -0.16f, -1.42f,  0.0f,  -0.98f, {235, 178, 38, 255});
        tri(-0.22f, -0.98f,  0.0f,  -1.74f,  0.22f, -0.98f, cCrest);
        tri(0.0f,   -0.98f,  0.16f, -1.42f,  0.28f, -0.98f, {235, 178, 38, 255});
        tri(0.20f,  -0.98f,  0.44f, -1.56f,  0.58f, -0.98f, cCrest);
        // Crown jewels
        drawNGon(ren, rot(0.0f, -1.12f), r * 0.09f, 4, angle, {224, 38, 52, 255});
        drawNGon(ren, rot(-0.34f, -1.08f), r * 0.07f, 4, angle, {52, 195, 235, 255});
        drawNGon(ren, rot(0.34f,  -1.08f), r * 0.07f, 4, angle, {52, 195, 235, 255});
    } else if (kind == BirdClass::OWL) {
        // Swept-Up Great Horned Owl 3-Facet Ear Tufts + Facial Disc
        tri(-0.68f, -0.88f, -1.02f, -1.52f, -0.24f, -0.96f, cRim);
        tri(-0.62f, -0.90f, -0.94f, -1.44f, -0.28f, -0.96f, cCrest);
        tri(0.24f,  -0.96f,  1.02f, -1.52f,  0.68f, -0.88f, cRim);
        tri(0.28f,  -0.96f,  0.94f, -1.44f,  0.62f, -0.90f, cCrest);
        // Owl concentric facial disc rings
        drawNGon(ren, rot(-0.30f, -0.45f), r * 0.30f, 8, angle, {162, 170, 212, 255});
        drawNGon(ren, rot(0.30f,  -0.45f), r * 0.30f, 8, angle, {162, 170, 212, 255});
    } else {
        // Toucan 3-Feather Tropical Sunburst Crest
        tri(-0.32f, -0.96f, -0.12f, -1.46f, 0.12f, -0.96f, {242, 118, 48, 255});
        tri(-0.08f, -0.96f,  0.28f, -1.54f, 0.42f, -0.96f, cCrest);
        tri(0.18f,  -0.96f,  0.56f, -1.36f, 0.58f, -0.92f, {92, 218, 168, 255});
    }

    // 7. Expressive Avian Eyes with Specular Highlights & Angry/Panicked Totem Brows
    float pupilScale = (hpRatio < 0.30f) ? 0.16f : ((hpRatio < 0.60f) ? 0.12f : 0.085f);
    float browSlopeAdj = (hpRatio < 0.38f) ? -0.11f : 0.0f;
    auto drawBirdEye = [&](float ex, float ey, float browSlope) {
        drawNGon(ren, rot(ex, ey), r * 0.23f, 8, angle, cRim);
        drawNGon(ren, rot(ex, ey), r * 0.19f, 8, angle,
                 (kind == BirdClass::OWL) ? SDL_Color{255, 224, 98, 255} : SDL_Color{255, 250, 236, 255});
        drawNGon(ren, rot(ex - 0.02f, ey), r * pupilScale, 8, angle, {18, 18, 24, 255});
        drawNGon(ren, rot(ex + 0.04f, ey - 0.05f), r * 0.055f, 6, angle, {255, 255, 255, 255});
        // Dynamic brow ridge: fierce V-shape when healthy, worried/shocked raise when damaged
        quad(ex - 0.24f, ey - 0.20f - browSlope,
             ex + 0.24f, ey - 0.20f + browSlope,
             ex + 0.22f, ey - 0.10f + browSlope,
             ex - 0.22f, ey - 0.10f - browSlope, cRim);
    };
    drawBirdEye(-0.30f, -0.46f, -0.06f - browSlopeAdj);
    drawBirdEye(0.30f,  -0.46f,  0.06f + browSlopeAdj);

    // Cross-shaped adhesive bandage on temple when heavily damaged
    if (hpRatio < 0.38f) {
        quad(0.36f, -0.74f, 0.58f, -0.66f, 0.52f, -0.52f, 0.30f, -0.60f, {242, 222, 172, 240});
        quad(0.48f, -0.74f, 0.40f, -0.52f, 0.48f, -0.48f, 0.56f, -0.70f, {242, 222, 172, 240});
    }

    // 8. Sculpted Multi-Faceted Beaks
    if (kind == BirdClass::TOUCAN) {
        // Giant 4-Facet Tropical Prism Beak projecting left toward the catapult
        tri(-0.38f, -0.44f, -1.52f, -0.14f, -0.38f, -0.08f, cBeak1);
        tri(-0.38f, -0.08f, -1.52f, -0.14f, -0.42f, 0.22f,  cBeak2);
        tri(-1.12f, -0.24f, -1.52f, -0.14f, -1.12f, -0.02f, {32, 28, 36, 255}); // Black toucan beak tip
        crease(-0.38f, -0.08f, -1.48f, -0.14f, {255, 238, 140, 220}, 1.6f);
    } else {
        // Symmetrical Diamond-Cut Raptor/Heron Beak with central ridge
        tri(-0.34f, -0.22f, 0.0f, -0.34f, 0.0f, 0.42f, cBeak1);
        tri(0.0f,   -0.34f, 0.34f, -0.22f, 0.0f, 0.42f, cBeak2);
        crease(0.0f, -0.32f, 0.0f, 0.40f, {255, 244, 195, 200}, 1.5f);
    }

    // 9. Cauldron / Iron Kettle Helmet (Feature Pack B)
    if (hasHelmet) {
        // Iron Cauldron Helmet Body
        quad(-0.62f, -1.35f, 0.62f, -1.35f, 0.72f, -0.72f, -0.72f, -0.72f, {78, 84, 98, 255});
        // Top dome
        tri(-0.62f, -1.35f, 0.0f, -1.65f, 0.62f, -1.35f, {92, 100, 116, 255});
        // Flared rim with brass rivet studs
        quad(-0.76f, -0.72f, 0.76f, -0.72f, 0.74f, -0.58f, -0.74f, -0.58f, {52, 56, 68, 255});
        for (int rv = -3; rv <= 3; ++rv) {
            drawNGon(ren, rot(rv * 0.20f, -0.65f), r * 0.045f, 4, angle, {228, 185, 75, 255});
        }
        // Top kettle handle loop
        drawNGon(ren, rot(0.0f, -1.72f), r * 0.12f, 6, angle, {65, 70, 82, 255});
        drawNGon(ren, rot(0.0f, -1.72f), r * 0.06f, 6, angle, {20, 24, 32, 255});
        // Ear flaps
        tri(-0.74f, -0.72f, -0.84f, -0.32f, -0.62f, -0.44f, {72, 78, 92, 255});
        tri(0.74f, -0.72f, 0.62f, -0.44f, 0.84f, -0.32f, {72, 78, 92, 255});
        // Armor crack if damaged
        if (helmetHpRatio < 0.55f) {
            crease(-0.35f, -1.35f, 0.15f, -0.82f, {25, 28, 36, 255}, 2.5f);
            crease(0.15f, -0.82f, 0.40f, -0.64f, {25, 28, 36, 255}, 2.0f);
        }
    }

    // 10. Commander Falcon Heavy Directional Tower Shield
    if (hasShield) {
        float sX = -0.92f;
        quad(sX - 0.42f, -0.95f, sX + 0.18f, -0.95f, sX + 0.14f, 0.95f, sX - 0.46f, 0.95f, {32, 36, 44, 255});
        quad(sX - 0.38f, -0.90f, sX + 0.14f, -0.90f, sX + 0.10f, 0.90f, sX - 0.42f, 0.90f, {78, 88, 106, 255});
        drawThickLine(ren, rot(sX - 0.14f, -0.75f), rot(sX - 0.14f, 0.75f), 3.2f, {230, 185, 65, 255});
        drawThickLine(ren, rot(sX - 0.36f, -0.10f), rot(sX + 0.08f, -0.10f), 3.2f, {230, 185, 65, 255});
        drawNGon(ren, rot(sX - 0.32f, -0.82f), r * 0.045f, 4, angle, {245, 205, 80, 255});
        drawNGon(ren, rot(sX + 0.08f, -0.82f), r * 0.045f, 4, angle, {245, 205, 80, 255});
        drawNGon(ren, rot(sX - 0.35f, 0.82f),  r * 0.045f, 4, angle, {245, 205, 80, 255});
        drawNGon(ren, rot(sX + 0.05f, 0.82f),  r * 0.045f, 4, angle, {245, 205, 80, 255});
        if (shieldHpRatio < 0.55f) {
            crease(sX - 0.32f, -0.35f, sX - 0.05f, 0.15f, {22, 24, 30, 255}, 2.4f);
            crease(sX - 0.05f, 0.15f, sX - 0.28f, 0.55f, {22, 24, 30, 255}, 2.0f);
        }
    }

    // 11. Baron Von Condor Suspended Steampunk Zeppelin
    if (inZeppelin) {
        float bY = -1.95f;
        drawConvexPoly(ren, {
            rot(-1.35f, bY), rot(-0.95f, bY - 0.65f), rot(0.95f, bY - 0.65f),
            rot(1.35f, bY),  rot(0.95f, bY + 0.65f),  rot(-0.95f, bY + 0.65f)
        }, {38, 28, 20, 255});
        quad(-0.90f, bY - 0.58f, 0.90f, bY - 0.58f, 0.90f, bY + 0.58f, -0.90f, bY + 0.58f, {185, 145, 95, 255});
        tri(-0.90f, bY - 0.58f, -1.28f, bY, -0.90f, bY + 0.58f, {155, 115, 75, 255});
        tri(0.90f, bY - 0.58f, 1.28f, bY, 0.90f, bY + 0.58f, {215, 175, 115, 255});
        crease(-0.45f, bY - 0.58f, -0.45f, bY + 0.58f, {125, 85, 45, 255}, 2.2f);
        crease(0.0f,   bY - 0.62f, 0.0f,   bY + 0.62f, {125, 85, 45, 255}, 2.5f);
        crease(0.45f,  bY - 0.58f, 0.45f,  bY + 0.58f, {125, 85, 45, 255}, 2.2f);
        drawNGon(ren, rot(1.28f, bY), r * 0.12f, 6, angle, {235, 185, 55, 255});
        tri(-1.28f, bY, -1.55f, bY - 0.45f, -1.15f, bY - 0.45f, {135, 90, 50, 255});
        tri(-1.28f, bY, -1.55f, bY + 0.45f, -1.15f, bY + 0.45f, {135, 90, 50, 255});
        crease(-0.75f, bY + 0.58f, -0.45f, -0.75f, {55, 50, 45, 255}, 1.4f);
        crease(-0.25f, bY + 0.58f, -0.15f, -0.75f, {55, 50, 45, 255}, 1.4f);
        crease(0.25f,  bY + 0.58f, 0.15f,  -0.75f, {55, 50, 45, 255}, 1.4f);
        crease(0.75f,  bY + 0.58f, 0.45f,  -0.75f, {55, 50, 45, 255}, 1.4f);
        if (balloonHpRatio < 0.55f) {
            crease(-0.35f, bY - 0.35f, 0.15f, bY + 0.25f, {45, 30, 20, 255}, 2.8f);
            crease(0.20f, bY - 0.20f, 0.55f, bY + 0.15f, {45, 30, 20, 255}, 2.4f);
        }
    }
}

// ============================================================================
// CUSTOM ASSET PIPELINE (Manifest + Custom BMP Sprites + Synthesized WAV Audio)
// ============================================================================
class AssetPipeline {
public:
    std::map<std::string, std::string> manifest;
    std::map<std::string, SDL_Texture*> textures;
    struct WavSound {
        SDL_AudioSpec spec{};
        uint8_t* buffer = nullptr;
        uint32_t length = 0;
    };
    std::map<std::string, WavSound> sounds;
    std::map<std::string, WavSound> musicTracks;
    std::string currentMusicKey;
    size_t bgmCursor = 0;
    bool musicEnabled = true;
    bool musicMuted = false;
    bool sfxMuted = false;

    // 16-channel polyphonic sound effect mixer with dynamic voice stealing
    struct SfxVoice {
        const int16_t* samples = nullptr;
        size_t length = 0;
        size_t cursor = 0;
        float gain = 1.0f;
        int priority = 0;
        bool active = false;
    };
    static constexpr size_t MAX_SFX_VOICES = 16;
    std::array<SfxVoice, MAX_SFX_VOICES> sfxVoices{};
    std::mutex audioMutex;

    float currentBgmGain = 0.32f;
    float targetBgmGain = 0.32f;
    uint32_t lastHitSoundTicks = 0;

    uint32_t audioDevice = 0;
    bool audioEnabled = true;
    bool useSpriteTextures = true;
    int loadedSpriteCount = 0;
    int loadedSoundCount = 0;

    std::vector<int16_t> mixScratch;

    static void ensureDir(const std::string& path) {
        mkdir(path.c_str(), 0755);
    }

    static void audioMixerCallback(void* userdata, uint8_t* stream, int len) {
        auto* pipe = static_cast<AssetPipeline*>(userdata);
        if (!pipe) {
            std::memset(stream, 0, len);
            return;
        }
        int16_t* out = reinterpret_cast<int16_t*>(stream);
        int numSamples = len / sizeof(int16_t);

        if (!pipe->audioEnabled) {
            std::memset(stream, 0, len);
            return;
        }

        std::lock_guard<std::mutex> lock(pipe->audioMutex);

        const int16_t* bgmBuf = nullptr;
        size_t bgmTotal = 0;
        if (!pipe->musicMuted && pipe->musicEnabled && !pipe->currentMusicKey.empty()) {
            auto it = pipe->musicTracks.find(pipe->currentMusicKey);
            if (it != pipe->musicTracks.end() && it->second.buffer && it->second.length >= 2) {
                bgmBuf = reinterpret_cast<const int16_t*>(it->second.buffer);
                bgmTotal = it->second.length / sizeof(int16_t);
            }
        }

        // Auto-duck BGM smoothly when high-priority sounds (explosions, abilities, victory) are active
        bool heavySoundActive = false;
        if (!pipe->sfxMuted) {
            for (const auto& v : pipe->sfxVoices) {
                if (v.active && v.priority >= 3) {
                    heavySoundActive = true;
                    break;
                }
            }
        }
        pipe->targetBgmGain = heavySoundActive ? 0.22f : 0.32f;

        for (int i = 0; i < numSamples; ++i) {
            // Smooth continuous exponential ducking (zero clicks or pops)
            pipe->currentBgmGain += (pipe->targetBgmGain - pipe->currentBgmGain) * 0.003f;

            float s = 0.0f;
            if (bgmBuf && bgmTotal > 0) {
                s += bgmBuf[pipe->bgmCursor] * pipe->currentBgmGain;
                pipe->bgmCursor++;
                if (pipe->bgmCursor >= bgmTotal) {
                    pipe->bgmCursor = 0;
                }
            }

            // Mix all 16 polyphonic voice channels simultaneously if SFX not muted
            if (!pipe->sfxMuted) {
                for (auto& v : pipe->sfxVoices) {
                    if (v.active && v.samples && v.cursor < v.length) {
                        s += v.samples[v.cursor] * v.gain;
                        v.cursor++;
                        if (v.cursor >= v.length) {
                            v.active = false;
                            v.samples = nullptr;
                        }
                    }
                }
            }

            // Soft saturation limiter to avoid harsh digital clipping when multiple blocks break
            float scaled = s / 32768.0f;
            float limited = std::tanh(scaled * 1.05f) * 0.95f;
            out[i] = static_cast<int16_t>(clampf(limited * 32767.0f, -32760.0f, 32760.0f));
        }
    }

    static int getSoundPriority(const std::string& key) {
        if (key == "sound.highscore" || key == "sound.achievement") return 5;
        if (key == "sound.victory" || key == "sound.truffle")       return 5;
        if (key == "sound.explosion" || key == "sound.thunder") return 4;
        if (key == "sound.slam" || key == "sound.dash" ||
            key == "sound.split" || key == "sound.launch" || key == "sound.boing" || key == "sound.magma" || key == "sound.glide") return 3;
        if (key == "sound.bird_defeat" || key == "sound.hit_metal" || key == "sound.wind") return 2;
        if (key == "sound.hit_stone" || key == "sound.hit_timber" ||
            key == "sound.hit_twig" || key == "sound.hit_ice") return 1;
        return 0;
    }

    void playMusic(const std::string& key) {
        if (currentMusicKey == key) return;
        if (audioDevice != 0) SDL_LockAudioDevice(audioDevice);
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            currentMusicKey = key;
            bgmCursor = 0;
        }
        if (audioDevice != 0) SDL_UnlockAudioDevice(audioDevice);
    }

    void playSound(const std::string& key, float gain = 1.0f) {
        if (!audioEnabled || sfxMuted || audioDevice == 0) return;
        auto it = sounds.find(key);
        if (it == sounds.end() || !it->second.buffer || it->second.length < 2) return;

        uint32_t now = SDL_GetTicks();
        int prio = getSoundPriority(key);

        // De-bounce rapid repeated debris hits to prevent audio clutter
        if (prio == 1 && (now - lastHitSoundTicks) < 45) {
            return;
        }
        if (prio == 1) lastHitSoundTicks = now;

        const int16_t* pcm = reinterpret_cast<const int16_t*>(it->second.buffer);
        size_t len = it->second.length / sizeof(int16_t);

        if (audioDevice != 0) SDL_LockAudioDevice(audioDevice);
        {
            std::lock_guard<std::mutex> lock(audioMutex);

            // Find an idle voice channel, or steal lowest priority / furthest progressed channel
            int bestIdx = -1;
            int lowestPrio = 999;
            size_t maxCursor = 0;

            for (size_t i = 0; i < MAX_SFX_VOICES; ++i) {
                if (!sfxVoices[i].active) {
                    bestIdx = static_cast<int>(i);
                    break;
                }
                if (sfxVoices[i].priority < lowestPrio ||
                    (sfxVoices[i].priority == lowestPrio && sfxVoices[i].cursor > maxCursor)) {
                    lowestPrio = sfxVoices[i].priority;
                    maxCursor = sfxVoices[i].cursor;
                    bestIdx = static_cast<int>(i);
                }
            }

            if (bestIdx >= 0) {
                if (!sfxVoices[bestIdx].active || prio >= sfxVoices[bestIdx].priority) {
                    sfxVoices[bestIdx].samples = pcm;
                    sfxVoices[bestIdx].length = len;
                    sfxVoices[bestIdx].cursor = 0;
                    sfxVoices[bestIdx].gain = gain * 0.90f;
                    sfxVoices[bestIdx].priority = prio;
                    sfxVoices[bestIdx].active = true;
                }
            }
        }
        if (audioDevice != 0) SDL_UnlockAudioDevice(audioDevice);
    }

    struct OnePoleLP {
        float a = 0.0f;
        float z = 0.0f;
        void setCutoff(float fc, float fs) {
            float x = 2.0f * PI * fc / fs;
            a = clampf(x / (x + 1.0f), 0.001f, 0.999f);
        }
        float process(float in) {
            z += a * (in - z);
            return z;
        }
    };

    struct OnePoleHP {
        float a = 0.0f, z = 0.0f;
        void setCutoff(float fc, float fs) {
            float x = 2.0f * PI * fc / fs;
            a = clampf(1.0f / (1.0f + x), 0.001f, 0.999f);
        }
        float process(float in) {
            float out = a * (in - z);
            z = in;
            return out;
        }
    };

    struct ResonantBP {
        float b0 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
        void setParams(float fc, float Q, float fs) {
            float w0 = 2.0f * PI * fc / fs;
            float alpha = std::sin(w0) / (2.0f * Q);
            float a0 = 1.0f + alpha;
            b0 = alpha / a0;
            b2 = -alpha / a0;
            a1 = (-2.0f * std::cos(w0)) / a0;
            a2 = (1.0f - alpha) / a0;
        }
        float process(float in) {
            float out = b0 * in + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    struct StudioReverb {
        struct Comb {
            std::vector<float> buf;
            size_t idx = 0;
            float feedback = 0.72f;
            float lp = 0.0f;
            float damp = 0.35f;
            void init(int len, float fb, float d) {
                buf.assign(len, 0.0f); idx = 0; feedback = fb; damp = d; lp = 0.0f;
            }
            float process(float in) {
                float out = buf[idx];
                lp = out * (1.0f - damp) + lp * damp;
                buf[idx] = in + lp * feedback;
                idx = (idx + 1) % buf.size();
                return out;
            }
        };
        struct AllPass {
            std::vector<float> buf;
            size_t idx = 0;
            float g = 0.5f;
            void init(int len, float gain) {
                buf.assign(len, 0.0f); idx = 0; g = gain;
            }
            float process(float in) {
                float bufOut = buf[idx];
                float out = -g * in + bufOut;
                buf[idx] = in + g * bufOut;
                idx = (idx + 1) % buf.size();
                return out;
            }
        };
        Comb combs[4];
        AllPass allpasses[2];
        void init(float sampleRate) {
            combs[0].init(static_cast<int>(sampleRate * 0.0297f), 0.74f, 0.32f);
            combs[1].init(static_cast<int>(sampleRate * 0.0371f), 0.72f, 0.34f);
            combs[2].init(static_cast<int>(sampleRate * 0.0413f), 0.70f, 0.36f);
            combs[3].init(static_cast<int>(sampleRate * 0.0437f), 0.68f, 0.38f);
            allpasses[0].init(static_cast<int>(sampleRate * 0.0051f), 0.5f);
            allpasses[1].init(static_cast<int>(sampleRate * 0.0017f), 0.5f);
        }
        float process(float in) {
            float combSum = combs[0].process(in) + combs[1].process(in) +
                            combs[2].process(in) + combs[3].process(in);
            float ap1 = allpasses[0].process(combSum * 0.25f);
            return allpasses[1].process(ap1);
        }
    };

    struct BiquadLP {
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
        void setParams(float fc, float Q, float fs) {
            float w0 = 2.0f * PI * fc / fs;
            float alpha = std::sin(w0) / (2.0f * Q);
            float a0 = 1.0f + alpha;
            b0 = ((1.0f - std::cos(w0)) / 2.0f) / a0;
            b1 = (1.0f - std::cos(w0)) / a0;
            b2 = b0;
            a1 = (-2.0f * std::cos(w0)) / a0;
            a2 = (1.0f - alpha) / a0;
        }
        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    static void renderUprightBass(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                 float duration, float vel) {
        size_t totalSamples = static_cast<size_t>(duration * sampleRate);
        BiquadLP lp;
        lp.setParams(std::min(240.0f, freq * 3.2f), 0.707f, sampleRate);

        float p1 = 0.0f;
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t dstIdx = startSample + i;
            if (dstIdx >= dest.size()) break;
            float t = static_cast<float>(i) / sampleRate;

            float attack = std::min(1.0f, t / 0.008f);
            float decayRate = 1.8f + freq * 0.005f;
            float env = attack * std::exp(-t * decayRate);

            p1 += 2.0f * PI * freq / sampleRate;
            float fundamental = std::sin(p1);
            float secondHarm  = 0.20f * std::sin(p1 * 2.0f) * std::exp(-t * 4.0f);
            float thirdHarm   = 0.04f * std::sin(p1 * 3.0f) * std::exp(-t * 8.0f);
            float thump = (t < 0.018f) ? (1.0f - t / 0.018f) * std::sin(2.0f * PI * 75.0f * t) * 0.22f : 0.0f;

            float raw = (fundamental + secondHarm + thirdHarm + thump) * env * vel;
            float warm = lp.process(raw);
            dest[dstIdx] += warm;
        }
    }

    static void renderAcousticGuitar(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                    float duration, float vel) {
        float cutoff = std::min(2600.0f, std::max(1200.0f, freq * 7.5f));
        BiquadLP lp;
        lp.setParams(cutoff, 0.707f, sampleRate);
        size_t totalSamples = static_cast<size_t>(duration * sampleRate);
        float p1 = 0.0f;
        float decayRate = 2.4f + freq * 0.003f;
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t dstIdx = startSample + i;
            if (dstIdx >= dest.size()) break;
            float t = static_cast<float>(i) / sampleRate;
            float attack = std::min(1.0f, t / 0.004f);
            float env = attack * std::exp(-t * decayRate);

            p1 += 2.0f * PI * freq / sampleRate;
            float h1 = std::sin(p1);
            float h2 = 0.38f * std::sin(p1 * 2.0004f) * std::exp(-t * 5.5f);
            float h3 = 0.16f * std::sin(p1 * 3.0009f) * std::exp(-t * 11.0f);
            float h4 = 0.06f * std::sin(p1 * 4.0016f) * std::exp(-t * 20.0f);
            float h5 = 0.02f * std::sin(p1 * 5.0025f) * std::exp(-t * 32.0f);

            float thump = (t < 0.006f) ? (1.0f - t / 0.006f) * std::sin(2.0f * PI * 280.0f * t) * 0.20f : 0.0f;
            float raw = (h1 + h2 + h3 + h4 + h5 + thump) * env * vel;
            dest[dstIdx] += lp.process(raw);
        }
    }

    static void renderPluck(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                            float duration, float vel, uint32_t& /*rng*/, float /*decay*/ = 0.994f, float /*damp*/ = 0.5f, float /*brightness*/ = 0.7f) {
        renderAcousticGuitar(dest, startSample, freq, sampleRate, duration, vel);
    }

    static void renderMarimba(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                              float duration, float vel, uint32_t& rng) {
        size_t totalSamples = static_cast<size_t>(duration * sampleRate);
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t dstIdx = startSample + i;
            if (dstIdx >= dest.size()) break;
            float t = static_cast<float>(i) / sampleRate;
            rng = rng * 1664525u + 1013904223u;
            float white = (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
            float mallet = (t < 0.0035f) ? (1.0f - t / 0.0035f) * white * 0.18f : 0.0f;
            float m1 = std::sin(2.0f * PI * freq * t) * std::exp(-t * (2.8f + freq * 0.0025f));
            float m2 = std::sin(2.0f * PI * (freq * 3.98f) * t) * std::exp(-t * 26.0f) * 0.28f;
            float m3 = std::sin(2.0f * PI * (freq * 9.25f) * t) * std::exp(-t * 60.0f) * 0.10f;
            dest[dstIdx] += (m1 + m2 + m3 + mallet) * vel;
        }
    }

    static void renderRhodesChord(std::vector<float>& dest, size_t startSample, const std::vector<float>& freqs,
                                  float sampleRate, float duration, float vel) {
        size_t totalSamples = static_cast<size_t>(duration * sampleRate);
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t dstIdx = startSample + i;
            if (dstIdx >= dest.size()) break;
            float t = static_cast<float>(i) / sampleRate;
            float chordVal = 0.0f;
            float lfo = 1.0f + 0.08f * std::sin(2.0f * PI * 0.55f * t);
            float env = std::min(1.0f, t / 0.035f) * std::exp(-t * 1.15f);
            for (float f : freqs) {
                float p1 = 2.0f * PI * f * t;
                float p2 = 2.0f * PI * (f * 2.002f) * t;
                chordVal += (std::sin(p1) + 0.20f * std::sin(p2) * std::exp(-t * 3.0f));
            }
            dest[dstIdx] += chordVal * env * lfo * vel;
        }
    }

    static void renderCajon(std::vector<float>& dest, size_t startSample, float sampleRate, float vel, bool isBass) {
        size_t totalSamples = static_cast<size_t>(0.24f * sampleRate);
        float p = 0.0f;
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t dstIdx = startSample + i;
            if (dstIdx >= dest.size()) break;
            float t = static_cast<float>(i) / sampleRate;
            if (isBass) {
                float f = 82.0f * std::exp(-t * 22.0f) + 44.0f;
                p += 2.0f * PI * f / sampleRate;
                float env = std::exp(-t * 13.0f);
                float tap = (t < 0.005f) ? (1.0f - t / 0.005f) * 0.30f : 0.0f;
                dest[dstIdx] += (std::tanh(1.4f * std::sin(p)) + tap) * env * vel;
            } else {
                float f = 280.0f * std::exp(-t * 35.0f) + 140.0f;
                p += 2.0f * PI * f / sampleRate;
                float env = std::exp(-t * 28.0f);
                dest[dstIdx] += std::sin(p) * env * vel;
            }
        }
    }

    static void renderShaker(std::vector<float>& dest, size_t startSample, float sampleRate, float vel, uint32_t& rng, ResonantBP& bpShaker) {
        size_t totalSamples = static_cast<size_t>(0.045f * sampleRate);
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t dstIdx = startSample + i;
            if (dstIdx >= dest.size()) break;
            float t = static_cast<float>(i) / sampleRate;
            rng = rng * 1664525u + 1013904223u;
            float white = (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
            float s = bpShaker.process(white);
            float env = std::exp(-t * 60.0f) * std::min(1.0f, t / 0.003f);
            dest[dstIdx] += s * env * vel;
        }
    }

    static void writeSynthesizedWav(const std::string& filename, int type, bool forceOverwrite) {
        if (!forceOverwrite) {
            std::ifstream check(filename, std::ios::binary);
            if (check.good()) return;
        }

        const int sampleRate = 44100;
        float duration = 0.35f;
        if (type == 0) duration = 0.38f; // Catapult Launch
        if (type == 1) duration = 0.40f; // Turbo Boar Dash
        if (type == 2) duration = 0.36f; // Jade Tri-Piglet Split
        if (type == 3) duration = 0.52f; // Trotter Anvil Slam
        if (type == 4) duration = 0.78f; // Vulcan Seismic Detonation
        if (type == 5) duration = 0.32f; // Wicker / Reed Shatter
        if (type == 6) duration = 0.38f; // Seasoned Timber Impact
        if (type == 7) duration = 0.50f; // Bird Defeat
        if (type == 8) duration = 1.25f; // Victory Fanfare
        if (type == 9) duration = 0.42f; // Stone / Slate Impact
        if (type == 10) duration = 0.06f; // UI Click
        if (type == 12) duration = 0.35f; // Glacial Ice / Crystal Shatter
        if (type == 13) duration = 0.40f; // Iron Cauldron / Helmet Clang
        if (type == 14) duration = 0.44f; // Trampoline Spring Boing
        if (type == 15) duration = 1.25f; // Gusting Crosswind whistling sweep
        if (type == 16) duration = 1.45f; // Atmospheric Thunder crack & low rumble
        if (type == 17) duration = 0.85f; // Geothermal Magma Vent eruption & sizzle
        if (type == 18) duration = 0.75f; // Golden Truffle Discovery Chime
        if (type == 19) duration = 1.40f; // Royal Swine Achievement Fanfare
        if (type == 20) duration = 45.714285f; // Title BGM (16 bars @ 84 BPM in D major)
        if (type == 21) duration = 69.818182f; // Battle BGM (32 bars @ 110 BPM modern acoustic groove)
        if (type == 22) duration = 0.65f; // Aeronaut Glider Flight (sound.glide)
        if (type == 23) duration = 0.85f; // Graviton Vortex hum & cosmic implosion sweep
        if (type == 24) duration = 0.55f; // Drill Hog supersonic rotary bore
        if (type == 25) duration = 0.45f; // Falcon Directional Tower Shield clang & ricochet
        if (type == 26) duration = 0.70f; // Zeppelin balloon puncture hiss & crash creak

        const int numSamples = static_cast<int>(sampleRate * duration);
        std::vector<int16_t> pcm(numSamples, 0);

        uint32_t rng = 0x9E3779B9u ^ (static_cast<uint32_t>(type) * 1013904223u);
        auto randFloat = [&]() -> float {
            rng = rng * 1664525u + 1013904223u;
            return (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
        };

        if (type == 20) {
            // --- TITLE BGM: Contemporary Acoustic Indie Theme in D Major (84 BPM, 16 Bars) ---
            const float beatLen = 60.0f / 84.0f;
            std::vector<float> mixMidsHighs(numSamples, 0.0f);
            std::vector<float> mixBass(numSamples, 0.0f);

            static const float chordRoots[16] = {
                146.83f, 123.47f,  98.00f, 110.00f, // D  -> Bm -> G  -> A
                185.00f, 123.47f,  82.41f, 110.00f, // F#m-> Bm -> Em -> A
                 98.00f, 110.00f, 185.00f, 123.47f, // G  -> A  -> F#m-> Bm
                 82.41f, 110.00f, 146.83f, 146.83f  // Em -> A  -> D  -> D
            };
            static const float chordThirds[16] = {
                185.00f, 146.83f, 123.47f, 146.83f,
                220.00f, 146.83f,  98.00f, 138.59f,
                123.47f, 138.59f, 220.00f, 146.83f,
                 98.00f, 138.59f, 185.00f, 185.00f
            };
            static const float chordFifths[16] = {
                220.00f, 185.00f, 146.83f, 164.81f,
                277.18f, 185.00f, 123.47f, 164.81f,
                146.83f, 164.81f, 277.18f, 185.00f,
                123.47f, 164.81f, 220.00f, 220.00f
            };
            static const float chordSevenths[16] = {
                277.18f, 220.00f, 185.00f, 196.00f,
                329.63f, 220.00f, 146.83f, 196.00f,
                185.00f, 196.00f, 329.63f, 220.00f,
                146.83f, 196.00f, 277.18f, 277.18f
            };

            static const float melodyNotes[16] = {
                739.99f, 880.00f, 739.99f, 659.25f, // F#5, A5, F#5, E5
                587.33f, 739.99f, 659.25f, 0.0f,    // D5,  F#5, E5,  rest
                783.99f, 880.00f, 739.99f, 587.33f, // G5,  A5,  F#5, D5
                659.25f, 739.99f, 587.33f, 0.0f     // E5,  F#5, D5,  cadence
            };

            for (int bar = 0; bar < 16; ++bar) {
                float barStart = bar * 4.0f * beatLen;
                size_t sBar = static_cast<size_t>(barStart * sampleRate);

                // 1. Warm Rhodes / Electric Piano Pad
                std::vector<float> chord{chordRoots[bar], chordThirds[bar], chordFifths[bar], chordSevenths[bar]};
                renderRhodesChord(mixMidsHighs, sBar, chord, sampleRate, 4.0f * beatLen, 0.28f);

                // 2. Warm Upright Bass on root and 5th (warm, deep, balanced)
                renderUprightBass(mixBass, sBar, chordRoots[bar] * 0.5f, sampleRate, beatLen * 2.8f, 0.30f);
                size_t sBeat2 = static_cast<size_t>((barStart + 2.0f * beatLen) * sampleRate);
                renderUprightBass(mixBass, sBeat2, chordFifths[bar] * 0.5f, sampleRate, beatLen * 1.8f, 0.24f);

                // 3. Contemporary Warm Fingerpicked Acoustic Guitar Arpeggio (8th notes)
                for (int sub = 0; sub < 8; ++sub) {
                    float tSub = barStart + sub * (beatLen * 0.5f);
                    size_t sSub = static_cast<size_t>(tSub * sampleRate);
                    float fArp = (sub == 0 || sub == 4) ? chordRoots[bar] :
                                 ((sub == 1 || sub == 5) ? chordThirds[bar] :
                                 ((sub == 2 || sub == 6) ? chordFifths[bar] : chordSevenths[bar]));
                    if (sub >= 4) fArp *= 2.0f;
                    renderAcousticGuitar(mixMidsHighs, sSub, fArp, sampleRate, beatLen * 1.2f, 0.28f);
                }

                // 4. Glockenspiel / Music Box Melody
                if (melodyNotes[bar] > 20.0f) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), melodyNotes[bar], sampleRate, beatLen * 2.5f, 0.42f, rng);
                }
            }

            // Apply Studio Room Reverb to mids/highs (bass stays dry, centered, and tight)
            StudioReverb rev; rev.init(sampleRate);
            std::vector<float> master(numSamples, 0.0f);
            for (int i = 0; i < numSamples; ++i) {
                float dry = mixMidsHighs[i];
                float wet = rev.process(dry);
                master[i] = dry + wet * 0.16f + mixBass[i];
            }

            // Seamless clickless crossfade on last 2048 samples
            const int crossfade = 2048;
            for (int i = 0; i < crossfade; ++i) {
                float alpha = static_cast<float>(i) / crossfade;
                int endIdx = numSamples - crossfade + i;
                master[endIdx] = master[endIdx] * (1.0f - alpha) + master[i] * alpha;
            }

            for (int i = 0; i < numSamples; ++i) {
                float s = std::tanh(master[i] * 1.05f);
                pcm[i] = static_cast<int16_t>(clampf(s * 25000.0f, -32000.0f, 32000.0f));
            }
        } else if (type == 21) {
            // --- BATTLE BGM: Modern Bouncy Acoustic Groove (110 BPM, 32 Bars, ~70s) ---
            const float beatLen = 60.0f / 110.0f;
            std::vector<float> mixMidsHighs(numSamples, 0.0f);
            std::vector<float> mixBass(numSamples, 0.0f);
            ResonantBP bpShaker; bpShaker.setParams(4800.0f, 2.2f, sampleRate);

            static const float chordRoots[32] = {
                130.81f, 110.00f, 146.83f,  98.00f, 164.81f, 110.00f, 146.83f,  98.00f, // C -> Am -> Dm -> G -> Em -> Am -> Dm -> G
                174.61f,  98.00f, 164.81f, 110.00f, 146.83f,  98.00f, 130.81f, 130.81f, // F -> G  -> Em -> Am -> Dm -> G  -> C  -> C7
                110.00f, 164.81f, 110.00f, 146.83f, 174.61f, 164.81f, 146.83f,  98.00f, // Am-> E7 -> Am -> D7 -> F  -> Em -> Dm -> G
                130.81f, 164.81f, 174.61f,  98.00f, 110.00f, 146.83f,  98.00f, 130.81f  // C -> Em -> F  -> G  -> Am -> Dm -> G  -> C
            };
            static const float chordThirds[32] = {
                164.81f, 130.81f, 174.61f, 123.47f, 196.00f, 130.81f, 174.61f, 123.47f,
                220.00f, 123.47f, 196.00f, 130.81f, 174.61f, 123.47f, 164.81f, 164.81f,
                130.81f, 207.65f, 130.81f, 185.00f, 220.00f, 196.00f, 174.61f, 123.47f,
                164.81f, 196.00f, 220.00f, 123.47f, 130.81f, 174.61f, 123.47f, 164.81f
            };
            static const float chordFifths[32] = {
                196.00f, 164.81f, 220.00f, 146.83f, 246.94f, 164.81f, 220.00f, 146.83f,
                261.63f, 146.83f, 246.94f, 164.81f, 220.00f, 146.83f, 196.00f, 196.00f,
                164.81f, 246.94f, 164.81f, 220.00f, 261.63f, 246.94f, 220.00f, 146.83f,
                196.00f, 246.94f, 261.63f, 146.83f, 164.81f, 220.00f, 146.83f, 196.00f
            };

            for (int bar = 0; bar < 32; ++bar) {
                float barStart = bar * 4.0f * beatLen;
                size_t sBar = static_cast<size_t>(barStart * sampleRate);

                // 1. Warm Rhodes / Electric Piano Pad
                std::vector<float> chord{chordRoots[bar], chordThirds[bar], chordFifths[bar]};
                renderRhodesChord(mixMidsHighs, sBar, chord, sampleRate, 4.0f * beatLen, (bar >= 16 && bar <= 23 ? 0.30f : 0.20f));

                for (int beat = 0; beat < 4; ++beat) {
                    float tBeat = barStart + beat * beatLen;
                    size_t sBeat = static_cast<size_t>(tBeat * sampleRate);

                    // 2. Wooden Cajon
                    if (bar >= 16 && bar <= 23) {
                        renderCajon(mixMidsHighs, sBeat, sampleRate, 0.24f, false);
                    } else {
                        if (beat == 0 || beat == 2) {
                            renderCajon(mixBass, sBeat, sampleRate, (beat == 0 ? 0.48f : 0.34f), true);
                        } else {
                            renderCajon(mixMidsHighs, sBeat, sampleRate, 0.30f, false);
                        }
                    }

                    // 3. Studio Shaker with 16th-note swing
                    for (int sub16 = 0; sub16 < 4; ++sub16) {
                        float swingOffset = (sub16 % 2 == 1) ? 0.025f : 0.0f;
                        float tShaker = tBeat + (sub16 * 0.25f + swingOffset) * beatLen;
                        size_t sShaker = static_cast<size_t>(tShaker * sampleRate);
                        float sVel = (sub16 == 0 ? 0.20f : (sub16 == 2 ? 0.32f : 0.12f));
                        renderShaker(mixMidsHighs, sShaker, sampleRate, sVel, rng, bpShaker);
                    }

                    // 4. Warm Upright Bass Walking Pattern (Bouncy, spacious, zero harsh buzz)
                    if (beat == 0) {
                        renderUprightBass(mixBass, sBeat, chordRoots[bar] * 0.5f, sampleRate, beatLen * 1.4f, 0.32f);
                    } else if (beat == 2) {
                        renderUprightBass(mixBass, sBeat, chordFifths[bar] * 0.5f, sampleRate, beatLen * 1.2f, 0.26f);
                    } else if (beat == 3) {
                        size_t sPickup = static_cast<size_t>((tBeat + beatLen * 0.5f) * sampleRate);
                        renderUprightBass(mixBass, sPickup, chordRoots[bar] * 0.5f * 1.05946f, sampleRate, beatLen * 0.7f, 0.20f);
                    }

                    // 5. Warm Acoustic Guitar Strum on Off-Beats (Rich, organic, zero harsh comb buzz)
                    size_t sOff = static_cast<size_t>((tBeat + beatLen * 0.5f) * sampleRate);
                    renderAcousticGuitar(mixMidsHighs, sOff, chordThirds[bar], sampleRate, beatLen * 0.85f, 0.22f);
                    renderAcousticGuitar(mixMidsHighs, sOff + 160, chordFifths[bar], sampleRate, beatLen * 0.85f, 0.18f);
                    renderAcousticGuitar(mixMidsHighs, sOff + 320, chordRoots[bar] * 2.0f, sampleRate, beatLen * 0.85f, 0.16f);
                }

                // 6. Rosewood Marimba Melody (Section 2 & 4 with breathing pauses!)
                if (bar == 8) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.2f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.5f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.2f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.5f * beatLen * sampleRate), 880.00f, sampleRate, beatLen * 1.5f, 0.48f, rng);
                } else if (bar == 9) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 587.33f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.0f * beatLen * sampleRate), 523.25f, sampleRate, beatLen * 1.5f, 0.45f, rng);
                } else if (bar == 12) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), 587.33f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.5f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.5f * beatLen * sampleRate), 698.46f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.5f * beatLen * sampleRate), 880.00f, sampleRate, beatLen * 1.5f, 0.48f, rng);
                } else if (bar == 13) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 698.46f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.0f * beatLen * sampleRate), 587.33f, sampleRate, beatLen * 1.5f, 0.45f, rng);
                } else if (bar == 24) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 523.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.45f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.0f * beatLen * sampleRate), 1046.50f, sampleRate, beatLen * 1.5f, 0.48f, rng);
                } else if (bar == 25) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 987.77f, sampleRate, beatLen * 1.1f, 0.44f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.5f, 0.45f, rng);
                } else if (bar == 28) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), 880.00f, sampleRate, beatLen * 1.1f, 0.44f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.5f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.5f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.5f, 0.45f, rng);
                } else if (bar == 31) {
                    renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 523.25f, sampleRate, beatLen * 2.5f, 0.42f, rng);
                }
            }

            // Apply Studio Room Reverb to mids/highs (bass stays clean, punchy, and dry)
            StudioReverb rev; rev.init(sampleRate);
            std::vector<float> master(numSamples, 0.0f);
            for (int i = 0; i < numSamples; ++i) {
                float dry = mixMidsHighs[i];
                float wet = rev.process(dry);
                master[i] = dry + wet * 0.15f + mixBass[i];
            }

            // Seamless clickless crossfade on last 2048 samples
            const int crossfade = 2048;
            for (int i = 0; i < crossfade; ++i) {
                float alpha = static_cast<float>(i) / crossfade;
                int endIdx = numSamples - crossfade + i;
                master[endIdx] = master[endIdx] * (1.0f - alpha) + master[i] * alpha;
            }

            for (int i = 0; i < numSamples; ++i) {
                float s = std::tanh(master[i] * 1.05f);
                pcm[i] = static_cast<int16_t>(clampf(s * 25000.0f, -32000.0f, 32000.0f));
            }
        } else {
            // --- MODERN SOUND EFFECTS (types 0 to 19, 22) ---
            float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
            float lpNoise = 0.0f, lpNoise2 = 0.0f;

            for (int i = 0; i < numSamples; ++i) {
                float t = static_cast<float>(i) / sampleRate;
                float attack = std::min(1.0f, t / 0.003f);
                float sample = 0.0f;
                float wn = randFloat();
                lpNoise = 0.88f * lpNoise + 0.12f * wn;
                lpNoise2 = 0.65f * lpNoise2 + 0.35f * wn;

                if (type == 0) {
                    // Catapult Launch: Organic wood tension stress creak + leather release click + aerodynamic whoosh
                    float fCreak = 120.0f + 260.0f * std::sin(t * 120.0f);
                    float fWhoosh = 420.0f * std::exp(-t * 8.0f) + 110.0f;
                    p1 += 2.0f * PI * fCreak / sampleRate;
                    p2 += 2.0f * PI * fWhoosh / sampleRate;
                    float creak = (t < 0.04f) ? std::sin(p1) * (1.0f - t / 0.04f) * 0.45f : 0.0f;
                    float snap = (t >= 0.035f && t < 0.045f) ? wn * 0.85f : 0.0f;
                    float whoosh = lpNoise2 * std::sin(p2) * std::sin(PI * (t / duration)) * 0.55f;
                    float body = std::sin(2.0f * PI * 130.0f * t) * std::exp(-t * 14.0f) * 0.40f;
                    sample = (creak + snap + whoosh + body) * attack;
                } else if (type == 1) {
                    // Turbo Boar Dash: Clean propulsion whoosh with sub-bass punch & stereo-phased rush
                    float fSub = 85.0f * std::exp(-t * 6.0f) + 35.0f;
                    p1 += 2.0f * PI * fSub / sampleRate;
                    p2 += 2.0f * PI * (fSub * 1.5f) / sampleRate;
                    float sub = std::tanh(2.2f * std::sin(p1)) * std::exp(-t * 7.5f) * 0.65f;
                    float rush = (wn - lpNoise) * std::exp(-t * 5.0f) * 0.50f;
                    float pop = (t < 0.015f) ? wn * (1.0f - t / 0.015f) * 0.55f : 0.0f;
                    sample = (sub + rush + pop) * attack;
                } else if (type == 2) {
                    // Tri-Piglet Split: 3-tone rosewood marimba arpeggio (C5 - E5 - G5)
                    int nIdx = (t < 0.08f) ? 0 : (t < 0.16f ? 1 : 2);
                    float lt = t - nIdx * 0.08f;
                    float f0 = (nIdx == 0 ? 523.25f : (nIdx == 1 ? 659.25f : 783.99f));
                    float m1 = std::sin(2.0f * PI * f0 * lt) * std::exp(-lt * 18.0f);
                    float m2 = std::sin(2.0f * PI * (f0 * 3.98f) * lt) * std::exp(-lt * 45.0f) * 0.25f;
                    float tap = (lt < 0.004f) ? wn * 0.20f : 0.0f;
                    sample = (m1 + m2 + tap) * std::min(1.0f, lt / 0.002f);
                } else if (type == 3) {
                    // Trotter Anvil Slam: Heavy cast-iron anvil clang + saturated 42Hz seismic shockwave
                    float fSub = 42.0f * std::exp(-t * 3.0f);
                    p1 += 2.0f * PI * fSub / sampleRate;
                    p2 += 2.0f * PI * 880.0f / sampleRate;
                    p3 += 2.0f * PI * 1420.0f / sampleRate;
                    float metal = (0.35f * std::sin(p2) + 0.25f * std::sin(p3)) * std::exp(-t * 22.0f);
                    float sub = std::tanh(2.8f * std::sin(p1)) * std::exp(-t * 5.5f) * 0.85f;
                    sample = (metal + sub) * attack;
                } else if (type == 4) {
                    // Vulcan Seismic Detonation: Modern cinematic explosion (crack transient + sub-bass + rubble wash)
                    float fSub = 65.0f * std::exp(-t * 4.0f) + 28.0f;
                    p1 += 2.0f * PI * fSub / sampleRate;
                    float crack = (t < 0.015f) ? wn * (1.0f - t / 0.015f) * 1.2f : 0.0f;
                    float subWave = std::tanh(3.0f * std::sin(p1)) * std::exp(-t * 3.5f) * 0.85f;
                    float rubble = lpNoise * std::exp(-t * 2.5f) * 0.70f;
                    sample = (crack + subWave + rubble) * attack;
                } else if (type == 5) {
                    // Wicker / Reed Shatter: Organic bamboo & twig fracture crack
                    float snapEnv = std::exp(-t * 36.0f) + ((t > 0.03f) ? 0.6f * std::exp(-(t - 0.03f) * 45.0f) : 0.0f);
                    sample = (0.40f * (wn - lpNoise) + 0.35f * lpNoise2) * snapEnv * 1.1f * attack;
                } else if (type == 6) {
                    // Seasoned Timber Impact: Resonant hollow oak plank knock (modes at 165Hz, 290Hz, 510Hz)
                    p1 += 2.0f * PI * 165.0f / sampleRate;
                    p2 += 2.0f * PI * 290.0f / sampleRate;
                    p3 += 2.0f * PI * 510.0f / sampleRate;
                    float knock = 0.55f * std::sin(p1) * std::exp(-t * 14.0f) +
                                  0.30f * std::sin(p2) * std::exp(-t * 24.0f) +
                                  0.18f * std::sin(p3) * std::exp(-t * 36.0f);
                    float crack = (t < 0.008f) ? wn * 0.40f : 0.0f;
                    sample = (knock + crack) * attack * 1.15f;
                } else if (type == 7) {
                    // Avian Vanquished: Comical organic air pop + feathered rush + glockenspiel sparkle
                    float fPop = 420.0f * std::exp(-t * 25.0f) + 140.0f;
                    p1 += 2.0f * PI * fPop / sampleRate;
                    float pop = std::sin(p1) * std::exp(-t * 18.0f) * 0.65f;
                    float feathers = (wn - lpNoise) * std::exp(-t * 8.0f) * 0.45f;
                    float sparkle = (t > 0.08f) ? std::sin(2.0f * PI * 1318.5f * t) * std::exp(-(t - 0.08f) * 14.0f) * 0.25f : 0.0f;
                    sample = (pop + feathers + sparkle) * attack;
                } else if (type == 8) {
                    // Victory Fanfare: Warm brass triad + cascading glockenspiel chimes
                    int st = (t < 0.22f) ? 0 : (t < 0.44f ? 1 : (t < 0.66f ? 2 : 3));
                    float stT = t - st * 0.22f;
                    float fLead = (st == 0 ? 523.25f : (st == 1 ? 659.25f : (st == 2 ? 783.99f : 1046.50f)));
                    p1 += 2.0f * PI * fLead / sampleRate;
                    float env = std::exp(-stT * (st == 3 ? 2.5f : 5.5f)) * std::min(1.0f, stT / 0.008f);
                    float brass = (0.55f * std::sin(p1) + 0.25f * std::sin(p1 * 2.0f) + 0.12f * std::sin(p1 * 3.0f)) * env;
                    sample = brass * 0.95f;
                } else if (type == 9) {
                    // Stone / Slate Impact: Heavy mineral thud (85Hz) + stone crumble
                    float fThud = 95.0f * std::exp(-t * 12.0f) + 42.0f;
                    p1 += 2.0f * PI * fThud / sampleRate;
                    float thud = std::tanh(1.8f * std::sin(p1)) * std::exp(-t * 15.0f) * 0.75f;
                    float grit = lpNoise2 * std::exp(-t * 30.0f) * 0.35f;
                    sample = (thud + grit) * attack;
                } else if (type == 11) {
                    // High Score Fanfare: Shimmering celesta & glockenspiel arpeggio
                    static const float chNotes[7] = {523.25f, 659.25f, 783.99f, 987.77f, 1046.50f, 1318.51f, 1567.98f};
                    float chVal = 0.0f;
                    for (int n = 0; n < 7; ++n) {
                        float nStart = n * 0.08f;
                        if (t >= nStart) {
                            float nt = t - nStart;
                            float f = chNotes[n];
                            float env = std::exp(-nt * (n == 6 ? 2.2f : 5.5f)) * std::min(1.0f, nt / 0.003f);
                            chVal += std::sin(2.0f * PI * f * nt) * env * 0.28f;
                        }
                    }
                    sample = chVal * attack;
                } else if (type == 12) {
                    // Glacial Ice Shatter: Shimmering glass overtones with crisp micro-fracture crackle
                    p1 += 2.0f * PI * 2240.0f / sampleRate;
                    p2 += 2.0f * PI * 3480.0f / sampleRate;
                    p3 += 2.0f * PI * 4920.0f / sampleRate;
                    float ring = (0.50f * std::sin(p1) + 0.32f * std::sin(p2) + 0.18f * std::sin(p3)) * std::exp(-t * 12.0f);
                    float crack = (wn - lpNoise) * std::exp(-t * 35.0f) * 0.65f;
                    sample = (ring + crack) * attack;
                } else if (type == 13) {
                    // Iron Cauldron Clang: Heavy cast iron bell chime with acoustic damping
                    p1 += 2.0f * PI * 540.0f / sampleRate;
                    p2 += 2.0f * PI * 1280.0f / sampleRate;
                    p3 += 2.0f * PI * 2450.0f / sampleRate;
                    float clang = 0.45f * std::sin(p1) * std::exp(-t * 10.0f) +
                                  0.35f * std::sin(p2) * std::exp(-t * 16.0f) +
                                  0.20f * std::sin(p3) * std::exp(-t * 26.0f);
                    sample = clang * attack * 1.1f;
                } else if (type == 14) {
                    // Trampoline Bounce: Organic rubber tension bend with elastic pitch scoop
                    float sweep = 110.0f + 240.0f * (1.0f - std::exp(-t * 14.0f)) * std::exp(-t * 5.0f);
                    p1 += 2.0f * PI * sweep / sampleRate;
                    float twang = std::sin(p1) * std::exp(-t * 6.5f) * 0.85f;
                    sample = twang * attack;
                } else if (type == 15) {
                    // Wind Gust: Smooth atmospheric breeze with gentle frequency-swept pink noise
                    float mod = std::sin(2.0f * PI * 1.5f * t);
                    float gust = lpNoise * (0.8f + 0.35f * mod) * std::sin(PI * (t / duration));
                    sample = gust * 1.1f;
                } else if (type == 16) {
                    // Thunder: Lightning crack + rolling subterranean sub-bass rumble
                    float fSub = 58.0f * std::exp(-t * 1.5f) + 24.0f;
                    p1 += 2.0f * PI * fSub / sampleRate;
                    float crack = wn * std::exp(-t * 26.0f) * 1.1f;
                    float sub = std::tanh(2.8f * std::sin(p1)) * std::exp(-t * 1.3f) * 0.85f;
                    float roll = lpNoise * std::exp(-t * 1.0f) * 0.65f;
                    sample = (crack + sub + roll) * attack;
                } else if (type == 17) {
                    // Magma: Geothermal vent pop + bubbling molten rock sizzle
                    float fPop = 130.0f * std::exp(-t * 7.0f) + 36.0f;
                    p1 += 2.0f * PI * fPop / sampleRate;
                    float pop = std::sin(p1) * std::exp(-t * 10.0f) * 0.75f;
                    float hiss = (wn - lpNoise) * std::exp(-t * 3.0f) * 0.55f;
                    sample = (pop + hiss) * attack;
                } else if (type == 18) {
                    // Golden Truffle: Sparkling 4-note ascending celestial music box chime (E6-G#6-B6-E7)
                    int nIdx = clampf(static_cast<int>(t / 0.14f), 0, 3);
                    float lt = t - nIdx * 0.14f;
                    static const float arpF[4] = {1318.51f, 1661.22f, 1975.53f, 2637.02f};
                    float f = arpF[nIdx];
                    p1 += 2.0f * PI * f / sampleRate;
                    float sparkle = std::sin(p1) * std::exp(-lt * 10.0f) * std::min(1.0f, lt / 0.003f);
                    sample = sparkle * attack * 1.25f;
                } else if (type == 19) {
                    // Achievement: Triumphant modern brass & chime fanfare
                    int fIdx = (t < 0.22f) ? 0 : (t < 0.44f ? 1 : (t < 0.66f ? 2 : 3));
                    float lt = (fIdx < 3) ? (t - fIdx * 0.22f) : (t - 0.66f);
                    static const float brF[4] = {587.33f, 739.99f, 880.00f, 1174.66f};
                    float targetF = brF[fIdx];
                    p1 += 2.0f * PI * targetF / sampleRate;
                    float bEnv = std::exp(-lt * (fIdx < 3 ? 5.0f : 2.2f)) * std::min(1.0f, lt / 0.012f);
                    float brass = (0.55f * std::sin(p1) + 0.28f * std::sin(p1 * 2.0f)) * bEnv;
                    sample = brass * attack * 1.25f;
                } else if (type == 22) {
                    // Aeronaut Glider Flight: Soaring aerodynamic wind rush with gentle wing flutter
                    float fGlide = 280.0f + 240.0f * (1.0f - std::exp(-t * 6.0f));
                    p1 += 2.0f * PI * fGlide / sampleRate;
                    float flutter = 1.0f + 0.20f * std::sin(2.0f * PI * 16.0f * t);
                    float windRush = (wn - lpNoise) * 0.50f;
                    float wing = std::sin(p1) * flutter * 0.40f;
                    sample = (windRush + wing) * std::exp(-t * 3.5f) * attack * 1.2f;
                } else if (type == 23) {
                    // Graviton Vortex: Deep cosmic sub-oscillator sweep + reverse vortex suction + implosion pop
                    float fVortex = 160.0f * (1.0f - (t / duration)) + 45.0f;
                    p1 += 2.0f * PI * fVortex / sampleRate;
                    float ringMod = std::sin(2.0f * PI * 28.0f * t);
                    float subHum = std::tanh(2.5f * std::sin(p1)) * (0.75f + 0.25f * ringMod);
                    float suction = lpNoise * (t / duration) * 0.85f;
                    float pop = (t > duration - 0.12f) ? (wn * std::exp(-(t - (duration - 0.12f)) * 40.0f)) : 0.0f;
                    sample = (subHum * 0.75f + suction + pop) * attack * 1.25f;
                } else if (type == 24) {
                    // Drill Bore: Warm mechanical rotary engine whir & resonant tungsten bore
                    float fDrill = 165.0f + 25.0f * std::sin(2.0f * PI * 14.0f * t);
                    p1 += 2.0f * PI * fDrill / sampleRate;
                    float motor = std::tanh(1.8f * std::sin(p1));
                    float subRumble = 0.35f * std::sin(p1 * 0.5f);
                    float churn = lpNoise * (0.30f + 0.15f * std::sin(2.0f * PI * 28.0f * t));
                    sample = (motor * 0.55f + subRumble + churn) * std::exp(-t * 3.4f) * attack * 0.70f;
                } else if (type == 25) {
                    // Directional Tower Shield: Resonant tempered steel deflection ping & ricochet
                    p1 += 2.0f * PI * 880.0f / sampleRate;
                    p2 += 2.0f * PI * 1760.0f / sampleRate;
                    float ring = (0.7f * std::sin(p1) + 0.35f * std::sin(p2)) * std::exp(-t * 14.0f);
                    float thud = std::sin(2.0f * PI * 110.0f * t) * std::exp(-t * 22.0f) * 0.8f;
                    sample = (ring + thud) * attack * 1.4f;
                } else if (type == 26) {
                    // Zeppelin Puncture: High-pressure canvas tearing hiss & groaning deflate
                    float canvasHiss = (wn - lpNoise) * std::exp(-t * 4.5f) * 1.1f;
                    float groan = std::sin(2.0f * PI * (90.0f - t * 40.0f) * t) * std::exp(-t * 2.5f) * 0.65f;
                    sample = (canvasHiss + groan) * attack * 1.25f;
                } else {
                    // Tactile Woodblock UI Click (type == 10)
                    float fClick = 520.0f * std::exp(-t * 40.0f) + 260.0f;
                    p1 += 2.0f * PI * fClick / sampleRate;
                    sample = std::sin(p1) * std::exp(-t * 55.0f) * attack * 0.85f;
                }

                int32_t val = static_cast<int32_t>(clampf(sample, -1.0f, 1.0f) * 26000.0f);
                pcm[i] = static_cast<int16_t>(val);
            }
        }

        std::ofstream out(filename, std::ios::binary);
        uint32_t dataBytes = static_cast<uint32_t>(numSamples * sizeof(int16_t));
        uint32_t riffSize = 36 + dataBytes;
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1;
        uint16_t numChannels = 1;
        uint32_t byteRate = sampleRate * numChannels * sizeof(int16_t);
        uint16_t blockAlign = numChannels * sizeof(int16_t);
        uint16_t bitsPerSample = 16;

        out.write("RIFF", 4);
        out.write(reinterpret_cast<const char*>(&riffSize), 4);
        out.write("WAVE", 4);
        out.write("fmt ", 4);
        out.write(reinterpret_cast<const char*>(&fmtSize), 4);
        out.write(reinterpret_cast<const char*>(&audioFormat), 2);
        out.write(reinterpret_cast<const char*>(&numChannels), 2);
        out.write(reinterpret_cast<const char*>(&sampleRate), 4);
        out.write(reinterpret_cast<const char*>(&byteRate), 4);
        out.write(reinterpret_cast<const char*>(&blockAlign), 2);
        out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        out.write("data", 4);
        out.write(reinterpret_cast<const char*>(&dataBytes), 4);
        out.write(reinterpret_cast<const char*>(pcm.data()), dataBytes);
    }

    void bootstrapDefaultAssets() {
        ensureDir("assets");
        ensureDir("assets/sprites");
        ensureDir("assets/sounds");

        ensureDir("assets/music");
        // Check asset version marker (v15 includes Graviton, Drill, Falcon, Condor characters)
        bool needUpgrade = true;
        {
            std::ifstream vf("assets/version.txt");
            std::string ver;
            if (vf.good() && std::getline(vf, ver) && ver == "v16") {
                needUpgrade = false;
            }
        }

        std::ifstream mfCheck("assets/manifest.cfg");
        if (!mfCheck.good() || needUpgrade) {
            std::ofstream mf("assets/manifest.cfg");
            mf << "# Snout & Siege Custom Asset Pipeline Manifest (v5)\n";
            mf << "# Replace any BMP sprite or WAV sound file and press F5 in-game to hot-reload.\n\n";
            mf << "sprite.pig_trotter   = assets/sprites/pig_trotter.bmp\n";
            mf << "sprite.pig_turbo     = assets/sprites/pig_turbo.bmp\n";
            mf << "sprite.pig_split     = assets/sprites/pig_split.bmp\n";
            mf << "sprite.pig_bomb      = assets/sprites/pig_bomb.bmp\n";
            mf << "sprite.pig_glider    = assets/sprites/pig_glider.bmp\n";
            mf << "sprite.pig_graviton  = assets/sprites/pig_graviton.bmp\n";
            mf << "sprite.pig_drill     = assets/sprites/pig_drill.bmp\n";
            mf << "sprite.bird_toucan   = assets/sprites/bird_toucan.bmp\n";
            mf << "sprite.bird_owl      = assets/sprites/bird_owl.bmp\n";
            mf << "sprite.bird_heron    = assets/sprites/bird_heron.bmp\n";
            mf << "sprite.bird_falcon   = assets/sprites/bird_falcon.bmp\n";
            mf << "sprite.bird_condor   = assets/sprites/bird_condor.bmp\n";
            mf << "sprite.mat_twig      = assets/sprites/mat_twig.bmp\n";
            mf << "sprite.mat_timber    = assets/sprites/mat_timber.bmp\n";
            mf << "sprite.mat_stone     = assets/sprites/mat_stone.bmp\n";
            mf << "sprite.mat_ice       = assets/sprites/mat_ice.bmp\n";
            mf << "sprite.mat_tnt       = assets/sprites/mat_tnt.bmp\n";
            mf << "sprite.mat_trampoline= assets/sprites/mat_trampoline.bmp\n";
            mf << "sprite.metal_helm    = assets/sprites/metal_helm.bmp\n";
            mf << "sprite.mat_magma     = assets/sprites/mat_magma.bmp\n";
            mf << "sprite.weather_storm = assets/sprites/weather_storm.bmp\n";
            mf << "sprite.item_truffle  = assets/sprites/item_truffle.bmp\n";
            mf << "sprite.ui_trophy     = assets/sprites/ui_trophy.bmp\n\n";
            mf << "sound.launch         = assets/sounds/launch.wav\n";
            mf << "sound.dash           = assets/sounds/ability_dash.wav\n";
            mf << "sound.split          = assets/sounds/ability_split.wav\n";
            mf << "sound.slam           = assets/sounds/ability_slam.wav\n";
            mf << "sound.explosion      = assets/sounds/explosion.wav\n";
            mf << "sound.hit_twig       = assets/sounds/hit_twig.wav\n";
            mf << "sound.hit_timber     = assets/sounds/hit_timber.wav\n";
            mf << "sound.hit_stone      = assets/sounds/hit_stone.wav\n";
            mf << "sound.hit_ice        = assets/sounds/hit_ice.wav\n";
            mf << "sound.hit_metal      = assets/sounds/hit_metal.wav\n";
            mf << "sound.boing          = assets/sounds/boing.wav\n";
            mf << "sound.wind           = assets/sounds/wind.wav\n";
            mf << "sound.thunder        = assets/sounds/thunder.wav\n";
            mf << "sound.magma          = assets/sounds/magma.wav\n";
            mf << "sound.truffle        = assets/sounds/truffle.wav\n";
            mf << "sound.achievement    = assets/sounds/achievement.wav\n";
            mf << "sound.bird_defeat    = assets/sounds/bird_defeat.wav\n";
            mf << "sound.victory        = assets/sounds/victory.wav\n";
            mf << "sound.glide          = assets/sounds/glide.wav\n";
            mf << "sound.vortex         = assets/sounds/vortex.wav\n";
            mf << "sound.drill          = assets/sounds/drill.wav\n";
            mf << "sound.shield         = assets/sounds/shield.wav\n";
            mf << "sound.zeppelin       = assets/sounds/zeppelin.wav\n";
            mf << "sound.ui_click       = assets/sounds/ui_click.wav\n";
            mf << "sound.highscore      = assets/sounds/highscore.wav\n\n";
            mf << "music.title          = assets/music/bgm_title.wav\n";
            mf << "music.battle         = assets/music/bgm_battle.wav\n";
        }
        mfCheck.close();

        writeSynthesizedWav("assets/sounds/launch.wav", 0, needUpgrade);
        writeSynthesizedWav("assets/sounds/ability_dash.wav", 1, needUpgrade);
        writeSynthesizedWav("assets/sounds/ability_split.wav", 2, needUpgrade);
        writeSynthesizedWav("assets/sounds/ability_slam.wav", 3, needUpgrade);
        writeSynthesizedWav("assets/sounds/explosion.wav", 4, needUpgrade);
        writeSynthesizedWav("assets/sounds/hit_twig.wav", 5, needUpgrade);
        writeSynthesizedWav("assets/sounds/hit_timber.wav", 6, needUpgrade);
        writeSynthesizedWav("assets/sounds/bird_defeat.wav", 7, needUpgrade);
        writeSynthesizedWav("assets/sounds/victory.wav", 8, needUpgrade);
        writeSynthesizedWav("assets/sounds/hit_stone.wav", 9, needUpgrade);
        writeSynthesizedWav("assets/sounds/hit_ice.wav", 12, needUpgrade);
        writeSynthesizedWav("assets/sounds/hit_metal.wav", 13, needUpgrade);
        writeSynthesizedWav("assets/sounds/boing.wav", 14, needUpgrade);
        writeSynthesizedWav("assets/sounds/wind.wav", 15, needUpgrade);
        writeSynthesizedWav("assets/sounds/thunder.wav", 16, needUpgrade);
        writeSynthesizedWav("assets/sounds/magma.wav", 17, needUpgrade);
        writeSynthesizedWav("assets/sounds/truffle.wav", 18, needUpgrade);
        writeSynthesizedWav("assets/sounds/achievement.wav", 19, needUpgrade);
        writeSynthesizedWav("assets/sounds/glide.wav", 22, needUpgrade);
        writeSynthesizedWav("assets/sounds/vortex.wav", 23, needUpgrade);
        writeSynthesizedWav("assets/sounds/drill.wav", 24, needUpgrade);
        writeSynthesizedWav("assets/sounds/shield.wav", 25, needUpgrade);
        writeSynthesizedWav("assets/sounds/zeppelin.wav", 26, needUpgrade);
        writeSynthesizedWav("assets/sounds/ui_click.wav", 10, needUpgrade);
        writeSynthesizedWav("assets/sounds/highscore.wav", 11, needUpgrade);
        writeSynthesizedWav("assets/music/bgm_title.wav", 20, needUpgrade);
        writeSynthesizedWav("assets/music/bgm_battle.wav", 21, needUpgrade);

        auto saveSpriteBmp = [&](const std::string& path, int w, int h, auto drawFn) {
            if (!needUpgrade) {
                std::ifstream f(path, std::ios::binary);
                if (f.good()) return;
            }
            SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
            if (!surf) return;
            uint32_t* px = static_cast<uint32_t*>(surf->pixels);
            int pitchPx = surf->pitch / 4;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    px[y * pitchPx + x] = drawFn(x, y, w, h);
                }
            }
            SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "wb");
            if (rw) SDL_SaveBMP_RW(surf, rw, 1);
            SDL_FreeSurface(surf);
        };

        auto packARGB = [](uint8_t a, uint8_t r, uint8_t g, uint8_t b) -> uint32_t {
            return (static_cast<uint32_t>(a) << 24) |
                   (static_cast<uint32_t>(r) << 16) |
                   (static_cast<uint32_t>(g) << 8)  |
                   static_cast<uint32_t>(b);
        };

        // Rich v2 Wicker/Reed texture with bamboo segments and diagonal bindings
        saveSpriteBmp("assets/sprites/mat_twig.bmp", 64, 64, [&](int x, int y, int w, int h) {
            if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 88, 62, 24);
            bool topLeftBevel = (x < 5 || y < 5);
            int cane = (y / 8);
            int caneOffset = (cane % 2) * 16;
            int nodeDist = std::abs(((x + caneOffset) % 32) - 16);
            float tubeShade = std::cos(((y % 8) - 3.5f) * 0.42f);
            int baseR = static_cast<int>(195 + 38 * tubeShade + (topLeftBevel ? 18 : 0));
            int baseG = static_cast<int>(156 + 34 * tubeShade + (topLeftBevel ? 15 : 0));
            int baseB = static_cast<int>(78  + 22 * tubeShade);
            if (nodeDist < 2) { baseR -= 42; baseG -= 36; baseB -= 24; }
            if (y % 8 == 0)   { baseR -= 35; baseG -= 30; baseB -= 20; }
            return packARGB(255,
                            static_cast<uint8_t>(clampf(baseR, 0, 255)),
                            static_cast<uint8_t>(clampf(baseG, 0, 255)),
                            static_cast<uint8_t>(clampf(baseB, 0, 255)));
        });

        // Rich v2 Seasoned Timber texture with wood grain, bevels, and iron corner brackets
        saveSpriteBmp("assets/sprites/mat_timber.bmp", 64, 64, [&](int x, int y, int w, int h) {
            if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 52, 28, 14);
            // Iron corner straps & brass rivets
            bool inCorner = (x < 10 || x >= w - 10) && (y < 10 || y >= h - 10);
            if (inCorner) {
                bool rivet = ((x == 5 || x == w - 6) && (y == 5 || y == h - 6));
                if (rivet) return packARGB(255, 228, 188, 96);
                return packARGB(255, 68, 72, 82);
            }
            float grain = std::sin(y * 0.52f + std::sin(x * 0.14f) * 1.8f) * 12.0f +
                          std::cos(y * 1.3f - x * 0.06f) * 5.0f;
            int bevel = (y < 6 || x < 6) ? 18 : ((y > h - 6 || x > w - 6) ? -18 : 0);
            int r = static_cast<int>(clampf(158.0f + grain + bevel, 0.0f, 255.0f));
            int g = static_cast<int>(clampf(94.0f  + grain * 0.6f + bevel * 0.7f, 0.0f, 255.0f));
            int b = static_cast<int>(clampf(52.0f  + grain * 0.35f + bevel * 0.5f, 0.0f, 255.0f));
            return packARGB(255, r, g, b);
        });

        // Rich v2 Carved Slate / Basalt texture with 3D beveled ashlar blocks & runes
        saveSpriteBmp("assets/sprites/mat_stone.bmp", 64, 64, [&](int x, int y, int w, int h) {
            if (x < 3 || x >= w - 3 || y < 3 || y >= h - 3) return packARGB(255, 36, 42, 52);
            int bevel = (x < 8 || y < 8) ? 24 : ((x >= w - 8 || y >= h - 8) ? -22 : 0);
            bool runeGroove = ((x == 16 || x == w - 17) && y >= 14 && y <= h - 15) ||
                              ((y == 16 || y == h - 17) && x >= 14 && x <= w - 15);
            if (runeGroove) bevel = -28;
            int noise = (((x * 73856093) ^ (y * 19349663)) & 0xF) - 8;
            int base = static_cast<int>(clampf(108 + bevel + noise, 30, 220));
            return packARGB(255,
                            static_cast<uint8_t>(base - 8),
                            static_cast<uint8_t>(base),
                            static_cast<uint8_t>( std::min(255, base + 16)));
        });

        // Glacial Frost Ice texture with crystalline refraction & diagonal cleavage veins
        saveSpriteBmp("assets/sprites/mat_ice.bmp", 64, 64, [&](int x, int y, int w, int h) {
            if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 110, 185, 225);
            bool topBevel = (x < 5 || y < 5);
            int diag = (x + y) % 24;
            int crossDiag = std::abs((x - y) % 32 - 16);
            float vein = (diag == 0 || diag == 1) ? 35.0f : ((crossDiag < 2) ? 25.0f : 0.0f);
            int baseR = static_cast<int>(clampf(165.0f + vein + (topBevel ? 45.0f : 0.0f), 0.0f, 255.0f));
            int baseG = static_cast<int>(clampf(225.0f + vein * 0.8f + (topBevel ? 20.0f : 0.0f), 0.0f, 255.0f));
            int baseB = static_cast<int>(clampf(250.0f + vein * 0.4f, 0.0f, 255.0f));
            return packARGB(255, static_cast<uint8_t>(baseR), static_cast<uint8_t>(baseG), static_cast<uint8_t>(baseB));
        });

        // Volatile Gunpowder Keg / Crate texture with iron reinforcing bands and bold TNT stenciling
        saveSpriteBmp("assets/sprites/mat_tnt.bmp", 64, 64, [&](int x, int y, int w, int h) {
            if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 42, 18, 16);
            bool inBand = (y >= 8 && y <= 16) || (y >= h - 17 && y <= h - 9);
            if (inBand) {
                bool rivet = ((x == 6 || x == w - 7 || x == w / 2) && (y == 12 || y == h - 13));
                if (rivet) return packARGB(255, 235, 195, 95);
                return packARGB(255, 52, 56, 64);
            }
            int plank = (y / 16);
            int baseR = 192, baseG = 45, baseB = 38;
            if (plank % 2 == 1) { baseR -= 18; baseG -= 8; baseB -= 6; }
            if (y % 16 == 0)   { baseR -= 36; baseG -= 14; baseB -= 10; }
            bool isT1 = (y >= 26 && y <= 38 && x >= 14 && x <= 22) && (y <= 29 || (x >= 17 && x <= 19));
            bool isN  = (y >= 26 && y <= 38 && x >= 26 && x <= 37) && (x <= 28 || x >= 35 || std::abs((x - 26) - (y - 26)) <= 1);
            bool isT2 = (y >= 26 && y <= 38 && x >= 41 && x <= 49) && (y <= 29 || (x >= 44 && x <= 46));
            if (isT1 || isN || isT2) {
                return packARGB(255, 255, 230, 95);
            }
            return packARGB(255, static_cast<uint8_t>(baseR), static_cast<uint8_t>(baseG), static_cast<uint8_t>(baseB));
        });

        // High-resolution 192x192 faceted origami base sprites with radial paper-fold shading (v3)
        auto makeHiResOrigamiSprite = [&](uint8_t br, uint8_t bg, uint8_t bb,
                                          uint8_t sr, uint8_t sg, uint8_t sb, bool isBird) {
            return [=](int x, int y, int w, int h) -> uint32_t {
                float nx = (x - w * 0.5f) / (w * 0.44f);
                float ny = (y - h * 0.5f) / (h * 0.44f);
                float dodec = std::max({std::abs(nx), std::abs(ny),
                                        (std::abs(nx) + std::abs(ny)) * 0.71f,
                                        (std::abs(nx) * 0.866f + std::abs(ny) * 0.5f),
                                        (std::abs(nx) * 0.5f + std::abs(ny) * 0.866f)});
                if (dodec > 1.0f) return packARGB(0, 0, 0, 0);
                if (dodec > 0.91f) return packARGB(255, 24, 20, 22);
                // Multi-facet angular paper-fold shading + subtle paper grain
                float ang = std::atan2(ny, nx);
                int sector = static_cast<int>((ang + PI) / (PI / 4.0f)) % 8;
                float sectorShade = (sector == 0 || sector == 1 || sector == 7) ? 1.08f :
                                    ((sector == 3 || sector == 4) ? 0.84f : 0.96f);
                float innerBev = (dodec < 0.62f) ? 1.05f : 0.97f;
                int grain = (((x * 374761393) ^ (y * 668265263)) & 0x7) - 3;
                float mult = sectorShade * innerBev;
                uint8_t r = static_cast<uint8_t>(clampf(br * mult + grain, 0.0f, 255.0f));
                uint8_t g = static_cast<uint8_t>(clampf(bg * mult + grain, 0.0f, 255.0f));
                uint8_t b = static_cast<uint8_t>(clampf(bb * mult + grain, 0.0f, 255.0f));
                if (!isBird && std::abs(nx - 0.22f) < 0.34f && std::abs(ny - 0.10f) < 0.26f) {
                    return packARGB(255, sr, sg, sb);
                }
                if (isBird && ny > -0.15f && ny < 0.30f && std::abs(nx) < (0.30f - ny) * 0.8f) {
                    return packARGB(255, sr, sg, sb);
                }
                return packARGB(255, r, g, b);
            };
        };

        saveSpriteBmp("assets/sprites/pig_trotter.bmp", 192, 192,
                      makeHiResOrigamiSprite(209, 102, 78, 238, 155, 130, false));
        saveSpriteBmp("assets/sprites/pig_turbo.bmp", 192, 192,
                      makeHiResOrigamiSprite(230, 168, 52, 250, 208, 112, false));
        saveSpriteBmp("assets/sprites/pig_split.bmp", 192, 192,
                      makeHiResOrigamiSprite(76, 168, 135, 142, 219, 190, false));
        saveSpriteBmp("assets/sprites/pig_bomb.bmp", 192, 192,
                      makeHiResOrigamiSprite(68, 74, 86, 214, 94, 48, false));
        saveSpriteBmp("assets/sprites/pig_glider.bmp", 192, 192,
                      makeHiResOrigamiSprite(82, 168, 238, 175, 225, 255, false));
        saveSpriteBmp("assets/sprites/pig_graviton.bmp", 192, 192,
                      makeHiResOrigamiSprite(110, 52, 185, 210, 165, 255, false));
        saveSpriteBmp("assets/sprites/pig_drill.bmp", 192, 192,
                      makeHiResOrigamiSprite(175, 102, 32, 245, 195, 120, false));

        saveSpriteBmp("assets/sprites/bird_toucan.bmp", 192, 192,
                      makeHiResOrigamiSprite(38, 122, 138, 242, 132, 62, true));
        saveSpriteBmp("assets/sprites/bird_owl.bmp", 192, 192,
                      makeHiResOrigamiSprite(78, 82, 126, 218, 176, 78, true));
        saveSpriteBmp("assets/sprites/bird_heron.bmp", 192, 192,
                      makeHiResOrigamiSprite(228, 222, 206, 246, 194, 52, true));
        saveSpriteBmp("assets/sprites/bird_falcon.bmp", 192, 192,
                      makeHiResOrigamiSprite(48, 68, 88, 230, 175, 45, true));
        saveSpriteBmp("assets/sprites/bird_condor.bmp", 192, 192,
                      makeHiResOrigamiSprite(34, 34, 40, 240, 185, 50, true));

        // Bouncy Trampoline Deflector texture with dark steel rim, brass corner studs, and emerald woven spring mat
        saveSpriteBmp("assets/sprites/mat_trampoline.bmp", 64, 64, [&](int x, int y, int w, int h) {
            if (x < 3 || x >= w - 3 || y < 3 || y >= h - 3) return packARGB(255, 32, 38, 48); // steel border
            // Brass corner studs
            bool corner = (x < 8 || x >= w - 8) && (y < 8 || y >= h - 8);
            if (corner) {
                bool stud = (x >= 4 && x <= 6 && y >= 4 && y <= 6) ||
                            (x >= w - 7 && x <= w - 5 && y >= 4 && y <= 6) ||
                            (x >= 4 && x <= 6 && y >= h - 7 && y <= h - 5) ||
                            (x >= w - 7 && x <= w - 5 && y >= h - 7 && y <= h - 5);
                if (stud) return packARGB(255, 235, 195, 80);
                return packARGB(255, 55, 62, 74);
            }
            // Woven emerald canvas lattice & spring chevrons
            int weave = ((x / 4) + (y / 4)) % 2;
            int r = (weave == 0) ? 36 : 28;
            int g = (weave == 0) ? 148 : 122;
            int b = (weave == 0) ? 84 : 68;
            // Golden center spring chevron
            int cx = w / 2, cy = h / 2;
            int dy = y - cy;
            int chevronX = std::abs(x - cx);
            if (std::abs(dy + 6 - chevronX / 2) <= 1 || std::abs(dy - 6 - chevronX / 2) <= 1) {
                return packARGB(255, 245, 215, 85);
            }
            return packARGB(255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
        });

        // Cauldron Helmet Icon texture (iron kettle with flared brim, rivets, and top handle)
        saveSpriteBmp("assets/sprites/metal_helm.bmp", 64, 64, [&](int x, int y, int w, int h) {
            float nx = (x - w * 0.5f) / (w * 0.42f);
            float ny = (y - h * 0.52f) / (h * 0.40f);
            // Top brass bail handle
            if (y >= 8 && y <= 16 && std::abs(x - w / 2) <= 10) {
                if (y <= 11 || std::abs(x - w / 2) >= 8) return packARGB(255, 215, 175, 75);
            }
            // Helmet dome
            if (ny < 0.0f) {
                float rDome = std::sqrt(nx * nx + ny * ny);
                if (rDome > 1.0f) return packARGB(0, 0, 0, 0);
                float shade = 0.75f + 0.45f * (1.0f - nx * 0.7f - ny * 0.5f);
                int base = static_cast<int>(clampf(110.0f * shade, 0.0f, 255.0f));
                // Central ridge line
                if (std::abs(nx) < 0.08f) base += 35;
                return packARGB(255, static_cast<uint8_t>(std::max(0, base - 5)), static_cast<uint8_t>(base), static_cast<uint8_t>(std::min(255, base + 15)));
            } else if (ny <= 0.55f && std::abs(nx) <= 1.08f) {
                // Lower cylinder & flared brim
                float shade = 0.8f + 0.35f * (1.0f - nx * 0.6f);
                int base = static_cast<int>(clampf(95.0f * shade, 0.0f, 255.0f));
                // Rivet row
                if (y >= h / 2 && y <= h / 2 + 3 && (x % 10 == 0)) {
                    return packARGB(255, 235, 195, 85);
                }
                return packARGB(255, static_cast<uint8_t>(std::max(0, base - 5)), static_cast<uint8_t>(base), static_cast<uint8_t>(std::min(255, base + 12)));
            } else if (ny > 0.55f && ny <= 0.85f && std::abs(nx) <= 1.25f) {
                // Flared brim
                return packARGB(255, 65, 72, 85);
            }
            return packARGB(0, 0, 0, 0);
        });

        // Volcanic Magma Vent texture: Obsidian basalt with jagged glowing molten lava veins
        saveSpriteBmp("assets/sprites/mat_magma.bmp", 64, 64, [&](int x, int y, int w, int h) {
            if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 30, 20, 24);
            // Lava vent aperture at top
            if (y <= 10 && x >= 18 && x <= w - 19) {
                float distTop = std::abs(x - w / 2) / 14.0f;
                if (distTop < 1.0f) {
                    int r = 255;
                    int g = static_cast<int>(180 + 75 * (1.0f - distTop));
                    int b = static_cast<int>(30 + 50 * (1.0f - distTop));
                    return packARGB(255, r, static_cast<uint8_t>(clampf(g, 0, 255)), static_cast<uint8_t>(clampf(b, 0, 255)));
                }
            }
            // Molten fissures branching down
            float fissure1 = std::abs((x - 32) - std::sin(y * 0.22f) * 10.0f);
            float fissure2 = std::abs((x - 16) - std::cos(y * 0.18f) * 8.0f);
            float fissure3 = std::abs((x - 48) + std::sin(y * 0.19f) * 9.0f);
            float minFissure = std::min({fissure1, fissure2, fissure3});
            if (minFissure < 2.0f) {
                return packARGB(255, 255, static_cast<uint8_t>(200 - minFissure * 40), 40);
            } else if (minFissure < 4.5f) {
                return packARGB(255, 220, 75, 20);
            }
            // Dark basalt rock texture with noise and bevel
            int bevel = (x < 6 || y < 6) ? 14 : ((x >= w - 6 || y >= h - 6) ? -16 : 0);
            int noise = (((x * 614689) ^ (y * 982451)) & 0xF) - 8;
            int baseR = static_cast<int>(clampf(42 + bevel + noise, 18, 90));
            int baseG = static_cast<int>(clampf(34 + bevel + noise, 14, 80));
            int baseB = static_cast<int>(clampf(38 + bevel + noise, 16, 85));
            return packARGB(255, static_cast<uint8_t>(baseR), static_cast<uint8_t>(baseG), static_cast<uint8_t>(baseB));
        });

        // Storm Cloud & Lightning Icon texture: Dark ominous storm cloud with electric lightning fork
        saveSpriteBmp("assets/sprites/weather_storm.bmp", 64, 64, [&](int x, int y, int w, int h) {
            float nx = (x - w * 0.5f) / (w * 0.44f);
            float ny = (y - h * 0.42f) / (h * 0.35f);
            // Cloud puffs (3 overlapping lobes)
            float c1 = std::sqrt((nx + 0.35f) * (nx + 0.35f) + (ny + 0.05f) * (ny + 0.05f));
            float c2 = std::sqrt(nx * nx + (ny - 0.15f) * (ny - 0.15f));
            float c3 = std::sqrt((nx - 0.38f) * (nx - 0.38f) + (ny + 0.08f) * (ny + 0.08f));
            float cBase = std::sqrt(nx * nx * 0.8f + (ny - 0.35f) * (ny - 0.35f) * 2.5f);
            bool inCloud = (c1 < 0.48f || c2 < 0.62f || c3 < 0.44f || (cBase < 0.85f && ny >= 0.0f && ny <= 0.55f));

            // Electric lightning fork
            bool bolt1 = (y >= 26 && y <= 40 && std::abs((x - 30) - (y - 26) * 0.3f) <= 1.5f);
            bool bolt2 = (y >= 38 && y <= 58 && std::abs((x - 36) + (y - 38) * 0.4f) <= 1.5f);
            bool boltStud = (y >= 38 && y <= 41 && x >= 28 && x <= 38);
            if (bolt1 || bolt2 || boltStud) {
                return packARGB(255, 255, 248, 120); // bright electric yellow
            }
            if (inCloud) {
                float shade = 0.85f - (ny + 0.5f) * 0.4f;
                int base = static_cast<int>(clampf(70.0f * shade, 25.0f, 130.0f));
                return packARGB(255, static_cast<uint8_t>(base), static_cast<uint8_t>(base + 4), static_cast<uint8_t>(base + 14));
            }
            return packARGB(0, 0, 0, 0);
        });

        // Golden Truffle Icon texture: Radiant origami truffle with faceted lobes and golden ambient glints
        saveSpriteBmp("assets/sprites/item_truffle.bmp", 64, 64, [&](int x, int y, int w, int h) {
            float nx = (x - w * 0.5f) / (w * 0.44f);
            float ny = (y - h * 0.52f) / (h * 0.42f);
            float dist = std::sqrt(nx * nx + ny * ny);
            if (dist > 1.0f) return packARGB(0, 0, 0, 0);

            float angle = std::atan2(ny, nx);
            float sector = std::fmod(angle + PI * 2.0f, PI / 3.0f) / (PI / 3.0f);
            float shade = 0.75f + 0.35f * (1.0f - nx * 0.5f - ny * 0.6f);
            if (sector < 0.15f || sector > 0.85f) shade *= 0.80f; // origami crease
            int r = static_cast<int>(clampf(248.0f * shade + 12.0f, 0.0f, 255.0f));
            int g = static_cast<int>(clampf(198.0f * shade, 0.0f, 255.0f));
            int b = static_cast<int>(clampf(58.0f * shade, 0.0f, 255.0f));

            // Diamond sparkle star at top-left
            float gx = (x - 22) / 7.0f, gy = (y - 18) / 7.0f;
            float gDist = std::sqrt(gx * gx + gy * gy);
            if (gDist < 1.0f && (std::abs(gx) < 0.28f || std::abs(gy) < 0.28f)) {
                return packARGB(255, 255, 255, 235);
            }
            return packARGB(255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
        });

        // Royal Trophy Cup Icon texture: Gleaming gold chalice with handles, pedestal, and star
        saveSpriteBmp("assets/sprites/ui_trophy.bmp", 64, 64, [&](int x, int y, int w, int h) {
            float nx = (x - w * 0.5f) / (w * 0.44f);
            float ny = (y - h * 0.50f) / (h * 0.44f);

            // 1. Pedestal base (mahogany plinth at bottom)
            if (y >= 52 && y <= 60 && std::abs(x - w / 2) <= 20) {
                int woodR = (y <= 54) ? 140 : 85;
                return packARGB(255, woodR, 45, 28);
            }
            // 2. Trophy stem & flared foot
            if (y >= 44 && y < 52 && std::abs(x - w / 2) <= (5 + (y - 44) * 1.4f)) {
                return packARGB(255, 225, 175, 55);
            }
            // 3. Side handles (curved loops)
            if (y >= 16 && y <= 36 && std::abs(nx) >= 0.55f && std::abs(nx) <= 0.95f) {
                float hy = (y - 26) / 10.0f;
                float hx = (std::abs(nx) - 0.75f) / 0.20f;
                if (hx * hx + hy * hy <= 1.0f && hx * hx + hy * hy >= 0.40f) {
                    return packARGB(255, 215, 160, 45);
                }
            }
            // 4. Main golden cup bowl
            if (y >= 10 && y <= 44 && std::abs(nx) <= 0.65f) {
                float cupWidth = 0.65f * (0.6f + 0.4f * (1.0f - (y - 10) / 34.0f));
                if (std::abs(nx) <= cupWidth) {
                    float shade = 0.8f + 0.35f * (1.0f - nx * 0.7f - ny * 0.3f);
                    if (y <= 13) shade += 0.25f; // rim highlight
                    if (std::abs(x - w / 2) <= 4 && std::abs(y - 26) <= 4) {
                        return packARGB(255, 255, 255, 220); // star emblem
                    }
                    int r = static_cast<int>(clampf(245.0f * shade, 0.0f, 255.0f));
                    int g = static_cast<int>(clampf(190.0f * shade, 0.0f, 255.0f));
                    int b = static_cast<int>(clampf(50.0f * shade, 0.0f, 255.0f));
                    return packARGB(255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
                }
            }
            return packARGB(0, 0, 0, 0);
        });

        if (needUpgrade) {
            std::ofstream vf("assets/version.txt");
            vf << "v16\n";
        }
    }

    bool loadAll(SDL_Renderer* ren) {
        bootstrapDefaultAssets();
        manifest.clear();
        std::ifstream mf("assets/manifest.cfg");
        std::string line;
        while (std::getline(mf, line)) {
            auto hashPos = line.find('#');
            if (hashPos != std::string::npos) line = line.substr(0, hashPos);
            auto eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;
            auto trim = [](std::string s) {
                while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
                while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
                return s;
            };
            std::string key = trim(line.substr(0, eqPos));
            std::string val = trim(line.substr(eqPos + 1));
            if (!key.empty() && !val.empty()) manifest[key] = val;
        }

        for (auto& kv : textures) {
            if (kv.second) SDL_DestroyTexture(kv.second);
        }
        textures.clear();

        if (audioDevice != 0) SDL_LockAudioDevice(audioDevice);
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            for (auto& v : sfxVoices) {
                v.active = false;
                v.samples = nullptr;
                v.cursor = 0;
            }
            bgmCursor = 0;
            currentMusicKey.clear();
            for (auto& kv : sounds) {
                if (kv.second.buffer) SDL_FreeWAV(kv.second.buffer);
            }
            sounds.clear();
            for (auto& kv : musicTracks) {
                if (kv.second.buffer) SDL_FreeWAV(kv.second.buffer);
            }
            musicTracks.clear();
        }
        if (audioDevice != 0) SDL_UnlockAudioDevice(audioDevice);

        loadedSpriteCount = 0;
        loadedSoundCount = 0;

        for (const auto& kv : manifest) {
            if (kv.first.rfind("sprite.", 0) == 0 && ren) {
                SDL_RWops* rw = SDL_RWFromFile(kv.second.c_str(), "rb");
                if (rw) {
                    SDL_Surface* surf = SDL_LoadBMP_RW(rw, 1);
                    if (surf) {
                        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                        if (tex) {
                            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                            textures[kv.first] = tex;
                            loadedSpriteCount++;
                        }
                        SDL_FreeSurface(surf);
                    }
                }
            } else if (kv.first.rfind("sound.", 0) == 0) {
                SDL_RWops* rw = SDL_RWFromFile(kv.second.c_str(), "rb");
                if (rw) {
                    WavSound ws{};
                    if (SDL_LoadWAV_RW(rw, 1, &ws.spec, &ws.buffer, &ws.length)) {
                        sounds[kv.first] = ws;
                        loadedSoundCount++;
                    }
                }
            } else if (kv.first.rfind("music.", 0) == 0) {
                SDL_RWops* rw = SDL_RWFromFile(kv.second.c_str(), "rb");
                if (rw) {
                    WavSound ws{};
                    if (SDL_LoadWAV_RW(rw, 1, &ws.spec, &ws.buffer, &ws.length)) {
                        musicTracks[kv.first] = ws;
                    }
                }
            }
        }

        if (audioDevice == 0) {
            SDL_AudioSpec desired{};
            desired.freq = 44100;
            desired.format = 0x8010; // AUDIO_S16LSB
            desired.channels = 1;
            desired.samples = 1024;
            desired.callback = audioMixerCallback;
            desired.userdata = this;
            audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
            if (audioDevice != 0) {
                SDL_PauseAudioDevice(audioDevice, 0);
            }
        }
        return (loadedSpriteCount > 0 && loadedSoundCount > 0);
    }

    SDL_Texture* getTexture(const std::string& key) const {
        auto it = textures.find(key);
        return (it != textures.end()) ? it->second : nullptr;
    }

    void shutdown() {
        if (audioDevice != 0) {
            SDL_PauseAudioDevice(audioDevice, 1);
            SDL_CloseAudioDevice(audioDevice);
            audioDevice = 0;
        }
        for (auto& kv : textures) {
            if (kv.second) SDL_DestroyTexture(kv.second);
        }
        textures.clear();
        for (auto& kv : sounds) {
            if (kv.second.buffer) SDL_FreeWAV(kv.second.buffer);
        }
        sounds.clear();
        for (auto& kv : musicTracks) {
            if (kv.second.buffer) SDL_FreeWAV(kv.second.buffer);
        }
        musicTracks.clear();
    }
};

// ============================================================================
// MAIN GAME ENGINE & 2D RIGID-BODY PHYSICS SOLVER
// ============================================================================
static constexpr int NUM_CAMPAIGN_LEVELS = 140;
static constexpr int NUM_VAULT_LEVELS = 10;
static constexpr int NUM_LEVELS = NUM_CAMPAIGN_LEVELS + NUM_VAULT_LEVELS; // 150

enum class ScreenState {
    TITLE = 0,
    LEVEL_SELECT = 1,
    PLAYING = 2,
    ACHIEVEMENTS = 3,
    EDITOR = 4,
    ENDLESS = 5
};

struct UIButton {
    std::string id;
    SDL_FRect rect{};
    std::string label;
    SDL_Color bgCol{38, 48, 68, 245};
    SDL_Color hoverCol{60, 80, 116, 255};
    SDL_Color borderCol{210, 172, 85, 255};
    SDL_Color textCol{248, 244, 230, 255};
    float textScale = 2.0f;

    bool contains(float mx, float my) const {
        return mx >= rect.x && mx <= (rect.x + rect.w) &&
               my >= rect.y && my <= (rect.y + rect.h);
    }
};

static void drawButton(SDL_Renderer* ren, const UIButton& btn, float mx, float my) {
    bool hovered = btn.contains(mx, my);
    SDL_Color fill = hovered ? btn.hoverCol : btn.bgCol;
    SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRectF(ren, &btn.rect);

    Vec2 p0(btn.rect.x, btn.rect.y);
    Vec2 p1(btn.rect.x + btn.rect.w, btn.rect.y);
    Vec2 p2(btn.rect.x + btn.rect.w, btn.rect.y + btn.rect.h);
    Vec2 p3(btn.rect.x, btn.rect.y + btn.rect.h);
    SDL_Color bCol = hovered ? SDL_Color{255, 235, 140, 255} : btn.borderCol;
    drawThickLine(ren, p0, p1, hovered ? 2.5f : 1.8f, bCol);
    drawThickLine(ren, p1, p2, hovered ? 2.5f : 1.8f, bCol);
    drawThickLine(ren, p2, p3, hovered ? 2.5f : 1.8f, bCol);
    drawThickLine(ren, p3, p0, hovered ? 2.5f : 1.8f, bCol);

    // Subtle 1px inner top highlight for modern sleek bevel
    SDL_SetRenderDrawColor(ren, 255, 255, 255, hovered ? 45 : 22);
    SDL_FRect hl{btn.rect.x + 2.0f, btn.rect.y + 2.0f, btn.rect.w - 4.0f, 1.5f};
    SDL_RenderFillRectF(ren, &hl);

    float scale = btn.textScale;
    float maxTextW = btn.rect.w - 18.0f;
    if (!btn.label.empty()) {
        float curW = static_cast<float>(btn.label.size()) * 6.0f * scale;
        if (curW > maxTextW && maxTextW > 10.0f) {
            scale = maxTextW / (static_cast<float>(btn.label.size()) * 6.0f);
        }
    }
    float textY = btn.rect.y + (btn.rect.h - 7.0f * scale) * 0.5f;
    SDL_Color tCol = hovered ? SDL_Color{255, 255, 245, 255} : btn.textCol;
    drawText(ren, btn.label, btn.rect.x + btn.rect.w * 0.5f, textY, scale, tCol, true, true);
}

// ============================================================================
// MAIN GAME ENGINE & 2D RIGID-BODY PHYSICS SOLVER
// ============================================================================
class SnoutAndSiegeGame {
public:
    AssetPipeline assets;
    ScreenState screen = ScreenState::TITLE;
    std::vector<LevelSpec> levels;
    int currentLevelIdx = 0;
    int levelSelectPage = 0;       // 0 for Levels 1-25, 1 for Levels 26-50

    std::array<int, NUM_LEVELS> bestStars{};
    std::array<int, NUM_LEVELS> bestScores{};

    // Active level state
    std::vector<PigType> remainingPigs;
    std::vector<PigBody> activePigs;
    std::vector<BlockBody> blocks;
    std::vector<BirdTarget> birds;
    std::vector<PendulumBody> pendulums;
    std::vector<Particle> particles;
    std::vector<FloatingScore> floatScores;

    // Feature Pack C: Atmospheric Hazards & Dynamic Weather
    float currentWind = 0.0f;
    float windGustTimer = 0.0f;
    float skyFlashAlpha = 0.0f;
    float stormStrikeTimer = 12.0f;
    std::vector<StormCloud> stormClouds;
    std::vector<LightningBolt> lightningBolts;

    // Feature Pack D: Secret Truffle Hunt, Bonus Vaults, and Achievement System
    std::array<bool, NUM_LEVELS> trufflesFound{};
    std::array<bool, 12> achievementsUnlocked{};
    std::array<int, 12> achievementProgress{};
    std::vector<AchievementDef> achievementDefs;
    GoldenTruffle levelTruffle;
    AchievementToast toast;

    // Turn / shot statistics for dynamic achievement tracking
    int shotBlocksDestroyed = 0;
    int shotTntDetonated = 0;
    bool shotHitTrampoline = false;
    bool shotUsedAnvil = false;
    int shotDrillPierces = 0;
    Vec2 shotLaunchPos{0.0f, 0.0f};

    // UI & High Score Notification State
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    std::vector<UIButton> activeButtons;
    SDL_Window* appWindow = nullptr;
    bool isFullscreen = false;
    bool isNewRecordThisRun = false;
    int prevLevelBest = 0;
    float recordAnimTimer = 0.0f;

    SnoutAndSiegeGame() {
        initAchievements();
    }

    void initAchievements() {
        achievementDefs = {
            {AchievementId::FIRST_BLOOD, "first_blood", "First Strike", "Defeat your first aristocratic Totem Bird sentry.", 1},
            {AchievementId::MARKSMAN, "marksman", "Long-Distance Swine", "Defeat a Totem Bird from over 750px slingshot range.", 1},
            {AchievementId::CAULDRON_CRACKER, "cauldron_cracker", "Cauldron Cracker", "Shatter 5 iron cauldron helmets off sentries.", 5},
            {AchievementId::SEISMIC_ANNIHILATION, "seismic_annihilation", "Seismic Annihilation", "Demolish 12 or more fortress blocks in a single shot.", 12},
            {AchievementId::ANVIL_EXECUTIONER, "anvil_executioner", "Anvil Executioner", "Crush an Archduke Heron with Trotter's Anvil Slam dive.", 1},
            {AchievementId::POWDER_MAESTRO, "powder_maestro", "Powder Keg Maestro", "Detonate 3 or more TNT gunpowder kegs in a single shot.", 3},
            {AchievementId::ACROBATIC_HOG, "acrobatic_hog", "Acrobatic Swine", "Defeat a Totem Bird on a ricochet after hitting a Trampoline.", 1},
            {AchievementId::TRUFFLE_SNIFFER, "truffle_sniffer", "Truffle Sniffer", "Discover your first Hidden Golden Truffle in the campaign.", 1},
            {AchievementId::TRUFFLE_CONNOISSEUR, "truffle_connoisseur", "Truffle Connoisseur", "Discover 20 Hidden Golden Truffles across the campaign.", 20},
            {AchievementId::VAULT_CONQUEROR, "vault_conqueror", "Vault Conqueror", "Conquer all 10 Secret Truffle Vault challenge levels.", 10},
            {AchievementId::VORTEX_MAESTRO, "vortex_maestro", "Event Horizon", "Suck 8 blocks or birds into Atticus's Graviton Vortex.", 8},
            {AchievementId::DEEP_BORE, "deep_bore", "Deep Bore", "Bore clean through 3 or more fortress blocks in a single drill shot.", 3}
        };
    }

    int getAchievementsMask() const {
        int mask = 0;
        for (int i = 0; i < static_cast<int>(AchievementId::COUNT); ++i) {
            if (achievementsUnlocked[i]) mask |= (1 << i);
        }
        return mask;
    }

    int getAchievementsCount() const {
        int count = 0;
        for (int i = 0; i < static_cast<int>(AchievementId::COUNT); ++i) {
            if (achievementsUnlocked[i]) count++;
        }
        return count;
    }

    int getTrufflesFoundCount() const {
        int count = 0;
        for (bool b : trufflesFound) {
            if (b) count++;
        }
        return count;
    }

    static int getVaultTruffleReq(int vaultIdx) {
        static const int reqs[10] = {2, 5, 8, 11, 14, 18, 22, 26, 30, 35};
        if (vaultIdx >= 0 && vaultIdx < NUM_VAULT_LEVELS) return reqs[vaultIdx];
        return 99;
    }

    void showAchievementToast(const std::string& title, const std::string& desc) {
        toast.title = title;
        toast.desc = desc;
        toast.timer = 4.5f;
        toast.slideY = -80.0f;
        toast.active = true;
    }

    void updateAchievementToast(float dt) {
        if (toast.active) {
            toast.timer -= dt;
            if (toast.timer <= 0.0f) {
                toast.active = false;
            } else if (toast.timer > 4.0f) {
                float t = (4.5f - toast.timer) / 0.5f;
                toast.slideY = -80.0f + t * 96.0f;
            } else if (toast.timer < 0.5f) {
                float t = toast.timer / 0.5f;
                toast.slideY = -80.0f + t * 96.0f;
            } else {
                toast.slideY = 16.0f;
            }
        }
    }

    void unlockAchievement(AchievementId id) {
        int idx = static_cast<int>(id);
        if (idx < 0 || idx >= static_cast<int>(AchievementId::COUNT) || achievementsUnlocked[idx]) return;
        achievementsUnlocked[idx] = true;
        if (idx < static_cast<int>(achievementDefs.size())) {
            achievementProgress[idx] = achievementDefs[idx].targetCount;
            showAchievementToast(achievementDefs[idx].title, achievementDefs[idx].desc);
        }
        assets.playSound("sound.achievement");
        saveProgress();
    }

    void collectGoldenTruffle() {
        if (!levelTruffle.exists || levelTruffle.collected) return;
        levelTruffle.collected = true;
        if (currentLevelIdx >= 0 && currentLevelIdx < NUM_LEVELS) {
            trufflesFound[currentLevelIdx] = true;
        }

        // Spawn 24 golden sparkle particles radiating outward
        for (int i = 0; i < 24; ++i) {
            Particle p{};
            p.pos = levelTruffle.pos;
            float ang = (i * (360.0f / 24.0f) + (SDL_GetTicks() % 15)) * (PI / 180.0f);
            float spd = 70.0f + (i % 6) * 35.0f;
            p.vel = Vec2(std::cos(ang) * spd, std::sin(ang) * spd);
            p.size = 6.0f + (i % 4) * 2.5f;
            p.life = 1.2f + (i % 5) * 0.2f;
            p.maxLife = p.life;
            p.kind = ParticleKind::GOLD_SPARKLE;
            int cVar = i % 3;
            p.color = (cVar == 0) ? SDL_Color{255, 235, 90, 255}
                                  : ((cVar == 1) ? SDL_Color{255, 205, 40, 255}
                                                 : SDL_Color{255, 255, 230, 255});
            particles.push_back(p);
        }

        assets.playSound("sound.truffle");
        addTrauma(0.32f);
        score += 2500;
        floatScores.push_back({levelTruffle.pos - camPos, "+2500 GOLDEN TRUFFLE!", {255, 220, 75, 255}, 1.8f, 1.8f});

        unlockAchievement(AchievementId::TRUFFLE_SNIFFER);
        if (getTrufflesFoundCount() >= 20) {
            unlockAchievement(AchievementId::TRUFFLE_CONNOISSEUR);
        }

        int tCount = getTrufflesFoundCount();
        for (int v = 0; v < NUM_VAULT_LEVELS; ++v) {
            if (tCount == getVaultTruffleReq(v)) {
                std::string vNum = (v + 1 < 10 ? "0" : "") + std::to_string(v + 1);
                floatScores.push_back({{WINDOW_W * 0.5f, 180.0f},
                                      "SECRET VAULT " + vNum + " UNLOCKED!",
                                      {225, 150, 255, 255}, 2.4f, 2.4f});
                break;
            }
        }
        saveProgress();
    }

    void toggleFullscreen() {
        isFullscreen = !isFullscreen;
        if (appWindow) {
            SDL_SetWindowFullscreen(appWindow, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
    }

    bool draggingSling = false;
    Vec2 slingPullPos{SLING_X, SLING_Y};
    int score = 0;
    int earnedStars = 0;
    bool levelComplete = false;
    bool levelFailed = false;
    bool fortressAwake = false;
    float settleTimer = 0.0f;
    float physicsAccumulator = 0.0f;
    static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
    float simSpeed = 1.0f; // 1.0f = normal speed, 2.5f = fast forward

    Vec2 camPos{0.0f, 0.0f};
    Vec2 camTarget{0.0f, 0.0f};
    Vec2 camShakeOffset{0.0f, 0.0f};
    float shakeTrauma = 0.0f;
    float shakeTime = 0.0f;
    bool isManualPanning = false;
    float manualPanTimer = 0.0f;
    float lastPanMouseX = 0.0f;

    // Visual Polish & Juice
    std::vector<Vec2> ghostTrajectory;
    std::vector<Vec2> currentFlightTrail;
    float slingRecoilTimer = 0.0f;
    Vec2 lastLaunchDir{1.0f, 0.0f};

    // Feature Pack E: Gale the Aeronaut & Aerial Ordnance
    std::vector<AerialBomb> aerialBombs;

    // Feature Pack E: Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
    bool sloMoActive = false;
    float sloMoTimer = 0.0f;
    float sloMoFactor = 0.22f;
    float letterboxY = 0.0f;
    Vec2 sloMoFocusPos{0.0f, 0.0f};
    bool enableSloMoCam = true;

    // Feature Pack E: Interactive Fortress Architect & Level Sandbox
    int editorTool = 1; // 0=ERASER, 1=BLOCK, 2=BIRD, 3=PENDULUM, 4=TRUFFLE
    MaterialTier editorMat = MaterialTier::TIMBER;
    float editorBlockW = 60.0f, editorBlockH = 22.0f;
    BirdClass editorBird = BirdClass::TOUCAN;
    bool editorBirdHelmet = false;
    bool editorSpikedPendulum = true;
    bool editorSnapGrid = true;
    float editorWind = 0.0f;
    bool editorStorm = false;
    int editorTheme = 0; // 0=Meadow, 1=Timber, 2=Quarry, 3=Glacier, 4=Thunder, 5=Volcano
    LevelSpec customLevel;
    LevelSpec customBackup;
    bool isTestPlaying = false;
    int customSlot = 1;

    // Feature Pack E: Endless Siege Gauntlet Mode
    int endlessWave = 1;
    int endlessScore = 0;
    int endlessBestWave = 0;
    float endlessComboMultiplier = 1.0f;
    bool endlessGameOver = false;

    void addTrauma(float amount) {
        // Toned down camera shake as requested: scale down trauma by 0.45x
        shakeTrauma = clampf(shakeTrauma + amount * 0.45f, 0.0f, 1.0f);
    }

    static BlockBody makeBlock(MaterialTier mat, float x, float y, float w, float h) {
        BlockBody b{};
        b.mat = mat;
        b.pos = {x, y};
        b.w = w;
        b.h = h;
        float density = (mat == MaterialTier::TWIG)       ? 0.75f :
                        (mat == MaterialTier::TIMBER     ? 1.45f :
                        (mat == MaterialTier::STONE      ? 2.85f :
                        (mat == MaterialTier::ICE        ? 0.95f :
                        (mat == MaterialTier::TRAMPOLINE ? 3.00f :
                        (mat == MaterialTier::MAGMA_VENT ? 8.00f : 1.35f))))); // TNT = 1.35f
        b.mass = (mat == MaterialTier::TRAMPOLINE) ? 6.0f :
                 (mat == MaterialTier::MAGMA_VENT) ? 8.0f :
                 std::max(1.0f, (w * h * 0.0012f) * density);
        b.invMass = 1.0f / b.mass;
        b.inertia = b.mass * (w * w + h * h) / 36.0f;
        b.invInertia = 1.0f / b.inertia;
        b.maxHp = (mat == MaterialTier::TWIG)       ? 68.0f :
                  (mat == MaterialTier::TIMBER     ? 170.0f :
                  (mat == MaterialTier::STONE      ? 350.0f :
                  (mat == MaterialTier::ICE        ? 38.0f :
                  (mat == MaterialTier::TRAMPOLINE ? 9999.0f :
                  (mat == MaterialTier::MAGMA_VENT ? 9999.0f : 35.0f))))); // TNT = 35 HP (volatile!)
        b.hp = b.maxHp;
        b.restitution = (mat == MaterialTier::TRAMPOLINE) ? 1.65f :
                        ((mat == MaterialTier::STONE)     ? 0.10f :
                        (mat == MaterialTier::ICE        ? 0.08f :
                        (mat == MaterialTier::MAGMA_VENT ? 0.15f : 0.16f)));
        b.friction = (mat == MaterialTier::TRAMPOLINE) ? 0.08f :
                     ((mat == MaterialTier::STONE)     ? 0.96f :
                     (mat == MaterialTier::ICE        ? 0.04f :
                     (mat == MaterialTier::MAGMA_VENT ? 0.95f : 0.88f)));
        b.tntFuse = -1.0f;
        b.ignited = false;
        return b;
    }

    void initLevels() {
        levels.clear();
        levels.reserve(NUM_LEVELS);

        auto makeBird = [](BirdClass kind, float x, float y, bool withHelmet = false, bool withShield = false, bool withZeppelin = false) -> BirdTarget {
            BirdTarget t{};
            t.kind = kind;
            t.pos = {x, y};
            t.radius = (kind == BirdClass::CONDOR) ? 28.0f :
                       (kind == BirdClass::HERON ? 26.0f :
                       (kind == BirdClass::FALCON ? 23.0f :
                       (kind == BirdClass::OWL ? 23.0f : 20.0f)));
            t.maxHp = (kind == BirdClass::CONDOR) ? 240.0f :
                      (kind == BirdClass::HERON ? 210.0f :
                      (kind == BirdClass::FALCON ? 150.0f :
                      (kind == BirdClass::OWL ? 140.0f : 78.0f)));
            t.hp = t.maxHp;
            t.mass = (kind == BirdClass::CONDOR) ? 2.8f :
                     (kind == BirdClass::HERON ? 2.4f :
                     (kind == BirdClass::FALCON ? 2.0f :
                     (kind == BirdClass::OWL ? 1.8f : 1.4f)));
            t.invMass = 1.0f / t.mass;
            t.hasHelmet = withHelmet;
            if (withHelmet) {
                t.helmetHp = (kind == BirdClass::CONDOR) ? 160.0f :
                             (kind == BirdClass::HERON ? 140.0f :
                             (kind == BirdClass::FALCON ? 110.0f :
                             (kind == BirdClass::OWL ? 100.0f : 70.0f)));
                t.maxHelmetHp = t.helmetHp;
            }
            t.hasShield = withShield || (kind == BirdClass::FALCON);
            if (t.hasShield) {
                t.shieldHp = 120.0f;
                t.maxShieldHp = 120.0f;
            }
            t.inZeppelin = withZeppelin || (kind == BirdClass::CONDOR);
            if (t.inZeppelin) {
                t.balloonHp = 70.0f;
                t.maxBalloonHp = 70.0f;
            }
            return t;
        };

        auto birdRad = [](BirdClass k) -> float {
            return (k == BirdClass::CONDOR) ? 28.0f :
                   (k == BirdClass::HERON ? 26.0f :
                   (k == BirdClass::FALCON ? 23.0f :
                   (k == BirdClass::OWL ? 23.0f : 20.0f)));
        };

        // Helper to add a rock-solid modular 1-, 2-, or 3-story architectural bay centered at cx
        // Floor 1 top = 508.0, Floor 2 top = 404.0, Floor 3 top = 314.0
        auto addModularBay = [&](LevelSpec& L, float cx, int stories,
                                 MaterialTier m1Pillar, MaterialTier m1Deck,
                                 MaterialTier m2Pillar, MaterialTier m2Deck,
                                 MaterialTier m3Pillar, MaterialTier m3Deck,
                                 int birdFloor1, BirdClass b1Kind,
                                 int birdFloor2, BirdClass b2Kind,
                                 int birdFloor3, BirdClass b3Kind) {
            if (stories >= 1) {
                L.blocks.push_back(makeBlock(m1Pillar, cx - 42.0f, 580.0f, 24.0f, 100.0f));
                L.blocks.push_back(makeBlock(m1Pillar, cx + 42.0f, 580.0f, 24.0f, 100.0f));
                L.blocks.push_back(makeBlock(m1Deck,   cx,         519.0f, 112.0f, 22.0f));
                if (birdFloor1 > 0) {
                    L.birds.push_back(makeBird(b1Kind, cx, GROUND_Y - birdRad(b1Kind)));
                }
            }
            if (stories >= 2) {
                L.blocks.push_back(makeBlock(m2Pillar, cx - 36.0f, 466.0f, 22.0f, 84.0f));
                L.blocks.push_back(makeBlock(m2Pillar, cx + 36.0f, 466.0f, 22.0f, 84.0f));
                L.blocks.push_back(makeBlock(m2Deck,   cx,         414.0f, 102.0f, 20.0f));
                if (birdFloor2 > 0) {
                    L.birds.push_back(makeBird(b2Kind, cx, 508.0f - birdRad(b2Kind)));
                }
            } else if (birdFloor2 > 0) {
                L.birds.push_back(makeBird(b2Kind, cx, 508.0f - birdRad(b2Kind)));
            }
            if (stories >= 3) {
                L.blocks.push_back(makeBlock(m3Pillar, cx - 31.0f, 368.0f, 20.0f, 72.0f));
                L.blocks.push_back(makeBlock(m3Pillar, cx + 31.0f, 368.0f, 20.0f, 72.0f));
                L.blocks.push_back(makeBlock(m3Deck,   cx,         323.0f, 92.0f, 18.0f));
                if (birdFloor3 > 0) {
                    L.birds.push_back(makeBird(b3Kind, cx, 404.0f - birdRad(b3Kind)));
                }
            } else if (stories == 2 && birdFloor3 > 0) {
                L.birds.push_back(makeBird(b3Kind, cx, 404.0f - birdRad(b3Kind)));
            }
        };

        auto addFrontShield = [&](LevelSpec& L, float x, MaterialTier mat, bool tall) {
            if (tall) {
                L.blocks.push_back(makeBlock(mat, x, 570.0f, 28.0f, 120.0f));
            } else {
                L.blocks.push_back(makeBlock(mat, x, 585.0f, 26.0f, 90.0f));
            }
        };

        // Helper to add a massive Megalithic Bay with wide stone foundation plinths and grand beam
        auto addMonolithBay = [&](LevelSpec& L, float cx, int stories,
                                  MaterialTier mPlinth, MaterialTier mPillar, MaterialTier mBeam,
                                  int birdFloor1, BirdClass b1Kind,
                                  int birdFloor2, BirdClass b2Kind,
                                  int birdFloor3, BirdClass b3Kind) {
            // Foundation plinths: w = 44.0, h = 40.0. bottom = 630.0, top = 590.0, pos.y = 610.0
            L.blocks.push_back(makeBlock(mPlinth, cx - 52.0f, 610.0f, 44.0f, 40.0f));
            L.blocks.push_back(makeBlock(mPlinth, cx + 52.0f, 610.0f, 44.0f, 40.0f));
            // Monolith pillars: w = 32.0, h = 90.0. bottom = 590.0, top = 500.0, pos.y = 545.0
            L.blocks.push_back(makeBlock(mPillar, cx - 50.0f, 545.0f, 32.0f, 90.0f));
            L.blocks.push_back(makeBlock(mPillar, cx + 50.0f, 545.0f, 32.0f, 90.0f));
            // Grand monolithic beam: w = 160.0, h = 28.0. bottom = 500.0, top = 472.0, pos.y = 486.0
            L.blocks.push_back(makeBlock(mBeam, cx, 486.0f, 160.0f, 28.0f));
            if (birdFloor1 > 0) {
                L.birds.push_back(makeBird(b1Kind, cx, GROUND_Y - birdRad(b1Kind)));
            }
            if (stories >= 2) {
                // Tier 2 stanchions: w = 26.0, h = 60.0. bottom = 472.0, top = 412.0, pos.y = 442.0
                L.blocks.push_back(makeBlock(mPillar, cx - 42.0f, 442.0f, 26.0f, 60.0f));
                L.blocks.push_back(makeBlock(mPillar, cx + 42.0f, 442.0f, 26.0f, 60.0f));
                // Upper plinth slab: w = 120.0, h = 24.0. bottom = 412.0, top = 388.0, pos.y = 400.0
                L.blocks.push_back(makeBlock(mBeam, cx, 400.0f, 120.0f, 24.0f));
                if (birdFloor2 > 0) {
                    L.birds.push_back(makeBird(b2Kind, cx, 472.0f - birdRad(b2Kind)));
                }
                if (birdFloor3 > 0) {
                    L.birds.push_back(makeBird(b3Kind, cx, 388.0f - birdRad(b3Kind)));
                }
            } else {
                if (birdFloor2 > 0) {
                    L.birds.push_back(makeBird(b2Kind, cx, 472.0f - birdRad(b2Kind)));
                }
            }
        };

        // Helper to add an Eastern Tiered Pagoda with flared eaves and bamboo posts
        auto addPagodaBay = [&](LevelSpec& L, float cx, int stories,
                                MaterialTier mPillar, MaterialTier mEaves,
                                int birdFloor1, BirdClass b1Kind,
                                int birdFloor2, BirdClass b2Kind,
                                int birdFloor3, BirdClass b3Kind) {
            // Tier 1 pillars: w = 20.0, h = 90.0. bottom = 630.0, top = 540.0, pos.y = 585.0
            L.blocks.push_back(makeBlock(mPillar, cx - 46.0f, 585.0f, 20.0f, 90.0f));
            L.blocks.push_back(makeBlock(mPillar, cx + 46.0f, 585.0f, 20.0f, 90.0f));
            // First flared eaves: w = 144.0, h = 18.0. bottom = 540.0, top = 522.0, pos.y = 531.0
            L.blocks.push_back(makeBlock(mEaves, cx, 531.0f, 144.0f, 18.0f));
            if (birdFloor1 > 0) {
                L.birds.push_back(makeBird(b1Kind, cx, GROUND_Y - birdRad(b1Kind)));
            }
            if (stories >= 2) {
                // Tier 2 pillars: w = 18.0, h = 76.0. bottom = 522.0, top = 446.0, pos.y = 484.0
                L.blocks.push_back(makeBlock(mPillar, cx - 40.0f, 484.0f, 18.0f, 76.0f));
                L.blocks.push_back(makeBlock(mPillar, cx + 40.0f, 484.0f, 18.0f, 76.0f));
                // Second flared eaves: w = 120.0, h = 16.0. bottom = 446.0, top = 430.0, pos.y = 438.0
                L.blocks.push_back(makeBlock(mEaves, cx, 438.0f, 120.0f, 16.0f));
                if (birdFloor2 > 0) {
                    L.birds.push_back(makeBird(b2Kind, cx, 522.0f - birdRad(b2Kind)));
                }
            } else if (birdFloor2 > 0) {
                L.birds.push_back(makeBird(b2Kind, cx, 522.0f - birdRad(b2Kind)));
            }
            if (stories >= 3) {
                // Tier 3 pillars: w = 16.0, h = 64.0. bottom = 430.0, top = 366.0, pos.y = 398.0
                L.blocks.push_back(makeBlock(mPillar, cx - 34.0f, 398.0f, 16.0f, 64.0f));
                L.blocks.push_back(makeBlock(mPillar, cx + 34.0f, 398.0f, 16.0f, 64.0f));
                // Top roof cap: w = 100.0, h = 16.0. bottom = 366.0, top = 350.0, pos.y = 358.0
                L.blocks.push_back(makeBlock(mEaves, cx, 358.0f, 100.0f, 16.0f));
                if (birdFloor3 > 0) {
                    L.birds.push_back(makeBird(b3Kind, cx, 430.0f - birdRad(b3Kind)));
                }
            } else if (stories == 2 && birdFloor3 > 0) {
                L.birds.push_back(makeBird(b3Kind, cx, 430.0f - birdRad(b3Kind)));
            }
        };

        // Helper to add a Stepped Pyramid / Ziggurat structure
        auto addPyramidBay = [&](LevelSpec& L, float cx, int tiers,
                                 MaterialTier mBase, MaterialTier mMid, MaterialTier mTop,
                                 BirdClass bTop) {
            // Tier 1: w = 140.0, h = 28.0. bottom = 630.0, top = 602.0, pos.y = 616.0
            L.blocks.push_back(makeBlock(mBase, cx, 616.0f, 140.0f, 28.0f));
            if (tiers >= 2) {
                // Tier 2: w = 100.0, h = 24.0. bottom = 602.0, top = 578.0, pos.y = 590.0
                L.blocks.push_back(makeBlock(mMid, cx, 590.0f, 100.0f, 24.0f));
            }
            if (tiers >= 3) {
                // Tier 3: w = 60.0, h = 22.0. bottom = 578.0, top = 556.0, pos.y = 567.0
                L.blocks.push_back(makeBlock(mTop, cx, 567.0f, 60.0f, 22.0f));
                L.birds.push_back(makeBird(bTop, cx, 556.0f - birdRad(bTop)));
            } else if (tiers == 2) {
                L.birds.push_back(makeBird(bTop, cx, 578.0f - birdRad(bTop)));
            } else {
                L.birds.push_back(makeBird(bTop, cx, 602.0f - birdRad(bTop)));
            }
        };

        // Helper to add a Heavy Bomb-Proof Casemate bunker with thick walls and parapets
        auto addFortifiedCasemate = [&](LevelSpec& L, float cx,
                                        MaterialTier mWall, MaterialTier mRoof, MaterialTier mCrate,
                                        int bInside, BirdClass bIKind,
                                        int bRoof, BirdClass bRKind) {
            // Walls: w = 36.0, h = 100.0. bottom = 630.0, top = 530.0, pos.y = 580.0
            L.blocks.push_back(makeBlock(mWall, cx - 60.0f, 580.0f, 36.0f, 100.0f));
            L.blocks.push_back(makeBlock(mWall, cx + 60.0f, 580.0f, 36.0f, 100.0f));
            // Center crate: w = 42.0, h = 32.0. bottom = 630.0, top = 598.0, pos.y = 614.0
            L.blocks.push_back(makeBlock(mCrate, cx, 614.0f, 42.0f, 32.0f));
            // Heavy roof slab: w = 170.0, h = 26.0. bottom = 530.0, top = 504.0, pos.y = 517.0
            L.blocks.push_back(makeBlock(mRoof, cx, 517.0f, 170.0f, 26.0f));
            // Parapets: w = 20.0, h = 28.0. bottom = 504.0, top = 476.0, pos.y = 490.0
            L.blocks.push_back(makeBlock(mWall, cx - 72.0f, 490.0f, 20.0f, 28.0f));
            L.blocks.push_back(makeBlock(mWall, cx + 72.0f, 490.0f, 20.0f, 28.0f));
            if (bInside > 0) {
                L.birds.push_back(makeBird(bIKind, cx, 598.0f - birdRad(bIKind)));
            }
            if (bRoof > 0) {
                L.birds.push_back(makeBird(bRKind, cx, 504.0f - birdRad(bRKind)));
            }
        };

        // Helper to add an Industrial Gantry structure with crane girders and optional pendulum
        auto addClockworkGantry = [&](LevelSpec& L, float cx,
                                      MaterialTier mPillar, MaterialTier mBeam,
                                      bool withPendulum, BirdClass bRoof) {
            // Stanchions: w = 28.0, h = 110.0. bottom = 630.0, top = 520.0, pos.y = 575.0
            L.blocks.push_back(makeBlock(mPillar, cx - 56.0f, 575.0f, 28.0f, 110.0f));
            L.blocks.push_back(makeBlock(mPillar, cx + 56.0f, 575.0f, 28.0f, 110.0f));
            // Girder deck: w = 170.0, h = 24.0. bottom = 520.0, top = 496.0, pos.y = 508.0
            L.blocks.push_back(makeBlock(mBeam, cx, 508.0f, 170.0f, 24.0f));
            // Upper stanchions: w = 22.0, h = 80.0. bottom = 496.0, top = 416.0, pos.y = 456.0
            L.blocks.push_back(makeBlock(mPillar, cx - 42.0f, 456.0f, 22.0f, 80.0f));
            L.blocks.push_back(makeBlock(mPillar, cx + 42.0f, 456.0f, 22.0f, 80.0f));
            // Top crane beam: w = 130.0, h = 20.0. bottom = 416.0, top = 396.0, pos.y = 406.0
            L.blocks.push_back(makeBlock(mBeam, cx, 406.0f, 130.0f, 20.0f));
            L.birds.push_back(makeBird(bRoof, cx, 396.0f - birdRad(bRoof)));
            if (withPendulum) {
                PendulumBody pb{};
                pb.anchor = {cx, 406.0f};
                pb.length = 150.0f;
                pb.radius = 26.0f;
                pb.mass = 10.0f;
                pb.spiked = true;
                L.pendulums.push_back(pb);
            }
        };

        // ====================================================================
        // CHAPTER I (LEVELS 1-10): DAWN MEADOW & THE WICKER OUTPOSTS
        // Levels 1-5: STANDARD PIG (TROTTER) ONLY
        // Levels 6-10: BRAMBLE (TURBO BOAR) UNLOCKED AT LEVEL 6
        // ====================================================================
        // LEVEL 1: Wicker Watchtower
        {
            LevelSpec L;
            L.name = "LEVEL 1: WICKER WATCHTOWER";
            L.subtitle = "Pull the catapult & launch the Trotter! Press SPACE mid-air for Anvil Slam!";
            L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
            addFrontShield(L, 710.0f, MaterialTier::TWIG, false);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 780, 580, 24, 100));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 900, 580, 24, 100));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 840, 519, 160, 22));
            L.blocks.push_back(makeBlock(MaterialTier::TWIG,   800, 466, 22, 84));
            L.blocks.push_back(makeBlock(MaterialTier::TWIG,   880, 466, 22, 84));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 840, 414, 130, 20));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 840, 610));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 840, 488));
            levels.push_back(L);
        }

        // LEVEL 2: Timber Bastion
        {
            LevelSpec L;
            L.name = "LEVEL 2: TIMBER BASTION";
            L.subtitle = "Standard pigs ready! Aim for the central timber pillar to trigger a collapse!";
            L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
            addFrontShield(L, 665.0f, MaterialTier::TIMBER, false);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 730, 575, 26, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850, 575, 26, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 970, 575, 26, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850, 509, 240, 22));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 790, 457, 22, 82));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 910, 457, 22, 82));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850, 406, 160, 20));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 790, 610));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 910, 610));
            L.birds.push_back(makeBird(BirdClass::OWL,    850, 475));
            levels.push_back(L);
        }

        // LEVEL 3: Twin Timber Watchtowers (REBALANCED FOR STANDARD PIGS)
        {
            LevelSpec L;
            L.name = "LEVEL 3: TWIN TIMBER WATCHTOWERS";
            L.subtitle = "Strike the outer timber supports to topple both towers into the bridge!";
            L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 715, 575, 24, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 795, 575, 24, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 755, 509, 116, 22));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 935, 575, 24, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 1015, 575, 24, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 975, 509, 116, 22));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 865, 487, 140, 22));
            L.blocks.push_back(makeBlock(MaterialTier::TWIG,   835, 434, 20, 84));
            L.blocks.push_back(makeBlock(MaterialTier::TWIG,   895, 434, 20, 84));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 865, 382, 100, 20));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 755, 610));
            L.birds.push_back(makeBird(BirdClass::OWL,    975, 607));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 865, 610));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 865, 456));
            levels.push_back(L);
        }

        // LEVEL 4: Skylight Bunker
        {
            LevelSpec L;
            L.name = "LEVEL 4: SKYLIGHT BUNKER";
            L.subtitle = "High timber bunker! Lob a high shot & use Trotter's Anvil Slam to crush the roof!";
            L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 690, 565, 36, 130));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 690, 435, 36, 130));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 800, 575, 26, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 950, 575, 26, 110));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 1040, 565, 34, 130));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 875, 509, 180, 22));
            L.blocks.push_back(makeBlock(MaterialTier::TWIG,   820, 456, 22, 84));
            L.blocks.push_back(makeBlock(MaterialTier::TWIG,   930, 456, 22, 84));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 875, 404, 150, 20));
            L.birds.push_back(makeBird(BirdClass::OWL,    748, 607));
            L.birds.push_back(makeBird(BirdClass::HERON,  875, 604));
            L.birds.push_back(makeBird(BirdClass::OWL,    875, 475));
            levels.push_back(L);
        }

        // LEVEL 5: Bamboo Colonnade
        {
            LevelSpec L;
            L.name = "LEVEL 5: BAMBOO COLONNADE";
            L.subtitle = "Chapter 1 Milestone! Demolish the colonnade to unlock Bramble the Turbo Boar!";
            L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
            addModularBay(L, 715, 2, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 2, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 995, 2, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 6: Stepped Meadow Gazebo (BRAMBLE UNLOCKED!)
        {
            LevelSpec L;
            L.name = "LEVEL 6: STEPPED MEADOW GAZEBO";
            L.subtitle = "[UNLOCKED] BRAMBLE! Press SPACE mid-air for supersonic dash through wood!";
            L.pigQueue = {PigType::TURBO, PigType::TURBO, PigType::TROTTER, PigType::TROTTER};
            addModularBay(L, 735, 1, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 875, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1015, 1, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 7: The Powder Viaduct
        {
            LevelSpec L;
            L.name = "LEVEL 7: THE POWDER VIADUCT";
            L.subtitle = "Target the central volatile TNT keg between the towers to trigger a massive collapse!";
            L.pigQueue = {PigType::TURBO, PigType::TROTTER, PigType::TURBO, PigType::TROTTER};
            addFrontShield(L, 660.0f, MaterialTier::TWIG, false);
            addModularBay(L, 750, 2, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 930, 2, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 840.0f, 616.0f, 28.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 8: The Bouncing Keep
        {
            LevelSpec L;
            L.name = "LEVEL 8: THE BOUNCING KEEP";
            L.subtitle = "Bounce off the woven ground trampoline to catapult pigs over the forward barricade!";
            L.pigQueue = {PigType::TURBO, PigType::TROTTER, PigType::TURBO, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 590.0f, 618.0f, 100.0f, 24.0f));
            addFrontShield(L, 680.0f, MaterialTier::TIMBER, true);
            addModularBay(L, 770, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 950, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 9: Meadow Redoubt
        {
            LevelSpec L;
            L.name = "LEVEL 9: MEADOW REDOUBT";
            L.subtitle = "Charge through the timber barricades to reach the aviary!";
            L.pigQueue = {PigType::TURBO, PigType::TURBO, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
            addFrontShield(L, 620.0f, MaterialTier::TIMBER, true);
            addModularBay(L, 715, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 995, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 10: Grand Aviary Citadel (Chapter 1 Boss)
        {
            LevelSpec L;
            L.name = "LEVEL 10: GRAND AVIARY CITADEL";
            L.subtitle = "Chapter 1 Finale! Defeat Archduke Heron to unlock Clover the Tri-Piglet!";
            L.pigQueue = {PigType::TURBO, PigType::TURBO, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
            addFrontShield(L, 625.0f, MaterialTier::TIMBER, true);
            addModularBay(L, 730, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 865, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL,    1, BirdClass::TOUCAN);
            addModularBay(L, 1000, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER II (LEVELS 11-20): AUTUMN WOODLAND PALISADES
        // Levels 11-20: CLOVER (TRI-PIGLET) UNLOCKED AT LEVEL 11
        // Available: TROTTER, TURBO, SPLIT (NO BOMB)
        // ====================================================================
        // LEVEL 11: Reed Colonnade (CLOVER UNLOCKED!)
        {
            LevelSpec L;
            L.name = "LEVEL 11: REED COLONNADE";
            L.subtitle = "[UNLOCKED] CLOVER! Press SPACE mid-air to split into a 3-way squadron!";
            L.pigQueue = {PigType::SPLIT, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
            addFrontShield(L, 635.0f, MaterialTier::TIMBER, false);
            addModularBay(L, 720, 2, MaterialTier::TWIG,   MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 990, 2, MaterialTier::TWIG,   MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 12: Cauldron Barracks
        {
            LevelSpec L;
            L.name = "LEVEL 12: CAULDRON BARRACKS";
            L.subtitle = "Beware iron-kettle helmets! Avian sentries withstand heavy hits before their armor shatters!";
            L.pigQueue = {PigType::TROTTER, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addModularBay(L, 715, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::OWL, 1, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::OWL, 855, 508.0f - birdRad(BirdClass::OWL), true));
            addModularBay(L, 995, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 995, GROUND_Y - birdRad(BirdClass::TOUCAN), true));
            levels.push_back(L);
        }

        // LEVEL 13: Walnut Stockade
        {
            LevelSpec L;
            L.name = "LEVEL 13: WALNUT STOCKADE";
            L.subtitle = "Dense walnut beams resist blunt impacts. Use Bramble's supersonic dash!";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
            addFrontShield(L, 645.0f, MaterialTier::TIMBER, true);
            addModularBay(L, 745, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 915, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 14: Crossbeam Stockade
        {
            LevelSpec L;
            L.name = "LEVEL 14: CROSSBEAM STOCKADE";
            L.subtitle = "Gusting crosswinds resist your siege! Adjust trajectory to compensate for headwind.";
            L.pigQueue = {PigType::SPLIT, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            L.wind = -65.0f;
            addFrontShield(L, 620.0f, MaterialTier::TIMBER, true);
            addModularBay(L, 710, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 845, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 985, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 15: Double Gable Powder Keep
        {
            LevelSpec L;
            L.name = "LEVEL 15: DOUBLE GABLE POWDER KEEP";
            L.subtitle = "Detonate the central TNT cache between the keeps to trigger a dual collapse!";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
            addFrontShield(L, 675.0f, MaterialTier::TIMBER, false);
            addModularBay(L, 765, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 935, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 850.0f, 616.0f, 28.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 16: Canopy Skyway
        {
            LevelSpec L;
            L.name = "LEVEL 16: CANOPY SKYWAY";
            L.subtitle = "A long suspended timber platform guarded by Commander Falcon's directional tower shield!";
            L.pigQueue = {PigType::DRILL, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
            addModularBay(L, 700, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 840, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 980, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 17: Foxfire Barricade
        {
            LevelSpec L;
            L.name = "LEVEL 17: FOXFIRE BARRICADE";
            L.subtitle = "Front timber baffles shelter nested owls. Strike the base with Anvil Slam!";
            L.pigQueue = {PigType::TROTTER, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
            addFrontShield(L, 640.0f, MaterialTier::TIMBER, true);
            addFrontShield(L, 675.0f, MaterialTier::TIMBER, false);
            addModularBay(L, 775, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 935, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 18: Pendulum Citadel
        {
            LevelSpec L;
            L.name = "LEVEL 18: PENDULUM CITADEL";
            L.subtitle = "Strike the massive iron wrecking ball to swing it through the central timber gallery!";
            L.pigQueue = {PigType::SPLIT, PigType::TURBO, PigType::TROTTER, PigType::SPLIT, PigType::TROTTER};
            addModularBay(L, 720, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1000, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 860, 580, 26, 100));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 860, 519, 100, 22));
            L.birds.push_back(makeBird(BirdClass::HERON, 860, 508.0f - birdRad(BirdClass::HERON)));
            PendulumBody pb{};
            pb.anchor = {860.0f, 260.0f};
            pb.length = 160.0f;
            pb.angle = 0.0f;
            pb.angVel = 0.0f;
            pb.radius = 26.0f;
            pb.mass = 8.0f;
            pb.spiked = false;
            L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 19: Warden's Grove
        {
            LevelSpec L;
            L.name = "LEVEL 19: WARDEN'S GROVE";
            L.subtitle = "Three reinforced keeps spanning the grove under heavy guard.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
            addFrontShield(L, 625.0f, MaterialTier::TIMBER, true);
            addModularBay(L, 715, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 995, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 20: Palisade Stronghold (Chapter 2 Boss)
        {
            LevelSpec L;
            L.name = "LEVEL 20: PALISADE STRONGHOLD";
            L.subtitle = "Chapter 2 Finale! Defeat Archduke Heron to unlock Vulcan the Seismic Bomb Hog!";
            L.pigQueue = {PigType::SPLIT, PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
            addFrontShield(L, 630.0f, MaterialTier::TIMBER, true);
            addModularBay(L, 720, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 860, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL,    1, BirdClass::TOUCAN);
            addModularBay(L, 1000, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER III (LEVELS 21-30): BASALT CANYON & RED MESA VIADUCTS
        // Levels 21-50: VULCAN (SEISMIC BOMB) UNLOCKED AT LEVEL 21
        // Available: ALL 4 PIG CLASSES (TROTTER, TURBO, SPLIT, BOMB)
        // ====================================================================
        // LEVEL 21: Canyon Viaduct (VULCAN UNLOCKED!)
        {
            LevelSpec L;
            L.name = "LEVEL 21: CANYON VIADUCT";
            L.subtitle = "[UNLOCKED] VULCAN! Press SPACE or CLICK to detonate seismic blast through basalt!";
            L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO};
            addFrontShield(L, 670.0f, MaterialTier::STONE, true);
            addModularBay(L, 765, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 935, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 22: Tri-Pillar Sanctum
        {
            LevelSpec L;
            L.name = "LEVEL 22: TRI-PILLAR SANCTUM";
            L.subtitle = "Fierce canyon tailwinds propel airborne swine with tremendous velocity!";
            L.pigQueue = {PigType::BOMB, PigType::SPLIT, PigType::TROTTER};
            L.wind = 110.0f;
            addModularBay(L, 705, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 850, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 995, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 23: Ironwood Abbey
        {
            LevelSpec L;
            L.name = "LEVEL 23: IRONWOOD ABBEY";
            L.subtitle = "Twin 3-story bell towers flank the central stone sanctuary!";
            L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TROTTER, PigType::TURBO};
            addFrontShield(L, 625.0f, MaterialTier::STONE, false);
            addModularBay(L, 715, 3, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 860, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1005, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 1, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 24: Basalt Powder Bunkers
        {
            LevelSpec L;
            L.name = "LEVEL 24: BASALT POWDER BUNKERS";
            L.subtitle = "Target the volatile TNT cache at the base of the central basalt wall!";
            L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::BOMB, PigType::TURBO};
            addFrontShield(L, 635.0f, MaterialTier::STONE, true);
            addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addFrontShield(L, 855.0f, MaterialTier::STONE, true);
            addModularBay(L, 985, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 616.0f, 28.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 25: Slate Casemate
        {
            LevelSpec L;
            L.name = "LEVEL 25: SLATE CASEMATE";
            L.subtitle = "Blast through the outer stone casemates or drop an Anvil Slam from above!";
            L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
            addFrontShield(L, 630.0f, MaterialTier::STONE, true);
            addModularBay(L, 720, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 860, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1000, 2, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 26: The Trampoline Redoubt
        {
            LevelSpec L;
            L.name = "LEVEL 26: THE TRAMPOLINE REDOUBT";
            L.subtitle = "Twin elastic trampolines! Bank supersonic ricochets into the inner basalt towers!";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 480.0f, 618.0f, 80.0f, 24.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 575.0f, 618.0f, 80.0f, 24.0f));
            addModularBay(L, 680, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 840, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 980, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 27: Pillar of Basalt
        {
            LevelSpec L;
            L.name = "LEVEL 27: PILLAR OF BASALT";
            L.subtitle = "A soaring 3-story basalt column flanked by twin support keeps.";
            L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TROTTER};
            addModularBay(L, 710, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 1000, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 28: Cavern Gate
        {
            LevelSpec L;
            L.name = "LEVEL 28: CAVERN GATE";
            L.subtitle = "Heavy front blast walls defend the cavern. Unleash Atticus the Graviton Boar to collapse their defenses!";
            L.pigQueue = {PigType::GRAVITON, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addFrontShield(L, 620.0f, MaterialTier::STONE, true);
            addFrontShield(L, 655.0f, MaterialTier::STONE, false);
            addModularBay(L, 750, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 910, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 29: Gorge Bulwark
        {
            LevelSpec L;
            L.name = "LEVEL 29: GORGE BULWARK";
            L.subtitle = "Three interlocked stone keeps defending the canyon pass.";
            L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::BOMB};
            addFrontShield(L, 615.0f, MaterialTier::STONE, true);
            addModularBay(L, 705, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 845, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 985, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 30: The High Canyon Perch (Chapter 3 Boss)
        {
            LevelSpec L;
            L.name = "LEVEL 30: THE HIGH CANYON PERCH";
            L.subtitle = "A stepped pyramid fortress rising to Archduke Heron's high throne!";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::SPLIT};
            addFrontShield(L, 600.0f, MaterialTier::STONE, false);
            addModularBay(L, 685, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 815, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::OWL,    0, BirdClass::TOUCAN);
            addModularBay(L, 945, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL,    1, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER IV (LEVELS 31-40): FROSTPEAK AURORA HIGHLANDS
        // ====================================================================
        // LEVEL 31: Glacial Ziggurat
        {
            LevelSpec L;
            L.name = "LEVEL 31: GLACIAL ZIGGURAT";
            L.subtitle = "Chapter 4! Brittle Ice columns shatter instantly into crystalline shards!";
            L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addModularBay(L, 660, 2, MaterialTier::ICE,    MaterialTier::ICE,    MaterialTier::TWIG,   MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 790, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 920, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1050, 2, MaterialTier::ICE,   MaterialTier::ICE,    MaterialTier::TWIG,   MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 32: Twin Glacial Bunkers
        {
            LevelSpec L;
            L.name = "LEVEL 32: TWIN GLACIAL BUNKERS";
            L.subtitle = "Two heavily armored stone bunkers! Deploy Rusty the Drill Hog to bore straight through the granite!";
            L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TROTTER, PigType::TURBO};
            addFrontShield(L, 635.0f, MaterialTier::STONE, true);
            addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addFrontShield(L, 855.0f, MaterialTier::STONE, true);
            addModularBay(L, 985, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 33: The Monolith Gate
        {
            LevelSpec L;
            L.name = "LEVEL 33: THE MONOLITH GATE";
            L.subtitle = "A towering 3-story front stone gatehouse shields the inner court!";
            L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
            addModularBay(L, 720, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 865, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1005, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,  MaterialTier::TWIG,   MaterialTier::TWIG, 1, BirdClass::OWL,   0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 34: Crystal Conservatory
        {
            LevelSpec L;
            L.name = "LEVEL 34: CRYSTAL CONSERVATORY";
            L.subtitle = "Shatter the brittle ice conservatory spires to topple the royal roost!";
            L.pigQueue = {PigType::SPLIT, PigType::TROTTER, PigType::BOMB, PigType::TURBO};
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 665, 565, 36, 130));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 665, 435, 36, 130));
            addModularBay(L, 780, 3, MaterialTier::ICE,    MaterialTier::ICE,    MaterialTier::ICE,    MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 915, 3, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1045, 2, MaterialTier::ICE,   MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 35: Alpine Stockade
        {
            LevelSpec L;
            L.name = "LEVEL 35: BLIZZARD STOCKADE";
            L.subtitle = "Howling alpine blizzard winds drag artillery shots backward!";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::BOMB};
            L.wind = -140.0f;
            addFrontShield(L, 625.0f, MaterialTier::STONE, true);
            addModularBay(L, 715, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 860, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1005, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 36: Aurora Pagoda
        {
            LevelSpec L;
            L.name = "LEVEL 36: AURORA PAGODA";
            L.subtitle = "Soaring 3-story pagoda beneath shimmering northern auroras.";
            L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addModularBay(L, 710, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 850, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 990, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 37: Glacier Redoubt
        {
            LevelSpec L;
            L.name = "LEVEL 37: GLACIER REDOUBT";
            L.subtitle = "Four reinforced bays alternating heavy stone and seasoned timber.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
            addFrontShield(L, 600.0f, MaterialTier::STONE, true);
            addModularBay(L, 680, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 810, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 940, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1070, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 38: Frostfire Spire
        {
            LevelSpec L;
            L.name = "LEVEL 38: FROSTFIRE SPIRE";
            L.subtitle = "Target the combustible TNT depot beside the central glacial spire!";
            L.pigQueue = {PigType::TROTTER, PigType::BOMB, PigType::TURBO, PigType::SPLIT};
            addFrontShield(L, 630.0f, MaterialTier::STONE, false);
            addModularBay(L, 730, 2, MaterialTier::ICE,   MaterialTier::ICE,    MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 870, 3, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 1010, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 800.0f, 618.0f, 24.0f, 24.0f));
            levels.push_back(L);
        }

        // LEVEL 39: Thunder Peak Sanctuary
        {
            LevelSpec L;
            L.name = "LEVEL 39: THUNDER PEAK SANCTUARY";
            L.subtitle = "Electrified cumulonimbus storm clouds unleash lightning bolts upon metal & stone!";
            L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.hasStorm = true;
            L.wind = -50.0f;
            addFrontShield(L, 635.0f, MaterialTier::STONE, true);
            addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addFrontShield(L, 855.0f, MaterialTier::STONE, true);
            addModularBay(L, 985, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 40: Bastion of the Winds (Chapter 4 Boss)
        {
            LevelSpec L;
            L.name = "LEVEL 40: BASTION OF THE WINDS";
            L.subtitle = "Four reinforced bays crowned by twin aerial watchtowers!";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::SPLIT};
            addModularBay(L, 660, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 790, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 920, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1050, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER V (LEVELS 41-50): THE OBSIDIAN IMPERIAL CITADEL
        // ====================================================================
        // LEVEL 41: Obsidian Watch
        {
            LevelSpec L;
            L.name = "LEVEL 41: OBSIDIAN WATCH";
            L.subtitle = "Three carved slate vaults supporting an upper timber command deck!";
            L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
            addFrontShield(L, 625.0f, MaterialTier::STONE, true);
            addModularBay(L, 715, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 995, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 42: Pendulum of the Obsidian Emperor
        {
            LevelSpec L;
            L.name = "LEVEL 42: PENDULUM OF THE OBSIDIAN EMPEROR";
            L.subtitle = "A massive spiked iron wrecking ball guards cauldron-helmed Archduke Herons!";
            L.pigQueue = {PigType::BOMB, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
            addModularBay(L, 690, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1020, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            // Grand central imperial throne room with swinging spiked wrecking ball
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 855, 580, 32, 100));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 855, 519, 110, 22));
            L.birds.push_back(makeBird(BirdClass::HERON, 855, 508.0f - birdRad(BirdClass::HERON), true));
            L.birds.push_back(makeBird(BirdClass::HERON, 780, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{};
            pb.anchor = {855.0f, 180.0f};
            pb.length = 150.0f;
            pb.angle = 0.0f;
            pb.angVel = 0.0f;
            pb.radius = 28.0f;
            pb.mass = 12.0f;
            pb.spiked = true;
            L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 43: The Heron's Vault
        {
            LevelSpec L;
            L.name = "LEVEL 43: THE HERON'S VAULT";
            L.subtitle = "Three Archduke Herons fortified behind a double-height slate monolith!";
            L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::BOMB, PigType::SPLIT};
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 660, 565, 36, 130));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 660, 435, 36, 130));
            addModularBay(L, 775, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 910, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1045, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 44: Volcanic Casemate
        {
            LevelSpec L;
            L.name = "LEVEL 44: VOLCANIC CASEMATE";
            L.subtitle = "Geothermal magma vents erupt fiery embers! Detonate adjacent TNT and melt ice defenses.";
            L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addFrontShield(L, 630.0f, MaterialTier::STONE, true);
            addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 885, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 805.0f, 616.0f, 40.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 45: Magma Bridge
        {
            LevelSpec L;
            L.name = "LEVEL 45: MAGMA BRIDGE";
            L.subtitle = "Elevated stone viaduct guarded by Baron Von Condor's airship and Commander Falcon's shield!";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
            addModularBay(L, 700, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 840, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            addModularBay(L, 980, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 46: Skybridge Spire
        {
            LevelSpec L;
            L.name = "LEVEL 46: SKYBRIDGE SPIRE";
            L.subtitle = "Four 3-story towers defending the approaches to the Imperial Eyrie!";
            L.pigQueue = {PigType::BOMB, PigType::SPLIT, PigType::TROTTER, PigType::TURBO, PigType::BOMB};
            addModularBay(L, 660, 3, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 790, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 920, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1050, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 47: Council of Archdukes
        {
            LevelSpec L;
            L.name = "LEVEL 47: COUNCIL OF ARCHDUKES";
            L.subtitle = "Three heavily fortified Heron thrones in reinforced slate vaults.";
            L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::SPLIT};
            addFrontShield(L, 615.0f, MaterialTier::STONE, true);
            addModularBay(L, 710, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 855, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            addModularBay(L, 1000, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 48: Obsidian Bastion Prime
        {
            LevelSpec L;
            L.name = "LEVEL 48: OBSIDIAN BASTION PRIME";
            L.subtitle = "Twin TNT kegs flank the central basalt gate! Detonate for a dual shockwave!";
            L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::BOMB};
            addFrontShield(L, 620.0f, MaterialTier::STONE, true);
            addModularBay(L, 725, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            addFrontShield(L, 855.0f, MaterialTier::STONE, true);
            addModularBay(L, 985, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 616.0f, 28.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 900.0f, 616.0f, 28.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 49: The Imperial Arena
        {
            LevelSpec L;
            L.name = "LEVEL 49: THE IMPERIAL ARENA";
            L.subtitle = "Four soaring stone and timber colosseum bays beneath the eclipse!";
            L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::BOMB};
            addModularBay(L, 650, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 780, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 910, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1040, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 50: The Emperor's Final High Eyrie (Ultimate Campaign Finale)
        {
            LevelSpec L;
            L.name = "LEVEL 50: EMPEROR'S FINAL HIGH EYRIE";
            L.subtitle = "The final siege! Storm clouds, lightning, crosswinds, magma vents, and the elite Avian Court guard Emperor Heron!";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -75.0f;
            L.hasStorm = true;
            addFrontShield(L, 580.0f, MaterialTier::STONE, true);
            addModularBay(L, 665, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 795, 3, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::HERON,  1, BirdClass::TOUCAN);
            addModularBay(L, 925, 3, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::HERON,  0, BirdClass::TOUCAN);
            addModularBay(L, 1055, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  0, BirdClass::OWL,    0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 515.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

// ====================================================================
        // CHAPTER VI (LEVELS 51-60): THE BAMBOO STEPPES & GILDED PAGODAS
        // ====================================================================
        // LEVEL 51: PAGODA OF THE WHISPERING REED
        {
            LevelSpec L;
            L.name = "LEVEL 51: PAGODA OF THE WHISPERING REED";
            L.subtitle = "Tiered bamboo pagoda with flared timber eaves swaying in the steppe breeze.";
            L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            L.wind = 35.0f;
            addPagodaBay(L, 850.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 690.0f, 580.0f, 22.0f, 100.0f));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 690.0f, 530.0f - birdRad(BirdClass::TOUCAN)));
            levels.push_back(L);
        }

        // LEVEL 52: BAMBOO GATEHOUSE
        {
            LevelSpec L;
            L.name = "LEVEL 52: BAMBOO GATEHOUSE";
            L.subtitle = "Twin bamboo gate towers flank an elevated timber watchway.";
            L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
            addPagodaBay(L, 740.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addPagodaBay(L, 960.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850.0f, 512.0f, 120.0f, 20.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 502.0f - birdRad(BirdClass::HERON)));
            levels.push_back(L);
        }

        // LEVEL 53: JADE TERRACE
        {
            LevelSpec L;
            L.name = "LEVEL 53: JADE TERRACE";
            L.subtitle = "A heavy stone plinth supports a bamboo shrine guarding the golden terrace.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::GLIDER, PigType::TROTTER};
            L.wind = -25.0f;
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 605.0f, 90.0f, 50.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 1020.0f, 605.0f, 90.0f, 50.0f));
            addPagodaBay(L, 860.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            L.birds.back().hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 54: STEPPE WINDBREAK
        {
            LevelSpec L;
            L.name = "LEVEL 54: STEPPE WINDBREAK";
            L.subtitle = "Violent steppe tailwinds propel siege hogs toward fortified bamboo bastions.";
            L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::DRILL, PigType::BOMB};
            L.wind = 65.0f;
            addFrontShield(L, 620.0f, MaterialTier::TWIG, true);
            addPagodaBay(L, 750.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addPagodaBay(L, 950.0f, 2, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 850.0f, 617.0f, 26.0f, 26.0f));
            levels.push_back(L);
        }

        // LEVEL 55: TWIN PAVILIONS OF THE LOTUS
        {
            LevelSpec L;
            L.name = "LEVEL 55: TWIN PAVILIONS OF THE LOTUS";
            L.subtitle = "Symmetrical pavilions built over reflection pools with trampoline banks.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 70.0f, 24.0f));
            addPagodaBay(L, 720.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addPagodaBay(L, 980.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850.0f, 610.0f, 40.0f, 40.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 590.0f - birdRad(BirdClass::HERON), true));
            levels.push_back(L);
        }

        // LEVEL 56: CRANE'S ROOST
        {
            LevelSpec L;
            L.name = "LEVEL 56: CRANE'S ROOST";
            L.subtitle = "Slender high-altitude bamboo stalks hoist shielded Falcon archers into the sky.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
            L.wind = -45.0f;
            addPagodaBay(L, 860.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::FALCON, 1, BirdClass::FALCON);
            L.birds[1].hasShield = true;
            L.birds[2].hasShield = true;
            addFrontShield(L, 660.0f, MaterialTier::STONE, false);
            levels.push_back(L);
        }

        // LEVEL 57: FLOATING LANTERN SHRINE
        {
            LevelSpec L;
            L.name = "LEVEL 57: FLOATING LANTERN SHRINE";
            L.subtitle = "A suspended shrine flanked by volatile lantern kegs above trampoline waters.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::DRILL, PigType::SPLIT};
            L.wind = 20.0f;
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 620.0f, 618.0f, 74.0f, 24.0f));
            addPagodaBay(L, 820.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 930.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 1020.0f, 580.0f, 24.0f, 100.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 1020.0f, 530.0f - birdRad(BirdClass::HERON)));
            levels.push_back(L);
        }

        // LEVEL 58: MONASTERY OF THE NORTH PEAK
        {
            LevelSpec L;
            L.name = "LEVEL 58: MONASTERY OF THE NORTH PEAK";
            L.subtitle = "Double stone casemates anchor high bamboo spires overlooking the misty pass.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
            L.wind = -50.0f;
            addFortifiedCasemate(L, 720.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addPagodaBay(L, 940.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.birds.back().hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 59: GILDED PAGODA CITADEL
        {
            LevelSpec L;
            L.name = "LEVEL 59: GILDED PAGODA CITADEL";
            L.subtitle = "Three-tiered grand pagoda complex with wide roof overhangs and explosive munitions.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -30.0f;
            addPagodaBay(L, 700.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addPagodaBay(L, 880.0f, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::FALCON);
            addPagodaBay(L, 1060.0f, 2, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 790.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 970.0f, 617.0f, 26.0f, 26.0f));
            levels.push_back(L);
        }

        // LEVEL 60: PALACE OF THE GOLDEN HERON
        {
            LevelSpec L;
            L.name = "LEVEL 60: PALACE OF THE GOLDEN HERON";
            L.subtitle = "Chapter Boss! The Grand Pagoda Palace defended by Archduke Heron and elite avian guards.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -60.0f;
            L.hasStorm = true;
            addFrontShield(L, 570.0f, MaterialTier::STONE, true);
            addPagodaBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addPagodaBay(L, 860.0f, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::OWL);
            addPagodaBay(L, 1040.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::CONDOR, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER VII (LEVELS 61-70): SUNKEN GROTTO & CORAL AQUEDUCTS
        // ====================================================================
        // LEVEL 61: SUNKEN AQUEDUCT VIADUCT
        {
            LevelSpec L;
            L.name = "LEVEL 61: SUNKEN AQUEDUCT VIADUCT";
            L.subtitle = "Multi-span stone arch viaduct with water-channel decking over deep grotto cisterns.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::TROTTER};
            addMonolithBay(L, 750.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 970.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::ICE, 860.0f, 615.0f, 30.0f, 30.0f));
            levels.push_back(L);
        }

        // LEVEL 62: LIMESTONE STALAGMITE TOWER
        {
            LevelSpec L;
            L.name = "LEVEL 62: LIMESTONE STALAGMITE TOWER";
            L.subtitle = "Slender limestone columns crowned by frost-glazed bridge spans.";
            L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 720.0f, 570.0f, 20.0f, 120.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 570.0f, 20.0f, 120.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 1000.0f, 570.0f, 20.0f, 120.0f));
            L.blocks.push_back(makeBlock(MaterialTier::ICE, 790.0f, 500.0f, 140.0f, 20.0f));
            L.blocks.push_back(makeBlock(MaterialTier::ICE, 930.0f, 500.0f, 140.0f, 20.0f));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 790.0f, 490.0f - birdRad(BirdClass::TOUCAN)));
            L.birds.push_back(makeBird(BirdClass::OWL, 930.0f, 490.0f - birdRad(BirdClass::OWL)));
            levels.push_back(L);
        }

        // LEVEL 63: THE CORAL CISTERN
        {
            LevelSpec L;
            L.name = "LEVEL 63: THE CORAL CISTERN";
            L.subtitle = "Massive megaliths flanked by subterranean trampoline deflectors.";
            L.pigQueue = {PigType::TURBO, PigType::GLIDER, PigType::SPLIT, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 560.0f, 618.0f, 74.0f, 24.0f));
            addMonolithBay(L, 780.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 990.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 64: SUBTERRANEAN POWDER VAULT
        {
            LevelSpec L;
            L.name = "LEVEL 64: SUBTERRANEAN POWDER VAULT";
            L.subtitle = "Underground powder magazine where explosive kegs support stone architraves.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 700.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 1020.0f, 617.0f, 26.0f, 26.0f));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 700.0f, 604.0f - birdRad(BirdClass::TOUCAN)));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 1020.0f, 604.0f - birdRad(BirdClass::TOUCAN)));
            levels.push_back(L);
        }

        // LEVEL 65: CAVERN OF THE ECHOING DROP
        {
            LevelSpec L;
            L.name = "LEVEL 65: CAVERN OF THE ECHOING DROP";
            L.subtitle = "A heavy spiked iron pendulum hangs suspended in the cavern gloom.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addMonolithBay(L, 680.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {850.0f, 160.0f}; pb.length = 180.0f; pb.radius = 26.0f; pb.mass = 10.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 66: TWIN CORAL PILLARS
        {
            LevelSpec L;
            L.name = "LEVEL 66: TWIN CORAL PILLARS";
            L.subtitle = "Symmetrical megalith piers with ice bridges defended by Falcon sentries.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
            addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            L.birds[1].hasShield = true;
            L.birds[3].hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 67: DROWNED AMPHITHEATER
        {
            LevelSpec L;
            L.name = "LEVEL 67: DROWNED AMPHITHEATER";
            L.subtitle = "Stepped stone tiers descend toward sunken trampoline nets.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
            addPyramidBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 620.0f, 618.0f, 74.0f, 24.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 1100.0f, 618.0f, 74.0f, 24.0f));
            levels.push_back(L);
        }

        // LEVEL 68: MAGMA GROTTO FISSURE
        {
            LevelSpec L;
            L.name = "LEVEL 68: MAGMA GROTTO FISSURE";
            L.subtitle = "Underground magma vents spew molten rocks beneath stone viaduct pillars.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 600.0f, 616.0f, 36.0f, 28.0f));
            addModularBay(L, 760.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 69: GROTTO SOVEREIGN SANCTUM
        {
            LevelSpec L;
            L.name = "LEVEL 69: GROTTO SOVEREIGN SANCTUM";
            L.subtitle = "Reinforced stone casemate flanked by crystalline ice towers.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::OWL);
            addModularBay(L, 660.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1060.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 70: ABYSSAL GATE OF EMPEROR HERON
        {
            LevelSpec L;
            L.name = "LEVEL 70: ABYSSAL GATE OF EMPEROR HERON";
            L.subtitle = "Chapter Boss! Colossal double-aqueduct bastion defending the entrance to the deep abyss.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -35.0f;
            addMonolithBay(L, 680.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::CONDOR);
            addMonolithBay(L, 1080.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER VIII (LEVELS 71-80): IRON FOUNDRY & CLOCKWORK GANTRIES
        // ====================================================================
        // LEVEL 71: FOUNDRY GANTRY LINE
        {
            LevelSpec L;
            L.name = "LEVEL 71: FOUNDRY GANTRY LINE";
            L.subtitle = "Industrial crane girder spans across heavy timber stanchions.";
            L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
            addClockworkGantry(L, 850.0f, MaterialTier::TIMBER, MaterialTier::TIMBER, false, BirdClass::OWL);
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 850.0f, GROUND_Y - birdRad(BirdClass::TOUCAN)));
            levels.push_back(L);
        }

        // LEVEL 72: THE SLAG FURNACES
        {
            LevelSpec L;
            L.name = "LEVEL 72: THE SLAG FURNACES";
            L.subtitle = "Active magma forge vents flank reinforced timber crate stacks.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::GLIDER, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 620.0f, 616.0f, 36.0f, 28.0f));
            addModularBay(L, 780.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 960.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1080.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 73: CLOCKWORK ASSEMBLY BAY
        {
            LevelSpec L;
            L.name = "LEVEL 73: CLOCKWORK ASSEMBLY BAY";
            L.subtitle = "Twin heavy gantries house a suspended spiked wrecking hammer.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addClockworkGantry(L, 720.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::TOUCAN);
            addClockworkGantry(L, 1000.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {860.0f, 180.0f}; pb.length = 170.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 74: HIGH FORGE CATWALKS
        {
            LevelSpec L;
            L.name = "LEVEL 74: HIGH FORGE CATWALKS";
            L.subtitle = "Elevated timber walkways spanning high stone piers over explosive munitions.";
            L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::TURBO, PigType::BOMB};
            L.wind = 25.0f;
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 575.0f, 24.0f, 110.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 575.0f, 24.0f, 110.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 1020.0f, 575.0f, 24.0f, 110.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 780.0f, 508.0f, 160.0f, 22.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 940.0f, 508.0f, 160.0f, 22.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 780.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 940.0f, 617.0f, 26.0f, 26.0f));
            L.birds.push_back(makeBird(BirdClass::OWL, 780.0f, 497.0f - birdRad(BirdClass::OWL)));
            L.birds.push_back(makeBird(BirdClass::HERON, 940.0f, 497.0f - birdRad(BirdClass::HERON), true));
            levels.push_back(L);
        }

        // LEVEL 75: MUNITIONS STORAGE CASEMATE
        {
            LevelSpec L;
            L.name = "LEVEL 75: MUNITIONS STORAGE CASEMATE";
            L.subtitle = "Fortified industrial bunker packed with TNT gunpowder kegs.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            addFortifiedCasemate(L, 850.0f, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 710.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 990.0f, 617.0f, 26.0f, 26.0f));
            levels.push_back(L);
        }

        // LEVEL 76: THE POUNDING HAMMER
        {
            LevelSpec L;
            L.name = "LEVEL 76: THE POUNDING HAMMER";
            L.subtitle = "Heavy granite pendulum swinging between dual stone foundry towers.";
            L.pigQueue = {PigType::TURBO, PigType::BOMB, PigType::SPLIT, PigType::TROTTER};
            addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 190.0f; pb.radius = 30.0f; pb.mass = 14.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 77: SMELTING VIADUCT
        {
            LevelSpec L;
            L.name = "LEVEL 77: SMELTING VIADUCT";
            L.subtitle = "High stone viaduct above magma vents with cross-braced timber tiers.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO};
            L.wind = -30.0f;
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
            addClockworkGantry(L, 780.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::OWL);
            addClockworkGantry(L, 980.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::HERON);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 78: THE IRON BASTION CRANE
        {
            LevelSpec L;
            L.name = "LEVEL 78: THE IRON BASTION CRANE";
            L.subtitle = "Asymmetrical gantry crane with counterweight megaliths.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
            addClockworkGantry(L, 800.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::FALCON);
            L.birds.back().hasShield = true;
            addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 79: OVERHEATED BOILER HOUSE
        {
            LevelSpec L;
            L.name = "LEVEL 79: OVERHEATED BOILER HOUSE";
            L.subtitle = "Triple magma vent defense with trampoline steam buffers.";
            L.pigQueue = {PigType::TURBO, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
            L.wind = 40.0f;
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 670.0f, 616.0f, 36.0f, 28.0f));
            addModularBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            addModularBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 80: GRAND FOUNDRY OF THE IRON CONDOR
        {
            LevelSpec L;
            L.name = "LEVEL 80: GRAND FOUNDRY OF THE IRON CONDOR";
            L.subtitle = "Chapter Boss! Colossal multi-tier clockwork gantry defended by an armored Zeppelin Condor.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -50.0f;
            L.hasStorm = true;
            addFrontShield(L, 570.0f, MaterialTier::STONE, true);
            addClockworkGantry(L, 700.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::FALCON);
            addClockworkGantry(L, 900.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::CONDOR);
            addClockworkGantry(L, 1100.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::HERON);
            L.birds[1].inZeppelin = true; L.birds[1].balloonHp = 120.0f;
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::CONDOR || bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER IX (LEVELS 81-90): ZEPHYR CRAGS & FLOATING SPIRE CASTLES
        // ====================================================================
        // LEVEL 81: GALE-FORCE OUTPOST
        {
            LevelSpec L;
            L.name = "LEVEL 81: GALE-FORCE OUTPOST";
            L.subtitle = "High wind drafts batter a swaying timber watchtower.";
            L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
            L.wind = -85.0f;
            addModularBay(L, 860.0f, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addFrontShield(L, 680.0f, MaterialTier::TIMBER, false);
            levels.push_back(L);
        }

        // LEVEL 82: ZEPHYR SUSPENSION BRIDGE
        {
            LevelSpec L;
            L.name = "LEVEL 82: ZEPHYR SUSPENSION BRIDGE";
            L.subtitle = "Two high stone spires connected by an elevated suspended bridge deck.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::TROTTER};
            L.wind = 55.0f;
            addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1000.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850.0f, 498.0f, 200.0f, 20.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 488.0f - birdRad(BirdClass::HERON)));
            levels.push_back(L);
        }

        // LEVEL 83: LIGHTNING ROOST
        {
            LevelSpec L;
            L.name = "LEVEL 83: LIGHTNING ROOST";
            L.subtitle = "Storm-charged clouds unleash static lightning strikes across high stone battlements.";
            L.pigQueue = {PigType::BOMB, PigType::GLIDER, PigType::DRILL, PigType::TROTTER};
            L.wind = -75.0f;
            L.hasStorm = true;
            addModularBay(L, 750.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 950.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds.back().hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 84: SKYSHIP MOORING PYLON
        {
            LevelSpec L;
            L.name = "LEVEL 84: SKYSHIP MOORING PYLON";
            L.subtitle = "A towering stone mast mooring an aerial Zeppelin Falcon sentry.";
            L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::DRILL, PigType::BOMB};
            L.wind = -90.0f;
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 540.0f, 32.0f, 180.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 860.0f, 439.0f, 120.0f, 22.0f));
            L.birds.push_back(makeBird(BirdClass::FALCON, 860.0f, 428.0f - birdRad(BirdClass::FALCON), false, true, true));
            levels.push_back(L);
        }

        // LEVEL 85: CROSSWIND RIDGE
        {
            LevelSpec L;
            L.name = "LEVEL 85: CROSSWIND RIDGE";
            L.subtitle = "Violent fluctuating gales test precision trajectory calculation.";
            L.pigQueue = {PigType::SPLIT, PigType::GLIDER, PigType::TURBO, PigType::TROTTER};
            L.wind = 80.0f;
            addPyramidBay(L, 780.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, BirdClass::OWL);
            addPyramidBay(L, 980.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, BirdClass::HERON);
            levels.push_back(L);
        }

        // LEVEL 86: CLOUD CITADEL GATE
        {
            LevelSpec L;
            L.name = "LEVEL 86: CLOUD CITADEL GATE";
            L.subtitle = "Grand stone pylon gate with trampoline safety nets over the abyss.";
            L.pigQueue = {PigType::TURBO, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
            L.wind = -60.0f;
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 74.0f, 24.0f));
            addMonolithBay(L, 780.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 87: TEMPEST EYRIE
        {
            LevelSpec L;
            L.name = "LEVEL 87: TEMPEST EYRIE";
            L.subtitle = "Gale-force storm headwinds defend a triple-deck stone mountain eyrie.";
            L.pigQueue = {PigType::GLIDER, PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            L.wind = -110.0f;
            L.hasStorm = true;
            addFrontShield(L, 590.0f, MaterialTier::STONE, true);
            addModularBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 920.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 88: TWIN FLOATING SPIRES
        {
            LevelSpec L;
            L.name = "LEVEL 88: TWIN FLOATING SPIRES";
            L.subtitle = "Dual slender stone towers connected by a high aerial bridge.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
            L.wind = -40.0f;
            addModularBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 1020.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 870.0f, 498.0f, 200.0f, 20.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 870.0f, 488.0f - birdRad(BirdClass::HERON), true));
            levels.push_back(L);
        }

        // LEVEL 89: ZEPPELIN AIR-FLEET DOCK
        {
            LevelSpec L;
            L.name = "LEVEL 89: ZEPPELIN AIR-FLEET DOCK";
            L.subtitle = "Two Zeppelin-mounted Condor captains docked atop high fortress pylons.";
            L.pigQueue = {PigType::GLIDER, PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
            L.wind = -70.0f;
            L.hasStorm = true;
            addMonolithBay(L, 750.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 990.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
            L.birds[1].inZeppelin = true;
            L.birds[3].inZeppelin = true;
            levels.push_back(L);
        }

        // LEVEL 90: SKY FORTRESS OF THE STORM ARCHON
        {
            LevelSpec L;
            L.name = "LEVEL 90: SKY FORTRESS OF THE STORM ARCHON";
            L.subtitle = "Chapter Boss! Storm lightning, extreme winds, and an armored Zeppelin Archduke Heron.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -100.0f;
            L.hasStorm = true;
            addFrontShield(L, 560.0f, MaterialTier::STONE, true);
            addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.birds[2].inZeppelin = true;
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER X (LEVELS 91-100): CRYSTAL CAVERNS & PRISMATIC MONOLITHS
        // ====================================================================
        // LEVEL 91: PRISMATIC ICE SPIRE
        {
            LevelSpec L;
            L.name = "LEVEL 91: PRISMATIC ICE SPIRE";
            L.subtitle = "Tall crystalline ice tower with stone foundation plinth.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::TROTTER};
            addModularBay(L, 850.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 92: GLACIAL CREVASSE BRIDGE
        {
            LevelSpec L;
            L.name = "LEVEL 92: GLACIAL CREVASSE BRIDGE";
            L.subtitle = "Long timber and stone bridge spanning two slippery ice piers.";
            L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
            addModularBay(L, 720.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 980.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 850.0f, 498.0f, 160.0f, 20.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 488.0f - birdRad(BirdClass::HERON)));
            levels.push_back(L);
        }

        // LEVEL 93: THE FROST VAULT
        {
            LevelSpec L;
            L.name = "LEVEL 93: THE FROST VAULT";
            L.subtitle = "Thick ice walls enclosing an explosive cache of gunpowder kegs.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            addFortifiedCasemate(L, 860.0f, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 720.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 1000.0f, 617.0f, 26.0f, 26.0f));
            levels.push_back(L);
        }

        // LEVEL 94: DIAMOND PILLAR HALL
        {
            LevelSpec L;
            L.name = "LEVEL 94: DIAMOND PILLAR HALL";
            L.subtitle = "Multi-column peristyle of alternating stone and crystal ice steles.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 575.0f, 22.0f, 110.0f));
            L.blocks.push_back(makeBlock(MaterialTier::ICE, 800.0f, 575.0f, 22.0f, 110.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 900.0f, 575.0f, 22.0f, 110.0f));
            L.blocks.push_back(makeBlock(MaterialTier::ICE, 1000.0f, 575.0f, 22.0f, 110.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 850.0f, 509.0f, 330.0f, 22.0f));
            L.birds.push_back(makeBird(BirdClass::OWL, 750.0f, 498.0f - birdRad(BirdClass::OWL)));
            L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 498.0f - birdRad(BirdClass::HERON)));
            L.birds.push_back(makeBird(BirdClass::OWL, 950.0f, 498.0f - birdRad(BirdClass::OWL)));
            levels.push_back(L);
        }

        // LEVEL 95: TRAMPOLINE REFLECTION GROTTO
        {
            LevelSpec L;
            L.name = "LEVEL 95: TRAMPOLINE REFLECTION GROTTO";
            L.subtitle = "Zig-zag trampoline deflectors allow high-velocity ricochets into frozen towers.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 700.0f, 618.0f, 70.0f, 24.0f));
            addModularBay(L, 860.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1020.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 96: THE GLACIAL PALISADE
        {
            LevelSpec L;
            L.name = "LEVEL 96: THE GLACIAL PALISADE";
            L.subtitle = "Layered ice slabs protect armored Falcon guards from direct frontal bombardment.";
            L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addFrontShield(L, 620.0f, MaterialTier::ICE, true);
            addFrontShield(L, 660.0f, MaterialTier::ICE, true);
            addModularBay(L, 780.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 960.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds[0].hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 97: CHASM OF THE FROZEN PENDULUM
        {
            LevelSpec L;
            L.name = "LEVEL 97: CHASM OF THE FROZEN PENDULUM";
            L.subtitle = "Heavy ice-sheathed wrecking pendulum poised above the frost altar.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addModularBay(L, 710.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1010.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 98: SUB-ZERO CASEMATE
        {
            LevelSpec L;
            L.name = "LEVEL 98: SUB-ZERO CASEMATE";
            L.subtitle = "Reinforced ice bunker with basalt lintels and thermal magma vents.";
            L.pigQueue = {PigType::DRILL, PigType::GRAVITON, PigType::BOMB, PigType::TURBO};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
            addFortifiedCasemate(L, 760.0f, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, 1, BirdClass::OWL, 1, BirdClass::FALCON);
            addModularBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds[1].hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 99: PRISMATIC GRAND COLONNADE
        {
            LevelSpec L;
            L.name = "LEVEL 99: PRISMATIC GRAND COLONNADE";
            L.subtitle = "Long span of alternating crystalline ice and polished stone columns.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addMonolithBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 900.0f, 2, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 1100.0f, 2, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 100: ICE CITADEL OF THE GLACIAL KING
        {
            LevelSpec L;
            L.name = "LEVEL 100: ICE CITADEL OF THE GLACIAL KING";
            L.subtitle = "Chapter Boss! Massive multi-tiered ice and granite castle defended by Emperor Heron's frosty court.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -45.0f;
            L.hasStorm = true;
            addFrontShield(L, 560.0f, MaterialTier::STONE, true);
            addModularBay(L, 680.0f, 3, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            addModularBay(L, 1080.0f, 3, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER XI (LEVELS 101-110): VOLCANIC CALDERA & ASHFLOW RAMPARTS
        // ====================================================================
        // LEVEL 101: CALDERA WATCHPOST
        {
            LevelSpec L;
            L.name = "LEVEL 101: CALDERA WATCHPOST";
            L.subtitle = "Heavy basalt tower constructed over bubbling magma vents.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 660.0f, 616.0f, 36.0f, 28.0f));
            addModularBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 980.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 102: STEPPED BASALT PYRAMID
        {
            LevelSpec L;
            L.name = "LEVEL 102: STEPPED BASALT PYRAMID";
            L.subtitle = "Three-tiered ziggurat with wide stone foundation slabs.";
            L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addPyramidBay(L, 850.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
            L.birds.back().hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 103: THE OBSIDIAN VIADUCT
        {
            LevelSpec L;
            L.name = "LEVEL 103: THE OBSIDIAN VIADUCT";
            L.subtitle = "High stone viaduct straddling active volcanic vents.";
            L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::BOMB, PigType::TURBO};
            L.wind = 25.0f;
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 620.0f, 616.0f, 36.0f, 28.0f));
            addMonolithBay(L, 780.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 104: ASHFLOW BUNKER
        {
            LevelSpec L;
            L.name = "LEVEL 104: ASHFLOW BUNKER";
            L.subtitle = "Low, ultra-heavy stone casemate with reinforced roof against falling volcanic bombs.";
            L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addFortifiedCasemate(L, 840.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::OWL);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 700.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 980.0f, 617.0f, 26.0f, 26.0f));
            levels.push_back(L);
        }

        // LEVEL 105: MOLTEN CHAMBER
        {
            LevelSpec L;
            L.name = "LEVEL 105: MOLTEN CHAMBER";
            L.subtitle = "Twin magma vents with trampoline safety deflectors.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 650.0f, 618.0f, 70.0f, 24.0f));
            addModularBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 106: THE BRIMSTONE BASTION
        {
            LevelSpec L;
            L.name = "LEVEL 106: THE BRIMSTONE BASTION";
            L.subtitle = "Volcanic rock fortress with integrated TNT demolition charges.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            L.wind = -20.0f;
            addMonolithBay(L, 760.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 870.0f, 617.0f, 26.0f, 26.0f));
            L.birds.push_back(makeBird(BirdClass::FALCON, 870.0f, 604.0f - birdRad(BirdClass::FALCON), false, true));
            levels.push_back(L);
        }

        // LEVEL 107: MAGMA CASCADE BRIDGE
        {
            LevelSpec L;
            L.name = "LEVEL 107: MAGMA CASCADE BRIDGE";
            L.subtitle = "Suspended stone bridge spanning across two volcanic spires.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
            L.wind = -40.0f;
            addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1000.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 850.0f, 498.0f, 200.0f, 20.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 488.0f - birdRad(BirdClass::HERON), true));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 850.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 108: THE FIRE DRAKE'S EYRIE
        {
            LevelSpec L;
            L.name = "LEVEL 108: THE FIRE DRAKE'S EYRIE";
            L.subtitle = "High basalt column topped by an armored Condor general.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
            L.wind = -55.0f;
            L.hasStorm = true;
            addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 1, BirdClass::CONDOR);
            L.birds.back().hasHelmet = true;
            addFrontShield(L, 680.0f, MaterialTier::STONE, true);
            levels.push_back(L);
        }

        // LEVEL 109: CALDERA GRAND REDOUBT
        {
            LevelSpec L;
            L.name = "LEVEL 109: CALDERA GRAND REDOUBT";
            L.subtitle = "Multi-bay fortified fortress with stepped plinths and magma vents.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -30.0f;
            L.hasStorm = true;
            addPyramidBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::OWL);
            addPyramidBay(L, 980.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 850.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 110: VOLCANIC CITADEL OF THE ASH EMPEROR
        {
            LevelSpec L;
            L.name = "LEVEL 110: VOLCANIC CITADEL OF THE ASH EMPEROR";
            L.subtitle = "Chapter Boss! Fortress surrounded by magma geysers and elite armored avian court.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -75.0f;
            L.hasStorm = true;
            addFrontShield(L, 560.0f, MaterialTier::STONE, true);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 500.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1160.0f, 616.0f, 36.0f, 28.0f));
            addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER XII (LEVELS 111-120): THE CELESTIAL OBSERVATORY & ASTRAL SPIRES
        // ====================================================================
        // LEVEL 111: ASTRAL SCRIPTORIUM
        {
            LevelSpec L;
            L.name = "LEVEL 111: ASTRAL SCRIPTORIUM";
            L.subtitle = "Tall slender stone steles supporting an elevated observation terrace.";
            L.pigQueue = {PigType::GRAVITON, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addModularBay(L, 850.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 112: THE MERIDIAN COLONNADE
        {
            LevelSpec L;
            L.name = "LEVEL 112: THE MERIDIAN COLONNADE";
            L.subtitle = "Stone colonnade with precision-balanced lintels under starry skies.";
            L.pigQueue = {PigType::DRILL, PigType::GRAVITON, PigType::TURBO, PigType::TROTTER};
            addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 113: ORRERY PENDULUM
        {
            LevelSpec L;
            L.name = "LEVEL 113: ORRERY PENDULUM";
            L.subtitle = "A suspended planetary wrecking ball between twin astronomical towers.";
            L.pigQueue = {PigType::BOMB, PigType::GRAVITON, PigType::TURBO, PigType::TROTTER};
            addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 114: THE HORIZON PYLON
        {
            LevelSpec L;
            L.name = "LEVEL 114: THE HORIZON PYLON";
            L.subtitle = "Cantilevered observation platform extending into high altitude air currents.";
            L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            L.wind = 50.0f;
            addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 1, BirdClass::OWL);
            L.birds[1].hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 115: ZODIAC GATEHOUSE
        {
            LevelSpec L;
            L.name = "LEVEL 115: ZODIAC GATEHOUSE";
            L.subtitle = "Dual stone pylons with high timber observatory dome.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
            addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::OWL);
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 580.0f, 24.0f, 100.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 1020.0f, 580.0f, 24.0f, 100.0f));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 700.0f, 530.0f - birdRad(BirdClass::TOUCAN)));
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 1020.0f, 530.0f - birdRad(BirdClass::TOUCAN)));
            levels.push_back(L);
        }

        // LEVEL 116: VORTEX PROVING GROUND
        {
            LevelSpec L;
            L.name = "LEVEL 116: VORTEX PROVING GROUND";
            L.subtitle = "Dense clusters of timber crates and stone steles designed for Atticus's vortex.";
            L.pigQueue = {PigType::GRAVITON, PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
            addModularBay(L, 720.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            addModularBay(L, 1000.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 117: THE ECLIPSE TOWER
        {
            LevelSpec L;
            L.name = "LEVEL 117: THE ECLIPSE TOWER";
            L.subtitle = "Stepped stone tower with basalt lintels and helmed avian scholars.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
            L.wind = -35.0f;
            addPyramidBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
            L.birds.back().hasHelmet = true;
            addFrontShield(L, 680.0f, MaterialTier::STONE, false);
            levels.push_back(L);
        }

        // LEVEL 118: ASTRAL VIADUCT
        {
            LevelSpec L;
            L.name = "LEVEL 118: ASTRAL VIADUCT";
            L.subtitle = "Sky-high stone bridge spanning two astronomical pavilions.";
            L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::TURBO, PigType::BOMB};
            L.wind = 60.0f;
            addModularBay(L, 720.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1000.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 498.0f, 180.0f, 20.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, 488.0f - birdRad(BirdClass::HERON), true));
            levels.push_back(L);
        }

        // LEVEL 119: THE SOLSTICE SHRINE
        {
            LevelSpec L;
            L.name = "LEVEL 119: THE SOLSTICE SHRINE";
            L.subtitle = "Triple-bay megalithic temple with pendulum center and trampoline wings.";
            L.pigQueue = {PigType::TURBO, PigType::GRAVITON, PigType::DRILL, PigType::BOMB};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
            addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {860.0f, 160.0f}; pb.length = 175.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 120: GRAND OBSERVATORY OF THE STAR LORD
        {
            LevelSpec L;
            L.name = "LEVEL 120: GRAND OBSERVATORY OF THE STAR LORD";
            L.subtitle = "Chapter Boss! Colossal multi-wing observatory defended by Archduke Heron and the Royal Astrologers.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -70.0f;
            L.hasStorm = true;
            addFrontShield(L, 560.0f, MaterialTier::STONE, true);
            addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER XIII (LEVELS 121-130): COLOSSEUM OF THE GREAT AVIAN KINGS
        // ====================================================================
        // LEVEL 121: COLOSSEUM OUTER GATE
        {
            LevelSpec L;
            L.name = "LEVEL 121: COLOSSEUM OUTER GATE";
            L.subtitle = "Heavy stone gatehouse with wide lintels and guarded parapets.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addMonolithBay(L, 850.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            L.birds.back().hasShield = true;
            addFrontShield(L, 680.0f, MaterialTier::STONE, true);
            levels.push_back(L);
        }

        // LEVEL 122: GLADIATORIAL ARENA TIER
        {
            LevelSpec L;
            L.name = "LEVEL 122: GLADIATORIAL ARENA TIER";
            L.subtitle = "Arena floor flanked by trampoline deflectors and timber holding cages.";
            L.pigQueue = {PigType::TURBO, PigType::GLIDER, PigType::DRILL, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 74.0f, 24.0f));
            addModularBay(L, 780.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 980.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 123: THE IMPERIAL LOGE
        {
            LevelSpec L;
            L.name = "LEVEL 123: THE IMPERIAL LOGE";
            L.subtitle = "Elevated royal viewing platform supported by megalith columns.";
            L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 1, BirdClass::HERON);
            L.birds.back().hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 124: HYPOGEUM POWDER VAULT
        {
            LevelSpec L;
            L.name = "LEVEL 124: HYPOGEUM POWDER VAULT";
            L.subtitle = "Under-arena powder magazine packed with TNT munitions.";
            L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
            addFortifiedCasemate(L, 850.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 710.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 990.0f, 617.0f, 26.0f, 26.0f));
            levels.push_back(L);
        }

        // LEVEL 125: STEPPED GRANDSTAND
        {
            LevelSpec L;
            L.name = "LEVEL 125: STEPPED GRANDSTAND";
            L.subtitle = "Ascending tiers of stone seating filled with avian spectators.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
            addPyramidBay(L, 760.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::OWL);
            addPyramidBay(L, 980.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
            levels.push_back(L);
        }

        // LEVEL 126: THE CHAMPION'S GAUNTLET
        {
            LevelSpec L;
            L.name = "LEVEL 126: THE CHAMPION'S GAUNTLET";
            L.subtitle = "Arena challenge course with swinging wrecking pendulum and trampoline nets.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
            addModularBay(L, 710.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 1010.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 127: TWIN ARENA PYLONS
        {
            LevelSpec L;
            L.name = "LEVEL 127: TWIN ARENA PYLONS";
            L.subtitle = "Towering arena gateway columns with high stone cross-beam.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO};
            addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            L.birds[1].hasShield = true;
            L.birds[3].hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 128: THE EMPEROR'S PODIUM
        {
            LevelSpec L;
            L.name = "LEVEL 128: THE EMPEROR'S PODIUM";
            L.subtitle = "Central elevated throne platform with stone battlements.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
            addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON);
            L.birds.back().hasHelmet = true;
            addFrontShield(L, 680.0f, MaterialTier::STONE, true);
            levels.push_back(L);
        }

        // LEVEL 129: GRAND AMPHITHEATER ARENA
        {
            LevelSpec L;
            L.name = "LEVEL 129: GRAND AMPHITHEATER ARENA";
            L.subtitle = "Multi-tiered stone colonnade spanning across the entire battlefield.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addMonolithBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addMonolithBay(L, 1060.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 130: THE AVIAN GRAND CHAMPIONSHIP
        {
            LevelSpec L;
            L.name = "LEVEL 130: THE AVIAN GRAND CHAMPIONSHIP";
            L.subtitle = "Chapter Boss! The ultimate gladiatorial tournament fortress defended by High Marshall Condor and Emperor Heron.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -60.0f;
            L.hasStorm = true;
            addFrontShield(L, 560.0f, MaterialTier::STONE, true);
            addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) { bd.hasHelmet = true; bd.helmetHp = 150.0f; bd.maxHelmetHp = 150.0f; } }
            levels.push_back(L);
        }

        // ====================================================================
        // CHAPTER XIV (LEVELS 131-140): SOVEREIGN HIGH SANCTUM & EMPEROR'S ZENITH
        // ====================================================================
        // LEVEL 131: SANCTUM OUTER RAMPARTS
        {
            LevelSpec L;
            L.name = "LEVEL 131: SANCTUM OUTER RAMPARTS";
            L.subtitle = "Fortress perimeter wall with reinforced battlements and magma geysers.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::DRILL, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 620.0f, 616.0f, 36.0f, 28.0f));
            addMonolithBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            levels.push_back(L);
        }

        // LEVEL 132: THE SOVEREIGN CAUSEWAY
        {
            LevelSpec L;
            L.name = "LEVEL 132: THE SOVEREIGN CAUSEWAY";
            L.subtitle = "Grand stone viaduct defended by Falcon shield-bearers.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
            L.wind = -30.0f;
            addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.birds[0].hasShield = true;
            L.birds[2].hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 133: THE MOATED KEEP
        {
            LevelSpec L;
            L.name = "LEVEL 133: THE MOATED KEEP";
            L.subtitle = "Stone citadel flanked by magma moats and trampoline buffers.";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::TROTTER};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 640.0f, 618.0f, 70.0f, 24.0f));
            addModularBay(L, 800.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            addModularBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1080.0f, 616.0f, 36.0f, 28.0f));
            levels.push_back(L);
        }

        // LEVEL 134: THE THUNDER BASTION
        {
            LevelSpec L;
            L.name = "LEVEL 134: THE THUNDER BASTION";
            L.subtitle = "Storm sky with heavy lightning and gale headwinds (-90).";
            L.pigQueue = {PigType::GLIDER, PigType::BOMB, PigType::DRILL, PigType::TURBO};
            L.wind = -90.0f;
            L.hasStorm = true;
            addFrontShield(L, 590.0f, MaterialTier::STONE, true);
            addModularBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 920.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds.back().hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 135: SANCTUM CLOCKWORK GANTRY
        {
            LevelSpec L;
            L.name = "LEVEL 135: SANCTUM CLOCKWORK GANTRY";
            L.subtitle = "Industrial defense gantry with swinging wrecking hammer.";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addClockworkGantry(L, 720.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
            addClockworkGantry(L, 1000.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{}; pb.anchor = {860.0f, 160.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 136: THE IMPERIAL STEPPED ZIGGURAT
        {
            LevelSpec L;
            L.name = "LEVEL 136: THE IMPERIAL STEPPED ZIGGURAT";
            L.subtitle = "Four-tiered grand stone pyramid with armored avian court.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addPyramidBay(L, 750.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
            addPyramidBay(L, 970.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
            L.birds[0].hasHelmet = true;
            L.birds[1].hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 137: SKYWAY TO THE ZENITH
        {
            LevelSpec L;
            L.name = "LEVEL 137: SKYWAY TO THE ZENITH";
            L.subtitle = "Twin floating spires connected by suspended stone bridge.";
            L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
            L.wind = -50.0f;
            L.hasStorm = true;
            addModularBay(L, 700.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 1020.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 497.0f, 220.0f, 22.0f));
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, 486.0f - birdRad(BirdClass::HERON), true));
            levels.push_back(L);
        }

        // LEVEL 138: THE HIGH GRAND CASEMATE
        {
            LevelSpec L;
            L.name = "LEVEL 138: THE HIGH GRAND CASEMATE";
            L.subtitle = "Ultra-heavy bomb-resistant bunker with double roof slabs.";
            L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 720.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 1000.0f, 617.0f, 26.0f, 26.0f));
            L.birds.back().hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 139: THE EMPEROR'S FINAL GUARD
        {
            LevelSpec L;
            L.name = "LEVEL 139: THE EMPEROR'S FINAL GUARD";
            L.subtitle = "Three-bay monolithic fortress defended by elite Zeppelin Condor captains.";
            L.pigQueue = {PigType::GLIDER, PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
            L.wind = -70.0f;
            L.hasStorm = true;
            addMonolithBay(L, 720.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
            L.birds[1].inZeppelin = true;
            L.birds[3].inZeppelin = true;
            levels.push_back(L);
        }

        // LEVEL 140: THE EMPEROR'S ZENITH - ABSOLUTE CLIMAX
        {
            LevelSpec L;
            L.name = "LEVEL 140: THE EMPEROR'S ZENITH - ABSOLUTE CLIMAX";
            L.subtitle = "The definitive campaign finale! A colossal, multi-tier imperial citadel combining every defense mechanism.";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -95.0f;
            L.hasStorm = true;
            addFrontShield(L, 550.0f, MaterialTier::STONE, true);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 490.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1170.0f, 616.0f, 36.0f, 28.0f));
            addMonolithBay(L, 660.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            addMonolithBay(L, 1060.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) { bd.hasHelmet = true; bd.helmetHp = 160.0f; bd.maxHelmetHp = 160.0f; } }
            levels.push_back(L);
        }

// ====================================================================
        // CHAPTER XV (LEVELS 141-150): THE 10 SECRET TRUFFLE VAULTS
        // High-concept secret puzzle vaults unlocked via Golden Truffles!
        // ====================================================================

        // LEVEL 141: Vault 01 - Powder Crypt of the Swine (Req: 2 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 141: VAULT 01 - POWDER CRYPT";
            L.subtitle = "Twin slate pillars house volatile TNT kegs ready for a chain reaction!";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
            // Pillars resting on ground
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 730, 575, 26, 110));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 970, 575, 26, 110));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 850, 509, 266, 22));
            // Pillars resting on deck 1
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 780, 452, 24, 92));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 920, 452, 24, 92));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 850, 396, 180, 20));
            // TNT kegs
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 790.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 910.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 485.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 890.0f, 485.0f, 26.0f, 26.0f));
            // Birds
            L.birds.push_back(makeBird(BirdClass::TOUCAN, 850, GROUND_Y - birdRad(BirdClass::TOUCAN)));
            L.birds.push_back(makeBird(BirdClass::OWL, 850, 498.0f - birdRad(BirdClass::OWL)));
            L.birds.push_back(makeBird(BirdClass::HERON, 850, 386.0f - birdRad(BirdClass::HERON), true));
            levels.push_back(L);
        }

        // LEVEL 142: Vault 02 - Trampoline Gauntlet (Req: 5 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 142: VAULT 02 - TRAMPOLINE GAUNTLET";
            L.subtitle = "Elastic trampoline deflectors line the cavern floor before the stone citadel!";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 530.0f, 618.0f, 70.0f, 24.0f));
            L.blocks.push_back(makeBlock(MaterialTier::STONE, 615.0f, 550.0f, 28.0f, 160.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 700.0f, 618.0f, 70.0f, 24.0f));
            addModularBay(L, 810, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 950, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            L.birds.back().hasHelmet = true;
            L.birds.back().helmetHp = 140.0f;
            L.birds.back().maxHelmetHp = 140.0f;
            levels.push_back(L);
        }

        // LEVEL 143: Vault 03 - Pendulum Cathedral (Req: 8 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 143: VAULT 03 - PENDULUM CATHEDRAL";
            L.subtitle = "An immense spiked iron wrecking pendulum hangs poised over the altar!";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            addModularBay(L, 710, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            addModularBay(L, 1010, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb{};
            pb.anchor = {860.0f, 150.0f};
            pb.length = 180.0f;
            pb.angle = 0.0f;
            pb.angVel = 0.0f;
            pb.radius = 28.0f;
            pb.mass = 12.0f;
            pb.spiked = true;
            L.pendulums.push_back(pb);
            levels.push_back(L);
        }

        // LEVEL 144: Vault 04 - Thunder Ridge Bastion (Req: 11 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 144: VAULT 04 - THUNDER RIDGE BASTION";
            L.subtitle = "Violent gale headwinds and lightning-charged skies defend the high ridge!";
            L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
            L.wind = -110.0f;
            L.hasStorm = true;
            addFrontShield(L, 600.0f, MaterialTier::STONE, true);
            addModularBay(L, 690, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 830, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 970, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.birds[1].hasHelmet = true; L.birds[1].helmetHp = 100.0f; L.birds[1].maxHelmetHp = 100.0f;
            L.birds[2].hasHelmet = true; L.birds[2].helmetHp = 140.0f; L.birds[2].maxHelmetHp = 140.0f;
            levels.push_back(L);
        }

        // LEVEL 145: Vault 05 - Obsidian Truffle Treasury (Req: 14 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 145: VAULT 05 - OBSIDIAN TRUFFLE TREASURY";
            L.subtitle = "The Grand Imperial Treasury! Magma geysers, trampolines, and the Emperor's Royal Guard!";
            L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::BOMB};
            L.wind = -45.0f;
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 500.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 74.0f, 24.0f));
            addFrontShield(L, 650.0f, MaterialTier::STONE, true);
            addModularBay(L, 740, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 880, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
            addModularBay(L, 1020, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 616.0f, 28.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 950.0f, 616.0f, 28.0f, 28.0f));
            for (auto& bd : L.birds) {
                if (bd.kind == BirdClass::HERON) {
                    bd.hasHelmet = true;
                    bd.helmetHp = 140.0f;
                    bd.maxHelmetHp = 140.0f;
                }
            }
            levels.push_back(L);
        }

        // LEVEL 146: Vault 06 - The Mirror Perils (Req: 18 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 146: VAULT 06 - THE MIRROR PERILS";
            L.subtitle = "Twin symmetrical towers with alternating trampoline bounce channels!";
            L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::GLIDER};
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 540.0f, 618.0f, 70.0f, 24.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 860.0f, 618.0f, 70.0f, 24.0f));
            addMonolithBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
            L.birds[1].hasShield = true;
            L.birds[3].hasShield = true;
            levels.push_back(L);
        }

        // LEVEL 147: Vault 07 - Graviton Chasm (Req: 22 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 147: VAULT 07 - GRAVITON CHASM";
            L.subtitle = "Deep magma abyss flanked by floating aeries and volatile powder kegs!";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 580.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
            addModularBay(L, 740.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addModularBay(L, 940.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 840.0f, 617.0f, 26.0f, 26.0f));
            L.birds.push_back(makeBird(BirdClass::CONDOR, 840.0f, 604.0f - birdRad(BirdClass::CONDOR), true));
            levels.push_back(L);
        }

        // LEVEL 148: Vault 08 - Clockwork Catastrophe (Req: 26 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 148: VAULT 08 - CLOCKWORK CATASTROPHE";
            L.subtitle = "Dual swinging wrecking pendulums over stacked TNT munitions magazine!";
            L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::DRILL};
            addClockworkGantry(L, 700.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
            addClockworkGantry(L, 1020.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
            L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
            PendulumBody pb1{}; pb1.anchor = {780.0f, 160.0f}; pb1.length = 170.0f; pb1.radius = 26.0f; pb1.mass = 11.0f; pb1.spiked = true; L.pendulums.push_back(pb1);
            PendulumBody pb2{}; pb2.anchor = {940.0f, 160.0f}; pb2.length = 170.0f; pb2.radius = 26.0f; pb2.mass = 11.0f; pb2.spiked = true; L.pendulums.push_back(pb2);
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 617.0f, 26.0f, 26.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TNT, 910.0f, 617.0f, 26.0f, 26.0f));
            levels.push_back(L);
        }

        // LEVEL 149: Vault 09 - Bore Master's Labyrinth (Req: 30 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 149: VAULT 09 - BORE MASTER'S LABYRINTH";
            L.subtitle = "Massive solid stone megaliths with narrow target channels designed for Drill Hog!";
            L.pigQueue = {PigType::DRILL, PigType::DRILL, PigType::GRAVITON, PigType::BOMB, PigType::TURBO};
            L.wind = -20.0f;
            addFrontShield(L, 600.0f, MaterialTier::STONE, true);
            addFortifiedCasemate(L, 750.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::FALCON);
            addFortifiedCasemate(L, 970.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::CONDOR);
            L.birds[1].hasShield = true;
            L.birds[3].hasHelmet = true;
            levels.push_back(L);
        }

        // LEVEL 150: Vault 10 - The Gilded Sovereign Sanctum (Req: 35 Truffles)
        {
            LevelSpec L;
            L.name = "LEVEL 150: VAULT 10 - GILDED SOVEREIGN SANCTUM";
            L.subtitle = "The ultimate secret truffle temple! 3-tier grand monoliths, golden ornamentation, and Emperor's personal retinue!";
            L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
            L.wind = -80.0f;
            L.hasStorm = true;
            addFrontShield(L, 510.0f, MaterialTier::STONE, true);
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 450.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1180.0f, 616.0f, 36.0f, 28.0f));
            L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 565.0f, 618.0f, 60.0f, 24.0f));
            addMonolithBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
            addMonolithBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
            addMonolithBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
            for (auto& bd : L.birds) {
                if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) {
                    bd.hasHelmet = true;
                    bd.helmetHp = 180.0f;
                    bd.maxHelmetHp = 180.0f;
                }
            }
            levels.push_back(L);
        }

        // 35 Hidden Golden Origami Truffles across Campaign (2-3 per chapter)
        struct TrufflePlacement { int lvlIdx; Vec2 pos; };
        const TrufflePlacement truffleDefs[35] = {
            // Chapter 1 (Levels 1-10)
            { 1, {850.0f, 380.0f}}, // Level 2
            { 4, {855.0f, 385.0f}}, // Level 5
            { 8, {855.0f, 290.0f}}, // Level 9
            // Chapter 2 (Levels 11-20)
            {12, {915.0f, 380.0f}}, // Level 13
            {16, {775.0f, 380.0f}}, // Level 17
            {19, {860.0f, 290.0f}}, // Level 20
            // Chapter 3 (Levels 21-30)
            {22, {715.0f, 290.0f}}, // Level 23
            {26, {855.0f, 290.0f}}, // Level 27
            {29, {945.0f, 290.0f}}, // Level 30
            // Chapter 4 (Levels 31-40)
            {32, {720.0f, 290.0f}}, // Level 33
            {35, {850.0f, 290.0f}}, // Level 36
            {39, {790.0f, 290.0f}}, // Level 40
            // Chapter 5 (Levels 41-50)
            {42, {910.0f, 290.0f}}, // Level 43
            {46, {855.0f, 290.0f}}, // Level 47
            {49, {795.0f, 290.0f}}, // Level 50
            // Chapter 6 (Levels 51-60)
            {52, {860.0f, 330.0f}}, // Level 53
            {56, {820.0f, 330.0f}}, // Level 57
            // Chapter 7 (Levels 61-70)
            {62, {860.0f, 440.0f}}, // Level 63
            {66, {880.0f, 530.0f}}, // Level 67
            {69, {880.0f, 440.0f}}, // Level 70
            // Chapter 8 (Levels 71-80)
            {72, {860.0f, 380.0f}}, // Level 73
            {77, {800.0f, 380.0f}}, // Level 78
            // Chapter 9 (Levels 81-90)
            {82, {850.0f, 420.0f}}, // Level 83
            {86, {820.0f, 290.0f}}, // Level 87
            // Chapter 10 (Levels 91-100)
            {92, {860.0f, 490.0f}}, // Level 93
            {97, {760.0f, 490.0f}}, // Level 98
            // Chapter 11 (Levels 101-110)
            {102, {880.0f, 440.0f}}, // Level 103
            {106, {850.0f, 470.0f}}, // Level 107
            {109, {880.0f, 290.0f}}, // Level 110
            // Chapter 12 (Levels 111-120)
            {112, {860.0f, 380.0f}}, // Level 113
            {117, {860.0f, 470.0f}}, // Level 118
            // Chapter 13 (Levels 121-130)
            {122, {860.0f, 440.0f}}, // Level 123
            {126, {860.0f, 470.0f}}, // Level 127
            // Chapter 14 (Levels 131-140)
            {132, {860.0f, 470.0f}}, // Level 133
            {136, {860.0f, 470.0f}}  // Level 137
        };
        for (const auto& tp : truffleDefs) {
            if (tp.lvlIdx >= 0 && tp.lvlIdx < static_cast<int>(levels.size())) {
                levels[tp.lvlIdx].hasTruffle = true;
                levels[tp.lvlIdx].trufflePos = tp.pos;
            }
        }

        // Calibrate 2-Star and 3-Star thresholds across all 150 levels based on actual level composition
        for (auto& L : levels) {
            int birdPts = 0;
            for (const auto& t : L.birds) {
                birdPts += (t.kind == BirdClass::HERON) ? 10000 :
                           (t.kind == BirdClass::OWL ? 7500 : 5000);
                if (t.hasHelmet) birdPts += 1000;
            }
            int blockPts = 0;
            for (const auto& b : L.blocks) {
                blockPts += (b.mat == MaterialTier::TWIG)       ? 500 :
                            (b.mat == MaterialTier::TIMBER     ? 900 :
                            (b.mat == MaterialTier::STONE      ? 1500 :
                            (b.mat == MaterialTier::ICE        ? 600 :
                            (b.mat == MaterialTier::TRAMPOLINE ? 0 : 2500))));
            }
            L.star2Threshold = ((birdPts + static_cast<int>(blockPts * 0.55f) + 1500) / 1000) * 1000;
            L.star3Threshold = ((birdPts + static_cast<int>(blockPts * 0.65f) + 6000) / 1000) * 1000;
        }
    }

    void loadCustomLevelSpec(const LevelSpec& L) {
        remainingPigs = L.pigQueue;
        blocks = L.blocks;
        birds = L.birds;
        pendulums = L.pendulums;
        activePigs.clear();
        particles.clear();
        floatScores.clear();
        aerialBombs.clear();
        score = 0;
        earnedStars = 0;
        levelComplete = false;
        levelFailed = false;
        fortressAwake = false;
        simSpeed = 1.0f;
        isNewRecordThisRun = false;
        recordAnimTimer = 0.0f;
        settleTimer = 0.0f;
        physicsAccumulator = 0.0f;
        camPos = {0.0f, 0.0f};
        camTarget = {0.0f, 0.0f};
        camShakeOffset = {0.0f, 0.0f};
        shakeTrauma = 0.0f;
        isManualPanning = false;
        manualPanTimer = 0.0f;
        ghostTrajectory.clear();
        currentFlightTrail.clear();
        slingRecoilTimer = 0.0f;
        draggingSling = false;
        slingPullPos = {SLING_X, SLING_Y};

        levelTruffle.exists = L.hasTruffle;
        levelTruffle.active = L.hasTruffle;
        levelTruffle.pos = L.trufflePos;
        levelTruffle.collected = false;
        levelTruffle.pulseTime = 0.0f;

        shotBlocksDestroyed = 0;
        shotTntDetonated = 0;
        shotHitTrampoline = false;
        shotUsedAnvil = false;
        shotDrillPierces = 0;
        shotLaunchPos = {SLING_X, SLING_Y};

        currentWind = L.wind;
        windGustTimer = 0.0f;
        skyFlashAlpha = 0.0f;
        stormStrikeTimer = 12.0f;
        lightningBolts.clear();
        stormClouds.clear();
        if (L.hasStorm) {
            StormCloud c1;
            c1.pos = {280.0f, 75.0f};
            c1.speed = 12.0f;
            c1.width = 175.0f;
            c1.height = 55.0f;
            StormCloud c2;
            c2.pos = {760.0f, 60.0f};
            c2.speed = 16.0f;
            c2.width = 210.0f;
            c2.height = 62.0f;
            stormClouds.push_back(c1);
            stormClouds.push_back(c2);
        }
        if (std::abs(L.wind) > 20.0f) {
            assets.playSound("sound.wind", 0.70f);
        }

        spawnNextPigInSlingshot();
    }

    void loadLevel(int idx) {
        if (idx < 0 || idx >= static_cast<int>(levels.size())) idx = 0;
        currentLevelIdx = idx;
        screen = ScreenState::PLAYING;
        const LevelSpec& L = levels[idx];
        loadCustomLevelSpec(L);
        levelTruffle.collected = (idx >= 0 && idx < NUM_LEVELS) ? trufflesFound[idx] : false;
        prevLevelBest = (idx >= 0 && idx < NUM_LEVELS) ? bestScores[idx] : 0;
    }

    void saveCustomLevel(int slot) {
        if (slot < 1 || slot > 3) slot = 1;
        std::string filename = "custom_siege_" + std::to_string(slot) + ".cfg";
        std::ofstream out(filename);
        if (!out.good()) return;
        out << "theme = " << editorTheme << "\n";
        out << "wind = " << editorWind << "\n";
        out << "storm = " << (editorStorm ? 1 : 0) << "\n";
        out << "truffle.exists = " << (customLevel.hasTruffle ? 1 : 0) << "\n";
        out << "truffle.x = " << customLevel.trufflePos.x << "\n";
        out << "truffle.y = " << customLevel.trufflePos.y << "\n";
        for (const auto& b : customLevel.blocks) {
            out << "block = " << static_cast<int>(b.mat) << " " << b.pos.x << " " << b.pos.y << " " << b.w << " " << b.h << "\n";
        }
        for (const auto& t : customLevel.birds) {
            out << "bird = " << static_cast<int>(t.kind) << " " << t.pos.x << " " << t.pos.y << " " << (t.hasHelmet ? 1 : 0) << "\n";
        }
        for (const auto& p : customLevel.pendulums) {
            out << "pendulum = " << p.anchor.x << " " << p.anchor.y << " " << p.length << " " << (p.spiked ? 1 : 0) << "\n";
        }
        floatScores.push_back({{WINDOW_W * 0.5f, 160.0f}, "SIEGE SLOT " + std::to_string(slot) + " SAVED!", {140, 240, 180, 255}, 1.4f, 1.4f});
        assets.playSound("sound.ui_click");
    }

    void loadCustomLevel(int slot) {
        if (slot < 1 || slot > 3) slot = 1;
        customSlot = slot;
        std::string filename = "custom_siege_" + std::to_string(slot) + ".cfg";
        std::ifstream in(filename);
        customLevel = LevelSpec{};
        customLevel.name = "CUSTOM SIEGE " + std::to_string(slot);
        customLevel.subtitle = "FORTRESS ARCHITECT SANDBOX";
        customLevel.pigQueue = {PigType::TROTTER, PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::GLIDER};

        if (in.good()) {
            std::string line;
            while (std::getline(in, line)) {
                if (line.rfind("theme = ", 0) == 0) {
                    editorTheme = std::atoi(line.substr(8).c_str());
                } else if (line.rfind("wind = ", 0) == 0) {
                    editorWind = std::atof(line.substr(7).c_str());
                    customLevel.wind = editorWind;
                } else if (line.rfind("storm = ", 0) == 0) {
                    editorStorm = (std::atoi(line.substr(8).c_str()) != 0);
                    customLevel.hasStorm = editorStorm;
                } else if (line.rfind("truffle.exists = ", 0) == 0) {
                    customLevel.hasTruffle = (std::atoi(line.substr(17).c_str()) != 0);
                } else if (line.rfind("truffle.x = ", 0) == 0) {
                    customLevel.trufflePos.x = std::atof(line.substr(12).c_str());
                } else if (line.rfind("truffle.y = ", 0) == 0) {
                    customLevel.trufflePos.y = std::atof(line.substr(12).c_str());
                } else if (line.rfind("block = ", 0) == 0) {
                    std::stringstream ss(line.substr(8));
                    int matInt; float bx, by, bw, bh;
                    if (ss >> matInt >> bx >> by >> bw >> bh) {
                        customLevel.blocks.push_back(makeBlock(static_cast<MaterialTier>(matInt), bx, by, bw, bh));
                    }
                } else if (line.rfind("bird = ", 0) == 0) {
                    std::stringstream ss(line.substr(7));
                    int kInt, helmInt; float tx, ty;
                    if (ss >> kInt >> tx >> ty >> helmInt) {
                        BirdTarget bt{};
                        bt.kind = static_cast<BirdClass>(kInt);
                        bt.pos = {tx, ty};
                        bt.radius = (bt.kind == BirdClass::HERON) ? 23.0f : ((bt.kind == BirdClass::OWL) ? 21.0f : 18.0f);
                        bt.hp = (bt.kind == BirdClass::HERON) ? 160.0f : ((bt.kind == BirdClass::OWL) ? 110.0f : 60.0f);
                        bt.maxHp = bt.hp;
                        bt.hasHelmet = (helmInt != 0);
                        if (bt.hasHelmet) { bt.helmetHp = 100.0f; bt.maxHelmetHp = 100.0f; }
                        customLevel.birds.push_back(bt);
                    }
                } else if (line.rfind("pendulum = ", 0) == 0) {
                    std::stringstream ss(line.substr(11));
                    float ax, ay, len; int spk;
                    if (ss >> ax >> ay >> len >> spk) {
                        PendulumBody pb{};
                        pb.anchor = {ax, ay};
                        pb.length = len;
                        pb.spiked = (spk != 0);
                        customLevel.pendulums.push_back(pb);
                    }
                }
            }
        } else {
            editorTheme = (slot - 1) % 6;
            editorWind = 0.0f;
            editorStorm = false;
            customLevel.wind = 0.0f;
            customLevel.hasStorm = false;
            customLevel.blocks.push_back(makeBlock(MaterialTier::STONE, 780.0f, GROUND_Y - 11.0f, 160.0f, 22.0f));
            customLevel.blocks.push_back(makeBlock(MaterialTier::TIMBER, 725.0f, GROUND_Y - 55.0f, 20.0f, 80.0f));
            customLevel.blocks.push_back(makeBlock(MaterialTier::TIMBER, 835.0f, GROUND_Y - 55.0f, 20.0f, 80.0f));
            customLevel.blocks.push_back(makeBlock(MaterialTier::TIMBER, 780.0f, GROUND_Y - 105.0f, 150.0f, 22.0f));

            BirdTarget bt1{};
            bt1.kind = BirdClass::TOUCAN;
            bt1.pos = {780.0f, GROUND_Y - 38.0f};
            bt1.radius = 18.0f; bt1.hp = 60.0f; bt1.maxHp = 60.0f;
            customLevel.birds.push_back(bt1);

            BirdTarget bt2{};
            bt2.kind = BirdClass::OWL;
            bt2.pos = {780.0f, GROUND_Y - 135.0f};
            bt2.radius = 21.0f; bt2.hp = 110.0f; bt2.maxHp = 110.0f;
            customLevel.birds.push_back(bt2);

            customLevel.hasTruffle = true;
            customLevel.trufflePos = {780.0f, GROUND_Y - 175.0f};
        }
        customBackup = customLevel;
    }

    void initEditor() {
        isTestPlaying = false;
        editorTool = 1;
        editorMat = MaterialTier::TIMBER;
        editorBlockW = 60.0f;
        editorBlockH = 22.0f;
        editorBird = BirdClass::TOUCAN;
        editorBirdHelmet = false;
        editorSpikedPendulum = true;
        editorSnapGrid = true;
        loadCustomLevel(customSlot);
    }

    void startEditorTestPlay() {
        customBackup = customLevel;
        customLevel.pigQueue = {PigType::TROTTER, PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::GLIDER, PigType::GRAVITON, PigType::DRILL};
        customLevel.name = "CUSTOM SIEGE PLAYTEST";
        customLevel.subtitle = "FORTRESS SANDBOX PROVING GROUND";
        isTestPlaying = true;
        loadCustomLevelSpec(customLevel);
    }

    void returnToEditor() {
        isTestPlaying = false;
        customLevel = customBackup;
        activePigs.clear();
        particles.clear();
        floatScores.clear();
        aerialBombs.clear();
        simSpeed = 1.0f;
        levelComplete = false;
        levelFailed = false;
    }

    void generateEndlessWave(int wave) {
        endlessWave = wave;
        LevelSpec spec;
        spec.name = "ENDLESS SIEGE GAUNTLET";
        spec.subtitle = "WAVE " + std::to_string(wave) + "  |  DEFEND THE SNOUT!";
        spec.wind = (wave >= 3) ? (((wave % 5) - 2) * 20.0f) : 0.0f;
        spec.hasStorm = (wave >= 5 && (wave % 4 == 0));
        spec.hasTruffle = (wave % 5 == 0);
        spec.trufflePos = {800.0f, 320.0f};

        spec.pigQueue.clear();
        spec.pigQueue.push_back(PigType::TROTTER);
        if (wave >= 2) spec.pigQueue.push_back(PigType::TURBO);
        if (wave >= 3) spec.pigQueue.push_back(PigType::SPLIT);
        if (wave >= 4) spec.pigQueue.push_back(PigType::BOMB);
        if (wave >= 5) spec.pigQueue.push_back(PigType::GLIDER);
        if (wave >= 6) spec.pigQueue.push_back(PigType::DRILL);
        if (wave >= 7) spec.pigQueue.push_back(PigType::GRAVITON);
        if (wave >= 8) spec.pigQueue.push_back((wave % 2 == 0) ? PigType::DRILL : PigType::GRAVITON);
        if (wave >= 10) spec.pigQueue.push_back(PigType::BOMB);

        int numBays = std::min(3, 1 + (wave / 5));
        float startX = 680.0f - (numBays - 1) * 75.0f;
        float baySpacing = 160.0f;

        MaterialTier wallMat = (wave <= 2) ? MaterialTier::TWIG :
                               (wave <= 5) ? MaterialTier::TIMBER :
                               (wave <= 9) ? MaterialTier::STONE :
                               ((wave % 2 == 0) ? MaterialTier::ICE : MaterialTier::STONE);

        for (int b = 0; b < numBays; ++b) {
            float bayX = startX + b * baySpacing;
            int floors = std::min(3, 1 + ((wave + b) / 4));

            spec.blocks.push_back(makeBlock(MaterialTier::STONE, bayX, GROUND_Y - 11.0f, 130.0f, 22.0f));

            float curY = GROUND_Y - 22.0f;
            for (int f = 0; f < floors; ++f) {
                float pillarH = 75.0f;
                spec.blocks.push_back(makeBlock(wallMat, bayX - 45.0f, curY - pillarH * 0.5f, 20.0f, pillarH));
                spec.blocks.push_back(makeBlock(wallMat, bayX + 45.0f, curY - pillarH * 0.5f, 20.0f, pillarH));

                if (f == 0 && wave >= 4 && (b % 2 == 1)) {
                    spec.blocks.push_back(makeBlock(MaterialTier::TNT, bayX, curY - 18.0f, 36.0f, 36.0f));
                } else {
                    BirdTarget bt{};
                    bt.kind = (wave >= 9 && f >= 2) ? BirdClass::CONDOR :
                              ((wave >= 6 && f >= 1) ? BirdClass::FALCON :
                              ((f >= 2 || wave >= 8) ? BirdClass::HERON :
                              ((f >= 1 || wave >= 4) ? BirdClass::OWL : BirdClass::TOUCAN)));
                    bt.radius = (bt.kind == BirdClass::CONDOR) ? 28.0f :
                                ((bt.kind == BirdClass::HERON) ? 26.0f :
                                ((bt.kind == BirdClass::FALCON) ? 23.0f :
                                ((bt.kind == BirdClass::OWL) ? 23.0f : 20.0f)));
                    bt.hp = (bt.kind == BirdClass::CONDOR) ? 240.0f :
                            ((bt.kind == BirdClass::HERON) ? 210.0f :
                            ((bt.kind == BirdClass::FALCON) ? 150.0f :
                            ((bt.kind == BirdClass::OWL) ? 140.0f : 78.0f)));
                    bt.maxHp = bt.hp;
                    bt.pos = {bayX, curY - bt.radius - 2.0f};
                    if (bt.kind == BirdClass::FALCON) {
                        bt.hasShield = true;
                        bt.shieldHp = 120.0f;
                        bt.maxShieldHp = 120.0f;
                    }
                    if (bt.kind == BirdClass::CONDOR) {
                        bt.inZeppelin = true;
                        bt.balloonHp = 70.0f;
                        bt.maxBalloonHp = 70.0f;
                    }
                    if (wave >= 6 && (f == floors - 1 || wave >= 10) && bt.kind != BirdClass::CONDOR) {
                        bt.hasHelmet = true;
                        bt.helmetHp = 100.0f;
                        bt.maxHelmetHp = 100.0f;
                    }
                    spec.birds.push_back(bt);
                }

                curY -= pillarH;
                MaterialTier roofMat = (wave >= 8 && f == floors - 1) ? MaterialTier::STONE : MaterialTier::TIMBER;
                spec.blocks.push_back(makeBlock(roofMat, bayX, curY - 11.0f, 125.0f, 22.0f));
                curY -= 22.0f;
            }
        }

        if (wave >= 8) {
            PendulumBody pb;
            pb.anchor = {startX + (numBays * 0.5f) * baySpacing, 120.0f};
            pb.length = 135.0f;
            pb.spiked = true;
            pb.radius = 24.0f;
            pb.mass = 4.5f;
            spec.pendulums.push_back(pb);
        }

        loadCustomLevelSpec(spec);
    }

    void startEndlessMode() {
        screen = ScreenState::ENDLESS;
        endlessWave = 1;
        endlessScore = 0;
        endlessComboMultiplier = 1.0f;
        endlessGameOver = false;
        generateEndlessWave(1);
    }

    int getHighestUnlockedLevel() const {
        for (int i = 0; i < NUM_CAMPAIGN_LEVELS; ++i) {
            if (bestStars[i] == 0) return i;
        }
        return NUM_CAMPAIGN_LEVELS - 1;
    }

    bool isLevelUnlocked(int idx) const {
        if (idx == 0) return true;
        if (idx < 0 || idx >= NUM_LEVELS) return false;
        if (idx >= NUM_CAMPAIGN_LEVELS) {
            int vaultIdx = idx - NUM_CAMPAIGN_LEVELS;
            return getTrufflesFoundCount() >= getVaultTruffleReq(vaultIdx);
        }
        return bestStars[idx - 1] > 0;
    }

    bool recordLevelScore(int lvlIdx, int newScore, int stars) {
        if (lvlIdx < 0 || lvlIdx >= NUM_LEVELS) return false;
        bool isNewRecord = (newScore > bestScores[lvlIdx]);
        bestScores[lvlIdx] = std::max(bestScores[lvlIdx], newScore);
        bestStars[lvlIdx] = std::max(bestStars[lvlIdx], stars);
        saveProgress();
        return isNewRecord;
    }

    int getTotalStars() const {
        int total = 0;
        for (int s : bestStars) total += s;
        return total;
    }

    int getLevelsCleared() const {
        int cleared = 0;
        for (int s : bestStars) if (s > 0) cleared++;
        return cleared;
    }

    PigBody createPigInstance(PigType type, Vec2 pos) {
        PigBody p{};
        p.type = type;
        p.pos = pos;
        if (type == PigType::TROTTER) {
            p.radius = 21.0f;
            p.mass = 2.8f;
            p.restitution = 0.36f;
        } else if (type == PigType::TURBO) {
            p.radius = 20.0f;
            p.mass = 2.4f;
            p.restitution = 0.32f;
        } else if (type == PigType::SPLIT) {
            p.radius = 17.0f;
            p.mass = 1.7f;
            p.restitution = 0.44f;
        } else if (type == PigType::GLIDER) {
            p.radius = 18.0f;
            p.mass = 2.0f;
            p.restitution = 0.38f;
        } else if (type == PigType::GRAVITON) {
            p.radius = 23.0f;
            p.mass = 3.4f;
            p.restitution = 0.28f;
        } else if (type == PigType::DRILL) {
            p.radius = 19.0f;
            p.mass = 3.6f;
            p.restitution = 0.15f;
        } else {
            p.radius = 24.0f;
            p.mass = 3.8f;
            p.restitution = 0.25f;
        }
        p.invMass = 1.0f / p.mass;
        p.active = true;
        p.launched = false;
        return p;
    }

    void spawnNextPigInSlingshot() {
        simSpeed = 1.0f;
        if (!currentFlightTrail.empty()) {
            ghostTrajectory = currentFlightTrail;
            currentFlightTrail.clear();
        }
        if (remainingPigs.empty()) return;
        PigType nextType = remainingPigs.front();
        remainingPigs.erase(remainingPigs.begin());
        activePigs.clear();
        activePigs.push_back(createPigInstance(nextType, {SLING_X, SLING_Y}));
        slingPullPos = {SLING_X, SLING_Y};
    }

    void launchCurrentPig(Vec2 pullVector) {
        if (activePigs.empty() || activePigs[0].launched) return;
        float pullLen = pullVector.length();
        if (pullLen < 12.0f) {
            slingPullPos = {SLING_X, SLING_Y};
            return;
        }
        if (pullLen > MAX_PULL) {
            pullVector = pullVector.normalized() * MAX_PULL;
        }
        PigBody& pig = activePigs[0];
        pig.pos = {SLING_X, SLING_Y};
        pig.vel = pullVector * (-LAUNCH_POWER);
        pig.launched = true;
        pig.restTimer = 0.0f;
        pig.lifeTimer = 0.0f;
        isManualPanning = false;
        manualPanTimer = 0.0f;
        slingRecoilTimer = 0.35f;
        lastLaunchDir = (pullVector.lengthSq() > 1.0f) ? (pullVector * -1.0f).normalized() : Vec2(1.0f, 0.0f);
        currentFlightTrail.clear();
        currentFlightTrail.push_back({SLING_X, SLING_Y});
        addTrauma(0.12f);
        assets.playSound("sound.launch");

        shotBlocksDestroyed = 0;
        shotTntDetonated = 0;
        shotHitTrampoline = false;
        shotUsedAnvil = false;
        shotDrillPierces = 0;
        shotLaunchPos = {SLING_X, SLING_Y};
    }

    void activateMidAirAbility() {
        if (activePigs.empty()) return;
        PigBody& mainPig = activePigs[0];
        if (!mainPig.launched || mainPig.abilityUsed || !mainPig.active) return;
        mainPig.abilityUsed = true;

        if (mainPig.type == PigType::TROTTER) {
            shotUsedAnvil = true;
            mainPig.vel.x *= 0.22f;
            mainPig.vel.y = 820.0f;
            mainPig.mass *= 1.85f;
            mainPig.invMass = 1.0f / mainPig.mass;
            spawnBurst(mainPig.pos, 18, {238, 155, 130, 255}, 210.0f);
            floatScores.push_back({mainPig.pos, "ANVIL SLAM!", {255, 200, 150, 255}, 1.0f, 1.0f});
            assets.playSound("sound.slam");
            addTrauma(0.65f);
        } else if (mainPig.type == PigType::TURBO) {
            float spd = mainPig.vel.length();
            Vec2 dir = (spd > 10.0f) ? mainPig.vel.normalized() : Vec2(1.0f, -0.1f).normalized();
            mainPig.vel = dir * std::max(880.0f, spd * 2.45f);
            spawnBurst(mainPig.pos, 24, {255, 208, 70, 255}, 260.0f);
            floatScores.push_back({mainPig.pos, "TURBO CHARGE!", {255, 225, 90, 255}, 1.0f, 1.0f});
            assets.playSound("sound.dash");
            addTrauma(0.45f);
        } else if (mainPig.type == PigType::SPLIT) {
            float baseAngle = std::atan2(mainPig.vel.y, mainPig.vel.x);
            float speed = std::max(520.0f, mainPig.vel.length() * 1.08f);
            float offsets[2] = {-0.23f, +0.23f};
            for (float off : offsets) {
                PigBody clone = mainPig;
                clone.radius = 15.0f;
                clone.abilityUsed = true;
                clone.pos = mainPig.pos + Vec2(0.0f, off * 45.0f);
                clone.vel = {std::cos(baseAngle + off) * speed, std::sin(baseAngle + off) * speed};
                activePigs.push_back(clone);
            }
            spawnBurst(mainPig.pos, 20, {110, 235, 185, 255}, 200.0f);
            floatScores.push_back({mainPig.pos, "TRI-SQUADRON!", {130, 250, 200, 255}, 1.0f, 1.0f});
            assets.playSound("sound.split");
            addTrauma(0.28f);
        } else if (mainPig.type == PigType::BOMB) {
            triggerBombDetonation(mainPig);
        } else if (mainPig.type == PigType::GLIDER) {
            // Gale the Aeronaut: Aerodynamic Glide Lift & Twin Iron Aerial Bomb Release
            mainPig.vel.y = -45.0f; // lift against gravity
            mainPig.vel.x = std::max(mainPig.vel.x * 1.25f, 320.0f); // forward glide surge
            spawnBurst(mainPig.pos, 18, {180, 230, 255, 255}, 220.0f);
            floatScores.push_back({mainPig.pos, "AERO GLIDE!", {160, 235, 255, 255}, 1.0f, 1.0f});
            assets.playSound("sound.glide");
            addTrauma(0.35f);

            // Drop Twin Iron Aerial Bombs
            AerialBomb b1;
            b1.pos = mainPig.pos + Vec2(-10.0f, 12.0f);
            b1.vel = Vec2(mainPig.vel.x * 0.25f - 25.0f, 75.0f);
            b1.radius = 9.0f;
            b1.blastRadius = 95.0f;
            b1.damage = 120.0f;
            b1.active = true;

            AerialBomb b2;
            b2.pos = mainPig.pos + Vec2(10.0f, 12.0f);
            b2.vel = Vec2(mainPig.vel.x * 0.35f + 25.0f, 55.0f);
            b2.radius = 9.0f;
            b2.blastRadius = 95.0f;
            b2.damage = 120.0f;
            b2.active = true;

            aerialBombs.push_back(b1);
            aerialBombs.push_back(b2);
        } else if (mainPig.type == PigType::GRAVITON) {
            // Atticus the Graviton Boar: Graviton Vortex ability pulling blocks and birds inward
            mainPig.vortexTimer = 1.35f;
            spawnBurst(mainPig.pos, 22, {185, 110, 255, 255}, 220.0f);
            floatScores.push_back({mainPig.pos, "GRAVITON VORTEX!", {210, 140, 255, 255}, 1.1f, 1.1f});
            assets.playSound("sound.vortex");
            addTrauma(0.50f);
        } else if (mainPig.type == PigType::DRILL) {
            // Rusty the Drill Hog: Supersonic rotary bore straight through fortress blocks
            mainPig.drillTimer = 1.8f;
            float spd = mainPig.vel.length();
            Vec2 dir = (spd > 10.0f) ? mainPig.vel.normalized() : Vec2(1.0f, 0.0f);
            mainPig.vel = dir * std::max(780.0f, spd * 1.85f);
            spawnBurst(mainPig.pos, 20, {255, 180, 50, 255}, 260.0f);
            floatScores.push_back({mainPig.pos, "SUPERSONIC BORE!", {255, 195, 75, 255}, 1.1f, 1.1f});
            assets.playSound("sound.drill", 0.85f);
            addTrauma(0.55f);
        }
    }

    void triggerGravitonImplosion(Vec2 center) {
        fortressAwake = true;
        const float blastRadius = 180.0f;
        Particle ring{};
        ring.pos = center;
        ring.isRing = true;
        ring.ringRadius = 16.0f;
        ring.ringMaxRadius = blastRadius;
        ring.life = 0.45f;
        ring.maxLife = 0.45f;
        ring.color = {175, 95, 255, 255};
        particles.push_back(ring);

        spawnBurst(center, 36, {190, 110, 255, 255}, 360.0f);
        spawnBurst(center, 24, {70, 230, 255, 255}, 280.0f);
        floatScores.push_back({center, "SINGULARITY!", {210, 140, 255, 255}, 1.2f, 1.2f});
        assets.playSound("sound.vortex");
        assets.playSound("sound.explosion");
        addTrauma(0.80f);

        int targetsHit = 0;
        for (auto& b : blocks) {
            if (b.destroyed) continue;
            Vec2 delta = b.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                b.vel += dir * (600.0f * falloff);
                b.angVel += 12.0f * falloff;
                damageBlock(b, 140.0f * falloff);
                targetsHit++;
            }
        }
        for (auto& t : birds) {
            if (t.defeated) continue;
            Vec2 delta = t.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                t.vel += dir * (560.0f * falloff);
                damageBird(t, 160.0f * falloff);
                targetsHit++;
            }
        }
        if (targetsHit >= 3) {
            achievementProgress[static_cast<size_t>(AchievementId::VORTEX_MAESTRO)]++;
            saveProgress();
            if (achievementProgress[static_cast<size_t>(AchievementId::VORTEX_MAESTRO)] >= 1) {
                unlockAchievement(AchievementId::VORTEX_MAESTRO);
            }
        }
    }

    void triggerBombDetonation(PigBody& bombPig) {
        if (bombPig.exploded) return;
        fortressAwake = true;
        bombPig.exploded = true;
        bombPig.abilityUsed = true;
        Vec2 center = bombPig.pos;
        const float blastRadius = 172.0f;

        Particle ring{};
        ring.pos = center;
        ring.isRing = true;
        ring.ringRadius = 16.0f;
        ring.ringMaxRadius = blastRadius;
        ring.life = 0.45f;
        ring.maxLife = 0.45f;
        ring.color = {255, 165, 60, 255};
        particles.push_back(ring);

        spawnBurst(center, 42, {255, 130, 45, 255}, 380.0f);
        spawnBurst(center, 24, {90, 95, 110, 255}, 260.0f);
        floatScores.push_back({center, "KABOOM!", {255, 140, 60, 255}, 1.2f, 1.2f});
        assets.playSound("sound.explosion");
        addTrauma(0.85f);

        int blocksBefore = shotBlocksDestroyed;
        for (auto& b : blocks) {
            if (b.destroyed) continue;
            Vec2 delta = b.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                b.vel += dir * (680.0f * falloff * b.invMass);
                b.angVel += ((b.pos.x > center.x) ? 1.0f : -1.0f) * 6.0f * falloff;
                if (b.mat == MaterialTier::TNT) {
                    if (!b.ignited) {
                        b.ignited = true;
                        b.tntFuse = 0.08f + ((b.pos.x > center.x) ? 0.04f : 0.01f);
                    }
                } else {
                    damageBlock(b, 240.0f * falloff);
                }
            }
        }

        int blocksAfter = shotBlocksDestroyed;
        if (blocksAfter - blocksBefore >= 5) {
            unlockAchievement(AchievementId::SEISMIC_ANNIHILATION);
        }

        if (levelTruffle.exists && !levelTruffle.collected) {
            if ((levelTruffle.pos - center).length() < blastRadius) {
                collectGoldenTruffle();
            }
        }

        for (auto& t : birds) {
            if (t.defeated) continue;
            Vec2 delta = t.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                t.vel += dir * (580.0f * falloff);
                damageBird(t, 240.0f * falloff);
            }
        }
    }

    void detonateAerialBomb(AerialBomb& ab) {
        if (!ab.active) return;
        ab.active = false;
        fortressAwake = true;
        Vec2 center = ab.pos;
        float blastRadius = ab.blastRadius;

        Particle ring{};
        ring.pos = center;
        ring.isRing = true;
        ring.ringRadius = 10.0f;
        ring.ringMaxRadius = blastRadius;
        ring.life = 0.35f;
        ring.maxLife = 0.35f;
        ring.color = {255, 180, 70, 255};
        particles.push_back(ring);

        spawnBurst(center, 28, {255, 140, 50, 255}, 320.0f);
        spawnBurst(center, 16, {100, 110, 125, 255}, 220.0f);
        floatScores.push_back({center, "AIR STRIKE!", {255, 190, 80, 255}, 1.1f, 1.1f});
        assets.playSound("sound.explosion");
        addTrauma(0.55f);

        for (auto& b : blocks) {
            if (b.destroyed) continue;
            Vec2 delta = b.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                b.vel += dir * (540.0f * falloff * b.invMass);
                b.angVel += ((b.pos.x > center.x) ? 1.0f : -1.0f) * 4.5f * falloff;
                if (b.mat == MaterialTier::TNT) {
                    if (!b.ignited) {
                        b.ignited = true;
                        b.tntFuse = 0.05f;
                    }
                } else {
                    damageBlock(b, ab.damage * falloff);
                }
            }
        }

        if (levelTruffle.exists && !levelTruffle.collected) {
            if ((levelTruffle.pos - center).length() < blastRadius) {
                collectGoldenTruffle();
            }
        }

        for (auto& t : birds) {
            if (t.defeated) continue;
            Vec2 delta = t.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                t.vel += dir * (480.0f * falloff);
                damageBird(t, ab.damage * falloff);
            }
        }
    }

    void triggerTntDetonation(BlockBody& tntBlock) {
        if (tntBlock.destroyed) return;
        tntBlock.destroyed = true;
        tntBlock.ignited = false;
        fortressAwake = true;
        Vec2 center = tntBlock.pos;
        const float blastRadius = 210.0f;

        shotTntDetonated++;
        if (shotTntDetonated >= 3) {
            unlockAchievement(AchievementId::POWDER_MAESTRO);
        }

        if (levelTruffle.exists && !levelTruffle.collected) {
            if ((levelTruffle.pos - center).length() < blastRadius) {
                collectGoldenTruffle();
            }
        }

        Particle ring{};
        ring.pos = center;
        ring.isRing = true;
        ring.ringRadius = 18.0f;
        ring.ringMaxRadius = blastRadius;
        ring.life = 0.48f;
        ring.maxLife = 0.48f;
        ring.color = {255, 145, 45, 255};
        particles.push_back(ring);

        spawnBurst(center, 46, {255, 120, 35, 255}, 420.0f);
        spawnBurst(center, 28, {255, 215, 60, 255}, 310.0f);
        spawnSplinters(center, MaterialTier::TNT, 16, 280.0f);

        score += 2500;
        floatScores.push_back({center, "BOOM! +2500", {255, 180, 70, 255}, 1.35f, 1.35f});
        assets.playSound("sound.explosion");
        addTrauma(0.92f);

        for (auto& b : blocks) {
            if (b.destroyed) continue;
            Vec2 delta = b.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                b.vel += dir * (760.0f * falloff * b.invMass);
                b.angVel += ((b.pos.x > center.x) ? 1.0f : -1.0f) * 7.5f * falloff;
                if (b.mat == MaterialTier::TNT) {
                    if (!b.ignited) {
                        b.ignited = true;
                        b.tntFuse = 0.08f + ((b.pos.x > center.x) ? 0.04f : 0.01f);
                    }
                } else {
                    damageBlock(b, 280.0f * falloff);
                }
            }
        }

        for (auto& t : birds) {
            if (t.defeated) continue;
            Vec2 delta = t.pos - center;
            float dist = delta.length();
            if (dist < blastRadius) {
                float falloff = 1.0f - (dist / blastRadius);
                Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
                t.vel += dir * (650.0f * falloff);
                damageBird(t, 280.0f * falloff);
            }
        }
    }

    void spawnBurst(Vec2 pos, int count, SDL_Color col, float maxSpeed) {
        for (int i = 0; i < count; ++i) {
            float a = (2.0f * PI * i) / count + (i % 3) * 0.17f;
            float spd = maxSpeed * (0.35f + 0.65f * ((i * 37) % 100) / 100.0f);
            Particle pt{};
            pt.pos = pos;
            pt.vel = {std::cos(a) * spd, std::sin(a) * spd - 60.0f};
            pt.size = 4.0f + (i % 5) * 1.5f;
            pt.life = 0.45f + (i % 4) * 0.14f;
            pt.maxLife = pt.life;
            pt.color = col;
            pt.kind = ParticleKind::GENERIC;
            particles.push_back(pt);
        }
    }

    void spawnSplinters(Vec2 pos, MaterialTier mat, int count, float maxSpeed) {
        for (int i = 0; i < count; ++i) {
            float a = (2.0f * PI * i) / count + ((i * 31) % 100) * 0.015f;
            float spd = maxSpeed * (0.40f + 0.60f * ((i * 47) % 100) / 100.0f);
            Particle pt{};
            pt.pos = pos + Vec2((i % 5 - 2) * 4.0f, (i % 3 - 1) * 4.0f);
            pt.vel = {std::cos(a) * spd, std::sin(a) * spd - 90.0f};
            pt.angVel = ((i % 2 == 0) ? 1.0f : -1.0f) * (8.0f + (i % 7) * 2.5f);
            pt.angle = (i * 0.7f);
            pt.life = 0.65f + (i % 5) * 0.16f;
            pt.maxLife = pt.life;

            if (mat == MaterialTier::TWIG) {
                // Wicker / Twig: thin dry reeds and straw splinters
                pt.kind = ParticleKind::WOOD_SPLINTER;
                pt.size = 4.5f + (i % 4) * 1.4f;
                int cVar = i % 3;
                pt.color = (cVar == 0) ? SDL_Color{235, 198, 118, 255} :
                           (cVar == 1) ? SDL_Color{204, 166, 92, 255} : SDL_Color{245, 222, 160, 255};
            } else if (mat == MaterialTier::TIMBER) {
                // Timber: mixture of flying wood splinters and chunky oak blocks
                pt.kind = (i % 3 == 0) ? ParticleKind::TIMBER_CHUNK : ParticleKind::WOOD_SPLINTER;
                pt.size = 5.5f + (i % 5) * 1.8f;
                int cVar = i % 3;
                pt.color = (cVar == 0) ? SDL_Color{174, 102, 54, 255} :
                           (cVar == 1) ? SDL_Color{128, 72, 38, 255} : SDL_Color{224, 168, 102, 255};
            } else if (mat == MaterialTier::STONE) {
                // Stone: heavy tumbling slate shards & basalt pebbles
                pt.kind = ParticleKind::STONE_SHARD;
                pt.size = 5.0f + (i % 5) * 2.0f;
                int cVar = i % 3;
                pt.color = (cVar == 0) ? SDL_Color{118, 130, 148, 255} :
                           (cVar == 1) ? SDL_Color{72, 80, 96, 255} : SDL_Color{172, 184, 202, 255};
            } else if (mat == MaterialTier::ICE) {
                // Glacial Frost Ice: crystalline diamond shards
                pt.kind = ParticleKind::ICE_CRYSTAL;
                pt.size = 5.0f + (i % 4) * 2.2f;
                int cVar = i % 3;
                pt.color = (cVar == 0) ? SDL_Color{180, 240, 255, 255} :
                           (cVar == 1) ? SDL_Color{225, 250, 255, 255} : SDL_Color{125, 212, 248, 255};
            } else {
                // TNT: charred wood splinters & ember spark fragments
                pt.kind = (i % 2 == 0) ? ParticleKind::TIMBER_CHUNK : ParticleKind::SPARK_GLINT;
                pt.size = 4.5f + (i % 4) * 1.6f;
                int cVar = i % 3;
                pt.color = (cVar == 0) ? SDL_Color{224, 52, 42, 255} :
                           (cVar == 1) ? SDL_Color{255, 210, 68, 255} : SDL_Color{64, 58, 62, 255};
            }
            particles.push_back(pt);
        }

        // Add 2-3 expanding masonry dust / sawdust puffs
        for (int p = 0; p < 3; ++p) {
            Particle puff{};
            puff.pos = pos + Vec2((p - 1) * 8.0f, (p % 2) * -6.0f);
            puff.vel = Vec2((p - 1) * 22.0f, -32.0f - p * 12.0f);
            puff.size = 11.0f + p * 4.0f;
            puff.life = 0.55f + p * 0.12f;
            puff.maxLife = puff.life;
            puff.kind = ParticleKind::SMOKE_PUFF;
            puff.color = (mat == MaterialTier::STONE) ? SDL_Color{165, 175, 192, 150} :
                         (mat == MaterialTier::TIMBER ? SDL_Color{198, 168, 134, 150} :
                         (mat == MaterialTier::ICE    ? SDL_Color{205, 235, 250, 150} :
                         (mat == MaterialTier::TNT    ? SDL_Color{85, 80, 85, 180} :
                                                        SDL_Color{218, 202, 164, 150})));
            particles.push_back(puff);
        }
    }

    void spawnFeatherBurst(Vec2 pos, BirdClass kind, int count, float maxSpeed) {
        for (int i = 0; i < count; ++i) {
            float a = (2.0f * PI * i) / count + ((i * 29) % 100) * 0.02f;
            float spd = maxSpeed * (0.35f + 0.65f * ((i * 53) % 100) / 100.0f);
            Particle pt{};
            pt.pos = pos + Vec2((i % 7 - 3) * 3.0f, (i % 5 - 2) * 3.0f);
            pt.vel = {std::cos(a) * spd, std::sin(a) * spd - 110.0f};
            pt.size = 5.0f + (i % 4) * 1.5f;
            pt.life = 1.15f + (i % 6) * 0.22f;
            pt.maxLife = pt.life;
            pt.kind = ParticleKind::FEATHER;
            pt.wobbleFreq = 10.0f + (i % 5) * 2.2f;
            pt.wobblePhase = i * 1.37f;

            if (kind == BirdClass::TOUCAN) {
                int cVar = i % 4;
                pt.color = (cVar == 0) ? SDL_Color{46, 178, 214, 255} :
                           (cVar == 1) ? SDL_Color{28, 126, 185, 255} :
                           (cVar == 2) ? SDL_Color{248, 172, 52, 255} : SDL_Color{224, 72, 56, 255};
            } else if (kind == BirdClass::OWL) {
                int cVar = i % 4;
                pt.color = (cVar == 0) ? SDL_Color{128, 92, 176, 255} :
                           (cVar == 1) ? SDL_Color{64, 68, 122, 255} :
                           (cVar == 2) ? SDL_Color{245, 212, 78, 255} : SDL_Color{192, 202, 235, 255};
            } else {
                int cVar = i % 4;
                pt.color = (cVar == 0) ? SDL_Color{224, 48, 62, 255} :
                           (cVar == 1) ? SDL_Color{245, 248, 255, 255} :
                           (cVar == 2) ? SDL_Color{255, 214, 64, 255} : SDL_Color{182, 32, 44, 255};
            }
            particles.push_back(pt);
        }

        // Add 3-4 golden sparkle glints
        for (int s = 0; s < 4; ++s) {
            Particle sp{};
            sp.pos = pos + Vec2((s - 1.5f) * 16.0f, (s % 2 - 0.5f) * 14.0f);
            sp.vel = Vec2((s - 1.5f) * 35.0f, -50.0f - s * 15.0f);
            sp.size = 6.0f + s * 1.5f;
            sp.life = 0.75f + s * 0.15f;
            sp.maxLife = sp.life;
            sp.kind = ParticleKind::SPARK_GLINT;
            sp.color = {255, 238, 110, 255};
            particles.push_back(sp);
        }

        // Plus expanding feather cloud puff
        for (int p = 0; p < 2; ++p) {
            Particle puff{};
            puff.pos = pos + Vec2((p * 2 - 1) * 6.0f, -4.0f);
            puff.vel = Vec2((p * 2 - 1) * 18.0f, -28.0f);
            puff.size = 14.0f + p * 5.0f;
            puff.life = 0.65f;
            puff.maxLife = puff.life;
            puff.kind = ParticleKind::SMOKE_PUFF;
            puff.color = {235, 240, 250, 140};
            particles.push_back(puff);
        }
    }

    void eruptMagmaVent(BlockBody& b) {
        assets.playSound("sound.magma");
        addTrauma(0.24f);
        for (int i = 0; i < 18; ++i) {
            Particle em{};
            em.pos = Vec2(b.pos.x + ((i % 5) - 2) * 5.0f, b.pos.y - b.h * 0.5f - 2.0f);
            float angle = -PI * 0.5f + ((i % 7) - 3) * 0.16f;
            float spd = 130.0f + (i % 6) * 35.0f;
            em.vel = Vec2(std::cos(angle) * spd, std::sin(angle) * spd);
            em.size = 5.0f + (i % 4) * 2.0f;
            em.life = 1.4f + (i % 5) * 0.25f;
            em.maxLife = em.life;
            em.kind = ParticleKind::EMBER;
            em.wobbleFreq = 8.0f + (i % 4) * 2.0f;
            em.wobblePhase = i * 0.8f;
            int cVar = i % 3;
            em.color = (cVar == 0) ? SDL_Color{255, 230, 80, 255} :
                       (cVar == 1) ? SDL_Color{255, 120, 30, 255} : SDL_Color{255, 60, 20, 255};
            particles.push_back(em);
        }
    }

    void triggerLightningStrike() {
        Vec2 strikeTarget{WINDOW_W * 0.72f, GROUND_Y};
        float highestY = GROUND_Y;
        BlockBody* targetBlock = nullptr;
        BirdTarget* targetBird = nullptr;

        for (auto& t : birds) {
            if (!t.defeated && t.hasHelmet && t.pos.y < highestY) {
                highestY = t.pos.y;
                strikeTarget = t.pos;
                targetBird = &t;
                targetBlock = nullptr;
            }
        }
        if (!targetBird) {
            for (auto& b : blocks) {
                if (!b.destroyed && (b.mat == MaterialTier::STONE) && b.pos.y < highestY) {
                    highestY = b.pos.y;
                    strikeTarget = b.pos;
                    targetBlock = &b;
                }
            }
        }
        if (!targetBird && !targetBlock) {
            for (auto& b : blocks) {
                if (!b.destroyed && b.pos.y < highestY) {
                    highestY = b.pos.y;
                    strikeTarget = b.pos;
                    targetBlock = &b;
                }
            }
        }

        LightningBolt bolt{};
        bolt.life = 0.24f;
        bolt.maxLife = 0.24f;
        bolt.width = 4.0f;
        bolt.color = SDL_Color{235, 248, 255, 255};
        Vec2 pt(strikeTarget.x + (std::sin(SDL_GetTicks() * 0.01f) * 45.0f), 45.0f);
        bolt.points.push_back(pt);
        int numSegs = 7;
        for (int s = 1; s < numSegs; ++s) {
            float frac = static_cast<float>(s) / numSegs;
            Vec2 basePt = pt + (strikeTarget - pt) * frac;
            float jitter = std::sin((s * 4.3f) + SDL_GetTicks() * 0.02f) * 26.0f;
            bolt.points.push_back(Vec2(basePt.x + jitter, basePt.y));
        }
        bolt.points.push_back(strikeTarget);
        lightningBolts.push_back(bolt);

        skyFlashAlpha = 0.85f;
        addTrauma(0.55f);
        assets.playSound("sound.thunder");

        if (targetBird) {
            damageBird(*targetBird, 65.0f);
        } else if (targetBlock) {
            damageBlock(*targetBlock, 75.0f);
        }

        // Electrostatic spark explosion at strike point
        for (int s = 0; s < 14; ++s) {
            Particle sp{};
            sp.pos = strikeTarget;
            float a = (PI * 2.0f * s) / 14.0f;
            float spd = 120.0f + (s % 4) * 35.0f;
            sp.vel = Vec2(std::cos(a) * spd, std::sin(a) * spd - 40.0f);
            sp.size = 5.0f + (s % 3);
            sp.life = 0.55f;
            sp.maxLife = 0.55f;
            sp.kind = ParticleKind::SPARK_GLINT;
            sp.color = {255, 245, 140, 255};
            particles.push_back(sp);
        }
    }

    void damageBlock(BlockBody& b, float dmg) {
        if (b.destroyed || dmg <= 2.5f) return;
        if (b.mat == MaterialTier::MAGMA_VENT) {
            eruptMagmaVent(b);
            // Thermal shockwave melts adjacent ice and ignites adjacent TNT (< 85px)
            for (auto& other : blocks) {
                if (other.destroyed || &other == &b) continue;
                float d = (other.pos - b.pos).length();
                if (d < 85.0f) {
                    if (other.mat == MaterialTier::ICE) damageBlock(other, 99.0f);
                    if (other.mat == MaterialTier::TNT && !other.ignited) {
                        other.ignited = true;
                        other.tntFuse = 0.08f;
                    }
                }
            }
            return;
        }
        b.hp -= dmg;
        if (b.mat == MaterialTier::TNT) {
            if (!b.ignited) {
                b.ignited = true;
                b.tntFuse = (b.hp <= 0.0f) ? 0.05f : 0.14f;
            } else if (b.hp <= 0.0f && b.tntFuse > 0.05f) {
                b.tntFuse = 0.05f;
            }
            return;
        }
        if (b.hp <= 0.0f) {
            b.destroyed = true;
            shotBlocksDestroyed++;
            addTrauma(b.mat == MaterialTier::STONE ? 0.32f :
                     (b.mat == MaterialTier::TIMBER ? 0.20f :
                     (b.mat == MaterialTier::ICE ? 0.18f : 0.12f)));
            int pts = (b.mat == MaterialTier::TWIG) ? 500 :
                      (b.mat == MaterialTier::TIMBER ? 900 :
                      (b.mat == MaterialTier::STONE ? 1500 : 600)); // ICE = 600
            score += pts;
            spawnSplinters(b.pos, b.mat, (b.mat == MaterialTier::STONE ? 20 : (b.mat == MaterialTier::ICE ? 22 : 16)), 240.0f);
            SDL_Color debCol = (b.mat == MaterialTier::TWIG) ? SDL_Color{245, 218, 128, 255} :
                               (b.mat == MaterialTier::TIMBER ? SDL_Color{236, 178, 112, 255} :
                               (b.mat == MaterialTier::STONE ? SDL_Color{195, 215, 238, 255} :
                                                               SDL_Color{160, 240, 255, 255})); // ICE
            floatScores.push_back({b.pos, "+" + std::to_string(pts), debCol, 0.95f, 0.95f});
            assets.playSound(b.mat == MaterialTier::TWIG ? "sound.hit_twig" :
                            (b.mat == MaterialTier::TIMBER ? "sound.hit_timber" :
                            (b.mat == MaterialTier::STONE ? "sound.hit_stone" : "sound.hit_ice")));
        } else if (dmg > 10.0f) {
            spawnSplinters(b.pos, b.mat, 4, 110.0f);
            if (dmg > 45.0f) addTrauma(0.08f);
        }
    }

    void damageBird(BirdTarget& t, float dmg) {
        if (t.defeated || dmg <= 2.0f) return;
        if (t.hasShield && t.shieldHp > 0.0f) {
            float absorbed = std::min(t.shieldHp, dmg);
            t.shieldHp -= absorbed;
            dmg -= absorbed;
            if (screen == ScreenState::ENDLESS && absorbed > 0.0f) {
                endlessScore += static_cast<int>(absorbed * 10.0f * endlessComboMultiplier);
                endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.15f);
            }
            assets.playSound("sound.shield");
            for (int s = 0; s < 6; ++s) {
                Particle sp{};
                sp.pos = t.pos + Vec2(-t.radius * 0.9f, (s - 2.5f) * 6.0f);
                sp.vel = Vec2(-100.0f - s * 20.0f, (s - 2.5f) * 50.0f);
                sp.size = 5.0f + s;
                sp.life = 0.5f;
                sp.maxLife = 0.5f;
                sp.kind = ParticleKind::SPARK_GLINT;
                sp.color = {255, 235, 120, 255};
                particles.push_back(sp);
            }
            if (t.shieldHp <= 0.0f) {
                t.hasShield = false;
                t.shieldHp = 0.0f;
                if (screen == ScreenState::ENDLESS) {
                    endlessScore += static_cast<int>(1200 * endlessComboMultiplier);
                    endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.3f);
                } else {
                    score += 1200;
                }
                floatScores.push_back({t.pos + Vec2(-t.radius, 0.0f), "+1200 SHIELD SHATTERED!", {240, 205, 90, 255}, 1.3f, 1.3f});
                assets.playSound("sound.shield");
                for (int p = 0; p < 4; ++p) {
                    Particle sPart{};
                    sPart.pos = t.pos + Vec2(-t.radius, (p - 1.5f) * 12.0f);
                    sPart.vel = Vec2(-120.0f - p * 35.0f, -140.0f + p * 60.0f);
                    sPart.size = t.radius * 0.8f;
                    sPart.angle = 0.0f;
                    sPart.angVel = 8.0f + p * 2.0f;
                    sPart.life = 1.8f;
                    sPart.maxLife = 1.8f;
                    sPart.kind = ParticleKind::SHIELD_DEBRIS;
                    sPart.color = {120, 130, 150, 255};
                    particles.push_back(sPart);
                }
            }
            if (dmg <= 2.0f) return;
        }

        if (t.inZeppelin && !t.balloonPopped && t.balloonHp > 0.0f) {
            float absorbed = std::min(t.balloonHp, dmg);
            t.balloonHp -= absorbed;
            dmg -= absorbed;
            if (t.balloonHp <= 0.0f) {
                t.balloonHp = 0.0f;
                t.balloonPopped = true;
                t.inZeppelin = false;
                t.vel.y = 320.0f;
                t.mass = 4.8f;
                t.invMass = 1.0f / t.mass;
                if (screen == ScreenState::ENDLESS) {
                    endlessScore += static_cast<int>(1500 * endlessComboMultiplier);
                    endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.35f);
                } else {
                    score += 1500;
                }
                floatScores.push_back({t.pos + Vec2(0.0f, -t.radius * 1.8f), "+1500 ZEPPELIN DOWNED!", {255, 175, 75, 255}, 1.4f, 1.4f});
                assets.playSound("sound.zeppelin");
                for (int b = 0; b < 6; ++b) {
                    Particle bPart{};
                    bPart.pos = t.pos + Vec2((b - 2.5f) * 12.0f, -t.radius * 1.8f);
                    bPart.vel = Vec2((b - 2.5f) * 55.0f, -90.0f - b * 15.0f);
                    bPart.size = t.radius * 0.7f;
                    bPart.angle = 0.0f;
                    bPart.angVel = 5.0f + b;
                    bPart.life = 2.2f;
                    bPart.maxLife = 2.2f;
                    bPart.kind = ParticleKind::BALLOON_SCRAP;
                    bPart.color = {185, 140, 90, 255};
                    particles.push_back(bPart);
                }
            }
            if (dmg <= 2.0f) return;
        }

        if (t.hasHelmet && t.helmetHp > 0.0f) {
            float absorbed = std::min(t.helmetHp, dmg);
            t.helmetHp -= absorbed;
            dmg -= absorbed;
            if (screen == ScreenState::ENDLESS && absorbed > 0.0f) {
                endlessScore += static_cast<int>(absorbed * 10.0f * endlessComboMultiplier);
                endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.1f);
            }
            assets.playSound("sound.hit_metal");
            // Metallic sparks on kettle impact
            for (int s = 0; s < 5; ++s) {
                Particle sp{};
                sp.pos = t.pos + Vec2((s - 2.0f) * 6.0f, -t.radius * 0.7f);
                sp.vel = Vec2((s - 2.0f) * 45.0f, -70.0f - (s % 3) * 20.0f);
                sp.size = 5.0f + s;
                sp.life = 0.5f + s * 0.1f;
                sp.maxLife = sp.life;
                sp.kind = ParticleKind::SPARK_GLINT;
                sp.color = {255, 220, 90, 255};
                particles.push_back(sp);
            }
            if (t.helmetHp <= 0.0f) {
                t.hasHelmet = false;
                t.helmetHp = 0.0f;
                if (screen == ScreenState::ENDLESS) {
                    endlessScore += static_cast<int>(1000 * endlessComboMultiplier);
                    endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.25f);
                } else {
                    score += 1000;
                }
                floatScores.push_back({t.pos + Vec2(0.0f, -t.radius * 0.8f), "+1000 HELMET SHATTERED!", {255, 230, 110, 255}, 1.3f, 1.3f});
                assets.playSound("sound.hit_metal");
                // Launch tumbling HELMET_DEBRIS piece
                Particle helmPart{};
                helmPart.pos = t.pos + Vec2(0.0f, -t.radius * 0.6f);
                helmPart.vel = Vec2((dmg > 0.0f ? 60.0f : -60.0f), -220.0f);
                helmPart.angle = 0.0f;
                helmPart.angVel = 12.0f;
                helmPart.size = t.radius * 1.3f;
                helmPart.life = 2.0f;
                helmPart.maxLife = 2.0f;
                helmPart.kind = ParticleKind::HELMET_DEBRIS;
                helmPart.color = {140, 150, 165, 255};
                particles.push_back(helmPart);

                // Achievement: CAULDRON_CRACKER (Knock off 10 enemy bird metal helmets)
                achievementProgress[static_cast<size_t>(AchievementId::CAULDRON_CRACKER)]++;
                saveProgress();
                if (achievementProgress[static_cast<size_t>(AchievementId::CAULDRON_CRACKER)] >= 10) {
                    unlockAchievement(AchievementId::CAULDRON_CRACKER);
                }
            }
            if (dmg <= 2.0f) return;
        }
        t.hp -= dmg;
        if (screen == ScreenState::ENDLESS && dmg > 0.0f) {
            endlessScore += static_cast<int>(dmg * 10.0f * endlessComboMultiplier);
            endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.1f);
        }
        if (t.hp <= 0.0f) {
            t.defeated = true;
            addTrauma(0.42f);
            int pts = (t.kind == BirdClass::CONDOR) ? 15000 :
                      (t.kind == BirdClass::FALCON ? 12500 :
                      (t.kind == BirdClass::HERON ? 10000 :
                      (t.kind == BirdClass::OWL ? 7500 : 5000)));
            if (screen == ScreenState::ENDLESS) {
                endlessScore += pts * endlessComboMultiplier;
                endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.5f);
            } else {
                score += pts;
            }
            spawnFeatherBurst(t.pos, t.kind, 28, 260.0f);
            floatScores.push_back({t.pos, "+" + std::to_string(pts), {255, 235, 110, 255}, 1.4f, 1.4f});
            assets.playSound("sound.bird_defeat");

            // Achievement: FIRST_BLOOD (Defeat your first bird enemy)
            unlockAchievement(AchievementId::FIRST_BLOOD);

            // Achievement: MARKSMAN (Hit a bird enemy from over 500 pixels away with a slingshot shot)
            if ((t.pos - shotLaunchPos).length() > 500.0f) {
                unlockAchievement(AchievementId::MARKSMAN);
            }

            // Achievement: ANVIL_EXECUTIONER (Crush a bird directly using Trotter's Anvil Slam ability)
            if (shotUsedAnvil) {
                unlockAchievement(AchievementId::ANVIL_EXECUTIONER);
            }

            // Achievement: ACROBATIC_HOG (Defeat any bird with a pig after bouncing off a trampoline)
            if (shotHitTrampoline) {
                unlockAchievement(AchievementId::ACROBATIC_HOG);
            }

            // Feature Pack E: Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
            bool allDefeated = true;
            for (const auto& otherBird : birds) {
                if (!otherBird.defeated) {
                    allDefeated = false;
                    break;
                }
            }
            if (allDefeated && enableSloMoCam && !sloMoActive) {
                sloMoActive = true;
                sloMoTimer = 1.35f;
                sloMoFocusPos = t.pos;
            }
        } else if (dmg > 12.0f) {
            spawnFeatherBurst(t.pos, t.kind, 5, 120.0f);
            if (dmg > 30.0f) addTrauma(0.15f);
        }
    }

    // Helper: Get the 4 world-space vertices of an oriented BlockBody
    static std::array<Vec2, 4> getBlockVerts(const BlockBody& b) {
        float c = std::cos(b.angle), s = std::sin(b.angle);
        float hw = b.w * 0.5f, hh = b.h * 0.5f;
        Vec2 ux(c, s), uy(-s, c);
        return {
            b.pos - ux * hw - uy * hh,
            b.pos + ux * hw - uy * hh,
            b.pos + ux * hw + uy * hh,
            b.pos - ux * hw + uy * hh
        };
    }

    static float getTargetFlatAngle(const BlockBody& b) {
        if (b.w >= b.h) {
            if (b.angle > PI * 0.5f) return PI;
            if (b.angle < -PI * 0.5f) return -PI;
            return 0.0f;
        } else {
            if (std::abs(b.angle) < 0.22f) return 0.0f;
            return (b.angle >= 0.0f) ? (PI * 0.5f) : (-PI * 0.5f);
        }
    }

    static float getTiltFromStable(const BlockBody& b) {
        return b.angle - getTargetFlatAngle(b);
    }

    // Computes bottom-up grounded support propagation from GROUND_Y through the block graph
    void updateGroundedSupportGraph(float subDt) {
        const size_t nBlocks = blocks.size();
        std::vector<float> bMinX(nBlocks, 1e9f), bMaxX(nBlocks, -1e9f);
        std::vector<float> bMinY(nBlocks, 1e9f), bMaxY(nBlocks, -1e9f);
        std::vector<float> supMinX(nBlocks, 1e9f), supMaxX(nBlocks, -1e9f);

        for (size_t i = 0; i < nBlocks; ++i) {
            blocks[i].hasGroundPath = false;
            blocks[i].stablySupported = false;
            if (blocks[i].destroyed) continue;
            auto verts = getBlockVerts(blocks[i]);
            for (const Vec2& v : verts) {
                bMinX[i] = std::min(bMinX[i], v.x);
                bMaxX[i] = std::max(bMaxX[i], v.x);
                bMinY[i] = std::min(bMinY[i], v.y);
                bMaxY[i] = std::max(bMaxY[i], v.y);
                if (v.y >= GROUND_Y - 3.5f) {
                    supMinX[i] = std::min(supMinX[i], v.x);
                    supMaxX[i] = std::max(supMaxX[i], v.x);
                }
            }
            if (supMinX[i] <= supMaxX[i]) {
                blocks[i].hasGroundPath = true;
                float tiltErr = std::abs(getTiltFromStable(blocks[i]));
                float margin = 2.5f;
                float eMin = supMinX[i] + margin;
                float eMax = supMaxX[i] - margin;
                if (eMin <= eMax && blocks[i].pos.x >= eMin && blocks[i].pos.x <= eMax && tiltErr < 0.12f) {
                    blocks[i].stablySupported = true;
                }
            }
        }

        // Propagate support bottom-up across 5 tiers
        for (int pass = 0; pass < 5; ++pass) {
            for (size_t i = 0; i < nBlocks; ++i) {
                if (blocks[i].destroyed) continue;
                for (size_t j = 0; j < nBlocks; ++j) {
                    if (i == j || blocks[j].destroyed) continue;
                    if (!blocks[j].hasGroundPath) continue;
                    if (blocks[j].pos.y <= blocks[i].pos.y + 2.0f) continue;
                    if (std::abs(bMaxY[i] - bMinY[j]) < 10.0f) {
                        float ovMinX = std::max(bMinX[i], bMinX[j]);
                        float ovMaxX = std::min(bMaxX[i], bMaxX[j]);
                        if (ovMaxX > ovMinX) {
                            blocks[i].hasGroundPath = true;
                            if (blocks[j].stablySupported) {
                                supMinX[i] = std::min(supMinX[i], ovMinX);
                                supMaxX[i] = std::max(supMaxX[i], ovMaxX);
                            }
                        }
                    }
                }
                if (supMinX[i] <= supMaxX[i]) {
                    float tiltErr = std::abs(getTiltFromStable(blocks[i]));
                    float margin = 2.5f;
                    float eMin = supMinX[i] + margin;
                    float eMax = supMaxX[i] - margin;
                    if (eMin <= eMax && blocks[i].pos.x >= eMin && blocks[i].pos.x <= eMax && tiltErr < 0.12f) {
                        blocks[i].stablySupported = true;
                    }
                }
            }
        }

        // Apply overhanging torque or freefall acceleration to non-stably-supported blocks
        for (size_t i = 0; i < nBlocks; ++i) {
            BlockBody& B = blocks[i];
            if (B.destroyed) continue;
            float tiltErr = getTiltFromStable(B);
            if (!B.hasGroundPath) {
                if (B.vel.y < 0.0f) B.vel.y *= 0.5f;
            } else if (!B.stablySupported) {
                if (supMinX[i] <= supMaxX[i]) {
                    float pivotX = (B.pos.x < supMinX[i]) ? supMinX[i] :
                                   (B.pos.x > supMaxX[i]  ? supMaxX[i] : B.pos.x);
                    float leverX = B.pos.x - pivotX;
                    if (std::abs(leverX) > 1.0f) {
                        B.angVel += clampf(leverX * 0.28f, -9.5f, 9.5f) * subDt;
                    }
                }
                if (std::abs(tiltErr) > 0.06f) {
                    float flatTorque = -clampf(tiltErr * 11.0f, -8.5f, 8.5f);
                    B.angVel += flatTorque * subDt;
                    if (std::abs(tiltErr) > 0.20f && std::abs(B.angVel) < 0.9f) {
                        damageBlock(B, 95.0f * subDt);
                    }
                }
                B.vel.y += 110.0f * subDt;
            } else {
                // Stably supported: high-grip horizontal damping + sleep threshold
                B.vel.x *= 0.90f;
                if (std::abs(B.vel.x) < 10.0f) B.vel.x = 0.0f;
                if (std::abs(B.angVel) < 0.25f && std::abs(tiltErr) <= 0.06f) {
                    B.angVel = 0.0f;
                    B.angle = getTargetFlatAngle(B);
                }
            }
        }
    }

    void stepPhysics(float dt) {
        // Feature Pack C: Atmospheric Hazards & Dynamic Weather
        windGustTimer += dt;
        if (currentLevelIdx >= 0 && currentLevelIdx < static_cast<int>(levels.size())) {
            float baseW = levels[currentLevelIdx].wind;
            if (baseW != 0.0f) {
                currentWind = baseW + std::sin(windGustTimer * 1.6f) * 6.0f;
            } else {
                currentWind = 0.0f;
            }
        }

        // Drifting storm clouds
        for (auto& sc : stormClouds) {
            sc.pos.x += sc.speed * dt;
            if (sc.pos.x > WINDOW_W + sc.width * 0.5f) {
                sc.pos.x = -sc.width * 0.5f;
            }
        }

        // Lightning decay and bolts update
        if (skyFlashAlpha > 0.0f) {
            skyFlashAlpha = std::max(0.0f, skyFlashAlpha - 2.8f * dt);
        }
        for (auto& lb : lightningBolts) {
            lb.life -= dt;
        }
        lightningBolts.erase(std::remove_if(lightningBolts.begin(), lightningBolts.end(),
                             [](const LightningBolt& b) { return b.life <= 0.0f; }),
                             lightningBolts.end());

        // Check if any pig is actively launched in the air
        bool anyPigLaunched = false;
        for (const auto& p : activePigs) {
            if (p.launched) { anyPigLaunched = true; break; }
        }

        if (currentLevelIdx >= 0 && currentLevelIdx < static_cast<int>(levels.size()) && levels[currentLevelIdx].hasStorm) {
            if (anyPigLaunched) {
                stormStrikeTimer -= dt;
                if (stormStrikeTimer <= 0.0f) {
                    triggerLightningStrike();
                    stormStrikeTimer = 7.0f + ((SDL_GetTicks() % 40) * 0.1f);
                }
            }
        }

        const int subSteps = 10;
        float subDt = dt / subSteps;

        for (int step = 0; step < subSteps; ++step) {
            // 1. Integrate Pigs (always active once launched)
            for (auto& p : activePigs) {
                if (!p.active || !p.launched) continue;
                // Atmospheric wind acceleration on dynamic airborne swine
                if (p.pos.y + p.radius < GROUND_Y - 2.0f) {
                    p.vel.x += (currentWind * 0.85f) * subDt;
                }
                p.vel.y += GRAVITY * subDt;
                p.pos += p.vel * subDt;
                p.angle += p.angVel * subDt;
                p.lifeTimer += subDt;

                if (p.fuseTimer > 0.0f) {
                    p.fuseTimer -= subDt;
                    if (p.fuseTimer <= 0.0f && !p.exploded) {
                        triggerBombDetonation(p);
                    }
                }

                if (p.vortexTimer > 0.0f) {
                    p.vortexTimer -= subDt;
                    fortressAwake = true;
                    float vortexRadius = 240.0f;
                    for (auto& b : blocks) {
                        if (b.destroyed) continue;
                        Vec2 toPig = p.pos - b.pos;
                        float d = toPig.length();
                        if (d > 10.0f && d < vortexRadius) {
                            float factor = (1.0f - d / vortexRadius);
                            Vec2 pull = toPig.normalized() * (450.0f * factor * subDt);
                            b.vel += pull;
                        }
                    }
                    for (auto& bt : birds) {
                        if (bt.defeated) continue;
                        Vec2 toPig = p.pos - bt.pos;
                        float d = toPig.length();
                        if (d > 10.0f && d < vortexRadius) {
                            float factor = (1.0f - d / vortexRadius);
                            Vec2 pull = toPig.normalized() * (380.0f * factor * subDt);
                            bt.vel += pull;
                        }
                    }
                    if ((rand() % 4) == 0) {
                        Particle cm{};
                        float a = (rand() % 360) * (PI / 180.0f);
                        float dist = 30.0f + (rand() % 120);
                        cm.pos = p.pos + Vec2(std::cos(a) * dist, std::sin(a) * dist);
                        cm.vel = Vec2(-std::sin(a) * 120.0f - std::cos(a) * 80.0f,
                                      std::cos(a) * 120.0f - std::sin(a) * 80.0f);
                        cm.size = 5.0f;
                        cm.life = 0.5f;
                        cm.maxLife = 0.5f;
                        cm.kind = ParticleKind::COSMIC_MOTE;
                        cm.color = {190, 110, 255, 255};
                        particles.push_back(cm);
                    }
                    if (p.vortexTimer <= 0.0f) {
                        triggerGravitonImplosion(p.pos);
                    }
                }

                if (p.drillTimer > 0.0f) {
                    p.drillTimer -= subDt;
                    if (p.pos.y >= GROUND_Y - p.radius - 4.0f) {
                        fortressAwake = true;
                        p.vel.x *= 0.94f;
                        for (auto& b : blocks) {
                            if (b.destroyed) continue;
                            if (std::abs(b.pos.x - p.pos.x) < 95.0f && b.pos.y > GROUND_Y - 95.0f) {
                                damageBlock(b, 60.0f * subDt);
                                b.vel.y += 35.0f * subDt;
                            }
                        }
                        if ((rand() % 3) == 0) {
                            spawnBurst(p.pos, 3, {160, 120, 75, 255}, 120.0f);
                        }
                    }
                }

                if (p.pos.y + p.radius > GROUND_Y) {
                    p.pos.y = GROUND_Y - p.radius;
                    if (p.vel.y > 0.0f) {
                        p.vel.y = -p.vel.y * p.restitution;
                        p.vel.x *= 0.80f;
                        p.angVel = p.vel.x / std::max(10.0f, p.radius);
                    }
                }
                if (p.pos.x < -120.0f || p.pos.x > WINDOW_W + 160.0f) {
                    p.active = false;
                }
            }

            // 2. Check Pig vs Block & Pig vs Bird collisions (wakes fortress upon first contact!)
            for (auto& p : activePigs) {
                if (!p.active || !p.launched) continue;
                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    resolveCircleOBB(p, b);
                }
                for (auto& t : birds) {
                    if (t.defeated) continue;
                    Vec2 d = t.pos - p.pos;
                    float dist = d.length();
                    float minDist = p.radius + t.radius;
                    if (dist < minDist) {
                        fortressAwake = true;
                        Vec2 n = (dist > 1e-4f) ? (d * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                        float pen = minDist - dist;
                        p.pos -= n * (pen * 0.4f);
                        t.pos += n * (pen * 0.6f);
                        Vec2 rv = t.vel - p.vel;
                        float vn = rv.dot(n);
                        if (vn < 0.0f) {
                            float e = (t.hasShield && t.shieldHp > 0.0f && p.pos.x <= t.pos.x) ? 0.75f : 0.35f;
                            float j = -(1.0f + e) * vn / (p.invMass + t.invMass);
                            Vec2 imp = n * j;
                            p.vel -= imp * p.invMass;
                            t.vel += imp * t.invMass;
                            damageBird(t, std::abs(j) * 0.28f);
                            addTrauma(0.32f);
                            if (p.type == PigType::BOMB && p.fuseTimer < 0.0f && !p.exploded) {
                                p.fuseTimer = 0.45f;
                            }
                        }
                    }
                }
                for (auto& pend : pendulums) {
                    Vec2 ballPos = pend.getBallPos();
                    Vec2 d = p.pos - ballPos;
                    float dist = d.length();
                    float minDist = p.radius + pend.radius;
                    if (dist < minDist) {
                        fortressAwake = true;
                        Vec2 n = (dist > 1e-4f) ? (d * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                        float pen = minDist - dist;
                        p.pos += n * pen;
                        Vec2 ballVel = pend.getBallVel();
                        Vec2 rv = p.vel - ballVel;
                        float vn = rv.dot(n);
                        if (vn < 0.0f) {
                            float e = 0.65f;
                            float j = -(1.0f + e) * vn / (p.invMass + 1.0f / pend.mass);
                            Vec2 imp = n * j;
                            p.vel += imp * p.invMass;
                            Vec2 rAnchor = ballPos - pend.anchor;
                            float torque = cross(rAnchor, -imp);
                            pend.angVel += torque / (pend.mass * pend.length * pend.length);
                            assets.playSound("sound.hit_metal");
                            addTrauma(0.35f);
                        }
                    }
                }

                // Proximity collection of Golden Truffle by airborne pig
                if (levelTruffle.exists && !levelTruffle.collected) {
                    if ((p.pos - levelTruffle.pos).length() < (p.radius + levelTruffle.radius)) {
                        collectGoldenTruffle();
                    }
                }
            }

            // Integrate Aerial Bombs
            for (auto& ab : aerialBombs) {
                if (!ab.active) continue;
                ab.vel.y += GRAVITY * subDt;
                ab.pos += ab.vel * subDt;
                ab.lifeTimer += subDt;

                if (step == 0) {
                    Particle sp{};
                    sp.pos = ab.pos - ab.vel.normalized() * (ab.radius * 0.8f);
                    sp.vel = Vec2(-ab.vel.x * 0.1f, -15.0f);
                    sp.size = 3.5f;
                    sp.life = 0.28f;
                    sp.maxLife = 0.28f;
                    sp.kind = ParticleKind::SPARK_GLINT;
                    sp.color = {255, 205, 80, 220};
                    particles.push_back(sp);
                }

                if (ab.pos.y + ab.radius >= GROUND_Y) {
                    detonateAerialBomb(ab);
                    continue;
                }

                for (auto& t : birds) {
                    if (t.defeated) continue;
                    if ((t.pos - ab.pos).length() < t.radius + ab.radius) {
                        detonateAerialBomb(ab);
                        break;
                    }
                }
                if (!ab.active) continue;

                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    Vec2 local = ab.pos - b.pos;
                    float c = std::cos(-b.angle), s = std::sin(-b.angle);
                    Vec2 rot(local.x * c - local.y * s, local.x * s + local.y * c);
                    float hx = b.w * 0.5f, hy = b.h * 0.5f;
                    float cx = clampf(rot.x, -hx, hx);
                    float cy = clampf(rot.y, -hy, hy);
                    if ((rot - Vec2(cx, cy)).lengthSq() < ab.radius * ab.radius) {
                        detonateAerialBomb(ab);
                        break;
                    }
                }
            }

            // 3. Before any pig hits the fortress, keep the fortress in 100% static equilibrium
            //    so blocks and perched birds never drift, sink, or rotate a single pixel!
            if (!fortressAwake) {
                continue;
            }

            // Moving Block vs Golden Truffle check
            if (levelTruffle.exists && !levelTruffle.collected) {
                for (const auto& b : blocks) {
                    if (b.destroyed) continue;
                    if (b.vel.lengthSq() > 100.0f) {
                        float r = std::max(b.w, b.h) * 0.5f;
                        if ((b.pos - levelTruffle.pos).length() < (r + levelTruffle.radius)) {
                            collectGoldenTruffle();
                            break;
                        }
                    }
                }
            }

            // 4. Integrate Blocks under Gravity & Update Burning TNT Fuses
            for (size_t bi = 0; bi < blocks.size(); ++bi) {
                BlockBody& b = blocks[bi];
                if (b.destroyed) continue;
                if (b.mat == MaterialTier::TNT && b.ignited && b.tntFuse > 0.0f) {
                    b.tntFuse -= subDt;
                    if (b.tntFuse <= 0.0f) {
                        triggerTntDetonation(b);
                        continue;
                    }
                }
                b.vel.y += GRAVITY * subDt;
                b.pos += b.vel * subDt;
                b.angle += b.angVel * subDt;
                if (b.angle > PI) b.angle -= 2.0f * PI;
                if (b.angle < -PI) b.angle += 2.0f * PI;
                b.vel.x *= 0.992f;
                b.vel.y *= 0.998f;
                b.angVel *= 0.985f;
            }

            // 5. Integrate Birds under Gravity
            for (auto& t : birds) {
                if (t.defeated) continue;
                if (t.inZeppelin && !t.balloonPopped) {
                    t.vel.y += (GRAVITY * 0.15f) * subDt;
                } else {
                    t.vel.y += GRAVITY * subDt;
                }
                t.pos += t.vel * subDt;
                t.angle += t.angVel * subDt;
                t.vel.x *= 0.985f;
                t.angVel *= 0.92f;
            }

            // 5b. Integrate Pendulums under Harmonic Gravity & Collide with Blocks and Birds
            for (auto& pend : pendulums) {
                float alpha = (-GRAVITY / pend.length) * std::sin(pend.angle) - 0.25f * pend.angVel;
                pend.angVel += alpha * subDt;
                pend.angle += pend.angVel * subDt;

                Vec2 ballPos = pend.getBallPos();
                Vec2 ballVel = pend.getBallVel();
                float ballSpeed = ballVel.length();

                // Pendulum ball vs Blocks (destructive wrecking ball impact)
                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    float c = std::cos(-b.angle), s = std::sin(-b.angle);
                    Vec2 rel = ballPos - b.pos;
                    Vec2 local(rel.x * c - rel.y * s, rel.x * s + rel.y * c);
                    float hw = b.w * 0.5f, hh = b.h * 0.5f;
                    Vec2 closest(clampf(local.x, -hw, hw), clampf(local.y, -hh, hh));
                    Vec2 diff = local - closest;
                    float distSq = diff.lengthSq();
                    if (distSq < pend.radius * pend.radius) {
                        float dist = std::sqrt(std::max(1e-6f, distSq));
                        Vec2 localN = diff * (1.0f / dist);
                        float wc = std::cos(b.angle), ws = std::sin(b.angle);
                        Vec2 worldN(localN.x * wc - localN.y * ws, localN.x * ws + localN.y * wc);
                        float pen = pend.radius - dist;
                        b.pos -= worldN * (pen * 0.7f);
                        float dmg = (ballSpeed * 0.45f + 25.0f) * (pend.spiked ? 2.5f : 1.5f);
                        damageBlock(b, dmg);
                        if (ballSpeed > 40.0f) {
                            assets.playSound(pend.spiked ? "sound.hit_metal" : "sound.hit_stone");
                            addTrauma(0.25f);
                        }
                        Vec2 rAnchor = ballPos - pend.anchor;
                        Vec2 imp = worldN * (dmg * 1.5f);
                        float torque = cross(rAnchor, imp);
                        pend.angVel += torque / (pend.mass * pend.length * pend.length);
                    }
                }

                // Pendulum ball vs Birds
                for (auto& t : birds) {
                    if (t.defeated) continue;
                    Vec2 d = t.pos - ballPos;
                    float dist = d.length();
                    float minDist = pend.radius + t.radius;
                    if (dist < minDist) {
                        Vec2 n = (dist > 1e-4f) ? (d * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                        float pen = minDist - dist;
                        t.pos += n * pen;
                        float dmg = (ballSpeed * 0.65f + 45.0f) * (pend.spiked ? 2.2f : 1.4f);
                        damageBird(t, dmg);
                        addTrauma(0.35f);
                        assets.playSound("sound.hit_metal");
                    }
                }
            }

            // 6. Update Bottom-Up Grounded Support Graph
            updateGroundedSupportGraph(subDt);

            // 7. Iterative Constraint Solver (Ground -> Block-Block -> Ground -> Bird-Block -> Bird-Ground)
            for (int iter = 0; iter < 6; ++iter) {
                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    resolveBlockGround(b, true, subDt);
                }

                for (size_t i = 0; i < blocks.size(); ++i) {
                    if (blocks[i].destroyed) continue;
                    for (size_t j = i + 1; j < blocks.size(); ++j) {
                        if (blocks[j].destroyed) continue;
                        resolveBlockBlock(blocks[i], blocks[j], true, subDt);
                    }
                }

                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    resolveBlockGround(b, false, subDt);
                }

                for (auto& t : birds) {
                    if (t.defeated) continue;
                    for (auto& b : blocks) {
                        if (b.destroyed) continue;
                        resolveBirdBlock(t, b, true, subDt);
                    }
                }

                for (auto& t : birds) {
                    if (t.defeated) continue;
                    if (t.pos.y + t.radius > GROUND_Y) {
                        float impactSpeed = t.vel.length();
                        t.pos.y = GROUND_Y - t.radius;
                        if (t.vel.y > 0.0f) {
                            if (iter == 0 && impactSpeed > 85.0f) {
                                damageBird(t, (impactSpeed - 85.0f) * 0.85f);
                            }
                            t.vel.y = -t.vel.y * 0.12f;
                            t.vel.x *= 0.72f;
                            if (std::abs(t.vel.x) < 12.0f) {
                                t.vel.x = 0.0f;
                                t.angVel = 0.0f;
                            } else {
                                t.angVel = t.vel.x / std::max(10.0f, t.radius);
                            }
                        }
                    }
                }
            }
        }

        for (auto& p : activePigs) {
            if (p.active && p.launched && p.lifeTimer < 4.5f) {
                if (p.trail.empty() || (p.pos - p.trail.back()).lengthSq() > 220.0f) {
                    p.trail.push_back(p.pos);
                    if (p.trail.size() > 28) p.trail.erase(p.trail.begin());
                }
                // Record flight arc for ghost trajectory memory
                if (currentFlightTrail.empty() || (p.pos - currentFlightTrail.back()).lengthSq() > 320.0f) {
                    if (currentFlightTrail.size() < 45) {
                        currentFlightTrail.push_back(p.pos);
                    }
                }
                // Pig faces forward along trajectory in mid-air
                if (p.vel.lengthSq() > 400.0f && p.pos.y + p.radius < GROUND_Y - 4.0f) {
                    p.angle = std::atan2(p.vel.y, p.vel.x);
                }
            }
        }

        for (auto& pt : particles) {
            pt.life -= dt;
            if (pt.isRing) {
                float pRatio = 1.0f - (pt.life / pt.maxLife);
                pt.ringRadius = 16.0f + pRatio * (pt.ringMaxRadius - 16.0f);
            } else if (pt.kind == ParticleKind::FEATHER) {
                // Feathers float and flutter with gentle air drag + wind push
                pt.vel.x += currentWind * 1.2f * dt;
                pt.vel.x *= (1.0f - 1.8f * dt);
                pt.vel.y += 110.0f * dt;
                if (pt.vel.y > 68.0f) pt.vel.y = 68.0f;
                pt.pos += pt.vel * dt;
                pt.pos.x += std::sin(pt.life * pt.wobbleFreq + pt.wobblePhase) * 48.0f * dt;
                pt.angle = std::sin(pt.life * pt.wobbleFreq + pt.wobblePhase) * 0.55f;
            } else if (pt.kind == ParticleKind::SMOKE_PUFF) {
                // Soft smoke puffs drift upward and expand + drift with wind
                pt.vel.x += currentWind * 0.7f * dt;
                pt.vel.x *= (1.0f - 1.4f * dt);
                pt.vel.y -= 14.0f * dt;
                pt.pos += pt.vel * dt;
            } else if (pt.kind == ParticleKind::EMBER) {
                // Volcanic embers rise upward with reverse gravity, sinusoidal sway, and wind
                pt.vel.y -= 95.0f * dt; // buoyant upward rise
                pt.vel.x += currentWind * 0.8f * dt;
                pt.vel.x *= (1.0f - 1.2f * dt);
                pt.pos += pt.vel * dt;
                pt.pos.x += std::sin(pt.life * pt.wobbleFreq + pt.wobblePhase) * 22.0f * dt;

                // Embers melt ICE and ignite TNT on contact!
                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    if (b.mat == MaterialTier::ICE || b.mat == MaterialTier::TNT) {
                        float hw = b.w * 0.5f, hh = b.h * 0.5f;
                        if (std::abs(pt.pos.x - b.pos.x) < hw + 4.0f && std::abs(pt.pos.y - b.pos.y) < hh + 4.0f) {
                            if (b.mat == MaterialTier::ICE) {
                                damageBlock(b, 20.0f); // melts ice!
                                pt.life = 0.0f; // quench ember
                                break;
                            } else if (b.mat == MaterialTier::TNT && !b.ignited) {
                                b.ignited = true;
                                b.tntFuse = 0.08f; // ignites TNT!
                                pt.life = 0.0f;
                                break;
                            }
                        }
                    }
                }
            } else if (pt.kind == ParticleKind::GOLD_SPARKLE) {
                // Radiating golden sparkle with slight atmospheric drag
                pt.vel *= (1.0f - 1.6f * dt);
                pt.pos += pt.vel * dt;
                pt.angle += pt.angVel * dt;
            } else if (pt.kind == ParticleKind::BALLOON_SCRAP) {
                pt.vel.x += currentWind * 0.9f * dt;
                pt.vel.x *= (1.0f - 1.5f * dt);
                pt.vel.y += 85.0f * dt;
                if (pt.vel.y > 55.0f) pt.vel.y = 55.0f;
                pt.pos += pt.vel * dt;
                pt.angle += pt.angVel * dt;
            } else if (pt.kind == ParticleKind::COSMIC_MOTE) {
                pt.vel *= (1.0f - 2.2f * dt);
                pt.pos += pt.vel * dt;
            } else {
                // Debris, splinters, chunks, sparks
                pt.vel.y += GRAVITY * 0.55f * dt;
                pt.pos += pt.vel * dt;
                pt.angle += pt.angVel * dt;
                if (pt.pos.y > GROUND_Y - 2.0f && pt.vel.y > 0.0f) {
                    pt.pos.y = GROUND_Y - 2.0f;
                    pt.vel.y = -pt.vel.y * 0.32f;
                    pt.vel.x *= 0.65f;
                }
            }
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(),
                        [](const Particle& p) { return p.life <= 0.0f; }), particles.end());

        for (auto& fs : floatScores) {
            fs.life -= dt;
            fs.pos.y -= 38.0f * dt;
        }
        floatScores.erase(std::remove_if(floatScores.begin(), floatScores.end(),
                          [](const FloatingScore& f) { return f.life <= 0.0f; }), floatScores.end());

        evaluateTurnAndWinState(dt);
    }

    void resolveBlockGround(BlockBody& b, bool allowDamage, float subDt) {
        auto verts = getBlockVerts(b);
        float maxPen = 0.0f;
        Vec2 contactSum{0.0f, 0.0f};
        int contactCount = 0;

        for (const Vec2& v : verts) {
            float pen = v.y - GROUND_Y;
            if (pen > 0.0f) {
                if (pen > maxPen) maxPen = pen;
                contactSum += v;
                contactCount++;
            }
        }
        if (contactCount == 0) return;

        b.pos.y -= maxPen;
        Vec2 contactPt = contactSum * (1.0f / contactCount);
        contactPt.y = GROUND_Y;

        Vec2 r = contactPt - b.pos;
        Vec2 n(0.0f, -1.0f);
        Vec2 vContact = b.vel + cross(b.angVel, r);
        float vn = vContact.dot(n);

        float tilt = getTiltFromStable(b);

        if (vn < 0.0f) {
            float impactSpeed = -vn;
            if (allowDamage && impactSpeed > 205.0f) {
                damageBlock(b, (impactSpeed - 205.0f) * 0.26f);
                addTrauma(0.16f);
            }
            float rn = cross(r, n);
            float denom = b.invMass + rn * rn * b.invInertia;
            float j = -vn / denom;
            Vec2 imp = n * j;
            b.vel += imp * b.invMass;
            b.angVel += cross(r, imp) * b.invInertia;

            // High ground friction at all angles so blocks grip the ground instead of sliding
            Vec2 tangent(1.0f, 0.0f);
            float vt = (b.vel + cross(b.angVel, r)).dot(tangent);
            float rt = cross(r, tangent);
            float tDenom = b.invMass + rt * rt * b.invInertia;
            float jt = clampf(-vt / tDenom, -j * b.friction, j * b.friction);
            Vec2 fImp = tangent * jt;
            b.vel += fImp * b.invMass;
            b.angVel += cross(r, fImp) * b.invInertia;
            b.vel.x *= 0.88f;
            if (std::abs(b.vel.x) < 10.0f) b.vel.x = 0.0f;
        }

        if (contactCount == 1) {
            if (std::abs(tilt) < 0.08f && std::abs(b.angVel) < 0.6f) {
                b.angle -= tilt * 0.35f;
                b.angVel *= 0.65f;
            } else {
                float flatTorque = -clampf(tilt * 12.0f, -9.0f, 9.0f);
                b.angVel += flatTorque * subDt;
                if (std::abs(tilt) > 0.20f && std::abs(b.angVel) < 0.9f) {
                    damageBlock(b, 95.0f * subDt);
                }
            }
        }
    }

    void resolveCircleOBB(PigBody& p, BlockBody& b) {
        float c = std::cos(-b.angle), s = std::sin(-b.angle);
        Vec2 rel = p.pos - b.pos;
        Vec2 local(rel.x * c - rel.y * s, rel.x * s + rel.y * c);
        float hw = b.w * 0.5f, hh = b.h * 0.5f;
        Vec2 closest(clampf(local.x, -hw, hw), clampf(local.y, -hh, hh));
        Vec2 diff = local - closest;
        float distSq = diff.lengthSq();
        if (distSq >= p.radius * p.radius) return;

        fortressAwake = true;

        float dist = std::sqrt(distSq);
        Vec2 localN;
        float pen = 0.0f;
        if (dist > 1e-4f) {
            localN = diff * (1.0f / dist);
            pen = p.radius - dist;
        } else {
            float dx = hw - std::abs(local.x);
            float dy = hh - std::abs(local.y);
            if (dx < dy) {
                localN = Vec2((local.x >= 0.0f) ? 1.0f : -1.0f, 0.0f);
                pen = p.radius + dx;
            } else {
                localN = Vec2(0.0f, (local.y >= 0.0f) ? 1.0f : -1.0f);
                pen = p.radius + dy;
            }
        }

        float wc = std::cos(b.angle), ws = std::sin(b.angle);
        Vec2 worldN(localN.x * wc - localN.y * ws, localN.x * ws + localN.y * wc);

        if (b.mat == MaterialTier::TRAMPOLINE) {
            shotHitTrampoline = true;
            p.pos += worldN * pen;
            float inSpeed = p.vel.length();
            float vn = p.vel.dot(worldN);
            if (vn < 0.0f) {
                float bounceSpeed = std::max(inSpeed * 1.65f, 420.0f);
                Vec2 refl = p.vel - worldN * (2.0f * vn);
                if (refl.lengthSq() > 1e-4f) {
                    p.vel = refl.normalized() * bounceSpeed;
                } else {
                    p.vel = worldN * bounceSpeed;
                }
                assets.playSound("sound.boing");
                addTrauma(0.22f);
                // Emerald & gold trampoline spark glints
                for (int s = 0; s < 6; ++s) {
                    Particle sp{};
                    sp.pos = p.pos;
                    sp.vel = worldN * 180.0f + Vec2((s - 2.5f) * 40.0f, -60.0f);
                    sp.size = 6.0f + s;
                    sp.life = 0.6f;
                    sp.maxLife = 0.6f;
                    sp.kind = ParticleKind::SPARK_GLINT;
                    sp.color = {100, 255, 160, 255};
                    particles.push_back(sp);
                }
            }
            return;
        }

        Vec2 contactPt = p.pos - worldN * p.radius;

        float totalInvMass = p.invMass + b.invMass;
        p.pos += worldN * (pen * (p.invMass / totalInvMass));
        b.pos -= worldN * (pen * (b.invMass / totalInvMass));

        Vec2 rB = contactPt - b.pos;
        Vec2 vB = b.vel + cross(b.angVel, rB);
        Vec2 rv = p.vel - vB;
        float vn = rv.dot(worldN);
        if (vn >= 0.0f) return;

        float rnB = cross(rB, worldN);
        float denom = p.invMass + b.invMass + (rnB * rnB) * b.invInertia;
        float e = std::min(p.restitution, b.restitution);
        float j = -(1.0f + e) * vn / denom;
        Vec2 impulse = worldN * j;

        float pigPierceFactor = 1.0f;
        float dmgMultiplier = 0.19f;
        if (p.type == PigType::TURBO && p.abilityUsed && b.mat != MaterialTier::STONE) {
            pigPierceFactor = 0.65f;
            dmgMultiplier = 0.35f;
        } else if (p.type == PigType::TROTTER && p.abilityUsed) {
            dmgMultiplier = 0.34f;
        } else if (p.type == PigType::SPLIT && (b.mat == MaterialTier::TWIG || b.mat == MaterialTier::ICE)) {
            dmgMultiplier = 0.38f;
        } else if (p.type == PigType::DRILL && p.drillTimer > 0.0f) {
            pigPierceFactor = 0.22f;
            dmgMultiplier = 0.68f;
            shotDrillPierces++;
            if (shotDrillPierces >= 3) {
                unlockAchievement(AchievementId::DEEP_BORE);
            }
            assets.playSound("sound.drill", 0.55f);
            for (int k = 0; k < 4; ++k) {
                Particle sp{};
                sp.pos = contactPt;
                sp.vel = Vec2(-p.vel.x * 0.4f + (k - 1.5f) * 50.0f, -80.0f - k * 30.0f);
                sp.size = 5.0f;
                sp.life = 0.4f;
                sp.maxLife = 0.4f;
                sp.kind = ParticleKind::SPARK_GLINT;
                sp.color = {255, 180, 50, 255};
                particles.push_back(sp);
            }
        }
        if (b.mat == MaterialTier::ICE) {
            dmgMultiplier = std::max(dmgMultiplier, 0.44f);
            pigPierceFactor = std::min(pigPierceFactor, 0.55f);
        }

        p.vel += impulse * (p.invMass * pigPierceFactor);
        b.vel -= impulse * b.invMass;
        b.angVel -= cross(rB, impulse) * b.invInertia;
        p.angVel = p.vel.x / std::max(12.0f, p.radius);

        damageBlock(b, std::abs(j) * dmgMultiplier);
        addTrauma(clampf(std::abs(j) * 0.0006f, 0.08f, 0.38f));

        if (p.type == PigType::BOMB && p.fuseTimer < 0.0f && !p.exploded) {
            p.fuseTimer = 0.42f;
        }
        if (b.mat == MaterialTier::TNT && !b.ignited) {
            b.ignited = true;
            b.tntFuse = 0.10f;
        }
    }

    void resolveBirdBlock(BirdTarget& t, BlockBody& b, bool allowDamage, float subDt) {
        float c = std::cos(-b.angle), s = std::sin(-b.angle);
        Vec2 rel = t.pos - b.pos;
        Vec2 local(rel.x * c - rel.y * s, rel.x * s + rel.y * c);
        float hw = b.w * 0.5f, hh = b.h * 0.5f;
        Vec2 closest(clampf(local.x, -hw, hw), clampf(local.y, -hh, hh));
        Vec2 diff = local - closest;
        float distSq = diff.lengthSq();
        if (distSq >= t.radius * t.radius) return;

        if (t.pos.y < b.pos.y && !b.stablySupported && !t.balloonPopped && t.mass <= 3.0f) {
            if (allowDamage && (t.vel - b.vel).length() > 60.0f) {
                damageBird(t, 42.0f);
            }
            return;
        }

        float dist = std::sqrt(distSq);
        Vec2 localN;
        float pen = 0.0f;
        if (dist > 1e-4f) {
            localN = diff * (1.0f / dist);
            pen = t.radius - dist;
        } else {
            float dx = hw - std::abs(local.x);
            float dy = hh - std::abs(local.y);
            if (dx < dy) {
                localN = Vec2((local.x >= 0.0f) ? 1.0f : -1.0f, 0.0f);
                pen = t.radius + dx;
            } else {
                localN = Vec2(0.0f, (local.y >= 0.0f) ? 1.0f : -1.0f);
                pen = t.radius + dy;
            }
        }

        float wc = std::cos(b.angle), ws = std::sin(b.angle);
        Vec2 worldN(localN.x * wc - localN.y * ws, localN.x * ws + localN.y * wc);
        Vec2 contactPt = t.pos - worldN * t.radius;

        if (worldN.y > 0.25f) {
            t.pos.x += (t.pos.x >= b.pos.x ? 1.0f : -1.0f) * pen * 0.4f;
            if (allowDamage) {
                damageBird(t, b.mass * 4.2f);
            }
        } else {
            if (std::abs(local.x) > hw - 3.0f && worldN.y < -0.2f) {
                float slipDir = (t.pos.x >= b.pos.x) ? 1.0f : -1.0f;
                t.vel.x += slipDir * 90.0f * subDt;
                t.vel.y += 60.0f * subDt;
            } else if (b.stablySupported && worldN.y < -0.7f) {
                // Perched on a stable deck: lift the bird 100% out of penetration without pushing the deck down!
                t.pos += worldN * pen;
            } else {
                float totalInv = t.invMass + b.invMass;
                float corr = std::max(0.0f, pen - 0.05f) * 0.65f / totalInv;
                t.pos += worldN * (corr * t.invMass);
                b.pos -= worldN * (corr * b.invMass);
            }
        }

        Vec2 rB = contactPt - b.pos;
        Vec2 vB = b.vel + cross(b.angVel, rB);
        Vec2 rv = t.vel - vB;
        float vn = rv.dot(worldN);
        if (vn >= 0.0f) return;

        float rnB = cross(rB, worldN);
        float denom = t.invMass + b.invMass + rnB * rnB * b.invInertia;
        float j = -vn / denom;
        Vec2 imp = worldN * j;
        t.vel += imp * t.invMass;
        if (worldN.y <= 0.25f && !b.stablySupported) {
            b.vel -= imp * b.invMass;
            b.angVel -= cross(rB, imp) * b.invInertia;
        }

        // Prevent perched birds from slowly rotating on top of stable beams
        if (b.stablySupported && worldN.y < -0.7f && std::abs(rv.x) < 28.0f) {
            t.vel.x = b.vel.x;
            t.angVel = 0.0f;
            t.angle *= 0.80f;
        } else {
            float mu = (std::abs(worldN.y) > 0.85f && std::abs(local.x) <= hw - 3.0f) ? 0.65f : 0.0f;
            Vec2 tangent(-worldN.y, worldN.x);
            float vt = rv.dot(tangent);
            float jt = clampf(-vt / denom, -j * mu, j * mu);
            t.vel += tangent * (jt * t.invMass);
            t.angVel = t.vel.x / std::max(10.0f, t.radius);
        }

        float relSpeed = -vn;
        if (allowDamage && relSpeed > 58.0f) {
            damageBird(t, (relSpeed - 58.0f) * 0.88f * std::max(1.0f, b.mass * 0.50f));
            if (t.balloonPopped || t.mass > 3.0f) {
                damageBlock(b, (relSpeed - 58.0f) * 1.15f * t.mass);
                spawnSplinters(b.pos, b.mat, 6, 160.0f);
            }
        }
    }

    // True OBB vs OBB Separating Axis Theorem (SAT) with Multi-Point Manifolds, Restitution Threshold & Positional Correction
    void resolveBlockBlock(BlockBody& A, BlockBody& B, bool allowDamage, float /*subDt*/) {
        auto vertsA = getBlockVerts(A);
        auto vertsB = getBlockVerts(B);

        float cA = std::cos(A.angle), sA = std::sin(A.angle);
        float cB = std::cos(B.angle), sB = std::sin(B.angle);
        Vec2 axes[4] = {
            {cA, sA}, {-sA, cA},
            {cB, sB}, {-sB, cB}
        };

        float minOverlap = 1e9f;
        Vec2 bestAxis{0.0f, 0.0f};

        for (int i = 0; i < 4; ++i) {
            Vec2 axis = axes[i];
            float minA = 1e9f, maxA = -1e9f;
            float minB = 1e9f, maxB = -1e9f;
            for (const Vec2& v : vertsA) {
                float p = v.dot(axis);
                minA = std::min(minA, p);
                maxA = std::max(maxA, p);
            }
            for (const Vec2& v : vertsB) {
                float p = v.dot(axis);
                minB = std::min(minB, p);
                maxB = std::max(maxB, p);
            }
            float overlap = std::min(maxA, maxB) - std::max(minA, minB);
            if (overlap <= 0.0f) return;
            if (overlap < minOverlap) {
                minOverlap = overlap;
                bestAxis = axis;
            }
        }

        Vec2 delta = B.pos - A.pos;
        Vec2 n = bestAxis;
        if (delta.dot(n) < 0.0f) {
            n = n * -1.0f;
        }

        // For vertically stacked blocks, align normal with true vertical axis without ad-hoc horizontal flings
        float tiltA = std::abs(getTiltFromStable(A));
        float tiltB = std::abs(getTiltFromStable(B));
        if (std::abs(delta.y) > (A.h + B.h) * 0.25f && std::abs(n.y) > 0.55f && tiltA < 0.08f && tiltB < 0.08f) {
            n = Vec2(0.0f, (delta.y > 0.0f) ? 1.0f : -1.0f);
        }

        auto pointInOBB = [](Vec2 pt, const BlockBody& box, float margin) -> bool {
            float c = std::cos(-box.angle), s = std::sin(-box.angle);
            Vec2 rel = pt - box.pos;
            float lx = std::abs(rel.x * c - rel.y * s);
            float ly = std::abs(rel.x * s + rel.y * c);
            return (lx <= box.w * 0.5f + margin) && (ly <= box.h * 0.5f + margin);
        };

        std::vector<Vec2> contactPts;
        for (const Vec2& v : vertsB) {
            if (pointInOBB(v, A, 2.0f)) {
                contactPts.push_back(v);
            }
        }
        for (const Vec2& v : vertsA) {
            if (pointInOBB(v, B, 2.0f)) {
                contactPts.push_back(v);
            }
        }

        if (contactPts.empty()) {
            float aMinX = 1e9f, aMaxX = -1e9f, aMinY = 1e9f, aMaxY = -1e9f;
            float bMinX = 1e9f, bMaxX = -1e9f, bMinY = 1e9f, bMaxY = -1e9f;
            for (int i = 0; i < 4; ++i) {
                aMinX = std::min(aMinX, vertsA[i].x); aMaxX = std::max(aMaxX, vertsA[i].x);
                aMinY = std::min(aMinY, vertsA[i].y); aMaxY = std::max(aMaxY, vertsA[i].y);
                bMinX = std::min(bMinX, vertsB[i].x); bMaxX = std::max(bMaxX, vertsB[i].x);
                bMinY = std::min(bMinY, vertsB[i].y); bMaxY = std::max(bMaxY, vertsB[i].y);
            }
            contactPts.push_back(Vec2(0.5f * (std::max(aMinX, bMinX) + std::min(aMaxX, bMaxX)),
                                      0.5f * (std::max(aMinY, bMinY) + std::min(aMaxY, bMaxY))));
        } else if (contactPts.size() > 2) {
            Vec2 tangent(-n.y, n.x);
            float minProj = 1e9f, maxProj = -1e9f;
            Vec2 pMin = contactPts[0], pMax = contactPts[0];
            for (const Vec2& pt : contactPts) {
                float proj = pt.dot(tangent);
                if (proj < minProj) { minProj = proj; pMin = pt; }
                if (proj > maxProj) { maxProj = proj; pMax = pt; }
            }
            contactPts.clear();
            contactPts.push_back(pMin);
            if (maxProj - minProj > 1.0f) {
                contactPts.push_back(pMax);
            }
        }

        // Directional Support-Aware Positional Correction:
        // If one block is resting on top of a stably-supported lower block, lift the upper block
        // 100% out of overlap without pushing or tilting the stably-supported lower block!
        const float slop = 0.01f;
        float pen = std::max(0.0f, minOverlap - slop);
        if (n.y < -0.7f && A.stablySupported) {
            // B is above A, and A is stably supported
            B.pos += n * pen;
            if (B.stablySupported) {
                if (B.vel.y > A.vel.y) B.vel.y = A.vel.y;
                B.vel.x *= 0.85f;
                if (std::abs(B.vel.x) < 10.0f) B.vel.x = 0.0f;
                B.angVel = 0.0f;
                B.angle = getTargetFlatAngle(B);
                return;
            }
        } else if (n.y > 0.7f && B.stablySupported) {
            // A is above B, and B is stably supported
            A.pos -= n * pen;
            if (A.stablySupported) {
                if (A.vel.y > B.vel.y) A.vel.y = B.vel.y;
                A.vel.x *= 0.85f;
                if (std::abs(A.vel.x) < 10.0f) A.vel.x = 0.0f;
                A.angVel = 0.0f;
                A.angle = getTargetFlatAngle(A);
                return;
            }
        } else {
            float totalInv = A.invMass + B.invMass;
            float corr = pen * 0.55f / totalInv;
            Vec2 dPosA = n * (-corr * A.invMass);
            Vec2 dPosB = n * ( corr * B.invMass);
            if (!A.hasGroundPath && dPosA.y < 0.0f) dPosA.y = 0.0f;
            if (!B.hasGroundPath && dPosB.y < 0.0f) dPosB.y = 0.0f;
            A.pos += dPosA;
            B.pos += dPosB;
        }

        float numPts = static_cast<float>(contactPts.size());
        float mu = 0.5f * (A.friction + B.friction);
        Vec2 tangent(-n.y, n.x);

        for (const Vec2& pt : contactPts) {
            Vec2 rA = pt - A.pos;
            Vec2 rB = pt - B.pos;
            Vec2 vA = A.vel + cross(A.angVel, rA);
            Vec2 vB = B.vel + cross(B.angVel, rB);
            Vec2 rv = vB - vA;
            float vn = rv.dot(n);
            if (vn >= 0.0f) continue;

            float rnA = cross(rA, n);
            float rnB = cross(rB, n);
            float denom = A.invMass + B.invMass + rnA * rnA * A.invInertia + rnB * rnB * B.invInertia;
            if (denom < 1e-6f) continue;

            // Resting restitution: zero restitution when relative speed is low (< 30.0f)
            float e = (std::abs(vn) < 30.0f) ? 0.0f : std::min(A.restitution, B.restitution);
            float j = -(1.0f + e) * vn / denom;
            j /= numPts;
            Vec2 imp = n * j;

            Vec2 dVelA = imp * (-A.invMass);
            Vec2 dVelB = imp * ( B.invMass);
            if (!B.hasGroundPath && dVelA.y < 0.0f) dVelA.y = 0.0f;
            if (!A.hasGroundPath && dVelB.y < 0.0f) dVelB.y = 0.0f;

            A.vel += dVelA;
            A.angVel -= cross(rA, imp) * A.invInertia;
            B.vel += dVelB;
            B.angVel += cross(rB, imp) * B.invInertia;

            // High block-on-block friction so stacked pieces grip realistically instead of skating
            Vec2 vA2 = A.vel + cross(A.angVel, rA);
            Vec2 vB2 = B.vel + cross(B.angVel, rB);
            Vec2 rv2 = vB2 - vA2;
            float vt = rv2.dot(tangent);
            float rtA = cross(rA, tangent);
            float rtB = cross(rB, tangent);
            float tDenom = A.invMass + B.invMass + rtA * rtA * A.invInertia + rtB * rtB * B.invInertia;
            if (tDenom > 1e-6f) {
                float jt = clampf(-vt / tDenom, -j * mu, j * mu);
                Vec2 fImp = tangent * jt;

                A.vel -= fImp * A.invMass;
                A.angVel -= cross(rA, fImp) * A.invInertia;
                B.vel += fImp * B.invMass;
                B.angVel += cross(rB, fImp) * B.invInertia;
            }

            float relSpd = -vn;
            if (allowDamage && relSpd > 190.0f) {
                float dmg = (relSpd - 190.0f) * 0.22f / numPts;
                damageBlock(A, dmg);
                damageBlock(B, dmg);
            }
        }
    }

    void evaluateTurnAndWinState(float dt) {
        if (levelComplete || levelFailed) return;

        int aliveBirds = 0;
        for (const auto& t : birds) {
            if (!t.defeated) aliveBirds++;
        }

        bool anyPigWaiting = false;
        bool anyPigMoving = false;
        for (auto& p : activePigs) {
            if (!p.active) continue;
            if (!p.launched) {
                anyPigWaiting = true;
            } else {
                if (p.vel.lengthSq() < 1100.0f || p.lifeTimer > 4.2f) {
                    p.restTimer += dt;
                    if (p.restTimer > 1.15f) {
                        if (p.type == PigType::BOMB && !p.exploded) {
                            triggerBombDetonation(p);
                        }
                        p.active = false;
                    } else {
                        anyPigMoving = true;
                    }
                } else {
                    p.restTimer = 0.0f;
                    anyPigMoving = true;
                }
            }
        }

        aerialBombs.erase(std::remove_if(aerialBombs.begin(), aerialBombs.end(),
                                         [](const AerialBomb& b) { return !b.active; }),
                          aerialBombs.end());
        for (const auto& ab : aerialBombs) {
            if (ab.active) anyPigMoving = true;
        }

        bool worldMoving = anyPigMoving;
        for (const auto& b : blocks) {
            if (!b.destroyed && (b.vel.lengthSq() > 1600.0f || (b.mat == MaterialTier::TNT && b.ignited))) worldMoving = true;
        }
        for (const auto& t : birds) {
            if (!t.defeated && t.vel.lengthSq() > 1600.0f) worldMoving = true;
        }

        // Feature Pack E: Endless Mode Wave Progression
        if (screen == ScreenState::ENDLESS) {
            if (aliveBirds == 0 && !worldMoving) {
                int unusedCount = static_cast<int>(remainingPigs.size()) + (anyPigWaiting ? 1 : 0);
                int bonus = unusedCount * 6000 + endlessWave * 4000;
                endlessScore += bonus;
                endlessWave++;
                endlessBestWave = std::max(endlessBestWave, endlessWave);
                saveProgress();
                floatScores.push_back({{WINDOW_W * 0.5f, 180.0f},
                                       "WAVE CLEARED! +" + std::to_string(bonus),
                                       {255, 230, 90, 255}, 1.8f, 1.8f});
                assets.playSound("sound.victory");
                generateEndlessWave(endlessWave);
                return;
            }
            if (!anyPigWaiting && !anyPigMoving && !worldMoving) {
                settleTimer += dt;
                if (settleTimer > 0.45f) {
                    settleTimer = 0.0f;
                    if (!remainingPigs.empty()) {
                        spawnNextPigInSlingshot();
                    } else if (aliveBirds > 0) {
                        endlessGameOver = true;
                    }
                }
            } else {
                settleTimer = 0.0f;
            }
            return;
        }

        // Feature Pack E: Fortress Architect Test Play Completion
        if (screen == ScreenState::EDITOR && isTestPlaying) {
            if (aliveBirds == 0 && !worldMoving) {
                levelComplete = true;
                return;
            }
            if (!anyPigWaiting && !anyPigMoving && !worldMoving) {
                settleTimer += dt;
                if (settleTimer > 0.45f) {
                    settleTimer = 0.0f;
                    if (!remainingPigs.empty()) {
                        spawnNextPigInSlingshot();
                    } else if (aliveBirds > 0) {
                        levelFailed = true;
                    }
                }
            } else {
                settleTimer = 0.0f;
            }
            return;
        }

        if (aliveBirds == 0 && !worldMoving) {
            int unusedCount = static_cast<int>(remainingPigs.size()) + (anyPigWaiting ? 1 : 0);
            int bonus = unusedCount * 6000;
            score += bonus;
            if (bonus > 0) {
                floatScores.push_back({{SLING_X, SLING_Y - 50.0f},
                                       "PIG BONUS +" + std::to_string(bonus),
                                       {255, 225, 90, 255}, 2.2f, 2.2f});
            }
            const LevelSpec& L = levels[currentLevelIdx];
            earnedStars = 1;
            if (score >= L.star2Threshold) earnedStars = 2;
            if (score >= L.star3Threshold) earnedStars = 3;

            bestStars[currentLevelIdx] = std::max(bestStars[currentLevelIdx], earnedStars);
            isNewRecordThisRun = (score > bestScores[currentLevelIdx]);
            bestScores[currentLevelIdx] = std::max(bestScores[currentLevelIdx], score);
            saveProgress();

            levelComplete = true;
            recordAnimTimer = 0.0f;

            // Check Achievement: VAULT_CONQUEROR (Clear all 5 Secret Truffle Vault challenge levels)
            bool allVaultsCleared = true;
            for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) {
                if (bestStars[v] <= 0) {
                    allVaultsCleared = false;
                    break;
                }
            }
            if (allVaultsCleared) {
                unlockAchievement(AchievementId::VAULT_CONQUEROR);
            }
            if (isNewRecordThisRun) {
                assets.playSound("sound.highscore");
                for (int i = 0; i < 48; ++i) {
                    float ang = (i / 48.0f) * 6.28318f;
                    float spd = 100.0f + static_cast<float>((i * 47) % 200);
                    Particle p{};
                    p.pos = {WINDOW_W * 0.5f, WINDOW_H * 0.38f};
                    p.vel = {std::cos(ang) * spd, std::sin(ang) * spd - 60.0f};
                    p.maxLife = 2.4f;
                    p.life = p.maxLife;
                    p.color = ((i % 3) == 0) ? SDL_Color{255, 230, 80, 255} :
                              (((i % 3) == 1) ? SDL_Color{255, 175, 40, 255} : SDL_Color{120, 240, 255, 255});
                    p.size = 5.0f;
                    particles.push_back(p);
                }
            } else {
                assets.playSound("sound.victory");
            }
            return;
        }

        if (!anyPigWaiting && !anyPigMoving && !worldMoving) {
            settleTimer += dt;
            if (settleTimer > 0.45f) {
                settleTimer = 0.0f;
                if (!remainingPigs.empty()) {
                    spawnNextPigInSlingshot();
                } else if (aliveBirds > 0) {
                    levelFailed = true;
                }
            }
        } else {
            settleTimer = 0.0f;
        }
    }

    void updateCamera(float dt) {
        // 1. Visceral Trauma-based Screen Shake (toned down to keep action clear)
        shakeTime += dt * 36.0f;
        if (shakeTrauma > 0.0f) {
            shakeTrauma = std::max(0.0f, shakeTrauma - 3.2f * dt);
            float intensity = shakeTrauma * shakeTrauma; // non-linear quadratic curve
            float maxShakeX = 4.5f;
            float maxShakeY = 3.0f;
            float ox = (std::sin(shakeTime * 1.55f) * 0.65f + std::sin(shakeTime * 3.45f) * 0.35f) * maxShakeX * intensity;
            float oy = (std::cos(shakeTime * 2.15f) * 0.65f + std::cos(shakeTime * 4.65f) * 0.35f) * maxShakeY * intensity;
            camShakeOffset = {ox, oy};
        } else {
            camShakeOffset = {0.0f, 0.0f};
        }

        if (screen != ScreenState::PLAYING && !(screen == ScreenState::EDITOR && isTestPlaying) && screen != ScreenState::ENDLESS) {
            camTarget = {0.0f, 0.0f};
            camPos = {0.0f, 0.0f};
            return;
        }

        // 2. Dynamic Camera Target Determination
        if (manualPanTimer > 0.0f) {
            manualPanTimer -= dt;
            if (manualPanTimer <= 0.0f) {
                isManualPanning = false;
            }
        }

        if (isManualPanning) {
            // Player is manually inspecting the level with right-click or keys
        } else if (sloMoActive) {
            float desiredX = sloMoFocusPos.x - WINDOW_W * 0.5f;
            camTarget.x = clampf(desiredX, 0.0f, 420.0f);
            camTarget.y = 0.0f;
        } else if (draggingSling) {
            // Actively aiming catapult: smoothly center on slingshot origin
            camTarget = {0.0f, 0.0f};
        } else if (!activePigs.empty() && activePigs[0].launched && activePigs[0].active) {
            // Pig in flight: frame the pig at 38% screen width so the fortress ahead is clearly visible
            const PigBody& pig = activePigs[0];
            float desiredX = pig.pos.x - WINDOW_W * 0.38f;
            float desiredY = (pig.pos.y < 320.0f) ? (pig.pos.y - 320.0f) : 0.0f;
            camTarget.x = clampf(desiredX, 0.0f, 420.0f);
            camTarget.y = clampf(desiredY, -240.0f, 0.0f);
        } else {
            // Action / Collapsing phase: focus on dynamically moving debris
            float moveSumX = 0.0f;
            int moveCount = 0;
            for (const auto& b : blocks) {
                if (!b.destroyed && b.vel.lengthSq() > 1800.0f) {
                    moveSumX += b.pos.x;
                    moveCount++;
                }
            }
            for (const auto& t : birds) {
                if (!t.defeated && t.vel.lengthSq() > 1800.0f) {
                    moveSumX += t.pos.x;
                    moveCount++;
                }
            }
            if (moveCount > 0) {
                float centerActionX = moveSumX / moveCount;
                float desiredX = centerActionX - WINDOW_W * 0.50f;
                camTarget.x = clampf(desiredX, 0.0f, 420.0f);
                camTarget.y = 0.0f;
            } else {
                // Everything is settled: return home to slingshot
                camTarget = {0.0f, 0.0f};
            }
        }

        // 3. Smooth Critically Damped Camera Interpolation
        float lerpSpeed = (camTarget.x == 0.0f) ? 3.6f : 5.8f;
        if (draggingSling) lerpSpeed = 9.0f;
        if (isManualPanning) lerpSpeed = 12.0f;
        float factor = clampf(lerpSpeed * dt, 0.0f, 1.0f);
        camPos.x += (camTarget.x - camPos.x) * factor;
        camPos.y += (camTarget.y - camPos.y) * factor;

        if (std::abs(camPos.x - camTarget.x) < 0.25f) camPos.x = camTarget.x;
        if (std::abs(camPos.y - camTarget.y) < 0.25f) camPos.y = camTarget.y;
    }

    void updateGame(float frameTime) {
        updateAchievementToast(frameTime);
        if (screen == ScreenState::PLAYING || (screen == ScreenState::EDITOR && isTestPlaying) || screen == ScreenState::ENDLESS) {
            assets.playMusic("music.battle");
            levelTruffle.pulseTime += frameTime;
            if (levelComplete) {
                recordAnimTimer += frameTime;
            }

            // Feature Pack E: Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
            float effectiveSpeed = simSpeed;
            if (sloMoActive) {
                sloMoTimer -= frameTime;
                effectiveSpeed *= sloMoFactor;
                letterboxY = std::min(46.0f, letterboxY + frameTime * 140.0f);
                if (sloMoTimer <= 0.0f) {
                    sloMoActive = false;
                }
            } else {
                letterboxY = std::max(0.0f, letterboxY - frameTime * 140.0f);
            }

            physicsAccumulator += frameTime * effectiveSpeed;
            int steps = 0;
            int maxSteps = (effectiveSpeed > 1.0f) ? 20 : 8;
            while (physicsAccumulator >= FIXED_TIMESTEP && steps < maxSteps) {
                stepPhysics(FIXED_TIMESTEP);
                physicsAccumulator -= FIXED_TIMESTEP;
                steps++;
            }
            if (steps >= maxSteps) {
                physicsAccumulator = 0.0f;
            }
            if (slingRecoilTimer > 0.0f) {
                slingRecoilTimer = std::max(0.0f, slingRecoilTimer - frameTime);
            }
            updateCamera(frameTime);
        } else {
            assets.playMusic("music.title");
            letterboxY = 0.0f;
            sloMoActive = false;
            physicsAccumulator = 0.0f;
            camPos = {0.0f, 0.0f};
            camTarget = {0.0f, 0.0f};
            camShakeOffset = {0.0f, 0.0f};
            shakeTrauma = 0.0f;
        }
    }

    void saveProgress() {
        std::ofstream out("save_progress.cfg");
        out << "audio.music_muted = " << (assets.musicMuted ? 1 : 0) << "\n";
        out << "audio.sfx_muted = " << (assets.sfxMuted ? 1 : 0) << "\n";
        out << "achievements.mask = " << getAchievementsMask() << "\n";
        out << "achievements.helmets = " << achievementProgress[static_cast<int>(AchievementId::CAULDRON_CRACKER)] << "\n";
        out << "endless.best_wave = " << endlessBestWave << "\n";
        for (int i = 0; i < NUM_LEVELS; ++i) {
            out << "level." << (i + 1) << ".stars = " << bestStars[i] << "\n";
            out << "level." << (i + 1) << ".score = " << bestScores[i] << "\n";
            out << "level." << (i + 1) << ".truffle = " << (trufflesFound[i] ? 1 : 0) << "\n";
        }
    }

    void loadProgress() {
        std::ifstream in("save_progress.cfg");
        if (!in.good()) return;
        std::string line;
        while (std::getline(in, line)) {
            if (line.rfind("audio.music_muted = ", 0) == 0) {
                assets.musicMuted = (std::atoi(line.substr(20).c_str()) != 0);
                continue;
            }
            if (line.rfind("audio.sfx_muted = ", 0) == 0) {
                assets.sfxMuted = (std::atoi(line.substr(18).c_str()) != 0);
                continue;
            }
            if (line.rfind("achievements.mask = ", 0) == 0) {
                int mask = std::atoi(line.substr(20).c_str());
                for (int a = 0; a < static_cast<int>(AchievementId::COUNT); ++a) {
                    if (mask & (1 << a)) {
                        achievementsUnlocked[a] = true;
                        if (a < static_cast<int>(achievementDefs.size())) {
                            achievementProgress[a] = achievementDefs[a].targetCount;
                        }
                    }
                }
                continue;
            }
            if (line.rfind("achievements.helmets = ", 0) == 0) {
                achievementProgress[static_cast<int>(AchievementId::CAULDRON_CRACKER)] = std::atoi(line.substr(23).c_str());
                continue;
            }
            if (line.rfind("endless.best_wave = ", 0) == 0) {
                endlessBestWave = std::atoi(line.substr(20).c_str());
                continue;
            }
            for (int i = 0; i < NUM_LEVELS; ++i) {
                std::string sk = "level." + std::to_string(i + 1) + ".stars = ";
                std::string sck = "level." + std::to_string(i + 1) + ".score = ";
                std::string tk = "level." + std::to_string(i + 1) + ".truffle = ";
                if (line.rfind(sk, 0) == 0) {
                    bestStars[i] = std::atoi(line.substr(sk.size()).c_str());
                } else if (line.rfind(sck, 0) == 0) {
                    bestScores[i] = std::atoi(line.substr(sck.size()).c_str());
                } else if (line.rfind(tk, 0) == 0) {
                    trufflesFound[i] = (std::atoi(line.substr(tk.size()).c_str()) != 0);
                }
            }
        }
    }

    // ========================================================================
    // RENDERING & INTERACTIVE UI SCREENS (TITLE, LEVEL SELECT, HUD)
    // ========================================================================
    int getActiveChapter() const {
        if (screen == ScreenState::EDITOR) return clampf(editorTheme, 0, 5);
        if (screen == ScreenState::ENDLESS) return (endlessWave - 1) % 6;
        if (currentLevelIdx >= NUM_CAMPAIGN_LEVELS) return 14;
        return clampf(currentLevelIdx / 10, 0, 13);
    }

    void renderBackgroundBackdrop(SDL_Renderer* ren, Vec2 cam = {0.0f, 0.0f}) {
        int chap = getActiveChapter();
        int theme = (chap == 14) ? 5 : (chap % 5);
        float animT = SDL_GetTicks() * 0.001f;

        // 6 Distinct Atmospheric Sky Palettes
        SDL_Color topCol, midCol, botCol, groundCol, trimCol;
        if (theme == 0) {
            // Chapter I (Levels 1-10): Dawn Meadow & Golden Sunrise Valley
            topCol = {28, 48, 88, 255};
            midCol = {78, 114, 152, 255};
            botCol = {238, 172, 102, 255};
            groundCol = {48, 76, 44, 255};
            trimCol   = {98, 146, 68, 255};
        } else if (theme == 1) {
            // Chapter II (Levels 11-20): Whispering Autumn Woodlands
            topCol = {42, 24, 56, 255};
            midCol = {112, 54, 68, 255};
            botCol = {214, 116, 64, 255};
            groundCol = {62, 40, 28, 255};
            trimCol   = {156, 84, 38, 255};
        } else if (theme == 2) {
            // Chapter III (Levels 21-30): Basalt Canyon & Red Mesa Viaducts
            topCol = {34, 24, 64, 255};
            midCol = {124, 58, 78, 255};
            botCol = {224, 112, 68, 255};
            groundCol = {78, 46, 34, 255};
            trimCol   = {164, 98, 56, 255};
        } else if (theme == 3) {
            // Chapter IV (Levels 31-40): Frostpeak Aurora Highlands
            topCol = {14, 24, 46, 255};
            midCol = {32, 64, 96, 255};
            botCol = {88, 142, 172, 255};
            groundCol = {46, 58, 74, 255};
            trimCol   = {205, 228, 245, 255};
        } else if (theme == 4) {
            // Chapter V (Levels 41-50): Obsidian Imperial Citadel (Volcanic Eclipse)
            topCol = {20, 14, 26, 255};
            midCol = {68, 24, 34, 255};
            botCol = {168, 52, 38, 255};
            groundCol = {34, 30, 40, 255};
            trimCol   = {128, 56, 44, 255};
        } else {
            // Chapter XV: The Secret Truffle Vaults (Twilight Amethyst & Royal Gold)
            topCol = {24, 14, 42, 255};
            midCol = {68, 32, 92, 255};
            botCol = {186, 120, 54, 255};
            groundCol = {28, 22, 44, 255};
            trimCol   = {224, 178, 65, 255};
        }

        SDL_SetRenderDrawColor(ren, topCol.r, topCol.g, topCol.b, 255);
        SDL_RenderClear(ren);

        // Extended 2-band vertical sky gradient with shake margins
        float pad = 120.0f;
        float midY = GROUND_Y * 0.56f - cam.y * 0.12f;
        float skyBotY = GROUND_Y - cam.y;
        SDL_Vertex skyVerts[6] = {
            {{-pad, -pad},              topCol, {0, 0}},
            {{WINDOW_W + pad, -pad},    topCol, {0, 0}},
            {{WINDOW_W + pad, midY},    midCol, {0, 0}},
            {{-pad, midY},              midCol, {0, 0}},
            {{WINDOW_W + pad, skyBotY}, botCol, {0, 0}},
            {{-pad, skyBotY},           botCol, {0, 0}}
        };
        int skyIdx[12] = {0, 1, 2, 0, 2, 3, 3, 2, 4, 3, 4, 5};
        SDL_RenderGeometry(ren, nullptr, skyVerts, 6, skyIdx, 12);

        // Chapter-Specific Celestial Bodies & Upper Sky Features (Distant 0.08x Parallax)
        Vec2 celOff(-cam.x * 0.08f, -cam.y * 0.06f);
        if (theme == 0) {
            // Chapter I: Morning Sun with soft corona rings & drifting origami clouds
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 58.0f, 12, animT * 0.1f, {255, 224, 130, 55});
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 40.0f, 12, -animT * 0.1f, {255, 232, 150, 110});
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 26.0f, 10, 0.0f, {255, 246, 200, 240});
            // Origami paper clouds (with gentle parallax)
            for (int c = 0; c < 3; ++c) {
                float cx = std::fmod(180.0f + c * 390.0f + animT * (12.0f + c * 4.0f) - cam.x * 0.15f, WINDOW_W + 260.0f) - 130.0f;
                float cy = 125.0f + (c % 2) * 42.0f + celOff.y;
                drawConvexPoly(ren, {{cx - 48, cy + 10}, {cx - 22, cy - 14}, {cx + 18, cy - 18},
                                     {cx + 52, cy + 10}}, {255, 250, 242, 165});
            }
        } else if (theme == 1) {
            // Chapter II: Giant Harvest Moon & twinkling stars
            drawNGon(ren, {1095.0f + celOff.x, 160.0f + celOff.y}, 46.0f, 12, 0.0f, {255, 198, 92, 65});
            drawNGon(ren, {1095.0f + celOff.x, 160.0f + celOff.y}, 32.0f, 10, 0.15f, {255, 218, 122, 235});
            drawNGon(ren, {1085.0f + celOff.x, 152.0f + celOff.y}, 7.0f, 6, 0.0f, {228, 182, 88, 180});
            drawNGon(ren, {1106.0f + celOff.x, 170.0f + celOff.y}, 5.0f, 6, 0.0f, {228, 182, 88, 180});
        } else if (theme == 2) {
            // Chapter III: Twin Desert Moons & high canyon stars
            drawNGon(ren, {1060.0f + celOff.x, 150.0f + celOff.y}, 26.0f, 10, 0.1f, {248, 232, 204, 225});
            drawNGon(ren, {1135.0f + celOff.x, 195.0f + celOff.y}, 14.0f, 8, 0.3f, {242, 148, 112, 215});
        } else if (theme == 3) {
            // Chapter IV: Shimmering Aurora Borealis Ribbons & Crescent Ice Moon
            for (int band = 0; band < 3; ++band) {
                float by = 110.0f + band * 34.0f + celOff.y;
                for (int seg = 0; seg < 18; ++seg) {
                    float x0 = -pad + seg * 85.0f + celOff.x;
                    float x1 = -pad + (seg + 1) * 85.0f + celOff.x;
                    float y0 = by + std::sin(animT * 1.4f + seg * 0.55f + band) * 16.0f;
                    float y1 = by + std::sin(animT * 1.4f + (seg + 1) * 0.55f + band) * 16.0f;
                    SDL_Color aCol = (band == 0) ? SDL_Color{64, 235, 175, 52}
                                                 : (band == 1 ? SDL_Color{72, 195, 245, 45}
                                                              : SDL_Color{145, 110, 245, 38});
                    drawConvexPoly(ren, {{x0, y0}, {x1, y1}, {x1, y1 + 28.0f}, {x0, y0 + 28.0f}}, aCol);
                }
            }
            drawNGon(ren, {1115.0f + celOff.x, 150.0f + celOff.y}, 28.0f, 8, 0.2f, {235, 248, 255, 230});
            drawNGon(ren, {1126.0f + celOff.x, 143.0f + celOff.y}, 22.0f, 8, 0.2f, {22, 42, 68, 255});
        } else if (theme == 4) {
            // Chapter V: Blazing Volcanic Solar Eclipse Corona
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 54.0f, 14, animT * 0.2f, {255, 92, 38, 75});
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 38.0f, 12, -animT * 0.15f, {255, 168, 58, 165});
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 27.0f, 12, 0.0f, {24, 16, 28, 255});
        } else {
            // Chapter XV: Royal Twilight Celestial Golden Truffle Eclipse & Sacred Constellation
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 58.0f, 16, animT * 0.15f, {224, 178, 65, 80});
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 42.0f, 14, -animT * 0.12f, {255, 215, 0, 160});
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 28.0f, 12, 0.0f, {32, 18, 48, 255});
            drawNGon(ren, {1080.0f + celOff.x, 165.0f + celOff.y}, 12.0f, 8, animT * 0.3f, {255, 235, 120, 220});
        }

        // Twinkling Geometric Stars (in Themes 1, 2, 3, 4, 5)
        if (theme != 0) {
            const Vec2 starPts[9] = {
                {140, 105}, {285, 150}, {460, 98}, {610, 138},
                {790, 102}, {945, 148}, {1010, 96}, {1190, 158}, {525, 182}
            };
            for (int i = 0; i < 9; ++i) {
                drawNGon(ren, starPts[i] + celOff, (i % 2 == 0) ? 3.2f : 2.2f, 4, 0.0f, {248, 240, 215, 185});
            }
        }

        // Chapter-Specific Horizon Silhouettes (0.32x Midground Parallax)
        float paraX = -cam.x * 0.32f;
        float paraY = -cam.y * 0.16f;
        float gy = GROUND_Y - cam.y;

        if (theme == 0) {
            // Chapter I: Rolling Emerald Hills & Origami Pine Trees + Distant Windmill
            drawTriangle(ren, {40 + paraX, gy},  {290 + paraX, 340 + paraY}, {580 + paraX, gy},  {66, 108, 92, 255});
            drawTriangle(ren, {290 + paraX, 340 + paraY}, {435 + paraX, gy}, {580 + paraX, gy}, {52, 88, 76, 255});
            drawTriangle(ren, {420 + paraX, gy}, {720 + paraX, 285 + paraY}, {1040 + paraX, gy}, {76, 122, 102, 255});
            drawTriangle(ren, {720 + paraX, 285 + paraY}, {880 + paraX, gy}, {1040 + paraX, gy}, {58, 98, 82, 255});
            drawTriangle(ren, {840 + paraX, gy}, {1120 + paraX, 355 + paraY}, {1320 + paraX, gy}, {64, 104, 88, 255});
            // Distant origami windmill on the left hillcrest
            drawConvexPoly(ren, {{278 + paraX, 345 + paraY}, {302 + paraX, 345 + paraY},
                                 {308 + paraX, 410 + paraY}, {272 + paraX, 410 + paraY}}, {232, 218, 195, 255});
            for (int b = 0; b < 4; ++b) {
                float ba = animT * 0.9f + b * (PI * 0.5f);
                drawThickLine(ren, {290 + paraX, 356 + paraY},
                              {290 + paraX + std::cos(ba) * 38.0f, 356 + paraY + std::sin(ba) * 38.0f},
                              3.5f, {248, 238, 210, 230});
            }
        } else if (theme == 1) {
            // Chapter II: Layered Autumn Ridges & Giant Origami Woodland Trees
            drawTriangle(ren, {30 + paraX, gy},  {310 + paraX, 310 + paraY}, {600 + paraX, gy},  {84, 46, 52, 255});
            drawTriangle(ren, {440 + paraX, gy}, {740 + paraX, 265 + paraY}, {1050 + paraX, gy}, {98, 54, 58, 255});
            drawTriangle(ren, {830 + paraX, gy}, {1115 + paraX, 325 + paraY}, {1320 + paraX, gy}, {74, 40, 48, 255});
            // Layered origami autumn pine/maple silhouettes along the horizon
            const float treeX[7] = {90.0f, 360.0f, 485.0f, 560.0f, 640.0f, 1090.0f, 1185.0f};
            for (int t = 0; t < 7; ++t) {
                float tx = treeX[t] + paraX;
                SDL_FRect trunk{tx - 5.0f, gy - 38.0f, 10.0f, 38.0f};
                SDL_SetRenderDrawColor(ren, 52, 30, 22, 255);
                SDL_RenderFillRectF(ren, &trunk);
                SDL_Color leafCol = (t % 2 == 0) ? SDL_Color{186, 78, 38, 240} : SDL_Color{218, 134, 42, 240};
                drawTriangle(ren, {tx - 34.0f, gy - 32.0f}, {tx, gy - 96.0f},  {tx + 34.0f, gy - 32.0f}, leafCol);
                drawTriangle(ren, {tx - 26.0f, gy - 68.0f}, {tx, gy - 126.0f}, {tx + 26.0f, gy - 68.0f}, leafCol);
            }
        } else if (theme == 2) {
            // Chapter III: Flat-Topped Red-Rock Mesas & Distant Canyon Viaduct Arches
            drawConvexPoly(ren, {{40 + paraX, gy}, {110 + paraX, 340 + paraY},
                                 {340 + paraX, 340 + paraY}, {410 + paraX, gy}}, {112, 54, 46, 255});
            drawConvexPoly(ren, {{225 + paraX, 340 + paraY}, {340 + paraX, 340 + paraY},
                                 {410 + paraX, gy}, {225 + paraX, gy}}, {88, 42, 38, 255});
            drawConvexPoly(ren, {{760 + paraX, gy}, {840 + paraX, 290 + paraY},
                                 {1080 + paraX, 290 + paraY}, {1170 + paraX, gy}}, {126, 62, 50, 255});
            drawConvexPoly(ren, {{960 + paraX, 290 + paraY}, {1080 + paraX, 290 + paraY},
                                 {1170 + paraX, gy}, {960 + paraX, gy}}, {96, 46, 40, 255});
            // Distant viaduct bridge spanning between the two mesas
            SDL_FRect bridgeDeck{340.0f + paraX, 365.0f + paraY, 480.0f, 14.0f};
            SDL_SetRenderDrawColor(ren, 74, 42, 46, 230);
            SDL_RenderFillRectF(ren, &bridgeDeck);
            for (int p = 0; p < 5; ++p) {
                SDL_FRect pier{385.0f + p * 88.0f + paraX, 379.0f + paraY, 18.0f, gy - (379.0f + paraY)};
                SDL_RenderFillRectF(ren, &pier);
            }
        } else if (theme == 3) {
            // Chapter IV: Jagged Snow-Capped Glacial Peaks
            drawTriangle(ren, {40 + paraX, gy},  {295 + paraX, 250 + paraY}, {550 + paraX, gy},  {54, 78, 108, 255});
            drawTriangle(ren, {295 + paraX, 250 + paraY}, {422 + paraX, gy}, {550 + paraX, gy}, {40, 60, 86, 255});
            drawTriangle(ren, {390 + paraX, gy}, {710 + paraX, 195 + paraY}, {1030 + paraX, gy}, {64, 92, 124, 255});
            drawTriangle(ren, {710 + paraX, 195 + paraY}, {870 + paraX, gy}, {1030 + paraX, gy}, {48, 70, 98, 255});
            drawTriangle(ren, {820 + paraX, gy}, {1105 + paraX, 275 + paraY}, {1310 + paraX, gy}, {52, 76, 106, 255});
            // Bright snow caps
            drawTriangle(ren, {295 + paraX, 250 + paraY}, {240 + paraX, 355 + paraY},
                              {350 + paraX, 355 + paraY}, {235, 246, 255, 235});
            drawTriangle(ren, {710 + paraX, 195 + paraY}, {642 + paraX, 315 + paraY},
                              {778 + paraX, 315 + paraY}, {235, 246, 255, 240});
            drawTriangle(ren, {1105 + paraX, 275 + paraY}, {1052 + paraX, 365 + paraY},
                              {1158 + paraX, 365 + paraY}, {230, 242, 255, 225});
        } else if (theme == 4) {
            // Chapter V: Jagged Volcanic Spires, Lava Veins & Imperial Battlements
            drawTriangle(ren, {50 + paraX, gy},  {280 + paraX, 240 + paraY}, {510 + paraX, gy},  {46, 30, 42, 255});
            drawTriangle(ren, {280 + paraX, 240 + paraY}, {395 + paraX, gy}, {510 + paraX, gy}, {32, 22, 30, 255});
            drawTriangle(ren, {430 + paraX, gy}, {715 + paraX, 205 + paraY}, {1000 + paraX, gy}, {54, 34, 46, 255});
            drawTriangle(ren, {715 + paraX, 205 + paraY}, {858 + paraX, gy}, {1000 + paraX, gy}, {38, 24, 34, 255});
            drawTriangle(ren, {860 + paraX, gy}, {1120 + paraX, 265 + paraY}, {1320 + paraX, gy}, {44, 28, 38, 255});
            // Glowing magma fissures on the central volcanic peak
            drawThickLine(ren, {715 + paraX, 205 + paraY}, {692 + paraX, 320 + paraY}, 3.5f, {255, 112, 38, 210});
            drawThickLine(ren, {715 + paraX, 205 + paraY}, {742 + paraX, 350 + paraY}, 3.0f, {255, 148, 48, 190});
        } else {
            // Chapter XV: Ancient Royal Truffle Vault Colonnades, Obelisks & Grand Ruin Spires
            drawConvexPoly(ren, {{60 + paraX, gy}, {120 + paraX, 230 + paraY}, {190 + paraX, 230 + paraY}, {250 + paraX, gy}}, {54, 32, 72, 255});
            drawTriangle(ren, {120 + paraX, 230 + paraY}, {155 + paraX, 170 + paraY}, {190 + paraX, 230 + paraY}, {78, 48, 98, 255});
            drawConvexPoly(ren, {{420 + paraX, gy}, {500 + paraX, 260 + paraY}, {680 + paraX, 260 + paraY}, {760 + paraX, gy}}, {48, 28, 64, 255});
            drawConvexPoly(ren, {{820 + paraX, gy}, {880 + paraX, 210 + paraY}, {960 + paraX, 210 + paraY}, {1020 + paraX, gy}}, {62, 36, 82, 255});
            drawTriangle(ren, {880 + paraX, 210 + paraY}, {920 + paraX, 150 + paraY}, {960 + paraX, 210 + paraY}, {88, 56, 112, 255});
            // Distant glowing gold rune beacons
            drawNGon(ren, {155 + paraX, 168 + paraY}, 6.0f, 6, animT * 1.2f, {255, 215, 0, 220});
            drawNGon(ren, {920 + paraX, 148 + paraY}, 6.0f, 6, -animT * 1.2f, {255, 215, 0, 220});
        }

        // Ambient Chapter Weather Particles
        for (int i = 0; i < 18; ++i) {
            float seed = i * 73.1f;
            float wx = std::fmod(seed * 19.0f + animT * (18.0f + (i % 5) * 6.0f), static_cast<float>(WINDOW_W));
            float wy = 0.0f;
            SDL_Color pCol{255, 255, 255, 150};
            float pRad = 3.0f;
            if (chap == 4) {
                // Rising glowing volcanic embers
                wy = gy - std::fmod(seed * 13.0f + animT * (32.0f + (i % 4) * 10.0f), GROUND_Y - 70.0f);
                pCol = (i % 2 == 0) ? SDL_Color{255, 142, 48, 200} : SDL_Color{255, 212, 82, 180};
                pRad = 3.2f;
            } else if (chap == 5) {
                // Rising mystic golden sparkle motes in the royal vaults
                wy = gy - std::fmod(seed * 11.0f + animT * (28.0f + (i % 3) * 8.0f), GROUND_Y - 50.0f);
                wx += std::sin(animT * 2.2f + i) * 14.0f;
                pCol = (i % 2 == 0) ? SDL_Color{255, 215, 80, 220} : SDL_Color{224, 160, 255, 180};
                pRad = 3.4f;
            } else {
                // Gently falling leaves / snow / pollen
                wy = 70.0f + std::fmod(seed * 17.0f + animT * (24.0f + (i % 4) * 7.0f), GROUND_Y - 75.0f);
                wx += std::sin(animT * 1.8f + i) * 18.0f;
                if (chap == 0) { pCol = {255, 248, 195, 155}; pRad = 2.8f; }
                else if (chap == 1) { pCol = (i % 2 == 0) ? SDL_Color{228, 112, 42, 210} : SDL_Color{242, 178, 52, 210}; pRad = 4.2f; }
                else if (chap == 2) { pCol = {238, 184, 132, 130}; pRad = 2.5f; }
                else { pCol = {240, 250, 255, 215}; pRad = 3.5f; }
            }
            drawNGon(ren, {wx, wy}, pRad, (chap == 1) ? 4 : 6, animT * 1.5f + i, pCol);
        }

        // Terraced Ground & Chapter Surface Detail with Shake Margins
        SDL_FRect groundBase{-pad, gy, WINDOW_W + 2.0f * pad, WINDOW_H - gy + pad};
        SDL_SetRenderDrawColor(ren, groundCol.r, groundCol.g, groundCol.b, 255);
        SDL_RenderFillRectF(ren, &groundBase);
        SDL_FRect grassTrim{-pad, gy, WINDOW_W + 2.0f * pad, 13.0f};
        SDL_SetRenderDrawColor(ren, trimCol.r, trimCol.g, trimCol.b, 255);
        SDL_RenderFillRectF(ren, &grassTrim);
        SDL_FRect subTrim{-pad, gy + 13.0f, WINDOW_W + 2.0f * pad, 5.0f};
        SDL_SetRenderDrawColor(ren, static_cast<uint8_t>(trimCol.r * 0.72f),
                               static_cast<uint8_t>(trimCol.g * 0.72f),
                               static_cast<uint8_t>(trimCol.b * 0.72f), 255);
        SDL_RenderFillRectF(ren, &subTrim);
    }

    void renderSiegeCatapultBase(SDL_Renderer* ren, float sx, float sy, float groundY = GROUND_Y) {
        // Heavy timber & iron siege catapult base plinth on the ground
        SDL_FRect plinth{sx - 28.0f, groundY - 14.0f, 56.0f, 14.0f};
        SDL_SetRenderDrawColor(ren, 64, 42, 26, 255);
        SDL_RenderFillRectF(ren, &plinth);
        drawThickLine(ren, {sx - 22.0f, groundY - 6.0f}, {sx, sy + 32.0f}, 8.0f, {72, 46, 28, 255});
        drawThickLine(ren, {sx + 22.0f, groundY - 6.0f}, {sx, sy + 32.0f}, 8.0f, {72, 46, 28, 255});
        drawThickLine(ren, {sx, groundY}, {sx, sy + 28.0f}, 16.0f, {88, 54, 34, 255});
        drawThickLine(ren, {sx, sy + 28.0f}, {sx + 20.0f, sy - 12.0f}, 11.0f, {112, 70, 44, 255});
        // Brass pivot bolt
        drawNGon(ren, {sx, sy + 26.0f}, 5.5f, 6, 0.0f, {226, 188, 92, 255});
    }

    void render(SDL_Renderer* ren) {
        activeButtons.clear();
        Vec2 cam = (screen == ScreenState::PLAYING || (screen == ScreenState::EDITOR && isTestPlaying) || screen == ScreenState::ENDLESS) ? (camPos + camShakeOffset) : Vec2(0.0f, 0.0f);
        renderBackgroundBackdrop(ren, cam);

        if (screen == ScreenState::TITLE) {
            renderTitleScreen(ren);
            renderAchievementToast(ren);
            return;
        }
        if (screen == ScreenState::LEVEL_SELECT) {
            renderLevelSelectScreen(ren);
            renderAchievementToast(ren);
            return;
        }
        if (screen == ScreenState::ACHIEVEMENTS) {
            renderAchievementsScreen(ren);
            renderAchievementToast(ren);
            return;
        }
        if (screen == ScreenState::EDITOR && !isTestPlaying) {
            renderEditorScreen(ren);
            renderAchievementToast(ren);
            return;
        }

        // ScreenState::PLAYING
        // Drifting cumulonimbus storm clouds
        for (const auto& sc : stormClouds) {
            Vec2 cPos = sc.pos - cam * 0.35f;
            float hw = sc.width * 0.5f, hh = sc.height * 0.5f;
            drawNGon(ren, {cPos.x - hw * 0.35f, cPos.y + 4.0f}, hh * 0.95f, 10, 0.0f, {45, 52, 68, 220});
            drawNGon(ren, {cPos.x, cPos.y - 6.0f}, hh * 1.15f, 10, 0.0f, {55, 62, 80, 230});
            drawNGon(ren, {cPos.x + hw * 0.40f, cPos.y + 2.0f}, hh * 0.90f, 10, 0.0f, {48, 55, 72, 220});
            drawNGon(ren, {cPos.x, cPos.y + 8.0f}, hh * 0.85f, 10, 0.0f, {38, 44, 58, 240});
            drawThickLine(ren, {cPos.x - hw * 0.3f, cPos.y - hh * 0.85f}, {cPos.x + hw * 0.35f, cPos.y - hh * 0.80f}, 2.2f, {140, 160, 195, 180});
        }

        // Active electric lightning bolts
        for (const auto& bolt : lightningBolts) {
            float bAlpha = clampf(bolt.life / bolt.maxLife, 0.0f, 1.0f);
            if (bolt.points.size() < 2) continue;
            SDL_Color coreCol = bolt.color;
            coreCol.a = static_cast<uint8_t>(255.0f * bAlpha);
            SDL_Color glowCol{180, 230, 255, static_cast<uint8_t>(140.0f * bAlpha)};
            for (size_t i = 0; i + 1 < bolt.points.size(); ++i) {
                Vec2 p0 = bolt.points[i] - cam;
                Vec2 p1 = bolt.points[i + 1] - cam;
                drawThickLine(ren, p0, p1, bolt.width * 2.2f, glowCol);
                drawThickLine(ren, p0, p1, bolt.width, coreCol);
            }
        }

        float screenGroundY = GROUND_Y - cam.y;
        for (size_t i = 0; i < remainingPigs.size(); ++i) {
            Vec2 qPos(125.0f - i * 42.0f - cam.x, screenGroundY - 18.0f);
            drawConvexPoly(ren, {{qPos.x - 15, screenGroundY + 2}, {qPos.x + 15, screenGroundY + 2},
                                 {qPos.x + 11, screenGroundY + 6}, {qPos.x - 11, screenGroundY + 6}}, {15, 18, 24, 110});
            renderOrigamiPigGeometry(ren, remainingPigs[i], qPos, 17.0f, 0.0f);
        }

        renderSiegeCatapultBase(ren, SLING_X - cam.x, SLING_Y - cam.y, screenGroundY);

        // Previous Shot Ghost Trail Dots (Trajectory Memory)
        if (!ghostTrajectory.empty()) {
            for (size_t i = 0; i < ghostTrajectory.size(); ++i) {
                Vec2 gPt = ghostTrajectory[i] - cam;
                if (gPt.y > screenGroundY) continue;
                // Delicate translucent parchment ghost marker
                drawNGon(ren, gPt, (i % 2 == 0) ? 3.5f : 2.2f, 4, (i * 0.25f), {255, 235, 160, 95});
                drawNGon(ren, gPt, 1.4f, 4, 0.0f, {32, 26, 20, 120});
            }
        }

        // Catapult back elastic band
        bool hasAimingPig = (!activePigs.empty() && !activePigs[0].launched);
        Vec2 pouchPos = slingPullPos;
        if (hasAimingPig) {
            pouchPos = activePigs[0].pos;
            drawThickLine(ren, {SLING_X + 18.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 5.5f, {218, 175, 110, 255});
        } else {
            Vec2 recoilOffset{0.0f, 0.0f};
            if (slingRecoilTimer > 0.0f) {
                float decay = slingRecoilTimer / 0.35f;
                float vibe = std::sin(slingRecoilTimer * 55.0f) * decay * 14.0f;
                recoilOffset = lastLaunchDir * vibe;
            }
            pouchPos = Vec2(SLING_X, SLING_Y) + recoilOffset;
            drawThickLine(ren, {SLING_X + 18.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 4.5f, {218, 175, 110, 230});
        }

        if (draggingSling && hasAimingPig) {
            Vec2 pull = slingPullPos - Vec2(SLING_X, SLING_Y);
            if (pull.length() > 12.0f) {
                Vec2 v0 = pull * (-LAUNCH_POWER);
                for (int i = 1; i <= 26; ++i) {
                    float t = i * 0.050f;
                    Vec2 pt(SLING_X + v0.x * t + 0.5f * (currentWind * 0.85f) * t * t - cam.x,
                            SLING_Y + v0.y * t + 0.5f * GRAVITY * t * t - cam.y);
                    if (pt.y > screenGroundY) break;
                    uint8_t alpha = static_cast<uint8_t>(std::max(40, 245 - i * 8));
                    drawNGon(ren, pt, 5.5f, 6, 0.0f, {24, 28, 38, static_cast<uint8_t>(alpha * 0.65f)});
                    drawNGon(ren, pt, (i % 3 == 0) ? 4.2f : 3.4f, 6, 0.0f, {255, 238, 165, alpha});
                    if (i % 2 == 0) {
                        drawNGon(ren, pt, 1.6f, 4, 0.0f, {255, 255, 255, alpha});
                    }
                }
            }
        }

        for (const auto& b : blocks) {
            if (b.destroyed) continue;
            renderBlock(ren, b, cam);
        }

        for (const auto& pend : pendulums) {
            renderPendulum(ren, pend, cam);
        }

        if (levelTruffle.exists && !levelTruffle.collected) {
            renderGoldenTruffle(ren, cam);
        }

        for (const auto& t : birds) {
            if (t.defeated) continue;
            renderBird(ren, t, cam);
        }

        for (const auto& p : activePigs) {
            for (size_t i = 0; i < p.trail.size(); ++i) {
                float trailFrac = static_cast<float>(i) / std::max(1, (int)p.trail.size());
                uint8_t alpha = static_cast<uint8_t>(50 + 190 * trailFrac);
                drawNGon(ren, p.trail[i] - cam, (i % 2 == 0) ? 4.2f : 2.8f, 6, 0.0f, {255, 242, 210, alpha});
            }
            if (p.active) {
                renderPig(ren, p, cam);
            }
        }

        // Render Falling Aerial Bombs
        for (const auto& ab : aerialBombs) {
            if (!ab.active) continue;
            Vec2 bPos = ab.pos - cam;
            drawNGon(ren, bPos, ab.radius, 10, ab.lifeTimer * 8.0f, {34, 38, 48, 255});
            drawNGon(ren, bPos, ab.radius * 0.72f, 10, ab.lifeTimer * 8.0f, {58, 64, 78, 255});
            drawNGon(ren, bPos + Vec2(-ab.radius * 0.25f, -ab.radius * 0.25f), ab.radius * 0.28f, 6, 0.0f, {180, 195, 215, 255});
            drawNGon(ren, bPos + Vec2(0.0f, -ab.radius * 0.9f), ab.radius * 0.35f, 4, 0.0f, {218, 175, 75, 255});
        }

        // Catapult front elastic band & leather cup
        if (hasAimingPig) {
            drawThickLine(ren, {SLING_X - 16.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 6.0f, {236, 194, 128, 255});
            drawNGon(ren, pouchPos - cam - Vec2(4.0f, 0.0f), 9.0f, 6, 0.0f, {92, 54, 32, 255});
        } else {
            drawThickLine(ren, {SLING_X - 16.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 5.0f, {236, 194, 128, 230});
            drawNGon(ren, pouchPos - cam, 7.5f, 6, 0.0f, {92, 54, 32, 255});
        }
        drawThickLine(ren, {SLING_X - cam.x, SLING_Y + 28.0f - cam.y},
                           {SLING_X - 18.0f - cam.x, SLING_Y - 12.0f - cam.y}, 12.0f, {128, 82, 50, 255});

        for (const auto& pt : particles) {
            float alphaRatio = clampf(pt.life / pt.maxLife, 0.0f, 1.0f);
            SDL_Color col = pt.color;
            col.a = static_cast<uint8_t>(255.0f * alphaRatio);
            Vec2 sPos = pt.pos - cam;

            if (pt.isRing || pt.kind == ParticleKind::RING) {
                int segs = 28;
                for (int i = 0; i < segs; ++i) {
                    float a0 = (2.0f * PI * i) / segs;
                    float a1 = (2.0f * PI * (i + 1)) / segs;
                    Vec2 p0(sPos.x + std::cos(a0) * pt.ringRadius, sPos.y + std::sin(a0) * pt.ringRadius);
                    Vec2 p1(sPos.x + std::cos(a1) * pt.ringRadius, sPos.y + std::sin(a1) * pt.ringRadius);
                    drawThickLine(ren, p0, p1, 6.0f * alphaRatio, col);
                }
            } else if (pt.kind == ParticleKind::FEATHER) {
                // Fluttering Feather with Quill Spine and Barb Vanes
                float c = std::cos(pt.angle), s = std::sin(pt.angle);
                float fLen = pt.size * 3.4f;
                float fWid = pt.size * 1.5f;
                Vec2 q0(sPos.x - c * fLen * 0.5f, sPos.y - s * fLen * 0.5f);
                Vec2 q1(sPos.x + c * fLen * 0.5f, sPos.y + s * fLen * 0.5f);
                Vec2 perp(-s * fWid, c * fWid);

                drawTriangle(ren, q0, q0 + perp * 0.85f, q1, col);
                SDL_Color darkVane = {
                    static_cast<uint8_t>(std::max(0, col.r - 25)),
                    static_cast<uint8_t>(std::max(0, col.g - 25)),
                    static_cast<uint8_t>(std::max(0, col.b - 25)),
                    col.a
                };
                drawTriangle(ren, q0, q0 - perp * 0.85f, q1, darkVane);

                SDL_Color quillCol = {
                    static_cast<uint8_t>(std::min(255, col.r + 55)),
                    static_cast<uint8_t>(std::min(255, col.g + 55)),
                    static_cast<uint8_t>(std::min(255, col.b + 55)),
                    col.a
                };
                drawThickLine(ren, q0, q1, 1.8f, quillCol);
            } else if (pt.kind == ParticleKind::WOOD_SPLINTER) {
                // Elongated Tumbling Wood / Wicker Sliver
                float c = std::cos(pt.angle), s = std::sin(pt.angle);
                float hLen = pt.size * 2.2f;
                Vec2 s0(sPos.x - c * hLen, sPos.y - s * hLen);
                Vec2 s1(sPos.x + c * hLen, sPos.y + s * hLen);
                drawThickLine(ren, s0, s1, 2.5f, col);
                drawThickLine(ren, {s0.x + 1.0f, s0.y + 1.0f}, {s1.x + 1.0f, s1.y + 1.0f}, 1.2f, {24, 18, 12, col.a});
            } else if (pt.kind == ParticleKind::TIMBER_CHUNK || pt.kind == ParticleKind::STONE_SHARD) {
                // Angular Tumbling Block Chunk / Pebble
                drawNGon(ren, sPos, pt.size * alphaRatio, 5, pt.angle, col);
                drawNGon(ren, sPos, pt.size * alphaRatio * 0.6f, 5, pt.angle, {255, 255, 255, static_cast<uint8_t>(col.a * 0.4f)});
            } else if (pt.kind == ParticleKind::SMOKE_PUFF) {
                // Soft expanding circular dust cloud
                float pRatio = 1.0f - (pt.life / pt.maxLife);
                float curSize = pt.size * (0.85f + pRatio * 1.5f);
                SDL_Color pCol = col;
                pCol.a = static_cast<uint8_t>(col.a * 0.45f * (1.0f - pRatio));
                drawNGon(ren, sPos, curSize, 8, pt.angle, pCol);
                drawNGon(ren, sPos, curSize * 0.65f, 8, -pt.angle, pCol);
            } else if (pt.kind == ParticleKind::SPARK_GLINT) {
                // 4-point golden star glint
                float gSize = pt.size * (0.6f + 0.4f * std::sin(alphaRatio * PI));
                drawThickLine(ren, {sPos.x - gSize, sPos.y}, {sPos.x + gSize, sPos.y}, 2.0f, col);
                drawThickLine(ren, {sPos.x, sPos.y - gSize}, {sPos.x, sPos.y + gSize}, 2.0f, col);
                drawNGon(ren, sPos, gSize * 0.35f, 4, 0.0f, {255, 255, 255, col.a});
            } else if (pt.kind == ParticleKind::ICE_CRYSTAL) {
                // Sharp Crystalline Diamond / Frost Shard with Specular Sparkle
                float c = std::cos(pt.angle), s = std::sin(pt.angle);
                float r = pt.size * alphaRatio * 1.35f;
                Vec2 pTop(sPos.x - s * r * 1.35f, sPos.y + c * r * 1.35f);
                Vec2 pBot(sPos.x + s * r * 1.35f, sPos.y - c * r * 1.35f);
                Vec2 pLeft(sPos.x - c * r * 0.7f, sPos.y - s * r * 0.7f);
                Vec2 pRight(sPos.x + c * r * 0.7f, sPos.y + s * r * 0.7f);
                drawTriangle(ren, pTop, pLeft, pRight, col);
                drawTriangle(ren, pBot, pLeft, pRight, {static_cast<uint8_t>(col.r * 0.78f),
                                                        static_cast<uint8_t>(col.g * 0.82f),
                                                        static_cast<uint8_t>(col.b), col.a});
                drawNGon(ren, sPos, r * 0.35f, 4, 0.0f, {255, 255, 255, col.a});
            } else if (pt.kind == ParticleKind::HELMET_DEBRIS) {
                // Tumbling Iron Cauldron Helmet Piece
                float c = std::cos(pt.angle), s = std::sin(pt.angle);
                auto rPt = [&](float lx, float ly) -> Vec2 {
                    return {sPos.x + (lx * c - ly * s) * pt.size, sPos.y + (lx * s + ly * c) * pt.size};
                };
                drawConvexPoly(ren, {rPt(-0.6f, 0.4f), rPt(0.6f, 0.4f), rPt(0.4f, -0.7f), rPt(-0.4f, -0.7f)},
                               {75, 82, 96, col.a});
                drawThickLine(ren, rPt(-0.75f, 0.4f), rPt(0.75f, 0.4f), 3.0f, {135, 142, 158, col.a});
                drawNGon(ren, rPt(0.0f, -0.8f), pt.size * 0.18f, 4, pt.angle, {218, 175, 75, col.a});
            } else if (pt.kind == ParticleKind::EMBER) {
                // Fiery Rising Volcanic Ember with Molten Core & Outer Halo
                float r = pt.size * (0.6f + 0.4f * alphaRatio);
                SDL_Color halo = col;
                halo.a = static_cast<uint8_t>(col.a * 0.35f);
                drawNGon(ren, sPos, r * 2.0f, 6, pt.angle, halo);
                drawNGon(ren, sPos, r, 6, pt.angle, col);
            } else if (pt.kind == ParticleKind::GOLD_SPARKLE) {
                // Radiating celestial golden star sparkle
                float gSize = pt.size * (0.7f + 0.5f * std::sin(alphaRatio * PI));
                drawThickLine(ren, {sPos.x - gSize, sPos.y}, {sPos.x + gSize, sPos.y}, 2.5f, col);
                drawThickLine(ren, {sPos.x, sPos.y - gSize}, {sPos.x, sPos.y + gSize}, 2.5f, col);
                float dSize = gSize * 0.65f;
                drawThickLine(ren, {sPos.x - dSize, sPos.y - dSize}, {sPos.x + dSize, sPos.y + dSize}, 1.5f, {255, 235, 120, col.a});
                drawThickLine(ren, {sPos.x - dSize, sPos.y + dSize}, {sPos.x + dSize, sPos.y - dSize}, 1.5f, {255, 235, 120, col.a});
                drawNGon(ren, sPos, gSize * 0.4f, 4, pt.angle, {255, 255, 255, col.a});
            } else if (pt.kind == ParticleKind::SHIELD_DEBRIS) {
                // Tumbling reinforced tower shield fragment
                float c = std::cos(pt.angle), s = std::sin(pt.angle);
                auto rPt = [&](float lx, float ly) -> Vec2 {
                    return {sPos.x + (lx * c - ly * s) * pt.size, sPos.y + (lx * s + ly * c) * pt.size};
                };
                drawConvexPoly(ren, {rPt(-0.5f, -0.6f), rPt(0.5f, -0.6f), rPt(0.3f, 0.6f), rPt(-0.3f, 0.6f)},
                               {78, 88, 106, col.a});
                drawThickLine(ren, rPt(-0.4f, 0.0f), rPt(0.4f, 0.0f), 2.2f, {230, 185, 65, col.a});
            } else if (pt.kind == ParticleKind::BALLOON_SCRAP) {
                // Floating shredded zeppelin canvas scrap
                float c = std::cos(pt.angle), s = std::sin(pt.angle);
                auto rPt = [&](float lx, float ly) -> Vec2 {
                    return {sPos.x + (lx * c - ly * s) * pt.size, sPos.y + (lx * s + ly * c) * pt.size};
                };
                drawTriangle(ren, rPt(-0.6f, -0.4f), rPt(0.6f, -0.2f), rPt(0.1f, 0.5f), {185, 145, 95, col.a});
                drawThickLine(ren, rPt(-0.6f, -0.4f), rPt(0.1f, 0.5f), 1.5f, {125, 85, 45, col.a});
            } else if (pt.kind == ParticleKind::COSMIC_MOTE) {
                // Swirling violet/cyan gravitational vortex mote
                float r = pt.size * (0.8f + 0.4f * std::sin(alphaRatio * PI));
                drawNGon(ren, sPos, r * 1.8f, 6, pt.angle, {180, 80, 255, static_cast<uint8_t>(col.a * 0.45f)});
                drawNGon(ren, sPos, r * 0.9f, 6, -pt.angle, {110, 230, 255, col.a});
                drawNGon(ren, sPos, r * 0.4f, 4, 0.0f, {255, 255, 255, col.a});
            } else {
                drawNGon(ren, sPos, pt.size * alphaRatio, 4, pt.life * 6.0f, col);
            }
        }

        for (const auto& fs : floatScores) {
            float pRatio = clampf(1.0f - fs.life / fs.maxLife, 0.0f, 1.0f);
            float popScale = 2.0f;
            if (pRatio < 0.22f) {
                popScale = 2.0f + 0.95f * std::sin((pRatio / 0.22f) * (PI * 0.5f));
            } else {
                popScale = 2.0f + 0.95f * std::cos(((pRatio - 0.22f) / 0.78f) * (PI * 0.5f));
            }
            float sx = fs.pos.x - cam.x;
            float sy = fs.pos.y - cam.y;
            uint8_t alpha = static_cast<uint8_t>(clampf(fs.life / fs.maxLife, 0.0f, 1.0f) * 255.0f);
            SDL_Color shadowCol{14, 18, 26, alpha};
            SDL_Color txtCol = fs.color;
            txtCol.a = alpha;
            drawText(ren, fs.text, sx + 2.0f, sy + 2.0f, popScale, shadowCol, true);
            drawText(ren, fs.text, sx, sy, popScale, txtCol, true);
        }

        // Full-screen lightning flash illumination
        if (skyFlashAlpha > 0.01f) {
            uint8_t flashA = static_cast<uint8_t>(clampf(skyFlashAlpha * 175.0f, 0.0f, 255.0f));
            SDL_SetRenderDrawColor(ren, 230, 245, 255, flashA);
            SDL_FRect screenRect{0.0f, 0.0f, static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)};
            SDL_RenderFillRectF(ren, &screenRect);
        }

        renderHUD(ren);
        renderAchievementToast(ren);

        // Feature Pack E: Cinematic Slo-Mo Letterboxing Bars & Banner
        if (letterboxY > 0.5f) {
            SDL_SetRenderDrawColor(ren, 12, 14, 20, 255);
            SDL_FRect topBarRect{0.0f, 0.0f, static_cast<float>(WINDOW_W), letterboxY};
            SDL_FRect btmBarRect{0.0f, static_cast<float>(WINDOW_H) - letterboxY, static_cast<float>(WINDOW_W), letterboxY};
            SDL_RenderFillRectF(ren, &topBarRect);
            SDL_RenderFillRectF(ren, &btmBarRect);
            if (sloMoActive) {
                drawText(ren, "** AVIAN RECKONING **", WINDOW_W * 0.5f, letterboxY * 0.5f - 8.0f, 1.8f, {255, 215, 95, 255}, true);
            }
        }
    }

    void renderTitleScreen(SDL_Renderer* ren) {
        renderSiegeCatapultBase(ren, 180.0f, SLING_Y);
        drawThickLine(ren, {180, SLING_Y + 28.0f}, {162, SLING_Y - 12.0f}, 12.0f, {126, 80, 48, 255});

        renderOrigamiPigGeometry(ren, PigType::TROTTER,  {180, SLING_Y - 4.0f},  25.0f, -0.12f);
        renderOrigamiPigGeometry(ren, PigType::TURBO,    {45,  GROUND_Y - 22.0f}, 22.0f, 0.0f);
        renderOrigamiPigGeometry(ren, PigType::SPLIT,    {95,  GROUND_Y - 19.0f}, 19.0f, 0.0f);
        renderOrigamiPigGeometry(ren, PigType::BOMB,     {242, GROUND_Y - 25.0f}, 25.0f, 0.0f);
        renderOrigamiPigGeometry(ren, PigType::GLIDER,   {295, GROUND_Y - 21.0f}, 21.0f, 0.0f);
        renderOrigamiPigGeometry(ren, PigType::GRAVITON, {345, GROUND_Y - 23.0f}, 23.0f, 0.0f);
        renderOrigamiPigGeometry(ren, PigType::DRILL,    {140, GROUND_Y - 20.0f}, 20.0f, 0.0f);

        renderTotemBirdGeometry(ren, BirdClass::TOUCAN, {960,  GROUND_Y - 22.0f}, 22.0f, 0.0f, 1.0f);
        renderTotemBirdGeometry(ren, BirdClass::FALCON, {1030, GROUND_Y - 23.0f}, 23.0f, 0.0f, 1.0f, false, 1.0f, true, 1.0f);
        renderTotemBirdGeometry(ren, BirdClass::OWL,    {1105, GROUND_Y - 25.0f}, 25.0f, 0.0f, 1.0f);
        renderTotemBirdGeometry(ren, BirdClass::HERON,  {1185, GROUND_Y - 29.0f}, 29.0f, 0.0f, 1.0f);
        renderTotemBirdGeometry(ren, BirdClass::CONDOR, {1250, GROUND_Y - 28.0f}, 28.0f, 0.0f, 1.0f, false, 1.0f, false, 1.0f, true, 1.0f);

        SDL_FRect card{WINDOW_W * 0.5f - 380.0f, 36.0f, 760.0f, 578.0f};
        SDL_SetRenderDrawColor(ren, 20, 25, 38, 240);
        SDL_RenderFillRectF(ren, &card);
        drawThickLine(ren, {card.x, card.y}, {card.x + card.w, card.y}, 3.0f, {236, 194, 98, 255});
        drawThickLine(ren, {card.x + card.w, card.y}, {card.x + card.w, card.y + card.h}, 3.0f, {236, 194, 98, 255});
        drawThickLine(ren, {card.x + card.w, card.y + card.h}, {card.x, card.y + card.h}, 3.0f, {236, 194, 98, 255});
        drawThickLine(ren, {card.x, card.y + card.h}, {card.x, card.y}, 3.0f, {236, 194, 98, 255});

        drawText(ren, "SNOUT & SIEGE", WINDOW_W * 0.5f, 54.0f, 4.3f, {255, 214, 92, 255}, true);
        drawText(ren, "RISE OF THE SWINE  -  CAMPAIGN, ARCHITECT SANDBOX & ENDLESS SIEGE",
                 WINDOW_W * 0.5f, 96.0f, 1.45f, {212, 224, 240, 255}, true);

        std::string progText = "* " + std::to_string(getTotalStars()) + " STARS (" +
                               std::to_string(getLevelsCleared()) + "/" + std::to_string(NUM_LEVELS) + " CLEARED) | TRUFFLES: " +
                               std::to_string(getTrufflesFoundCount()) + "/35 | TROPHIES: " +
                               std::to_string(getAchievementsCount()) + "/10 | ENDLESS BEST: WAVE " +
                               std::to_string(endlessBestWave) + " *";
        drawText(ren, progText, WINDOW_W * 0.5f, 122.0f, 1.35f, {255, 238, 175, 255}, true);

        auto addBtn = [&](const std::string& id, float x, float y, float w, float h,
                          const std::string& label, SDL_Color bg, SDL_Color hov, float sc = 2.0f) {
            UIButton b;
            b.id = id; b.rect = {x, y, w, h}; b.label = label;
            b.bgCol = bg; b.hoverCol = hov; b.textScale = sc;
            activeButtons.push_back(b);
            drawButton(ren, b, mouseX, mouseY);
        };

        float bw = 520.0f, bh = 36.0f, bx = WINDOW_W * 0.5f - bw * 0.5f;
        float halfW = (bw - 14.0f) * 0.5f;

        int nextLvl = getHighestUnlockedLevel();
        std::string playLabel = (getLevelsCleared() > 0) ? ("CONTINUE CAMPAIGN (LVL " + std::to_string(nextLvl + 1) + ") >>")
                                                         : "START CAMPAIGN (LEVEL 01) >>";
        addBtn("START_CAMPAIGN", bx, 146.0f, bw, bh, playLabel,
               {48, 112, 76, 250}, {66, 148, 102, 255}, 1.75f);

        addBtn("OPEN_LEVEL_SELECT", bx, 188.0f, halfW, bh, "LEVEL SELECT (55 LVLS)",
               {52, 74, 114, 250}, {76, 104, 156, 255}, 1.50f);

        std::string achLabel = "TROPHY HALL (" + std::to_string(getAchievementsCount()) + "/10)";
        addBtn("OPEN_ACHIEVEMENTS", bx + halfW + 14.0f, 188.0f, halfW, bh, achLabel,
               {58, 36, 78, 250}, {86, 54, 114, 255}, 1.50f);

        addBtn("OPEN_EDITOR", bx, 230.0f, bw, bh, "FORTRESS ARCHITECT (SANDBOX BUILDER) >>",
               {45, 78, 118, 250}, {65, 105, 155, 255}, 1.55f);

        addBtn("START_ENDLESS", bx, 272.0f, bw, bh, "ENDLESS SIEGE GAUNTLET (SURVIVAL) >>",
               {118, 48, 52, 250}, {158, 68, 72, 255}, 1.55f);

        // Dedicated Music and SFX Mute buttons side-by-side
        std::string musLabel = assets.musicMuted ? "MUSIC: MUTED" : "MUSIC: ON";
        SDL_Color musBg = assets.musicMuted ? SDL_Color{96, 48, 48, 250} : SDL_Color{42, 82, 108, 250};
        addBtn("TOGGLE_MUSIC", bx, 314.0f, halfW, bh, musLabel, musBg, {62, 116, 148, 255}, 1.50f);

        std::string sfxLabel = assets.sfxMuted ? "SFX: MUTED" : "SFX: ON";
        SDL_Color sfxBg = assets.sfxMuted ? SDL_Color{96, 48, 48, 250} : SDL_Color{42, 82, 108, 250};
        addBtn("TOGGLE_SFX", bx + halfW + 14.0f, 314.0f, halfW, bh, sfxLabel, sfxBg, {62, 116, 148, 255}, 1.50f);

        std::string fsLabel = std::string("FULLSCREEN: ") + (isFullscreen ? "ON  [F11]" : "OFF [F11]");
        addBtn("TOGGLE_FULLSCREEN", bx, 356.0f, halfW, bh, fsLabel,
               {68, 56, 96, 250}, {96, 80, 134, 255}, 1.50f);
        addBtn("QUIT_GAME", bx + halfW + 14.0f, 356.0f, halfW, bh, "QUIT GAME",
               {108, 46, 48, 250}, {148, 64, 66, 255}, 1.50f);

        // Hero swine introduction ribbon
        SDL_FRect heroBox{card.x + 24.0f, 402.0f, card.w - 48.0f, 62.0f};
        SDL_SetRenderDrawColor(ren, 22, 34, 52, 230);
        SDL_RenderFillRectF(ren, &heroBox);
        drawThickLine(ren, {heroBox.x, heroBox.y}, {heroBox.x + heroBox.w, heroBox.y}, 2.0f, {100, 160, 220, 200});
        drawThickLine(ren, {heroBox.x + heroBox.w, heroBox.y}, {heroBox.x + heroBox.w, heroBox.y + heroBox.h}, 2.0f, {100, 160, 220, 200});
        drawThickLine(ren, {heroBox.x + heroBox.w, heroBox.y + heroBox.h}, {heroBox.x, heroBox.y + heroBox.h}, 2.0f, {100, 160, 220, 200});
        drawThickLine(ren, {heroBox.x, heroBox.y + heroBox.h}, {heroBox.x, heroBox.y}, 2.0f, {100, 160, 220, 200});
        drawText(ren, "** 5TH HEROIC SWINE: GALE THE AERONAUT **", WINDOW_W * 0.5f, 412.0f, 1.45f, {140, 220, 255, 255}, true);
        drawText(ren, "Origami Sky-Blue Swine with Aviator Goggles: Mid-Air Aero Glide + Twin Aerial Iron Bombs!",
                 WINDOW_W * 0.5f, 436.0f, 1.20f, {210, 235, 255, 255}, true);

        std::string camStatus = enableSloMoCam ? "ENABLED" : "DISABLED";
        drawText(ren, "HOTKEYS: [C] FINISHING SLO-MO CAM (" + camStatus + ")  [M] MUSIC  [S] SFX  [F11] FULLSCREEN  [ESC] QUIT",
                 WINDOW_W * 0.5f, 480.0f, 1.25f, {178, 194, 214, 255}, true);

        drawText(ren, "CONTROLS: [SPACE] SPECIAL ABILITY  [TAB] SPEED (2.5X)  [RMB / A, D] PAN ARENA",
                 WINDOW_W * 0.5f, 502.0f, 1.25f, {150, 215, 185, 255}, true);
    }

    void renderLevelSelectScreen(SDL_Renderer* ren) {
        SDL_FRect topBar{0, 0, WINDOW_W, 70.0f};
        SDL_SetRenderDrawColor(ren, 20, 24, 36, 240);
        SDL_RenderFillRectF(ren, &topBar);

        std::string titleStr = (levelSelectPage == 6) ? "THE SECRET TRUFFLE VAULTS (LEVELS 141-150)" :
                               ("SIEGE CAMPAIGN - PAGE " + std::to_string(levelSelectPage + 1) + " OF 6");
        drawText(ren, titleStr, 260.0f, 15.0f, 1.70f, {255, 214, 92, 255}, false);
        std::string starSummary = "EARNED: " + std::to_string(getTotalStars()) + " STARS | TRUFFLES: " +
                                  std::to_string(getTrufflesFoundCount()) + " / 35 COLLECTED";
        drawText(ren, starSummary, 260.0f, 43.0f, 1.25f, {165, 238, 195, 255}, false);

        auto addBtn = [&](const std::string& id, float x, float y, float w, float h,
                          const std::string& label, SDL_Color bg, SDL_Color hov, float sc = 1.6f) {
            UIButton b;
            b.id = id; b.rect = {x, y, w, h}; b.label = label;
            b.bgCol = bg; b.hoverCol = hov; b.textScale = sc;
            activeButtons.push_back(b);
            drawButton(ren, b, mouseX, mouseY);
        };

        addBtn("BACK_TITLE", 15.0f, 14.0f, 85.0f, 42.0f, "< TITLE",
               {64, 52, 78, 250}, {94, 76, 114, 255}, 1.55f);

        // Mute buttons in top bar
        std::string musLabel = assets.musicMuted ? "MUS: OFF" : "MUS: ON";
        SDL_Color musBg = assets.musicMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
        addBtn("TOGGLE_MUSIC", 106.0f, 14.0f, 76.0f, 42.0f, musLabel, musBg, {62, 102, 126, 255}, 1.35f);

        std::string sfxLabel = assets.sfxMuted ? "SFX: OFF" : "SFX: ON";
        SDL_Color sfxBg = assets.sfxMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
        addBtn("TOGGLE_SFX", 188.0f, 14.0f, 70.0f, 42.0f, sfxLabel, sfxBg, {62, 102, 126, 255}, 1.35f);

        // Page Navigation on Right
        addBtn("PAGE_PREV", 724.0f, 14.0f, 64.0f, 42.0f, "< PREV",
               {48, 64, 96, 250}, {76, 104, 156, 255}, 1.35f);

        for (int p = 0; p < 6; ++p) {
            SDL_Color pBg = (levelSelectPage == p) ? SDL_Color{198, 132, 42, 255} : SDL_Color{48, 64, 96, 250};
            addBtn("GOTO_PAGE_" + std::to_string(p), 794.0f + p * 38.0f, 14.0f, 34.0f, 42.0f,
                   "P" + std::to_string(p + 1), pBg, {76, 104, 156, 255}, 1.40f);
        }

        addBtn("PAGE_NEXT", 1028.0f, 14.0f, 64.0f, 42.0f, "NEXT >",
               {48, 64, 96, 250}, {76, 104, 156, 255}, 1.35f);

        SDL_Color vaultBg = (levelSelectPage == 6) ? SDL_Color{198, 132, 42, 255} : SDL_Color{58, 36, 78, 250};
        addBtn("GOTO_PAGE_6", 1100.0f, 14.0f, 165.0f, 42.0f, "VAULTS (141-150)", vaultBg, {86, 54, 114, 255}, 1.35f);

        if (levelSelectPage == 6) {
            // 10 Secret Truffle Vaults (2 columns x 5 rows)
            const int cols = 2, rows = 5;
            const float cardW = 590.0f, cardH = 96.0f;
            const float gapX = 20.0f, gapY = 12.0f;
            const float startX = (WINDOW_W - (cols * cardW + gapX)) * 0.5f;
            const float startY = 84.0f;

            for (int c = 0; c < cols; ++c) {
                for (int r = 0; r < rows; ++r) {
                    int v = c * 5 + r;
                    if (v >= NUM_VAULT_LEVELS) break;
                    int idx = NUM_CAMPAIGN_LEVELS + v;
                    float cx = startX + c * (cardW + gapX);
                    float cy = startY + r * (cardH + gapY);
                    int req = getVaultTruffleReq(v);
                    bool unlocked = isLevelUnlocked(idx);
                    bool cleared = (bestStars[idx] > 0);

                    UIButton cardBtn;
                    cardBtn.id = unlocked ? ("PLAY_LEVEL_" + std::to_string(idx)) : "LOCKED_VAULT";
                    cardBtn.rect = {cx, cy, cardW, cardH};
                    cardBtn.label = "";

                    if (!unlocked) {
                        cardBtn.bgCol = {24, 20, 34, 235};
                        cardBtn.hoverCol = {24, 20, 34, 235};
                        cardBtn.borderCol = {72, 60, 92, 180};
                    } else if (cleared) {
                        cardBtn.bgCol = {36, 48, 68, 248};
                        cardBtn.hoverCol = {54, 72, 98, 255};
                        cardBtn.borderCol = (bestStars[idx] == 3) ? SDL_Color{255, 215, 0, 255} : SDL_Color{140, 180, 210, 255};
                    } else {
                        cardBtn.bgCol = {46, 28, 64, 245};
                        cardBtn.hoverCol = {72, 42, 98, 255};
                        cardBtn.borderCol = {224, 178, 65, 255};
                    }
                    activeButtons.push_back(cardBtn);
                    drawButton(ren, cardBtn, mouseX, mouseY);

                    std::string vNumStr = "SECRET VAULT " + std::string(v + 1 < 10 ? "0" : "") + std::to_string(v + 1);
                    if (!unlocked) {
                        drawText(ren, vNumStr, cx + 18.0f, cy + 14.0f, 1.9f, {150, 140, 170, 255}, false);
                        std::string reqStr = "[LOCKED - REQUIRES " + std::to_string(req) + " GOLDEN TRUFFLES]";
                        drawText(ren, reqStr, cx + 18.0f, cy + 42.0f, 1.45f, {220, 85, 85, 255}, false);
                        std::string progStr = "COLLECT IN CAMPAIGN (CURRENT: " + std::to_string(getTrufflesFoundCount()) + " / " + std::to_string(req) + ")";
                        drawText(ren, progStr, cx + 18.0f, cy + 68.0f, 1.25f, {170, 160, 190, 255}, false);
                        continue;
                    }

                    drawText(ren, levels[idx].name, cx + 18.0f, cy + 12.0f, 1.9f, {255, 218, 90, 255}, false);
                    drawText(ren, levels[idx].subtitle, cx + 18.0f, cy + 38.0f, 1.25f, {220, 225, 240, 255}, false);

                    // Stars
                    for (int s = 0; s < 3; ++s) {
                        Vec2 sc(cx + cardW - 100.0f + s * 28.0f, cy + 24.0f);
                        bool earned = (s < bestStars[idx]);
                        SDL_Color fCol = earned ? SDL_Color{255, 215, 0, 255} : SDL_Color{55, 50, 72, 255};
                        SDL_Color oCol = earned ? SDL_Color{218, 148, 28, 255} : SDL_Color{42, 38, 56, 255};
                        drawStarIcon(ren, sc, 10.5f, 4.8f, fCol, oCol);
                    }

                    if (cleared) {
                        std::string rec = "RECORD: " + formatNumber(bestScores[idx]) + " PTS  >> ENTER VAULT RAID >>";
                        drawText(ren, rec, cx + 18.0f, cy + 66.0f, 1.35f, {140, 245, 170, 255}, false);
                    } else {
                        drawText(ren, ">> VAULT UNLOCKED - CLICK TO ENTER THE RAID >>", cx + 18.0f, cy + 66.0f, 1.35f, {255, 220, 100, 255}, false);
                    }
                }
            }
            return;
        }

        const int cols = 5, rows = 5;
        const float cardW = 226.0f, cardH = 104.0f;
        const float gapX = 18.0f, gapY = 14.0f;
        const float startX = (WINDOW_W - (cols * cardW + (cols - 1) * gapX)) * 0.5f;
        const float startY = 86.0f;
        const int pageOffset = levelSelectPage * 25;
        int nextPlayableIdx = getHighestUnlockedLevel();

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                int idx = pageOffset + r * cols + c;
                if (idx >= NUM_CAMPAIGN_LEVELS) break;
                float cx = startX + c * (cardW + gapX);
                float cy = startY + r * (cardH + gapY);

                bool unlocked = isLevelUnlocked(idx);
                bool cleared = (bestStars[idx] > 0);
                bool isNext = (idx == nextPlayableIdx);

                UIButton cardBtn;
                cardBtn.id = unlocked ? ("PLAY_LEVEL_" + std::to_string(idx)) : "LOCKED_CARD";
                cardBtn.rect = {cx, cy, cardW, cardH};
                cardBtn.label = "";

                if (!unlocked) {
                    cardBtn.bgCol = {24, 28, 38, 230};
                    cardBtn.hoverCol = {24, 28, 38, 230};
                    cardBtn.borderCol = {62, 70, 84, 180};
                } else if (cleared) {
                    cardBtn.bgCol = {34, 52, 62, 245};
                    cardBtn.hoverCol = {52, 78, 94, 255};
                    cardBtn.borderCol = (bestStars[idx] == 3) ? SDL_Color{255, 212, 64, 255}
                                                              : SDL_Color{130, 168, 175, 255};
                } else if (isNext) {
                    cardBtn.bgCol = {42, 64, 46, 245};
                    cardBtn.hoverCol = {58, 92, 64, 255};
                    cardBtn.borderCol = {120, 245, 140, 255};
                } else {
                    cardBtn.bgCol = {34, 42, 62, 242};
                    cardBtn.hoverCol = {56, 72, 102, 255};
                    cardBtn.borderCol = {110, 130, 160, 255};
                }
                activeButtons.push_back(cardBtn);
                drawButton(ren, cardBtn, mouseX, mouseY);

                std::string numStr = "LEVEL " + std::string(idx + 1 < 10 ? "0" : "") + std::to_string(idx + 1);

                if (!unlocked) {
                    drawText(ren, numStr, cx + 12.0f, cy + 12.0f, 1.8f, {130, 138, 150, 255}, false);
                    drawText(ren, "[LOCKED]", cx + 12.0f, cy + 42.0f, 1.6f, {210, 84, 84, 255}, false);
                    std::string req = "BEAT LVL " + std::to_string(idx);
                    drawText(ren, req, cx + 12.0f, cy + 72.0f, 1.3f, {110, 120, 135, 255}, false);
                    continue;
                }

                std::string fullTitle = levels[idx].name;
                auto colon = fullTitle.find(':');
                std::string shortTitle = (colon != std::string::npos) ? fullTitle.substr(colon + 2) : fullTitle;
                if (shortTitle.size() > 22) shortTitle = shortTitle.substr(0, 22);

                drawText(ren, numStr, cx + 12.0f, cy + 10.0f, 1.85f, isNext ? SDL_Color{140, 255, 170, 255} : SDL_Color{255, 218, 102, 255}, false);
                drawText(ren, shortTitle, cx + 12.0f, cy + 32.0f, 1.20f, {225, 232, 245, 255}, false);

                // Stars
                for (int s = 0; s < 3; ++s) {
                    Vec2 sc(cx + 152.0f + s * 24.0f, cy + 16.0f);
                    bool earned = (s < bestStars[idx]);
                    SDL_Color fCol = earned ? SDL_Color{255, 212, 64, 255} : SDL_Color{55, 62, 78, 255};
                    SDL_Color oCol = earned ? SDL_Color{218, 148, 28, 255} : SDL_Color{42, 48, 62, 255};
                    drawStarIcon(ren, sc, 9.5f, 4.2f, fCol, oCol);
                }

                // Truffle indicator if level has a hidden golden truffle
                if (levels[idx].hasTruffle) {
                    if (trufflesFound[idx]) {
                        drawText(ren, "[TRUFFLE *]", cx + 146.0f, cy + 36.0f, 1.15f, {255, 215, 0, 255}, false);
                    } else {
                        drawText(ren, "[TRUFFLE ?]", cx + 146.0f, cy + 36.0f, 1.15f, {218, 165, 80, 190}, false);
                    }
                }

                if (cleared) {
                    std::string rec = "BEST: " + formatNumber(bestScores[idx]);
                    drawText(ren, rec, cx + 12.0f, cy + 56.0f, 1.35f, {165, 235, 190, 255}, false);
                } else if (isNext) {
                    drawText(ren, ">> UP NEXT <<", cx + 12.0f, cy + 56.0f, 1.35f, {120, 255, 160, 255}, false);
                } else {
                    drawText(ren, "UNBEATEN", cx + 12.0f, cy + 56.0f, 1.30f, {150, 165, 185, 255}, false);
                }

                // Milestone indicators on Level 6, 11, 21
                if (idx == 5) {
                    drawText(ren, "* Bramble Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {255, 210, 80, 255}, false);
                } else if (idx == 10) {
                    drawText(ren, "* Clover Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {140, 255, 200, 255}, false);
                } else if (idx == 20) {
                    drawText(ren, "* Vulcan Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {255, 150, 110, 255}, false);
                } else {
                    drawText(ren, "CLICK TO PLAY >>", cx + 12.0f, cy + 80.0f, 1.25f, {245, 206, 118, 255}, false);
                }
            }
        }
    }

    void renderBlock(SDL_Renderer* ren, const BlockBody& b, Vec2 cam = {0.0f, 0.0f}) {
        Vec2 screenPos = b.pos - cam;
        float c = std::cos(b.angle), s = std::sin(b.angle);
        auto rotPt = [&](float lx, float ly) -> Vec2 {
            return {screenPos.x + lx * c - ly * s, screenPos.y + lx * s + ly * c};
        };
        float hw = b.w * 0.5f, hh = b.h * 0.5f;
        Vec2 p0 = rotPt(-hw, -hh), p1 = rotPt(hw, -hh), p2 = rotPt(hw, hh), p3 = rotPt(-hw, hh);

        // Soft drop shadow behind block
        Vec2 off(4.0f, 5.0f);
        drawConvexPoly(ren, {p0 + off, p1 + off, p2 + off, p3 + off}, {12, 14, 22, 85});

        std::string texKey = (b.mat == MaterialTier::TWIG)   ? "sprite.mat_twig" :
                             (b.mat == MaterialTier::TIMBER ? "sprite.mat_timber" :
                             (b.mat == MaterialTier::STONE  ? "sprite.mat_stone" :
                             (b.mat == MaterialTier::ICE    ? "sprite.mat_ice" :
                             (b.mat == MaterialTier::TNT    ? "sprite.mat_tnt" :
                             (b.mat == MaterialTier::MAGMA_VENT ? "sprite.mat_magma" : "sprite.mat_trampoline")))));
        SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture(texKey) : nullptr;
        if (tex) {
            SDL_FRect dst{screenPos.x - b.w * 0.5f, screenPos.y - b.h * 0.5f, b.w, b.h};
            SDL_RenderCopyExF(ren, tex, nullptr, &dst, b.angle * (180.0 / PI), nullptr, SDL_FLIP_NONE);
        } else {
            SDL_Color fill = (b.mat == MaterialTier::TWIG)   ? SDL_Color{216, 180, 102, 255} :
                             (b.mat == MaterialTier::TIMBER ? SDL_Color{156, 90, 52, 255} :
                             (b.mat == MaterialTier::STONE  ? SDL_Color{94, 106, 122, 255} :
                             (b.mat == MaterialTier::ICE    ? SDL_Color{170, 230, 250, 230} :
                             (b.mat == MaterialTier::TNT    ? SDL_Color{192, 45, 36, 255} :
                             (b.mat == MaterialTier::MAGMA_VENT ? SDL_Color{42, 34, 38, 255} :
                                                               SDL_Color{36, 115, 82, 255})))));
            drawConvexPoly(ren, {p0, p1, p2, p3}, fill);
        }

        if (b.mat == MaterialTier::ICE) {
            Vec2 midL = rotPt(-hw, 0.0f);
            Vec2 midR = rotPt(hw, 0.0f);
            Vec2 midT = rotPt(0.0f, -hh);
            Vec2 midB = rotPt(0.0f, hh);
            drawThickLine(ren, midL, midT, 1.4f, {235, 250, 255, 170});
            drawThickLine(ren, midT, midR, 1.4f, {235, 250, 255, 170});
            drawThickLine(ren, midR, midB, 1.4f, {120, 195, 230, 190});
            drawThickLine(ren, midB, midL, 1.4f, {120, 195, 230, 190});
            drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 2.0f), rotPt(hw - 2.0f, -hh + 2.0f), 2.0f, {255, 255, 255, 220});
            drawThickLine(ren, p0, p1, 2.2f, {140, 215, 240, 245});
            drawThickLine(ren, p1, p2, 2.2f, {80, 168, 200, 245});
            drawThickLine(ren, p2, p3, 2.2f, {80, 168, 200, 245});
            drawThickLine(ren, p3, p0, 2.2f, {140, 215, 240, 245});
        } else if (b.mat == MaterialTier::TNT) {
            drawThickLine(ren, rotPt(-hw, -hh * 0.45f), rotPt(hw, -hh * 0.45f), 3.0f, {34, 38, 44, 255});
            drawThickLine(ren, rotPt(-hw,  hh * 0.45f), rotPt(hw,  hh * 0.45f), 3.0f, {34, 38, 44, 255});
            drawThickLine(ren, p0, p1, 2.3f, {48, 20, 18, 255});
            drawThickLine(ren, p1, p2, 2.3f, {48, 20, 18, 255});
            drawThickLine(ren, p2, p3, 2.3f, {48, 20, 18, 255});
            drawThickLine(ren, p3, p0, 2.3f, {48, 20, 18, 255});
            float tScale = std::min(b.w, b.h) / 22.0f;
            SDL_Color tntTextCol = (b.ignited && (static_cast<int>(b.tntFuse * 30.0f) % 2 == 0)) ?
                                   SDL_Color{255, 255, 255, 255} : SDL_Color{255, 235, 130, 255};
            drawText(ren, "TNT", screenPos.x, screenPos.y - 3.5f * tScale, tScale, tntTextCol, true, true);
            if (b.ignited) {
                Vec2 fuseTip = rotPt(0.0f, -hh - 5.0f);
                drawNGon(ren, fuseTip, 5.0f, 6, b.tntFuse * 40.0f, {255, 220, 60, 255});
                drawNGon(ren, fuseTip, 2.5f, 4, 0.0f, {255, 255, 255, 255});
            }
        } else if (b.mat == MaterialTier::MAGMA_VENT) {
            // Dark basalt rock with fiery glowing cracks and molten aperture
            drawThickLine(ren, rotPt(-hw * 0.45f, -hh * 0.2f), rotPt(hw * 0.35f, hh * 0.35f), 2.8f, {255, 80, 20, 255});
            drawThickLine(ren, rotPt(hw * 0.25f, -hh * 0.5f), rotPt(-hw * 0.15f, hh * 0.4f), 2.2f, {255, 180, 40, 255});
            drawThickLine(ren, rotPt(-hw * 0.35f, -hh + 3.0f), rotPt(hw * 0.35f, -hh + 3.0f), 3.5f, {255, 220, 60, 255});
            drawThickLine(ren, p0, p1, 2.4f, {32, 22, 24, 255});
            drawThickLine(ren, p1, p2, 2.4f, {32, 22, 24, 255});
            drawThickLine(ren, p2, p3, 2.4f, {32, 22, 24, 255});
            drawThickLine(ren, p3, p0, 2.4f, {32, 22, 24, 255});
        } else if (b.mat == MaterialTier::TRAMPOLINE) {
            // High-tension trampoline deflector frame & woven emerald mat
            drawConvexPoly(ren, {p0, p1, p2, p3}, {32, 38, 48, 255});
            float inHw = hw - 8.0f;
            float inHh = hh - 5.0f;
            Vec2 m0 = rotPt(-inHw, -inHh), m1 = rotPt(inHw, -inHh),
                 m2 = rotPt(inHw, inHh),   m3 = rotPt(-inHw, inHh);
            drawConvexPoly(ren, {m0, m1, m2, m3}, {38, 120, 84, 255});
            int numRibs = static_cast<int>(b.w / 14.0f);
            if (numRibs < 2) numRibs = 2;
            for (int r = 1; r < numRibs; ++r) {
                float rx = -inHw + (2.0f * inHw * r) / numRibs;
                drawThickLine(ren, rotPt(rx, -inHh), rotPt(rx, inHh), 1.8f, {58, 172, 122, 220});
            }
            for (int r = 0; r < numRibs - 1; ++r) {
                float rx = -inHw + (2.0f * inHw * (r + 0.5f)) / numRibs;
                drawThickLine(ren, rotPt(rx - 3.5f, 0.0f), rotPt(rx, -3.5f), 1.8f, {255, 230, 95, 240});
                drawThickLine(ren, rotPt(rx, -3.5f), rotPt(rx + 3.5f, 0.0f), 1.8f, {255, 230, 95, 240});
            }
            drawThickLine(ren, p0, p1, 2.5f, {78, 86, 102, 255});
            drawThickLine(ren, p1, p2, 2.5f, {78, 86, 102, 255});
            drawThickLine(ren, p2, p3, 2.5f, {78, 86, 102, 255});
            drawThickLine(ren, p3, p0, 2.5f, {78, 86, 102, 255});
            drawNGon(ren, rotPt(-hw + 5.0f, -hh + 5.0f), 2.8f, 4, 0.0f, {228, 185, 75, 255});
            drawNGon(ren, rotPt(hw - 5.0f, -hh + 5.0f),  2.8f, 4, 0.0f, {228, 185, 75, 255});
            drawNGon(ren, rotPt(hw - 5.0f, hh - 5.0f),   2.8f, 4, 0.0f, {228, 185, 75, 255});
            drawNGon(ren, rotPt(-hw + 5.0f, hh - 5.0f),  2.8f, 4, 0.0f, {228, 185, 75, 255});
        } else {
            // Distinct Architectural Detailing Based on Material & Aspect Ratio
            if (b.mat == MaterialTier::STONE) {
                if (b.w >= 60.0f && b.w / b.h >= 2.0f) {
                    // Wide Stone Architrave / Lintel: Render masonry relief groove & vertical block seams
                    drawThickLine(ren, rotPt(-hw + 6.0f, 0.0f), rotPt(hw - 6.0f, 0.0f), 1.6f, {65, 75, 88, 220});
                    int seams = static_cast<int>(b.w / 40.0f);
                    for (int sm = 1; sm < seams; ++sm) {
                        float sx = -hw + (b.w * sm) / seams;
                        drawThickLine(ren, rotPt(sx, -hh + 3.0f), rotPt(sx, hh - 3.0f), 1.6f, {65, 75, 88, 220});
                    }
                    drawThickLine(ren, rotPt(-hw + 8.0f, hh - 2.5f), rotPt(hw - 8.0f, hh - 2.5f), 1.8f, {48, 56, 68, 240});
                } else if (b.h >= 60.0f && b.h / b.w >= 2.0f) {
                    // Tall Classical Column / Stele: Capital and base rims + vertical fluting grooves
                    drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 7.0f), rotPt(hw - 2.0f, -hh + 7.0f), 2.0f, {135, 148, 168, 240});
                    drawThickLine(ren, rotPt(-hw + 2.0f, hh - 7.0f), rotPt(hw - 2.0f, hh - 7.0f), 2.0f, {55, 62, 74, 240});
                    drawThickLine(ren, rotPt(-hw * 0.35f, -hh + 9.0f), rotPt(-hw * 0.35f, hh - 9.0f), 1.4f, {68, 76, 88, 200});
                    drawThickLine(ren, rotPt(hw * 0.35f, -hh + 9.0f), rotPt(hw * 0.35f, hh - 9.0f), 1.4f, {68, 76, 88, 200});
                } else if (b.w >= 40.0f && b.h >= 40.0f) {
                    // Heavy Dressed Ashlar Megalith / Plinth: Inset masonry relief panel
                    drawThickLine(ren, rotPt(-hw + 6.0f, -hh + 6.0f), rotPt(hw - 6.0f, -hh + 6.0f), 1.5f, {125, 138, 155, 200});
                    drawThickLine(ren, rotPt(hw - 6.0f, -hh + 6.0f), rotPt(hw - 6.0f, hh - 6.0f), 1.5f, {55, 62, 72, 200});
                    drawThickLine(ren, rotPt(hw - 6.0f, hh - 6.0f), rotPt(-hw + 6.0f, hh - 6.0f), 1.5f, {55, 62, 72, 200});
                    drawThickLine(ren, rotPt(-hw + 6.0f, hh - 6.0f), rotPt(-hw + 6.0f, -hh + 6.0f), 1.5f, {125, 138, 155, 200});
                }
            } else if (b.mat == MaterialTier::TIMBER) {
                if (b.w >= 34.0f && b.h >= 34.0f && std::abs(b.w - b.h) < 16.0f) {
                    // Reinforced Timber Crate: Cross-brace "X" with iron corner rivets
                    drawThickLine(ren, rotPt(-hw + 5.0f, -hh + 5.0f), rotPt(hw - 5.0f, hh - 5.0f), 2.2f, {112, 60, 32, 240});
                    drawThickLine(ren, rotPt(-hw + 5.0f, hh - 5.0f), rotPt(hw - 5.0f, -hh + 5.0f), 2.2f, {112, 60, 32, 240});
                    drawNGon(ren, rotPt(-hw + 5.0f, -hh + 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                    drawNGon(ren, rotPt(hw - 5.0f, -hh + 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                    drawNGon(ren, rotPt(hw - 5.0f, hh - 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                    drawNGon(ren, rotPt(-hw + 5.0f, hh - 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                } else if (b.w >= 60.0f && b.w / b.h >= 2.0f) {
                    // Wide Timber Girder: Longitudinal grain lines and iron bolt pegs
                    drawThickLine(ren, rotPt(-hw + 14.0f, -hh * 0.3f), rotPt(hw - 14.0f, -hh * 0.3f), 1.4f, {115, 62, 34, 210});
                    drawThickLine(ren, rotPt(-hw + 14.0f,  hh * 0.3f), rotPt(hw - 14.0f,  hh * 0.3f), 1.4f, {115, 62, 34, 210});
                    drawNGon(ren, rotPt(-hw + 10.0f, 0.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                    drawNGon(ren, rotPt(hw - 10.0f, 0.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                } else if (b.h >= 55.0f && b.h / b.w >= 2.0f) {
                    // Tall Timber Post: Vertical wood grain and iron collar bands
                    drawThickLine(ren, rotPt(0.0f, -hh + 12.0f), rotPt(0.0f, hh - 12.0f), 1.4f, {115, 62, 34, 210});
                    drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 14.0f), rotPt(hw - 2.0f, -hh + 14.0f), 2.4f, {52, 48, 44, 245});
                    drawThickLine(ren, rotPt(-hw + 2.0f,  hh - 14.0f), rotPt(hw - 2.0f,  hh - 14.0f), 2.4f, {52, 48, 44, 245});
                }
            } else if (b.mat == MaterialTier::TWIG) {
                // Bamboo / Wicker nodes along length
                if (b.w >= 50.0f) {
                    int nodes = static_cast<int>(b.w / 22.0f);
                    for (int nd = 1; nd < nodes; ++nd) {
                        float nx = -hw + (b.w * nd) / nodes;
                        drawThickLine(ren, rotPt(nx, -hh + 2.0f), rotPt(nx, hh - 2.0f), 2.0f, {158, 126, 62, 230});
                        drawNGon(ren, rotPt(nx, 0.0f), 2.2f, 4, 0.0f, {245, 210, 130, 240});
                    }
                } else if (b.h >= 50.0f) {
                    int nodes = static_cast<int>(b.h / 22.0f);
                    for (int nd = 1; nd < nodes; ++nd) {
                        float ny = -hh + (b.h * nd) / nodes;
                        drawThickLine(ren, rotPt(-hw + 2.0f, ny), rotPt(hw - 2.0f, ny), 2.0f, {158, 126, 62, 230});
                        drawNGon(ren, rotPt(0.0f, ny), 2.2f, 4, 0.0f, {245, 210, 130, 240});
                    }
                }
            }
            // Inner highlight bevel along top edge & crisp woodblock outline
            drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 2.0f), rotPt(hw - 2.0f, -hh + 2.0f),
                          1.8f, {255, 245, 220, 70});
            drawThickLine(ren, p0, p1, 2.3f, {28, 22, 18, 235});
            drawThickLine(ren, p1, p2, 2.3f, {28, 22, 18, 235});
            drawThickLine(ren, p2, p3, 2.3f, {28, 22, 18, 235});
            drawThickLine(ren, p3, p0, 2.3f, {28, 22, 18, 235});
        }

        float hpRatio = b.hp / b.maxHp;
        if (hpRatio < 0.66f) {
            SDL_Color crackCol = (b.mat == MaterialTier::ICE) ? SDL_Color{255, 255, 255, 230} : SDL_Color{22, 18, 16, 235};
            drawThickLine(ren, rotPt(-hw * 0.5f, -hh * 0.6f), rotPt(hw * 0.2f, hh * 0.4f),
                          2.2f, crackCol);
        }
        if (hpRatio < 0.35f) {
            SDL_Color crackCol = (b.mat == MaterialTier::ICE) ? SDL_Color{255, 255, 255, 245} : SDL_Color{16, 12, 10, 255};
            drawThickLine(ren, rotPt(hw * 0.6f, -hh * 0.5f), rotPt(-hw * 0.3f, hh * 0.6f),
                          2.6f, crackCol);
        }
    }

    void renderPendulum(SDL_Renderer* ren, const PendulumBody& pend, Vec2 cam = {0.0f, 0.0f}) {
        Vec2 screenAnchor = pend.anchor - cam;
        Vec2 bPos = pend.getBallPos();
        Vec2 screenBall = bPos - cam;

        // 1. Pivot bracket at ceiling
        drawNGon(ren, screenAnchor, 11.0f, 6, 0.0f, {85, 92, 108, 255});
        drawNGon(ren, screenAnchor, 6.0f, 6, 0.0f, {218, 175, 75, 255});

        // 2. Chain links from anchor to ball
        Vec2 seg = screenBall - screenAnchor;
        float segLen = seg.length();
        int numLinks = static_cast<int>(segLen / 16.0f);
        if (numLinks < 2) numLinks = 2;
        Vec2 linkDir = (segLen > 1.0f) ? (seg * (1.0f / segLen)) : Vec2(0.0f, 1.0f);
        Vec2 linkPerp(-linkDir.y, linkDir.x);

        for (int i = 1; i < numLinks; ++i) {
            float frac = static_cast<float>(i) / numLinks;
            Vec2 lp = screenAnchor + seg * frac;
            if (i % 2 == 0) {
                drawThickLine(ren, lp - linkDir * 6.0f - linkPerp * 3.5f,
                                   lp + linkDir * 6.0f - linkPerp * 3.5f, 2.5f, {72, 78, 92, 255});
                drawThickLine(ren, lp - linkDir * 6.0f + linkPerp * 3.5f,
                                   lp + linkDir * 6.0f + linkPerp * 3.5f, 2.5f, {72, 78, 92, 255});
                drawThickLine(ren, lp - linkDir * 6.0f - linkPerp * 3.5f,
                                   lp - linkDir * 6.0f + linkPerp * 3.5f, 2.5f, {95, 102, 118, 255});
                drawThickLine(ren, lp + linkDir * 6.0f - linkPerp * 3.5f,
                                   lp + linkDir * 6.0f + linkPerp * 3.5f, 2.5f, {50, 54, 65, 255});
            } else {
                drawThickLine(ren, lp - linkDir * 7.0f, lp + linkDir * 7.0f, 4.0f, {90, 96, 112, 255});
                drawThickLine(ren, lp - linkDir * 5.0f, lp + linkDir * 5.0f, 2.0f, {145, 152, 168, 255});
            }
        }

        // 3. Drop shadow under ball
        drawNGon(ren, screenBall + Vec2(5.0f, 6.0f), pend.radius, 12, pend.angle, {15, 18, 26, 90});

        // 4. Spiked Iron Wrecking Ball
        if (pend.spiked) {
            for (int s = 0; s < 8; ++s) {
                float sa = pend.angle + s * (2.0f * PI / 8.0f);
                Vec2 spTip  = screenBall + Vec2(std::cos(sa), std::sin(sa)) * (pend.radius + 12.0f);
                Vec2 pLeft  = screenBall + Vec2(std::cos(sa + 0.18f), std::sin(sa + 0.18f)) * (pend.radius - 2.0f);
                Vec2 pRight = screenBall + Vec2(std::cos(sa - 0.18f), std::sin(sa - 0.18f)) * (pend.radius - 2.0f);
                drawTriangle(ren, spTip, pLeft, pRight, {82, 88, 102, 255});
                drawTriangle(ren, spTip, pLeft, screenBall + Vec2(std::cos(sa), std::sin(sa)) * (pend.radius - 2.0f), {55, 60, 72, 255});
            }
        }

        drawNGon(ren, screenBall, pend.radius, 12, pend.angle, {58, 62, 75, 255});
        drawNGon(ren, screenBall, pend.radius * 0.82f, 10, pend.angle, {72, 78, 94, 255});
        drawThickLine(ren, screenBall - Vec2(pend.radius * 0.8f, 0.0f),
                           screenBall + Vec2(pend.radius * 0.8f, 0.0f), 5.0f, {42, 46, 56, 255});
        for (int r = -2; r <= 2; ++r) {
            drawNGon(ren, screenBall + Vec2(r * 8.0f, 0.0f), 2.2f, 4, 0.0f, {218, 175, 75, 255});
        }
        drawNGon(ren, screenBall + Vec2(-pend.radius * 0.35f, -pend.radius * 0.35f), pend.radius * 0.28f, 6, 0.0f, {140, 150, 170, 200});
    }

    void renderPig(SDL_Renderer* ren, const PigBody& p, Vec2 cam = {0.0f, 0.0f}) {
        Vec2 screenPos = p.pos - cam;
        float screenGroundY = GROUND_Y - cam.y;
        if (p.pos.y + p.radius >= GROUND_Y - 6.0f) {
            drawConvexPoly(ren, {{screenPos.x - p.radius * 0.85f, screenGroundY + 2},
                                 {screenPos.x + p.radius * 0.85f, screenGroundY + 2},
                                 {screenPos.x + p.radius * 0.65f, screenGroundY + 6},
                                 {screenPos.x - p.radius * 0.65f, screenGroundY + 6}}, {12, 15, 22, 105});
        }
        std::string key = (p.type == PigType::TROTTER)  ? "sprite.pig_trotter" :
                          (p.type == PigType::TURBO    ? "sprite.pig_turbo" :
                          (p.type == PigType::SPLIT    ? "sprite.pig_split" :
                          (p.type == PigType::BOMB     ? "sprite.pig_bomb" :
                          (p.type == PigType::GLIDER   ? "sprite.pig_glider" :
                          (p.type == PigType::GRAVITON ? "sprite.pig_graviton" : "sprite.pig_drill")))));
        SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture(key) : nullptr;
        if (tex) {
            SDL_FRect dst{screenPos.x - p.radius * 1.15f, screenPos.y - p.radius * 1.15f,
                          p.radius * 2.3f, p.radius * 2.3f};
            SDL_RenderCopyExF(ren, tex, nullptr, &dst, p.angle * (180.0 / PI), nullptr, SDL_FLIP_NONE);
        }
        renderOrigamiPigGeometry(ren, p.type, screenPos, p.radius, p.angle);
    }

    void renderBird(SDL_Renderer* ren, const BirdTarget& t, Vec2 cam = {0.0f, 0.0f}) {
        Vec2 screenPos = t.pos - cam;
        float screenGroundY = GROUND_Y - cam.y;
        if (t.pos.y + t.radius >= GROUND_Y - 6.0f) {
            drawConvexPoly(ren, {{screenPos.x - t.radius * 0.85f, screenGroundY + 2},
                                 {screenPos.x + t.radius * 0.85f, screenGroundY + 2},
                                 {screenPos.x + t.radius * 0.65f, screenGroundY + 6},
                                 {screenPos.x - t.radius * 0.65f, screenGroundY + 6}}, {12, 15, 22, 105});
        }
        std::string key = (t.kind == BirdClass::TOUCAN) ? "sprite.bird_toucan" :
                          (t.kind == BirdClass::OWL    ? "sprite.bird_owl" :
                          (t.kind == BirdClass::FALCON ? "sprite.bird_falcon" :
                          (t.kind == BirdClass::CONDOR ? "sprite.bird_condor" : "sprite.bird_heron")));
        SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture(key) : nullptr;
        if (tex) {
            SDL_FRect dst{screenPos.x - t.radius * 1.1f, screenPos.y - t.radius * 1.1f,
                          t.radius * 2.2f, t.radius * 2.2f};
            SDL_RenderCopyExF(ren, tex, nullptr, &dst, t.angle * (180.0 / PI), nullptr, SDL_FLIP_NONE);
        }
        float helmRatio = (t.maxHelmetHp > 0.0f) ? (t.helmetHp / t.maxHelmetHp) : 1.0f;
        float shieldRatio = (t.maxShieldHp > 0.0f) ? (t.shieldHp / t.maxShieldHp) : 1.0f;
        float balloonRatio = (t.maxBalloonHp > 0.0f) ? (t.balloonHp / t.maxBalloonHp) : 1.0f;
        renderTotemBirdGeometry(ren, t.kind, screenPos, t.radius, t.angle, t.hp / t.maxHp, t.hasHelmet, helmRatio, t.hasShield, shieldRatio, t.inZeppelin, balloonRatio);
    }

    void renderGoldenTruffle(SDL_Renderer* ren, Vec2 cam = {0.0f, 0.0f}) {
        if (!levelTruffle.exists || levelTruffle.collected) return;
        Vec2 screenPos = levelTruffle.pos - cam;
        float pulse = std::sin(levelTruffle.pulseTime * 4.5f);
        float auraRad = levelTruffle.radius * (1.25f + 0.20f * pulse);

        // Radiant golden aura
        drawNGon(ren, screenPos, auraRad * 1.5f, 14, levelTruffle.pulseTime * 0.5f, {255, 215, 0, 40});
        drawNGon(ren, screenPos, auraRad * 1.1f, 12, -levelTruffle.pulseTime * 0.8f, {255, 230, 80, 75});

        // Sprite texture if loaded
        SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture("sprite.item_truffle") : nullptr;
        if (tex) {
            float sSize = levelTruffle.radius * 2.4f;
            SDL_FRect dst{screenPos.x - sSize * 0.5f, screenPos.y - sSize * 0.5f, sSize, sSize};
            SDL_RenderCopyF(ren, tex, nullptr, &dst);
        } else {
            // Origami geometric golden truffle
            float r = levelTruffle.radius;
            drawNGon(ren, screenPos, r, 8, 0.0f, {218, 165, 32, 255});
            drawNGon(ren, screenPos, r * 0.75f, 8, PI * 0.125f, {255, 215, 0, 255});
            drawNGon(ren, screenPos, r * 0.45f, 6, 0.0f, {255, 245, 140, 255});
        }

        // 4-point rotating golden specular glint flare
        float gSize = 8.0f + 4.0f * pulse;
        drawThickLine(ren, {screenPos.x - gSize, screenPos.y}, {screenPos.x + gSize, screenPos.y}, 2.0f, {255, 248, 180, 230});
        drawThickLine(ren, {screenPos.x, screenPos.y - gSize}, {screenPos.x, screenPos.y + gSize}, 2.0f, {255, 248, 180, 230});
        drawNGon(ren, screenPos, 2.5f, 4, 0.0f, {255, 255, 255, 255});
    }

    void renderAchievementToast(SDL_Renderer* ren) {
        if (!toast.active) return;
        float tw = 520.0f, th = 66.0f;
        float tx = WINDOW_W * 0.5f - tw * 0.5f;
        float ty = toast.slideY;

        // Shadow
        SDL_FRect shd{tx + 4.0f, ty + 5.0f, tw, th};
        SDL_SetRenderDrawColor(ren, 10, 10, 18, 110);
        SDL_RenderFillRectF(ren, &shd);

        // Toast background: royal amethyst/violet
        SDL_FRect bg{tx, ty, tw, th};
        SDL_SetRenderDrawColor(ren, 32, 18, 48, 250);
        SDL_RenderFillRectF(ren, &bg);

        // Vibrant gold border
        drawThickLine(ren, {tx, ty}, {tx + tw, ty}, 2.5f, {255, 215, 0, 255});
        drawThickLine(ren, {tx + tw, ty}, {tx + tw, ty + th}, 2.5f, {255, 215, 0, 255});
        drawThickLine(ren, {tx + tw, ty + th}, {tx, ty + th}, 2.5f, {255, 215, 0, 255});
        drawThickLine(ren, {tx, ty + th}, {tx, ty}, 2.5f, {255, 215, 0, 255});

        // Trophy icon on the left
        SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture("sprite.ui_trophy") : nullptr;
        if (tex) {
            SDL_FRect iconDst{tx + 14.0f, ty + 11.0f, 44.0f, 44.0f};
            SDL_RenderCopyF(ren, tex, nullptr, &iconDst);
        } else {
            drawNGon(ren, {tx + 36.0f, ty + 33.0f}, 16.0f, 6, 0.0f, {255, 215, 0, 255});
        }

        // Header & Title
        drawText(ren, "*** ACHIEVEMENT UNLOCKED! ***", tx + 72.0f, ty + 10.0f, 1.40f, {255, 225, 90, 255}, false);
        drawText(ren, toast.title, tx + 72.0f, ty + 26.0f, 1.85f, {255, 255, 255, 255}, false);
        drawText(ren, toast.desc, tx + 72.0f, ty + 46.0f, 1.15f, {220, 210, 245, 255}, false);

        // Corner sparkle star glints
        drawNGon(ren, {tx + 8.0f, ty + 8.0f}, 3.0f, 4, 0.0f, {255, 240, 160, 240});
        drawNGon(ren, {tx + tw - 8.0f, ty + 8.0f}, 3.0f, 4, 0.0f, {255, 240, 160, 240});
    }

    void renderAchievementsScreen(SDL_Renderer* ren) {
        // Top bar
        SDL_FRect topBar{0, 0, WINDOW_W, 70.0f};
        SDL_SetRenderDrawColor(ren, 20, 16, 32, 245);
        SDL_RenderFillRectF(ren, &topBar);
        drawThickLine(ren, {0, 70.0f}, {WINDOW_W, 70.0f}, 2.0f, {224, 178, 65, 220});

        auto addBtn = [&](const std::string& id, float x, float y, float w, float h,
                          const std::string& label, SDL_Color bg, SDL_Color hov, float sc = 1.6f) {
            UIButton b;
            b.id = id; b.rect = {x, y, w, h}; b.label = label;
            b.bgCol = bg; b.hoverCol = hov; b.textScale = sc;
            activeButtons.push_back(b);
            drawButton(ren, b, mouseX, mouseY);
        };

        addBtn("BACK_TITLE", 20.0f, 14.0f, 120.0f, 42.0f, "< TITLE",
               {64, 42, 82, 250}, {96, 64, 124, 255}, 1.65f);

        drawText(ren, "ROYAL SWINE TROPHY HALL & ACHIEVEMENTS", WINDOW_W * 0.5f, 14.0f, 2.35f, {255, 215, 0, 255}, true);
        std::string statStr = "* ACHIEVEMENTS: " + std::to_string(getAchievementsCount()) + " / 10 UNLOCKED  |  " +
                              "GOLDEN TRUFFLES: " + std::to_string(getTrufflesFoundCount()) + " / 35 COLLECTED *";
        drawText(ren, statStr, WINDOW_W * 0.5f, 43.0f, 1.45f, {235, 215, 170, 255}, true);

        // Grid of 12 achievements: 6 rows x 2 cols
        const float cardW = 580.0f, cardH = 78.0f;
        const float colX[2] = {45.0f, 655.0f};
        const float rowY[6] = {76.0f, 160.0f, 244.0f, 328.0f, 412.0f, 496.0f};

        for (size_t i = 0; i < achievementDefs.size() && i < 12; ++i) {
            const auto& def = achievementDefs[i];
            bool unlocked = achievementsUnlocked[i];
            int c = static_cast<int>(i % 2);
            int r = static_cast<int>(i / 2);
            float cx = colX[c], cy = rowY[r];

            // Card background & border
            SDL_FRect cardRect{cx, cy, cardW, cardH};
            SDL_SetRenderDrawColor(ren, unlocked ? 38 : 20, unlocked ? 24 : 22, unlocked ? 54 : 32, 240);
            SDL_RenderFillRectF(ren, &cardRect);

            SDL_Color borderCol = unlocked ? SDL_Color{235, 195, 65, 255} : SDL_Color{62, 68, 86, 180};
            drawThickLine(ren, {cx, cy}, {cx + cardW, cy}, unlocked ? 2.2f : 1.5f, borderCol);
            drawThickLine(ren, {cx + cardW, cy}, {cx + cardW, cy + cardH}, unlocked ? 2.2f : 1.5f, borderCol);
            drawThickLine(ren, {cx + cardW, cy + cardH}, {cx, cy + cardH}, unlocked ? 2.2f : 1.5f, borderCol);
            drawThickLine(ren, {cx, cy + cardH}, {cx, cy}, unlocked ? 2.2f : 1.5f, borderCol);

            // Icon / Trophy on left
            if (unlocked) {
                SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture("sprite.ui_trophy") : nullptr;
                if (tex) {
                    SDL_FRect iconDst{cx + 14.0f, cy + 11.0f, 56.0f, 56.0f};
                    SDL_RenderCopyF(ren, tex, nullptr, &iconDst);
                } else {
                    drawNGon(ren, {cx + 42.0f, cy + 39.0f}, 22.0f, 6, 0.0f, {255, 215, 0, 255});
                }
            } else {
                drawNGon(ren, {cx + 42.0f, cy + 39.0f}, 20.0f, 6, 0.0f, {45, 50, 65, 255});
                drawText(ren, "?", cx + 42.0f, cy + 28.0f, 2.0f, {120, 130, 150, 255}, true);
            }

            // Title & Description
            SDL_Color titleCol = unlocked ? SDL_Color{255, 224, 90, 255} : SDL_Color{160, 170, 188, 255};
            drawText(ren, def.title, cx + 84.0f, cy + 10.0f, 1.80f, titleCol, false);

            SDL_Color descCol = unlocked ? SDL_Color{225, 230, 245, 255} : SDL_Color{130, 138, 154, 255};
            drawText(ren, def.desc, cx + 84.0f, cy + 33.0f, 1.20f, descCol, false);

            // Status Badge
            if (unlocked) {
                drawText(ren, "[UNLOCKED *]", cx + 84.0f, cy + 55.0f, 1.30f, {120, 250, 150, 255}, false);
            } else {
                std::string st = "[LOCKED]";
                if (def.id == AchievementId::CAULDRON_CRACKER) {
                    st += "  PROGRESS: " + std::to_string(achievementProgress[static_cast<int>(AchievementId::CAULDRON_CRACKER)]) + "/10";
                } else if (def.id == AchievementId::TRUFFLE_CONNOISSEUR) {
                    st += "  FOUND: " + std::to_string(getTrufflesFoundCount()) + "/20";
                } else if (def.id == AchievementId::VAULT_CONQUEROR) {
                    int vaultsCleared = 0;
                    for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) {
                        if (bestStars[v] > 0) vaultsCleared++;
                    }
                    st += "  CLEARED: " + std::to_string(vaultsCleared) + "/" + std::to_string(NUM_VAULT_LEVELS);
                }
                drawText(ren, st, cx + 84.0f, cy + 55.0f, 1.25f, {210, 95, 95, 255}, false);
            }
        }

        // Bottom Banner
        SDL_FRect botBar{0, WINDOW_H - 42.0f, WINDOW_W, 42.0f};
        SDL_SetRenderDrawColor(ren, 18, 14, 28, 230);
        SDL_RenderFillRectF(ren, &botBar);
        drawText(ren, "* COLLECT HIDDEN GOLDEN TRUFFLES IN CAMPAIGN TO UNLOCK 10 SECRET VAULT CHALLENGE LEVELS *",
                 WINDOW_W * 0.5f, WINDOW_H - 28.0f, 1.40f, {255, 215, 80, 255}, true);
    }

    void renderEditorScreen(SDL_Renderer* ren) {
        // 1. Slingshot Launchpad in Editor
        renderSiegeCatapultBase(ren, 180.0f, SLING_Y);
        drawThickLine(ren, {180, SLING_Y + 28.0f}, {162, SLING_Y - 12.0f}, 12.0f, {126, 80, 48, 255});
        renderOrigamiPigGeometry(ren, PigType::TROTTER, {180, SLING_Y - 4.0f}, 24.0f, -0.08f);
        drawText(ren, "SWINE LAUNCHPAD", 180.0f, SLING_Y + 44.0f, 1.25f, {210, 225, 245, 220}, true);

        // 2. Construction Zone Indicator & Grid
        drawThickLine(ren, {360.0f, 72.0f}, {WINDOW_W - 10.0f, 72.0f}, 1.5f, {80, 160, 220, 120});
        drawThickLine(ren, {360.0f, 72.0f}, {360.0f, GROUND_Y}, 1.5f, {80, 160, 220, 120});
        drawText(ren, "** FORTRESS ARCHITECT CONSTRUCTION ZONE **", 780.0f, 84.0f, 1.30f, {120, 190, 255, 160}, true);

        if (editorSnapGrid) {
            SDL_SetRenderDrawColor(ren, 140, 180, 220, 50);
            for (float gx = 360.0f; gx <= WINDOW_W - 10.0f; gx += 20.0f) {
                for (float gy = 80.0f; gy <= GROUND_Y; gy += 20.0f) {
                    SDL_FRect dot{gx - 1.0f, gy - 1.0f, 2.0f, 2.0f};
                    SDL_RenderFillRectF(ren, &dot);
                }
            }
        }

        // 3. Render Placed Elements from customLevel
        for (const auto& b : customLevel.blocks) {
            renderBlock(ren, b, {0.0f, 0.0f});
        }
        for (const auto& p : customLevel.pendulums) {
            renderPendulum(ren, p, {0.0f, 0.0f});
        }
        for (const auto& bt : customLevel.birds) {
            renderBird(ren, bt, {0.0f, 0.0f});
        }
        if (customLevel.hasTruffle) {
            GoldenTruffle bk = levelTruffle;
            levelTruffle.exists = true;
            levelTruffle.collected = false;
            levelTruffle.pos = customLevel.trufflePos;
            levelTruffle.radius = 16.0f;
            renderGoldenTruffle(ren, {0.0f, 0.0f});
            levelTruffle = bk;
        }

        // 4. Ghost Placement Cursor
        if (mouseX > 218.0f && mouseY > 72.0f && mouseY < GROUND_Y + 10.0f) {
            float gx = editorSnapGrid ? (std::round(mouseX / 20.0f) * 20.0f) : mouseX;
            float gy = editorSnapGrid ? (std::round(mouseY / 20.0f) * 20.0f) : mouseY;

            if (editorTool == 0) { // BLOCK
                SDL_FRect r{gx - editorBlockW * 0.5f, gy - editorBlockH * 0.5f, editorBlockW, editorBlockH};
                SDL_SetRenderDrawColor(ren, 255, 235, 120, 130);
                SDL_RenderFillRectF(ren, &r);
                drawThickLine(ren, {r.x, r.y}, {r.x + r.w, r.y}, 2.0f, {255, 215, 0, 240});
                drawThickLine(ren, {r.x + r.w, r.y}, {r.x + r.w, r.y + r.h}, 2.0f, {255, 215, 0, 240});
                drawThickLine(ren, {r.x + r.w, r.y + r.h}, {r.x, r.y + r.h}, 2.0f, {255, 215, 0, 240});
                drawThickLine(ren, {r.x, r.y + r.h}, {r.x, r.y}, 2.0f, {255, 215, 0, 240});
            } else if (editorTool == 1) { // BIRD
                drawNGon(ren, {gx, gy}, 20.0f, 12, 0.0f, {255, 215, 0, 140});
                drawText(ren, "BIRD", gx, gy - 6.0f, 1.20f, {255, 255, 255, 220}, true);
            } else if (editorTool == 2) { // PENDULUM
                drawNGon(ren, {gx, gy}, 6.0f, 8, 0.0f, {255, 215, 0, 200});
                drawThickLine(ren, {gx, gy}, {gx + 30.0f, gy + 80.0f}, 2.5f, {220, 220, 240, 160});
                drawNGon(ren, {gx + 30.0f, gy + 80.0f}, 18.0f, 10, 0.0f, {255, 140, 90, 160});
            } else if (editorTool == 3) { // TRUFFLE
                drawNGon(ren, {gx, gy}, 16.0f, 10, 0.0f, {255, 215, 0, 180});
                drawText(ren, "TRUFFLE", gx, gy - 5.0f, 1.15f, {255, 255, 255, 220}, true);
            } else if (editorTool == 4) { // ERASER
                drawNGon(ren, {mouseX, mouseY}, 24.0f, 16, 0.0f, {245, 60, 60, 130});
                drawThickLine(ren, {mouseX - 12.0f, mouseY - 12.0f}, {mouseX + 12.0f, mouseY + 12.0f}, 3.0f, {255, 255, 255, 240});
                drawThickLine(ren, {mouseX - 12.0f, mouseY + 12.0f}, {mouseX + 12.0f, mouseY - 12.0f}, 3.0f, {255, 255, 255, 240});
            }
        }

        auto addBtn = [&](const std::string& id, float x, float y, float w, float h,
                          const std::string& label, SDL_Color bg, SDL_Color hov, float sc = 1.45f) {
            UIButton b;
            b.id = id; b.rect = {x, y, w, h}; b.label = label;
            b.bgCol = bg; b.hoverCol = hov; b.textScale = sc;
            activeButtons.push_back(b);
            drawButton(ren, b, mouseX, mouseY);
        };

        // 5. Left Tool Dock (0, 70, 215, WINDOW_H - 70)
        SDL_FRect leftDock{0, 70.0f, 215.0f, WINDOW_H - 70.0f};
        SDL_SetRenderDrawColor(ren, 16, 20, 32, 246);
        SDL_RenderFillRectF(ren, &leftDock);
        drawThickLine(ren, {215.0f, 70.0f}, {215.0f, WINDOW_H}, 2.0f, {55, 75, 105, 220});

        drawText(ren, "SIEGE TOOLS", 107.0f, 78.0f, 1.40f, {255, 215, 80, 255}, true);

        // Tool buttons
        auto toolCol = [&](int t) {
            return (editorTool == t) ? SDL_Color{58, 120, 175, 255} : SDL_Color{35, 48, 70, 240};
        };
        addBtn("TOOL_BLOCK", 12.0f, 96.0f, 92.0f, 30.0f, "BLOCK", toolCol(0), {75, 140, 195, 255}, 1.40f);
        addBtn("TOOL_BIRD", 110.0f, 96.0f, 92.0f, 30.0f, "BIRD", toolCol(1), {75, 140, 195, 255}, 1.40f);
        addBtn("TOOL_PENDULUM", 12.0f, 130.0f, 92.0f, 30.0f, "PENDULUM", toolCol(2), {75, 140, 195, 255}, 1.25f);
        addBtn("TOOL_TRUFFLE", 110.0f, 130.0f, 92.0f, 30.0f, "TRUFFLE", toolCol(3), {75, 140, 195, 255}, 1.30f);
        addBtn("TOOL_ERASER", 12.0f, 164.0f, 190.0f, 30.0f, "DEMOLISH ERASER",
               (editorTool == 4 ? SDL_Color{175, 45, 45, 255} : SDL_Color{75, 30, 35, 240}), {215, 60, 60, 255}, 1.35f);

        // Sub-tool palette
        if (editorTool == 0) {
            drawText(ren, "MATERIAL", 107.0f, 204.0f, 1.30f, {240, 220, 150, 255}, true);
            auto matCol = [&](MaterialTier m) {
                return (editorMat == m) ? SDL_Color{62, 128, 182, 255} : SDL_Color{32, 44, 64, 240};
            };
            addBtn("MAT_WOOD", 12.0f, 220.0f, 92.0f, 26.0f, "TIMBER", matCol(MaterialTier::TIMBER), {70, 135, 190, 255}, 1.30f);
            addBtn("MAT_STONE", 110.0f, 220.0f, 92.0f, 26.0f, "STONE", matCol(MaterialTier::STONE), {70, 135, 190, 255}, 1.30f);
            addBtn("MAT_ICE", 12.0f, 250.0f, 92.0f, 26.0f, "ICE", matCol(MaterialTier::ICE), {70, 135, 190, 255}, 1.30f);
            addBtn("MAT_TNT", 110.0f, 250.0f, 92.0f, 26.0f, "TNT", matCol(MaterialTier::TNT), {185, 55, 55, 255}, 1.30f);
            addBtn("MAT_BOUNCE", 12.0f, 280.0f, 190.0f, 26.0f, "TRAMPOLINE BOUNCER", matCol(MaterialTier::TRAMPOLINE), {70, 135, 190, 255}, 1.25f);

            drawText(ren, "BLOCK SIZE", 107.0f, 316.0f, 1.30f, {240, 220, 150, 255}, true);
            auto sizeCol = [&](float w, float h) {
                return (editorBlockW == w && editorBlockH == h) ? SDL_Color{62, 128, 182, 255} : SDL_Color{32, 44, 64, 240};
            };
            addBtn("BLOCK_SIZE_SM", 12.0f, 332.0f, 190.0f, 24.0f, "SM POST (16x50)", sizeCol(16.0f, 50.0f), {70, 135, 190, 255}, 1.20f);
            addBtn("BLOCK_SIZE_MED", 12.0f, 360.0f, 190.0f, 24.0f, "MED BEAM (20x70)", sizeCol(20.0f, 70.0f), {70, 135, 190, 255}, 1.20f);
            addBtn("BLOCK_SIZE_LG", 12.0f, 388.0f, 190.0f, 24.0f, "LG PILLAR (24x90)", sizeCol(24.0f, 90.0f), {70, 135, 190, 255}, 1.20f);
            addBtn("BLOCK_SIZE_HORIZ", 12.0f, 416.0f, 190.0f, 24.0f, "HORIZ PLANK (80x18)", sizeCol(80.0f, 18.0f), {70, 135, 190, 255}, 1.20f);
            addBtn("BLOCK_SIZE_SQR", 12.0f, 444.0f, 190.0f, 24.0f, "SQUARE BLOCK (36x36)", sizeCol(36.0f, 36.0f), {70, 135, 190, 255}, 1.20f);
        } else if (editorTool == 1) {
            drawText(ren, "BIRD SPECIES", 107.0f, 204.0f, 1.30f, {240, 220, 150, 255}, true);
            auto birdCol = [&](BirdClass bc) {
                return (editorBird == bc) ? SDL_Color{62, 128, 182, 255} : SDL_Color{32, 44, 64, 240};
            };
            addBtn("BIRD_TOUCAN", 12.0f, 224.0f, 190.0f, 26.0f, "SENTRY TOUCAN (HP 78)", birdCol(BirdClass::TOUCAN), {70, 135, 190, 255}, 1.20f);
            addBtn("BIRD_OWL",    12.0f, 254.0f, 190.0f, 26.0f, "WARDEN OWL (HP 140)",   birdCol(BirdClass::OWL),    {70, 135, 190, 255}, 1.20f);
            addBtn("BIRD_HERON",  12.0f, 284.0f, 190.0f, 26.0f, "ARCHDUKE HERON (HP 210)", birdCol(BirdClass::HERON),  {70, 135, 190, 255}, 1.20f);
            addBtn("BIRD_FALCON", 12.0f, 314.0f, 190.0f, 26.0f, "COMMANDER FALCON (SHIELD)", birdCol(BirdClass::FALCON), {70, 135, 190, 255}, 1.15f);
            addBtn("BIRD_CONDOR", 12.0f, 344.0f, 190.0f, 26.0f, "BARON VON CONDOR (ZEPPELIN)", birdCol(BirdClass::CONDOR), {70, 135, 190, 255}, 1.10f);

            std::string helmText = std::string("HELMET: ") + (editorBirdHelmet ? "IRON (+100)" : "NONE");
            SDL_Color helmBg = editorBirdHelmet ? SDL_Color{160, 110, 40, 255} : SDL_Color{36, 48, 68, 240};
            addBtn("TOGGLE_HELMET", 12.0f, 380.0f, 190.0f, 28.0f, helmText, helmBg, {190, 140, 60, 255}, 1.25f);
        } else if (editorTool == 2) {
            drawText(ren, "PENDULUM BOB", 107.0f, 204.0f, 1.30f, {240, 220, 150, 255}, true);
            std::string spikeText = std::string("SPIKES: ") + (editorSpikedPendulum ? "YES (LETHAL)" : "NO (BLUNT)");
            SDL_Color spkBg = editorSpikedPendulum ? SDL_Color{165, 45, 45, 255} : SDL_Color{36, 48, 68, 240};
            addBtn("TOGGLE_SPIKE", 12.0f, 224.0f, 190.0f, 32.0f, spikeText, spkBg, {200, 65, 65, 255}, 1.30f);
            drawText(ren, "Click on arena to set anchor.", 107.0f, 275.0f, 1.15f, {190, 205, 225, 220}, true);
            drawText(ren, "Heavy wrecking ball swings", 107.0f, 295.0f, 1.15f, {190, 205, 225, 220}, true);
            drawText(ren, "with harmonic physics gravity.", 107.0f, 315.0f, 1.15f, {190, 205, 225, 220}, true);
        } else if (editorTool == 3) {
            drawText(ren, "SECRET TRUFFLE", 107.0f, 204.0f, 1.30f, {255, 215, 80, 255}, true);
            drawText(ren, "Click arena to place hidden", 107.0f, 235.0f, 1.15f, {210, 225, 240, 220}, true);
            drawText(ren, "Golden Truffle (+2,500 pts).", 107.0f, 255.0f, 1.15f, {210, 225, 240, 220}, true);
            drawText(ren, "Only 1 truffle per fortress.", 107.0f, 275.0f, 1.15f, {210, 225, 240, 220}, true);
        } else if (editorTool == 4) {
            drawText(ren, "DEMOLITION ERASER", 107.0f, 204.0f, 1.30f, {255, 120, 120, 255}, true);
            drawText(ren, "Click on any placed block,", 107.0f, 235.0f, 1.15f, {210, 225, 240, 220}, true);
            drawText(ren, "bird, pendulum, or truffle", 107.0f, 255.0f, 1.15f, {210, 225, 240, 220}, true);
            drawText(ren, "to instantly demolish it.", 107.0f, 275.0f, 1.15f, {210, 225, 240, 220}, true);
        }

        // Environment & Grid Controls (dock bottom)
        std::string gridText = std::string("GRID: ") + (editorSnapGrid ? "ON (20PX)" : "FREEFORM");
        addBtn("TOGGLE_GRID", 12.0f, 480.0f, 190.0f, 26.0f, gridText,
               (editorSnapGrid ? SDL_Color{42, 95, 135, 245} : SDL_Color{32, 44, 64, 240}), {65, 125, 175, 255}, 1.25f);

        std::string themeText = "THEME: CH." + std::to_string(editorTheme + 1);
        addBtn("CYCLE_THEME", 12.0f, 510.0f, 190.0f, 26.0f, themeText, {46, 56, 82, 245}, {72, 88, 125, 255}, 1.25f);

        addBtn("WIND_DEC", 12.0f, 540.0f, 42.0f, 26.0f, "<<", {38, 52, 74, 245}, {60, 80, 114, 255}, 1.35f);
        std::string windLabel = "W: " + std::to_string(static_cast<int>(editorWind));
        drawText(ren, windLabel, 107.0f, 546.0f, 1.30f, {220, 240, 255, 255}, true);
        addBtn("WIND_INC", 160.0f, 540.0f, 42.0f, 26.0f, ">>", {38, 52, 74, 245}, {60, 80, 114, 255}, 1.35f);

        std::string stormText = std::string("STORM: ") + (editorStorm ? "YES (LIGHTNING)" : "CALM");
        addBtn("TOGGLE_STORM", 12.0f, 570.0f, 190.0f, 26.0f, stormText,
               (editorStorm ? SDL_Color{135, 95, 42, 245} : SDL_Color{32, 44, 64, 240}), {175, 125, 65, 255}, 1.20f);

        drawText(ren, "[LMB] PLACE  [DEL] CLEAR", 107.0f, 608.0f, 1.15f, {165, 190, 215, 200}, true);
        drawText(ren, "[T] TEST PLAY  [S] SAVE", 107.0f, 626.0f, 1.15f, {165, 190, 215, 200}, true);

        // 6. Top Header Bar (0, 0, WINDOW_W, 70)
        SDL_FRect topBar{0, 0, WINDOW_W, 70.0f};
        SDL_SetRenderDrawColor(ren, 18, 22, 34, 248);
        SDL_RenderFillRectF(ren, &topBar);
        drawThickLine(ren, {0, 70.0f}, {WINDOW_W, 70.0f}, 2.0f, {55, 75, 105, 220});

        addBtn("BACK_TITLE", 14.0f, 14.0f, 84.0f, 42.0f, "< TITLE", {64, 52, 78, 250}, {94, 76, 114, 255}, 1.55f);

        auto slotCol = [&](int s) {
            return (customSlot == s) ? SDL_Color{48, 120, 185, 255} : SDL_Color{36, 48, 70, 245};
        };
        addBtn("EDITOR_SLOT_1", 106.0f, 14.0f, 68.0f, 42.0f, "SLOT 1", slotCol(1), {68, 145, 215, 255}, 1.45f);
        addBtn("EDITOR_SLOT_2", 180.0f, 14.0f, 68.0f, 42.0f, "SLOT 2", slotCol(2), {68, 145, 215, 255}, 1.45f);
        addBtn("EDITOR_SLOT_3", 254.0f, 14.0f, 68.0f, 42.0f, "SLOT 3", slotCol(3), {68, 145, 215, 255}, 1.45f);

        addBtn("EDITOR_SAVE", 330.0f, 14.0f, 68.0f, 42.0f, "SAVE", {42, 85, 125, 245}, {62, 115, 168, 255}, 1.45f);
        addBtn("EDITOR_LOAD", 404.0f, 14.0f, 68.0f, 42.0f, "LOAD", {48, 72, 98, 245}, {72, 102, 138, 255}, 1.45f);
        addBtn("EDITOR_CLEAR", 478.0f, 14.0f, 72.0f, 42.0f, "CLEAR", {118, 48, 48, 245}, {158, 68, 68, 255}, 1.45f);

        // Center Level Info
        std::string slotInfo = "FORTRESS ARCHITECT - SLOT #" + std::to_string(customSlot) + " (" + customLevel.name + ")";
        drawText(ren, slotInfo, 560.0f, 15.0f, 1.50f, {255, 215, 80, 255}, false);

        std::string countsInfo = "BLOCKS: " + std::to_string(customLevel.blocks.size()) +
                                 " | BIRDS: " + std::to_string(customLevel.birds.size()) +
                                 " | PENDULUMS: " + std::to_string(customLevel.pendulums.size()) +
                                 (customLevel.hasTruffle ? " | TRUFFLE: YES" : " | TRUFFLE: NO");
        drawText(ren, countsInfo, 560.0f, 38.0f, 1.25f, {185, 215, 245, 255}, false);

        addBtn("EDITOR_TEST_PLAY", 1106.0f, 14.0f, 158.0f, 42.0f, "TEST SIEGE >>",
               {48, 125, 76, 255}, {68, 165, 102, 255}, 1.70f);
    }

    void renderHUD(SDL_Renderer* ren) {
        SDL_FRect topBar{0, 0, WINDOW_W, 66.0f};
        SDL_SetRenderDrawColor(ren, 20, 24, 36, 234);
        SDL_RenderFillRectF(ren, &topBar);

        auto addBtn = [&](const std::string& id, float x, float y, float w, float h,
                          const std::string& label, SDL_Color bg, SDL_Color hov, float sc = 1.55f) {
            UIButton b;
            b.id = id; b.rect = {x, y, w, h}; b.label = label;
            b.bgCol = bg; b.hoverCol = hov; b.textScale = sc;
            activeButtons.push_back(b);
            drawButton(ren, b, mouseX, mouseY);
        };

        if (screen == ScreenState::ENDLESS) {
            drawText(ren, "ENDLESS SIEGE GAUNTLET", 18.0f, 15.0f, 1.85f, {246, 212, 118, 255}, false);
            std::string sub = "SURVIVAL WAVE " + std::to_string(endlessWave) + " | BEST: WAVE " + std::to_string(endlessBestWave);
            drawText(ren, sub, 18.0f, 40.0f, 1.25f, {205, 214, 228, 255}, false);

            // Sleek central score & multiplier card
            SDL_FRect scoreCard{400.0f, 8.0f, 250.0f, 50.0f};
            SDL_SetRenderDrawColor(ren, 12, 16, 26, 200);
            SDL_RenderFillRectF(ren, &scoreCard);
            drawThickLine(ren, {scoreCard.x, scoreCard.y}, {scoreCard.x + scoreCard.w, scoreCard.y}, 1.5f, {110, 130, 160, 200});
            drawThickLine(ren, {scoreCard.x + scoreCard.w, scoreCard.y}, {scoreCard.x + scoreCard.w, scoreCard.y + scoreCard.h}, 1.5f, {110, 130, 160, 200});
            drawThickLine(ren, {scoreCard.x + scoreCard.w, scoreCard.y + scoreCard.h}, {scoreCard.x, scoreCard.y + scoreCard.h}, 1.5f, {110, 130, 160, 200});
            drawThickLine(ren, {scoreCard.x, scoreCard.y + scoreCard.h}, {scoreCard.x, scoreCard.y}, 1.5f, {110, 130, 160, 200});

            std::string scoreStr = "SCORE: " + formatNumber(endlessScore);
            drawText(ren, scoreStr, scoreCard.x + scoreCard.w * 0.5f, 12.0f, 1.85f, {255, 245, 200, 255}, true);
            std::string bestStr = "COMBO: " + std::to_string(static_cast<int>(endlessComboMultiplier)) + "X | SWINE: " + std::to_string(activePigs.size());
            drawText(ren, bestStr, scoreCard.x + scoreCard.w * 0.5f, 35.0f, 1.35f, {165, 238, 195, 255}, true);

            // Header buttons
            std::string spdLabel = (simSpeed > 1.0f) ? "SPEED: 2.5X" : "SPEED: 1.0X";
            SDL_Color spdBg = (simSpeed > 1.0f) ? SDL_Color{180, 108, 28, 245} : SDL_Color{38, 56, 76, 245};
            addBtn("TOGGLE_SPEED", 668.0f, 14.0f, 96.0f, 38.0f, spdLabel, spdBg, {60, 88, 116, 255}, 1.35f);

            std::string musLabel = assets.musicMuted ? "MUSIC: OFF" : "MUSIC: ON";
            SDL_Color musBg = assets.musicMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
            addBtn("TOGGLE_MUSIC", 772.0f, 14.0f, 88.0f, 38.0f, musLabel, musBg, {62, 102, 126, 255}, 1.40f);

            std::string sfxLabel = assets.sfxMuted ? "SFX: OFF" : "SFX: ON";
            SDL_Color sfxBg = assets.sfxMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
            addBtn("TOGGLE_SFX", 868.0f, 14.0f, 84.0f, 38.0f, sfxLabel, sfxBg, {62, 102, 126, 255}, 1.40f);

            addBtn("RETRY_ENDLESS", 960.0f, 14.0f, 94.0f, 38.0f, "RESTART",
                   {78, 62, 48, 245}, {112, 88, 66, 255}, 1.45f);
            addBtn("BACK_TITLE", 1064.0f, 14.0f, 96.0f, 38.0f, "MENU",
                   {62, 52, 82, 245}, {92, 76, 118, 255}, 1.50f);

            SDL_FRect bottomBar{0, WINDOW_H - 42.0f, WINDOW_W, 42.0f};
            SDL_SetRenderDrawColor(ren, 18, 22, 34, 210);
            SDL_RenderFillRectF(ren, &bottomBar);

            std::string pigDesc = "READY: NONE";
            if (!activePigs.empty()) {
                PigType pt = activePigs[0].type;
                if (pt == PigType::TROTTER) pigDesc = "PIG: TROTTER [SPACE = ANVIL SLAM]";
                else if (pt == PigType::TURBO) pigDesc = "PIG: BRAMBLE [SPACE = SUPERSONIC DASH]";
                else if (pt == PigType::SPLIT) pigDesc = "PIG: CLOVER [SPACE = 3-WAY SPLIT]";
                else if (pt == PigType::BOMB) pigDesc = "PIG: VULCAN [SPACE = SEISMIC BLAST]";
                else if (pt == PigType::GLIDER) pigDesc = "PIG: GALE [SPACE = AERO GLIDE & BOMBS]";
                else if (pt == PigType::GRAVITON) pigDesc = "PIG: ATTICUS [SPACE = GRAVITON VORTEX]";
                else if (pt == PigType::DRILL) pigDesc = "PIG: RUSTY [SPACE = SUPERSONIC DRILL]";
            }
            drawText(ren, pigDesc, 20.0f, WINDOW_H - 29.0f, 1.65f, {245, 232, 185, 255}, false);

            std::string sysInfo = "ENDLESS MODE | DEFEND REALM | [TAB] SPEED  [RMB/A,D] PAN  [SPACE] ABILITY  [M] MUSIC  [S] SFX";
            drawText(ren, sysInfo, 530.0f, WINDOW_H - 28.0f, 1.25f, {195, 218, 190, 255}, false);

            if (endlessGameOver) {
                SDL_FRect card{WINDOW_W * 0.5f - 300.0f, 120.0f, 600.0f, 440.0f};
                SDL_SetRenderDrawColor(ren, 22, 18, 32, 248);
                SDL_RenderFillRectF(ren, &card);
                drawThickLine(ren, {card.x, card.y}, {card.x + card.w, card.y}, 3.0f, {242, 112, 92, 255});
                drawThickLine(ren, {card.x + card.w, card.y}, {card.x + card.w, card.y + card.h}, 3.0f, {242, 112, 92, 255});
                drawThickLine(ren, {card.x + card.w, card.y + card.h}, {card.x, card.y + card.h}, 3.0f, {242, 112, 92, 255});
                drawThickLine(ren, {card.x, card.y + card.h}, {card.x, card.y}, 3.0f, {242, 112, 92, 255});

                drawText(ren, "SIEGE GAUNTLET OVERWHELMED!", WINDOW_W * 0.5f, 155.0f, 2.7f, {255, 110, 95, 255}, true);
                drawText(ren, "THE AVIAN SWARM EXHAUSTED YOUR HEROIC PIG ARSENAL", WINDOW_W * 0.5f, 205.0f, 1.50f, {215, 225, 240, 255}, true);

                std::string wavesStr = "WAVES SURVIVED: " + std::to_string(std::max(0, endlessWave - 1));
                drawText(ren, wavesStr, WINDOW_W * 0.5f, 260.0f, 2.4f, {255, 215, 80, 255}, true);

                std::string finScoreStr = "FINAL SCORE: " + formatNumber(endlessScore);
                drawText(ren, finScoreStr, WINDOW_W * 0.5f, 310.0f, 2.0f, {240, 248, 255, 255}, true);

                std::string bestWaveStr = "PERSONAL BEST: WAVE " + std::to_string(endlessBestWave);
                drawText(ren, bestWaveStr, WINDOW_W * 0.5f, 355.0f, 1.7f, {165, 238, 195, 255}, true);

                float by = 425.0f;
                addBtn("RETRY_ENDLESS", WINDOW_W * 0.5f - 220.0f, by, 205.0f, 52.0f,
                       "RETRY GAUNTLET >>", {48, 118, 76, 255}, {68, 158, 102, 255}, 1.85f);
                addBtn("BACK_TITLE", WINDOW_W * 0.5f + 15.0f, by, 205.0f, 52.0f,
                       "< MAIN MENU", {62, 52, 82, 250}, {92, 76, 118, 255}, 1.85f);
            }
            return;
        }

        if (screen == ScreenState::EDITOR && isTestPlaying) {
            addBtn("RETURN_TO_EDITOR", 18.0f, 14.0f, 210.0f, 38.0f, "< RETURN TO ARCHITECT",
                   {110, 48, 64, 245}, {150, 68, 88, 255}, 1.40f);

            drawText(ren, "CUSTOM SIEGE: " + customLevel.name, 240.0f, 15.0f, 1.70f, {246, 212, 118, 255}, false);
            std::string sub = "ARCHITECT TEST PLAY (SLOT #" + std::to_string(customSlot) + ") | BLOCKS: " +
                              std::to_string(blocks.size()) + " | BIRDS: " + std::to_string(birds.size());
            drawText(ren, sub, 240.0f, 40.0f, 1.20f, {205, 214, 228, 255}, false);

            // Sleek central score & record card
            SDL_FRect scoreCard{600.0f, 8.0f, 190.0f, 50.0f};
            SDL_SetRenderDrawColor(ren, 12, 16, 26, 200);
            SDL_RenderFillRectF(ren, &scoreCard);
            drawThickLine(ren, {scoreCard.x, scoreCard.y}, {scoreCard.x + scoreCard.w, scoreCard.y}, 1.5f, {110, 130, 160, 200});
            drawThickLine(ren, {scoreCard.x + scoreCard.w, scoreCard.y}, {scoreCard.x + scoreCard.w, scoreCard.y + scoreCard.h}, 1.5f, {110, 130, 160, 200});
            drawThickLine(ren, {scoreCard.x + scoreCard.w, scoreCard.y + scoreCard.h}, {scoreCard.x, scoreCard.y + scoreCard.h}, 1.5f, {110, 130, 160, 200});
            drawThickLine(ren, {scoreCard.x, scoreCard.y + scoreCard.h}, {scoreCard.x, scoreCard.y}, 1.5f, {110, 130, 160, 200});

            std::string scoreStr = "SCORE: " + formatNumber(score);
            drawText(ren, scoreStr, scoreCard.x + scoreCard.w * 0.5f, 14.0f, 1.80f, {255, 245, 200, 255}, true);
            drawText(ren, "TEST MODE", scoreCard.x + scoreCard.w * 0.5f, 36.0f, 1.30f, {165, 238, 195, 255}, true);

            // Speed, SFX, Music, Reset
            std::string spdLabel = (simSpeed > 1.0f) ? "SPEED: 2.5X" : "SPEED: 1.0X";
            SDL_Color spdBg = (simSpeed > 1.0f) ? SDL_Color{180, 108, 28, 245} : SDL_Color{38, 56, 76, 245};
            addBtn("TOGGLE_SPEED", 800.0f, 14.0f, 96.0f, 38.0f, spdLabel, spdBg, {60, 88, 116, 255}, 1.35f);

            std::string musLabel = assets.musicMuted ? "MUSIC: OFF" : "MUSIC: ON";
            SDL_Color musBg = assets.musicMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
            addBtn("TOGGLE_MUSIC", 904.0f, 14.0f, 88.0f, 38.0f, musLabel, musBg, {62, 102, 126, 255}, 1.40f);

            std::string sfxLabel = assets.sfxMuted ? "SFX: OFF" : "SFX: ON";
            SDL_Color sfxBg = assets.sfxMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
            addBtn("TOGGLE_SFX", 1000.0f, 14.0f, 84.0f, 38.0f, sfxLabel, sfxBg, {62, 102, 126, 255}, 1.40f);

            addBtn("RETRY_LEVEL", 1092.0f, 14.0f, 84.0f, 38.0f, "RESET",
                   {78, 62, 48, 245}, {112, 88, 66, 255}, 1.50f);

            SDL_FRect bottomBar{0, WINDOW_H - 42.0f, WINDOW_W, 42.0f};
            SDL_SetRenderDrawColor(ren, 18, 22, 34, 210);
            SDL_RenderFillRectF(ren, &bottomBar);

            std::string pigDesc = "READY: NONE";
            if (!activePigs.empty()) {
                PigType pt = activePigs[0].type;
                if (pt == PigType::TROTTER) pigDesc = "PIG: TROTTER [SPACE = ANVIL SLAM]";
                else if (pt == PigType::TURBO) pigDesc = "PIG: BRAMBLE [SPACE = SUPERSONIC DASH]";
                else if (pt == PigType::SPLIT) pigDesc = "PIG: CLOVER [SPACE = 3-WAY SPLIT]";
                else if (pt == PigType::BOMB) pigDesc = "PIG: VULCAN [SPACE = SEISMIC BLAST]";
                else if (pt == PigType::GLIDER) pigDesc = "PIG: GALE [SPACE = AERO GLIDE & BOMBS]";
                else if (pt == PigType::GRAVITON) pigDesc = "PIG: ATTICUS [SPACE = GRAVITON VORTEX]";
                else if (pt == PigType::DRILL) pigDesc = "PIG: RUSTY [SPACE = SUPERSONIC DRILL]";
            }
            drawText(ren, pigDesc, 20.0f, WINDOW_H - 29.0f, 1.65f, {245, 232, 185, 255}, false);

            std::string sysInfo = "ARCHITECT TEST PLAY | [TAB] SPEED  [RMB/A,D] PAN  [R] RESET  [SPACE] ABILITY";
            drawText(ren, sysInfo, 530.0f, WINDOW_H - 28.0f, 1.25f, {195, 218, 190, 255}, false);

            if (levelComplete || levelFailed) {
                SDL_FRect card{WINDOW_W * 0.5f - 300.0f, 120.0f, 600.0f, 400.0f};
                SDL_SetRenderDrawColor(ren, 18, 22, 36, 248);
                SDL_RenderFillRectF(ren, &card);
                drawThickLine(ren, {card.x, card.y}, {card.x + card.w, card.y}, 3.0f, {236, 194, 98, 255});
                drawThickLine(ren, {card.x + card.w, card.y}, {card.x + card.w, card.y + card.h}, 3.0f, {236, 194, 98, 255});
                drawThickLine(ren, {card.x + card.w, card.y + card.h}, {card.x, card.y + card.h}, 3.0f, {236, 188, 98, 255});
                drawThickLine(ren, {card.x, card.y + card.h}, {card.x, card.y}, 3.0f, {236, 194, 98, 255});

                if (levelComplete) {
                    drawText(ren, "CUSTOM FORTRESS DEMOLISHED!", WINDOW_W * 0.5f, 160.0f, 2.7f, {255, 216, 92, 255}, true);
                    std::string finStr = "FINAL SCORE: " + formatNumber(score);
                    drawText(ren, finStr, WINDOW_W * 0.5f, 225.0f, 2.4f, {250, 252, 255, 255}, true);
                } else {
                    drawText(ren, "FORTRESS WITHSTOOD TEST!", WINDOW_W * 0.5f, 160.0f, 2.7f, {242, 112, 92, 255}, true);
                    drawText(ren, "BIRDS SURVIVED THE BARRAGE", WINDOW_W * 0.5f, 225.0f, 1.8f, {215, 222, 235, 255}, true);
                }

                float by = 350.0f;
                addBtn("RETRY_LEVEL", card.x + 50.0f, by, 220.0f, 50.0f,
                       "RETRY TEST", {78, 62, 48, 250}, {112, 88, 66, 255}, 1.8f);
                addBtn("RETURN_TO_EDITOR", card.x + 310.0f, by, 240.0f, 50.0f,
                       "< BACK TO ARCHITECT", {110, 48, 64, 250}, {150, 68, 88, 255}, 1.6f);
            }
            return;
        }

        const LevelSpec& L = levels[currentLevelIdx];
        std::string lvlTitle = L.name;
        float titleScale = 1.95f;
        if (lvlTitle.size() > 20) titleScale = 1.60f;
        if (lvlTitle.size() > 26) titleScale = 1.35f;
        if (lvlTitle.size() > 34) titleScale = 1.15f;
        drawText(ren, lvlTitle, 18.0f, 15.0f, titleScale, {246, 212, 118, 255}, false);

        std::string sub = L.subtitle;
        if (sub.size() > 30) sub = sub.substr(0, 27) + "...";
        drawText(ren, sub, 18.0f, 40.0f, 1.15f, {205, 214, 228, 255}, false);

        // Top-Bar Wind Gauge & Weather Card Widget
        SDL_FRect weatherCard{265.0f, 8.0f, 155.0f, 50.0f};
        SDL_SetRenderDrawColor(ren, 12, 16, 26, 200);
        SDL_RenderFillRectF(ren, &weatherCard);
        drawThickLine(ren, {weatherCard.x, weatherCard.y}, {weatherCard.x + weatherCard.w, weatherCard.y}, 1.5f, {100, 125, 155, 200});
        drawThickLine(ren, {weatherCard.x + weatherCard.w, weatherCard.y}, {weatherCard.x + weatherCard.w, weatherCard.y + weatherCard.h}, 1.5f, {100, 125, 155, 200});
        drawThickLine(ren, {weatherCard.x + weatherCard.w, weatherCard.y + weatherCard.h}, {weatherCard.x, weatherCard.y + weatherCard.h}, 1.5f, {100, 125, 155, 200});
        drawThickLine(ren, {weatherCard.x, weatherCard.y + weatherCard.h}, {weatherCard.x, weatherCard.y}, 1.5f, {100, 125, 155, 200});

        std::string wBadge = L.hasStorm ? "ATMOSPHERE: STORM" :
                             (std::abs(currentWind) > 80.0f ? "ATMOSPHERE: GALE" :
                             (std::abs(currentWind) > 10.0f ? "ATMOSPHERE: BREEZE" : "ATMOSPHERE: CALM"));
        SDL_Color badgeCol = L.hasStorm ? SDL_Color{255, 220, 90, 255} :
                             (std::abs(currentWind) > 80.0f ? SDL_Color{140, 220, 255, 255} : SDL_Color{180, 210, 235, 255});
        drawText(ren, wBadge, weatherCard.x + weatherCard.w * 0.5f, 12.0f, 1.20f, badgeCol, true);

        std::string windStr;
        if (std::abs(currentWind) < 10.0f) {
            windStr = "WIND: CALM";
        } else if (currentWind < -10.0f) {
            windStr = "<< " + std::to_string(static_cast<int>(currentWind)) + " MPH";
        } else {
            windStr = "+" + std::to_string(static_cast<int>(currentWind)) + " MPH >>";
        }
        SDL_Color windCol = (std::abs(currentWind) > 80.0f) ? SDL_Color{255, 230, 140, 255} : SDL_Color{220, 240, 255, 255};
        drawText(ren, windStr, weatherCard.x + weatherCard.w * 0.5f, 33.0f, 1.50f, windCol, true);

        // Sleek central score & record card
        SDL_FRect scoreCard{430.0f, 8.0f, 224.0f, 50.0f};
        SDL_SetRenderDrawColor(ren, 12, 16, 26, 200);
        SDL_RenderFillRectF(ren, &scoreCard);
        drawThickLine(ren, {scoreCard.x, scoreCard.y}, {scoreCard.x + scoreCard.w, scoreCard.y}, 1.5f, {110, 130, 160, 200});
        drawThickLine(ren, {scoreCard.x + scoreCard.w, scoreCard.y}, {scoreCard.x + scoreCard.w, scoreCard.y + scoreCard.h}, 1.5f, {110, 130, 160, 200});
        drawThickLine(ren, {scoreCard.x + scoreCard.w, scoreCard.y + scoreCard.h}, {scoreCard.x, scoreCard.y + scoreCard.h}, 1.5f, {110, 130, 160, 200});
        drawThickLine(ren, {scoreCard.x, scoreCard.y + scoreCard.h}, {scoreCard.x, scoreCard.y}, 1.5f, {110, 130, 160, 200});

        std::string scoreStr = "SCORE: " + formatNumber(score);
        drawText(ren, scoreStr, scoreCard.x + scoreCard.w * 0.5f, 12.0f, 1.85f, {255, 245, 200, 255}, true);
        std::string bestStr = "BEST: " + (bestScores[currentLevelIdx] > 0 ? formatNumber(bestScores[currentLevelIdx]) : "---");
        drawText(ren, bestStr, scoreCard.x + scoreCard.w * 0.5f, 35.0f, 1.35f, {165, 238, 195, 255}, true);

        // Truffle status badge on levels with a hidden golden truffle
        if (L.hasTruffle) {
            SDL_FRect trufBadge{430.0f, 68.0f, 224.0f, 24.0f};
            SDL_SetRenderDrawColor(ren, 26, 18, 38, 225);
            SDL_RenderFillRectF(ren, &trufBadge);
            SDL_Color trufBrd = levelTruffle.collected ? SDL_Color{255, 215, 0, 255} : SDL_Color{218, 165, 32, 200};
            drawThickLine(ren, {trufBadge.x, trufBadge.y}, {trufBadge.x + trufBadge.w, trufBadge.y}, 1.5f, trufBrd);
            drawThickLine(ren, {trufBadge.x + trufBadge.w, trufBadge.y}, {trufBadge.x + trufBadge.w, trufBadge.y + trufBadge.h}, 1.5f, trufBrd);
            drawThickLine(ren, {trufBadge.x + trufBadge.w, trufBadge.y + trufBadge.h}, {trufBadge.x, trufBadge.y + trufBadge.h}, 1.5f, trufBrd);
            drawThickLine(ren, {trufBadge.x, trufBadge.y + trufBadge.h}, {trufBadge.x, trufBadge.y}, 1.5f, trufBrd);

            if (levelTruffle.collected) {
                float pulse = 0.5f + 0.5f * std::sin(levelTruffle.pulseTime * 4.0f);
                SDL_Color gCol = {255, static_cast<uint8_t>(215 + 40 * pulse), static_cast<uint8_t>(80 + 40 * pulse), 255};
                drawText(ren, "[TRUFFLE COLLECTED *]", trufBadge.x + trufBadge.w * 0.5f, trufBadge.y + 5.0f, 1.25f, gCol, true);
            } else {
                drawText(ren, "* SECRET TRUFFLE HIDDEN *", trufBadge.x + trufBadge.w * 0.5f, trufBadge.y + 5.0f, 1.25f, {255, 215, 80, 255}, true);
            }
        }

        // Header Action Buttons: Sim Speed, Mute Music, Mute SFX, Retry, Levels, Menu
        std::string spdLabel = (simSpeed > 1.0f) ? "SPEED: 2.5X" : "SPEED: 1.0X";
        SDL_Color spdBg = (simSpeed > 1.0f) ? SDL_Color{180, 108, 28, 245} : SDL_Color{38, 56, 76, 245};
        addBtn("TOGGLE_SPEED", 668.0f, 14.0f, 96.0f, 38.0f, spdLabel, spdBg, {60, 88, 116, 255}, 1.35f);

        std::string musLabel = assets.musicMuted ? "MUSIC: OFF" : "MUSIC: ON";
        SDL_Color musBg = assets.musicMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
        addBtn("TOGGLE_MUSIC", 772.0f, 14.0f, 88.0f, 38.0f, musLabel, musBg, {62, 102, 126, 255}, 1.40f);

        std::string sfxLabel = assets.sfxMuted ? "SFX: OFF" : "SFX: ON";
        SDL_Color sfxBg = assets.sfxMuted ? SDL_Color{88, 42, 42, 245} : SDL_Color{42, 72, 88, 245};
        addBtn("TOGGLE_SFX", 868.0f, 14.0f, 84.0f, 38.0f, sfxLabel, sfxBg, {62, 102, 126, 255}, 1.40f);

        addBtn("RETRY_LEVEL", 960.0f, 14.0f, 84.0f, 38.0f, "RETRY",
               {78, 62, 48, 245}, {112, 88, 66, 255}, 1.50f);
        addBtn("OPEN_LEVEL_SELECT", 1052.0f, 14.0f, 102.0f, 38.0f, "LEVELS",
               {48, 68, 102, 245}, {72, 98, 144, 255}, 1.50f);
        addBtn("BACK_TITLE", 1162.0f, 14.0f, 96.0f, 38.0f, "MENU",
               {62, 52, 82, 245}, {92, 76, 118, 255}, 1.50f);

        SDL_FRect bottomBar{0, WINDOW_H - 42.0f, WINDOW_W, 42.0f};
        SDL_SetRenderDrawColor(ren, 18, 22, 34, 210);
        SDL_RenderFillRectF(ren, &bottomBar);

        std::string pigDesc = "READY: NONE";
        if (!activePigs.empty()) {
            PigType pt = activePigs[0].type;
            if (pt == PigType::TROTTER) pigDesc = "PIG: TROTTER [SPACE = ANVIL SLAM]";
            else if (pt == PigType::TURBO) pigDesc = "PIG: BRAMBLE [SPACE = SUPERSONIC DASH]";
            else if (pt == PigType::SPLIT) pigDesc = "PIG: CLOVER [SPACE = 3-WAY SPLIT]";
            else if (pt == PigType::BOMB) pigDesc = "PIG: VULCAN [SPACE = SEISMIC BLAST]";
            else if (pt == PigType::GLIDER) pigDesc = "PIG: GALE [SPACE = AERO GLIDE & BOMBS]";
            else if (pt == PigType::GRAVITON) pigDesc = "PIG: ATTICUS [SPACE = GRAVITON VORTEX]";
            else if (pt == PigType::DRILL) pigDesc = "PIG: RUSTY [SPACE = SUPERSONIC DRILL]";
        }
        drawText(ren, pigDesc, 20.0f, WINDOW_H - 29.0f, 1.65f, {245, 232, 185, 255}, false);

        std::string sysInfo;
        if (currentLevelIdx >= NUM_CAMPAIGN_LEVELS) {
            std::string vNum = (currentLevelIdx - NUM_CAMPAIGN_LEVELS + 1 < 10 ? "0" : "") +
                               std::to_string(currentLevelIdx - NUM_CAMPAIGN_LEVELS + 1);
            sysInfo = "CHAPTER XV: THE SECRET VAULTS | SECRET VAULT " + vNum +
                      " | [TAB] SPEED  [RMB/A,D] PAN  [R] RETRY  [M] MUSIC  [S] SFX";
        } else {
            sysInfo = "CH." + std::to_string(getActiveChapter() + 1) + " | LVL " +
                      std::to_string(currentLevelIdx + 1) + "/" + std::to_string(NUM_CAMPAIGN_LEVELS) +
                      " | [TAB] SPEED  [RMB/A,D] PAN  [R] RETRY  [M] MUSIC  [S] SFX";
        }
        drawText(ren, sysInfo, 530.0f, WINDOW_H - 28.0f, 1.25f, {195, 218, 190, 255}, false);

        // ====================================================================
        // LEVEL COMPLETE / FAILED MODAL WITH HIGH SCORE CELEBRATION
        // ====================================================================
        if (levelComplete || levelFailed) {
            SDL_FRect card{WINDOW_W * 0.5f - 340.0f, 75.0f, 680.0f, 530.0f};
            SDL_SetRenderDrawColor(ren, 18, 22, 36, 248);
            SDL_RenderFillRectF(ren, &card);
            drawThickLine(ren, {card.x, card.y}, {card.x + card.w, card.y}, 3.0f, {236, 194, 98, 255});
            drawThickLine(ren, {card.x + card.w, card.y}, {card.x + card.w, card.y + card.h}, 3.0f, {236, 194, 98, 255});
            drawThickLine(ren, {card.x + card.w, card.y + card.h}, {card.x, card.y + card.h}, 3.0f, {236, 188, 98, 255});
            drawThickLine(ren, {card.x, card.y + card.h}, {card.x, card.y}, 3.0f, {236, 194, 98, 255});

            if (levelComplete) {
                drawText(ren, "FORTRESS DEMOLISHED!", WINDOW_W * 0.5f, 98.0f, 3.2f, {255, 216, 92, 255}, true);

                // 3 Large Celebratory Stars
                for (int s = 0; s < 3; ++s) {
                    Vec2 sc(WINDOW_W * 0.5f + (s - 1) * 80.0f, 168.0f);
                    bool earned = (s < earnedStars);
                    float pulse = (earned && isNewRecordThisRun) ? (1.0f + 0.12f * std::sin(recordAnimTimer * 5.0f + s * 1.5f)) : 1.0f;
                    SDL_Color fCol = earned ? SDL_Color{255, 212, 64, 255} : SDL_Color{62, 68, 84, 255};
                    SDL_Color oCol = earned ? SDL_Color{218, 148, 28, 255} : SDL_Color{45, 50, 64, 255};
                    drawStarIcon(ren, sc, 26.0f * pulse, 11.5f * pulse, fCol, oCol);
                }

                // Final Score
                std::string finStr = "FINAL SCORE: " + formatNumber(score);
                drawText(ren, finStr, WINDOW_W * 0.5f, 215.0f, 2.5f, {250, 252, 255, 255}, true);

                // High Score celebration banner or record comparison
                if (isNewRecordThisRun) {
                    float pulse = 0.5f + 0.5f * std::sin(recordAnimTimer * 6.0f);
                    SDL_Color bannerCol = {static_cast<uint8_t>(255), static_cast<uint8_t>(215 + 40 * pulse), static_cast<uint8_t>(80), 255};
                    drawText(ren, "** NEW PERSONAL HIGH SCORE! **", WINDOW_W * 0.5f, 262.0f, 2.2f, bannerCol, true);

                    std::string compStr;
                    if (prevLevelBest > 0) {
                        compStr = "PREVIOUS RECORD: " + formatNumber(prevLevelBest) +
                                  "   (+ " + formatNumber(score - prevLevelBest) + " PTS!)";
                    } else {
                        compStr = "FIRST CLEAR RECORD ESTABLISHED!";
                    }
                    drawText(ren, compStr, WINDOW_W * 0.5f, 298.0f, 1.6f, {150, 255, 195, 255}, true);
                } else {
                    std::string bestCmp = "PERSONAL BEST: " + formatNumber(bestScores[currentLevelIdx]) +
                                          "  (DIFF: " + formatNumber(score - bestScores[currentLevelIdx]) + ")";
                    drawText(ren, bestCmp, WINDOW_W * 0.5f, 275.0f, 1.8f, {175, 215, 245, 255}, true);
                }

                // Milestone Pig Unlock Alert or Secret Truffle / Vault Conquered Alert
                if (currentLevelIdx == 4) {
                    SDL_FRect mBox{card.x + 30.0f, 342.0f, card.w - 60.0f, 48.0f};
                    SDL_SetRenderDrawColor(ren, 48, 62, 38, 240);
                    SDL_RenderFillRectF(ren, &mBox);
                    drawThickLine(ren, {mBox.x, mBox.y}, {mBox.x + mBox.w, mBox.y}, 2.0f, {255, 215, 80, 255});
                    drawThickLine(ren, {mBox.x, mBox.y + mBox.h}, {mBox.x + mBox.w, mBox.y + mBox.h}, 2.0f, {255, 215, 80, 255});
                    drawText(ren, "** MILESTONE UNLOCKED: BRAMBLE (TURBO PIG) AVAILABLE! **",
                             WINDOW_W * 0.5f, 350.0f, 1.55f, {255, 235, 110, 255}, true);
                    drawText(ren, "Tap SPACE in mid-air to trigger Supersonic Dash ability!",
                             WINDOW_W * 0.5f, 370.0f, 1.30f, {220, 240, 220, 255}, true);
                } else if (currentLevelIdx == 9) {
                    SDL_FRect mBox{card.x + 30.0f, 342.0f, card.w - 60.0f, 48.0f};
                    SDL_SetRenderDrawColor(ren, 38, 62, 58, 240);
                    SDL_RenderFillRectF(ren, &mBox);
                    drawThickLine(ren, {mBox.x, mBox.y}, {mBox.x + mBox.w, mBox.y}, 2.0f, {120, 245, 195, 255});
                    drawThickLine(ren, {mBox.x, mBox.y + mBox.h}, {mBox.x + mBox.w, mBox.y + mBox.h}, 2.0f, {120, 245, 195, 255});
                    drawText(ren, "** MILESTONE UNLOCKED: CLOVER (SPLIT PIG) AVAILABLE! **",
                             WINDOW_W * 0.5f, 350.0f, 1.55f, {150, 255, 210, 255}, true);
                    drawText(ren, "Tap SPACE in mid-air to split into 3 aerodynamic swine!",
                             WINDOW_W * 0.5f, 370.0f, 1.30f, {220, 245, 235, 255}, true);
                } else if (currentLevelIdx == 19) {
                    SDL_FRect mBox{card.x + 30.0f, 342.0f, card.w - 60.0f, 48.0f};
                    SDL_SetRenderDrawColor(ren, 68, 38, 34, 240);
                    SDL_RenderFillRectF(ren, &mBox);
                    drawThickLine(ren, {mBox.x, mBox.y}, {mBox.x + mBox.w, mBox.y}, 2.0f, {255, 140, 90, 255});
                    drawThickLine(ren, {mBox.x, mBox.y + mBox.h}, {mBox.x + mBox.w, mBox.y + mBox.h}, 2.0f, {255, 140, 90, 255});
                    drawText(ren, "** MILESTONE UNLOCKED: VULCAN (BOMB PIG) AVAILABLE! **",
                             WINDOW_W * 0.5f, 350.0f, 1.55f, {255, 180, 120, 255}, true);
                    drawText(ren, "Tap SPACE or impact to detonate a devastating seismic shockwave!",
                             WINDOW_W * 0.5f, 370.0f, 1.30f, {255, 220, 200, 255}, true);
                } else if (L.hasTruffle) {
                    SDL_FRect tBox{card.x + 30.0f, 342.0f, card.w - 60.0f, 48.0f};
                    SDL_SetRenderDrawColor(ren, 36, 24, 48, 240);
                    SDL_RenderFillRectF(ren, &tBox);
                    SDL_Color tBrd = levelTruffle.collected ? SDL_Color{255, 215, 0, 255} : SDL_Color{160, 140, 90, 200};
                    drawThickLine(ren, {tBox.x, tBox.y}, {tBox.x + tBox.w, tBox.y}, 2.0f, tBrd);
                    drawThickLine(ren, {tBox.x, tBox.y + tBox.h}, {tBox.x + tBox.w, tBox.y + tBox.h}, 2.0f, tBrd);
                    if (levelTruffle.collected) {
                        drawText(ren, "* SECRET GOLDEN TRUFFLE DISCOVERED! (+2,500 PTS) *",
                                 WINDOW_W * 0.5f, 350.0f, 1.55f, {255, 225, 90, 255}, true);
                        drawText(ren, "Vault Progress: " + std::to_string(getTrufflesFoundCount()) + " / 15 Hidden Truffles Recovered",
                                 WINDOW_W * 0.5f, 370.0f, 1.30f, {240, 235, 190, 255}, true);
                    } else {
                        drawText(ren, "* SECRET GOLDEN TRUFFLE STILL HIDDEN IN THIS FORTRESS! *",
                                 WINDOW_W * 0.5f, 350.0f, 1.45f, {220, 180, 80, 255}, true);
                        drawText(ren, "Search hidden cavities or shatter barriers to discover it!",
                                 WINDOW_W * 0.5f, 370.0f, 1.30f, {190, 185, 210, 255}, true);
                    }
                } else if (currentLevelIdx >= NUM_CAMPAIGN_LEVELS) {
                    SDL_FRect vBox{card.x + 30.0f, 342.0f, card.w - 60.0f, 48.0f};
                    SDL_SetRenderDrawColor(ren, 42, 22, 60, 240);
                    SDL_RenderFillRectF(ren, &vBox);
                    drawThickLine(ren, {vBox.x, vBox.y}, {vBox.x + vBox.w, vBox.y}, 2.0f, {255, 215, 0, 255});
                    drawThickLine(ren, {vBox.x, vBox.y + vBox.h}, {vBox.x + vBox.w, vBox.y + vBox.h}, 2.0f, {255, 215, 0, 255});
                    drawText(ren, "*** SECRET TRUFFLE VAULT CONQUERED! ***",
                             WINDOW_W * 0.5f, 350.0f, 1.60f, {255, 225, 90, 255}, true);
                    drawText(ren, "Ancient royal swine treasures plundered with supreme precision!",
                             WINDOW_W * 0.5f, 370.0f, 1.30f, {230, 215, 255, 255}, true);
                }

                float by = 460.0f;
                addBtn("RETRY_LEVEL", card.x + 35.0f, by, 185.0f, 50.0f,
                       "RETRY LEVEL", {78, 62, 48, 250}, {112, 88, 66, 255}, 1.8f);
                addBtn("OPEN_LEVEL_SELECT", card.x + 245.0f, by, 190.0f, 50.0f,
                       "LEVEL SELECT", {52, 74, 114, 250}, {76, 104, 156, 255}, 1.8f);
                if (currentLevelIdx + 1 < NUM_LEVELS) {
                    addBtn("NEXT_LEVEL_BTN", card.x + 460.0f, by, 185.0f, 50.0f,
                           "NEXT LEVEL >>", {48, 118, 76, 255}, {68, 158, 102, 255}, 1.9f);
                }
            } else {
                drawText(ren, "BIRDS STILL PERCHED!", WINDOW_W * 0.5f, 170.0f, 3.2f,
                         {242, 112, 92, 255}, true);
                drawText(ren, "THE AVIAN FORTRESS WITHSTOOD THE BARRAGE",
                         WINDOW_W * 0.5f, 228.0f, 1.9f, {215, 222, 235, 255}, true);
                std::string currBest = "LEVEL BEST: " + (bestScores[currentLevelIdx] > 0 ? formatNumber(bestScores[currentLevelIdx]) : "---");
                drawText(ren, currBest, WINDOW_W * 0.5f, 275.0f, 1.7f, {165, 238, 195, 255}, true);

                float by = 360.0f;
                addBtn("RETRY_LEVEL", WINDOW_W * 0.5f - 220.0f, by, 200.0f, 52.0f,
                       "RETRY LEVEL", {48, 118, 76, 255}, {68, 158, 102, 255}, 2.0f);
                addBtn("OPEN_LEVEL_SELECT", WINDOW_W * 0.5f + 20.0f, by, 200.0f, 52.0f,
                       "LEVEL SELECT", {52, 74, 114, 250}, {76, 104, 156, 255}, 2.0f);
            }
        }
    }

    bool handleButtonClick(const std::string& id, bool& running) {
        assets.playSound("sound.ui_click", 0.55f);

        if (id == "START_CAMPAIGN") {
            int nextLvl = getHighestUnlockedLevel();
            loadLevel(nextLvl);
            screen = ScreenState::PLAYING;
            return true;
        }
        if (id == "TOGGLE_SPEED") {
            simSpeed = (simSpeed > 1.0f) ? 1.0f : 2.5f;
            return true;
        }
        if (id == "TOGGLE_MUSIC") {
            assets.musicMuted = !assets.musicMuted;
            saveProgress();
            return true;
        }
        if (id == "TOGGLE_SFX") {
            assets.sfxMuted = !assets.sfxMuted;
            saveProgress();
            return true;
        }
        if (id == "TOGGLE_FULLSCREEN") {
            toggleFullscreen();
            return true;
        }
        if (id == "OPEN_LEVEL_SELECT") {
            screen = ScreenState::LEVEL_SELECT;
            return true;
        }
        if (id == "BACK_TITLE") {
            screen = ScreenState::TITLE;
            return true;
        }
        if (id == "QUIT_GAME") {
            running = false;
            return true;
        }
        if (id == "RETRY_LEVEL") {
            if (screen == ScreenState::EDITOR && isTestPlaying) {
                startEditorTestPlay();
            } else if (screen == ScreenState::ENDLESS) {
                startEndlessMode();
            } else {
                loadLevel(currentLevelIdx);
                screen = ScreenState::PLAYING;
            }
            return true;
        }
        if (id == "OPEN_EDITOR") {
            screen = ScreenState::EDITOR;
            initEditor();
            return true;
        }
        if (id == "START_ENDLESS" || id == "RETRY_ENDLESS") {
            startEndlessMode();
            return true;
        }
        if (id == "EDITOR_TEST_PLAY") {
            startEditorTestPlay();
            return true;
        }
        if (id == "RETURN_TO_EDITOR") {
            returnToEditor();
            return true;
        }
        if (id == "EDITOR_SLOT_1") {
            customSlot = 1;
            loadCustomLevel(1);
            return true;
        }
        if (id == "EDITOR_SLOT_2") {
            customSlot = 2;
            loadCustomLevel(2);
            return true;
        }
        if (id == "EDITOR_SLOT_3") {
            customSlot = 3;
            loadCustomLevel(3);
            return true;
        }
        if (id == "EDITOR_SAVE") {
            saveCustomLevel(customSlot);
            return true;
        }
        if (id == "EDITOR_LOAD") {
            loadCustomLevel(customSlot);
            return true;
        }
        if (id == "EDITOR_CLEAR") {
            customLevel.blocks.clear();
            customLevel.birds.clear();
            customLevel.pendulums.clear();
            customLevel.hasTruffle = false;
            floatScores.push_back({{WINDOW_W * 0.5f, 120.0f}, "FORTRESS CLEARED!", {255, 140, 140, 255}, 1.4f, 1.4f});
            return true;
        }
        if (id == "TOOL_BLOCK") { editorTool = 0; return true; }
        if (id == "TOOL_BIRD") { editorTool = 1; return true; }
        if (id == "TOOL_PENDULUM") { editorTool = 2; return true; }
        if (id == "TOOL_TRUFFLE") { editorTool = 3; return true; }
        if (id == "TOOL_ERASER") { editorTool = 4; return true; }

        if (id == "MAT_WOOD") { editorMat = MaterialTier::TIMBER; return true; }
        if (id == "MAT_STONE") { editorMat = MaterialTier::STONE; return true; }
        if (id == "MAT_ICE") { editorMat = MaterialTier::ICE; return true; }
        if (id == "MAT_TNT") { editorMat = MaterialTier::TNT; return true; }
        if (id == "MAT_BOUNCE") { editorMat = MaterialTier::TRAMPOLINE; return true; }

        if (id == "BLOCK_SIZE_SM") { editorBlockW = 16.0f; editorBlockH = 50.0f; return true; }
        if (id == "BLOCK_SIZE_MED") { editorBlockW = 20.0f; editorBlockH = 70.0f; return true; }
        if (id == "BLOCK_SIZE_LG") { editorBlockW = 24.0f; editorBlockH = 90.0f; return true; }
        if (id == "BLOCK_SIZE_HORIZ") { editorBlockW = 80.0f; editorBlockH = 18.0f; return true; }
        if (id == "BLOCK_SIZE_SQR") { editorBlockW = 36.0f; editorBlockH = 36.0f; return true; }

        if (id == "BIRD_TOUCAN") { editorBird = BirdClass::TOUCAN; return true; }
        if (id == "BIRD_OWL") { editorBird = BirdClass::OWL; return true; }
        if (id == "BIRD_HERON") { editorBird = BirdClass::HERON; return true; }
        if (id == "BIRD_FALCON") { editorBird = BirdClass::FALCON; return true; }
        if (id == "BIRD_CONDOR") { editorBird = BirdClass::CONDOR; return true; }

        if (id == "TOGGLE_HELMET") { editorBirdHelmet = !editorBirdHelmet; return true; }
        if (id == "TOGGLE_SPIKE") { editorSpikedPendulum = !editorSpikedPendulum; return true; }
        if (id == "TOGGLE_GRID") { editorSnapGrid = !editorSnapGrid; return true; }
        if (id == "CYCLE_THEME") { editorTheme = (editorTheme + 1) % 6; return true; }
        if (id == "WIND_DEC") { editorWind = std::max(-120.0f, editorWind - 30.0f); return true; }
        if (id == "WIND_INC") { editorWind = std::min(120.0f, editorWind + 30.0f); return true; }
        if (id == "TOGGLE_STORM") { editorStorm = !editorStorm; return true; }

        if (id == "NEXT_LEVEL_BTN") {
            if (currentLevelIdx + 1 < NUM_LEVELS) {
                loadLevel(currentLevelIdx + 1);
                screen = ScreenState::PLAYING;
            }
            return true;
        }
        if (id == "OPEN_ACHIEVEMENTS") {
            screen = ScreenState::ACHIEVEMENTS;
            return true;
        }
        if (id == "PAGE_PREV") {
            if (levelSelectPage > 0) levelSelectPage--;
            return true;
        }
        if (id == "PAGE_NEXT") {
            if (levelSelectPage < 6) levelSelectPage++;
            return true;
        }
        if (id == "GOTO_PAGE_0") { levelSelectPage = 0; return true; }
        if (id == "GOTO_PAGE_1") { levelSelectPage = 1; return true; }
        if (id == "GOTO_PAGE_2") { levelSelectPage = 2; return true; }
        if (id == "GOTO_PAGE_3") { levelSelectPage = 3; return true; }
        if (id == "GOTO_PAGE_4") { levelSelectPage = 4; return true; }
        if (id == "GOTO_PAGE_5") { levelSelectPage = 5; return true; }
        if (id == "GOTO_PAGE_6") { levelSelectPage = 6; return true; }
        if (id.rfind("PLAY_LEVEL_", 0) == 0) {
            int idx = std::atoi(id.substr(11).c_str());
            if (idx >= 0 && idx < NUM_LEVELS && isLevelUnlocked(idx)) {
                loadLevel(idx);
                screen = ScreenState::PLAYING;
            }
            return true;
        }
        return false;
    }
};


// ============================================================================
// AUTOMATED HEADLESS SELF-TEST & SCREENSHOT VERIFICATION SUITE
// ============================================================================
static int runSelfTestAndExportScreenshots(bool exportBmp) {
    std::string existingSave;
    {
        std::ifstream in("save_progress.cfg");
        if (in.good()) {
            std::stringstream ss;
            ss << in.rdbuf();
            existingSave = ss.str();
        }
    }

    setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("SDL_AUDIODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Snout & Siege Self-Test", 0, 0, WINDOW_W, WINDOW_H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    SnoutAndSiegeGame game;
    bool assetsOk = game.assets.loadAll(ren);
    game.initLevels();

    std::cout << "[SELF-TEST] Backend: " << (gSDL.loaded ? "SDL2 Dynamic Runtime" : "X11/Software Fallback")
              << " | Loaded " << game.assets.loadedSpriteCount << " BMP sprites, "
              << game.assets.loadedSoundCount << " WAV sounds, and "
              << game.levels.size() << " levels.\n";
    if (!assetsOk || game.assets.loadedSpriteCount < 23 || game.assets.loadedSoundCount < 25 ||
        static_cast<int>(game.levels.size()) != NUM_LEVELS) {
        std::cerr << "[SELF-TEST] Asset or " << NUM_LEVELS << "-level count verification failed!\n";
        return 2;
    }

    // Verify all 150 levels stand rock-solid at idle AND when awake for 300 frames (5 seconds)
    // checking block tilt, block displacement, bird rotation, bird displacement, and pendulum equilibrium!
    int stabilityFailures = 0;
    for (int lvl = 0; lvl < NUM_LEVELS; ++lvl) {
        game.loadLevel(lvl);
        game.fortressAwake = true; // Even with physics awake, untouched towers, birds, & pendulums must not drift or rotate!
        auto initBlocks = game.blocks;
        auto initBirds = game.birds;
        for (int f = 0; f < 300; ++f) game.stepPhysics(1.0f / 60.0f);
        float maxTilt = 0.0f, maxDisp = 0.0f, maxBirdRot = 0.0f, maxBirdDisp = 0.0f;
        for (size_t i = 0; i < game.blocks.size(); ++i) {
            maxTilt = std::max(maxTilt, std::abs(game.blocks[i].angle));
            maxDisp = std::max(maxDisp, (game.blocks[i].pos - initBlocks[i].pos).length());
        }
        for (size_t i = 0; i < game.birds.size(); ++i) {
            maxBirdRot = std::max(maxBirdRot, std::abs(game.birds[i].angle));
            maxBirdDisp = std::max(maxBirdDisp, (game.birds[i].pos - initBirds[i].pos).length());
        }
        for (size_t i = 0; i < game.pendulums.size(); ++i) {
            maxTilt = std::max(maxTilt, std::abs(game.pendulums[i].angle));
        }
        if (maxTilt > 0.01f || maxDisp > 1.5f || maxBirdRot > 0.01f || maxBirdDisp > 1.5f) {
            stabilityFailures++;
            std::cerr << "[SELF-TEST] Level " << (lvl + 1) << " failed 300-frame stability: "
                      << "maxTilt=" << maxTilt << " maxDisp=" << maxDisp
                      << " maxBirdRot=" << maxBirdRot << " maxBirdDisp=" << maxBirdDisp << "\n";
            for (size_t i = 0; i < game.blocks.size(); ++i) {
                float d = (game.blocks[i].pos - initBlocks[i].pos).length();
                if (d > 0.5f) {
                    std::cerr << "  Block " << i << " moved " << d << "px: init=("
                              << initBlocks[i].pos.x << "," << initBlocks[i].pos.y << ",w=" << initBlocks[i].w << ",h=" << initBlocks[i].h << ") now=("
                              << game.blocks[i].pos.x << "," << game.blocks[i].pos.y << ")\n";
                }
            }
        }
    }
    if (stabilityFailures > 0) {
        std::cerr << "[SELF-TEST] " << stabilityFailures << " levels failed stability check!\n";
        return 3;
    }
    std::cout << "[SELF-TEST] All " << NUM_LEVELS << " levels verified rock-solid at idle (maxTilt = 0.000 rad).\n";

    auto saveFrame = [&](const std::string& path) {
        if (!exportBmp) return;
        game.render(ren);
        SDL_RenderPresent(ren);
        SDL_Surface* snap = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_W, WINDOW_H, 32,
                                                           SDL_PIXELFORMAT_ARGB8888);
        SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_ARGB8888, snap->pixels, snap->pitch);
        SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "wb");
        if (rw) SDL_SaveBMP_RW(snap, rw, 1);
        SDL_FreeSurface(snap);
    };

    // Test per-level high score and level unlocking
    game.recordLevelScore(0, 44200, 3);
    game.recordLevelScore(1, 36800, 3);
    game.recordLevelScore(2, 29500, 2);

    game.screen = ScreenState::TITLE;
    saveFrame("screenshot_title.bmp");

    // Export Royal Swine Trophy Hall & Achievements Screen
    game.screen = ScreenState::ACHIEVEMENTS;
    saveFrame("screenshot_achievements.bmp");

    // Export Level Select Pages 1, 2, and 6 (Vaults)
    game.screen = ScreenState::LEVEL_SELECT;
    game.levelSelectPage = 0; saveFrame("screenshot_level_select_p1.bmp");
    game.levelSelectPage = 1; saveFrame("screenshot_level_select_p2.bmp");
    game.levelSelectPage = 6; saveFrame("screenshot_level_select_p3.bmp");

    // Export screenshots of Chapter Backgrounds across levels
    game.screen = ScreenState::PLAYING;
    game.loadLevel(0);  saveFrame("screenshot_chap1.bmp");
    game.loadLevel(1);  saveFrame("screenshot_truffle_level.bmp");
    game.loadLevel(2);  saveFrame("screenshot_level3.bmp");
    game.loadLevel(10); saveFrame("screenshot_chap2.bmp");
    game.loadLevel(20); saveFrame("screenshot_chap3.bmp");
    game.loadLevel(30); saveFrame("screenshot_chap4.bmp");
    game.loadLevel(40); saveFrame("screenshot_chap5.bmp");
    game.loadLevel(NUM_CAMPAIGN_LEVELS); saveFrame("screenshot_vault_level.bmp");
    game.loadLevel(NUM_CAMPAIGN_LEVELS); saveFrame("screenshot_chap6_vault.bmp");

    // Export Fortress Architect level builder
    game.screen = ScreenState::EDITOR;
    game.isTestPlaying = false;
    game.initEditor();
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::STONE, 760.0f, GROUND_Y - 12.0f, 160.0f, 24.0f));
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 710.0f, GROUND_Y - 55.0f, 20.0f, 70.0f));
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 810.0f, GROUND_Y - 55.0f, 20.0f, 70.0f));
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 760.0f, GROUND_Y - 98.0f, 140.0f, 18.0f));
    BirdTarget eb1{}; eb1.kind = BirdClass::OWL; eb1.pos = {760.0f, GROUND_Y - 130.0f}; eb1.radius = 24.0f; eb1.hp = 120.0f; eb1.maxHp = 120.0f; eb1.hasHelmet = true; eb1.helmetHp = 100.0f; eb1.maxHelmetHp = 100.0f;
    game.customLevel.birds.push_back(eb1);
    saveFrame("screenshot_editor.bmp");

    // Export Endless Siege Gauntlet Mode
    game.startEndlessMode();
    saveFrame("screenshot_endless.bmp");

    // Export Gale the Aeronaut (Glider Pig) in mid-air aerodynamic flight with aerial bombs
    game.loadLevel(0);
    game.activePigs.clear();
    PigBody gale = game.createPigInstance(PigType::GLIDER, {580.0f, 280.0f});
    gale.launched = true;
    gale.vel = {320.0f, -40.0f};
    game.activePigs.push_back(gale);
    AerialBomb ab1{}; ab1.active = true; ab1.pos = {540.0f, 340.0f}; ab1.vel = {60.0f, 160.0f}; ab1.radius = 9.0f;
    AerialBomb ab2{}; ab2.active = true; ab2.pos = {500.0f, 370.0f}; ab2.vel = {40.0f, 180.0f}; ab2.radius = 9.0f;
    game.aerialBombs = {ab1, ab2};
    saveFrame("screenshot_pig_glider.bmp");

    // Export Atticus the Graviton Boar activating cosmic vortex
    game.loadLevel(27); // Level 28 (Cavern Gate)
    game.activePigs.clear();
    PigBody atticus = game.createPigInstance(PigType::GRAVITON, {600.0f, 320.0f});
    atticus.launched = true;
    atticus.abilityUsed = true;
    atticus.vortexTimer = 1.2f;
    atticus.vel = {40.0f, 10.0f};
    game.activePigs.push_back(atticus);
    for (int k = 0; k < 25; ++k) {
        float ang = k * (6.28318f / 25.0f);
        float r = 50.0f + 30.0f * (k % 3);
        Particle cm{};
        cm.pos = Vec2(600.0f + std::cos(ang) * r, 320.0f + std::sin(ang) * r);
        cm.vel = Vec2(-std::sin(ang) * 90.0f, std::cos(ang) * 90.0f);
        cm.kind = ParticleKind::COSMIC_MOTE;
        cm.color = {175, 95, 255, 255};
        cm.size = 5.0f;
        cm.life = 1.0f;
        cm.maxLife = 1.0f;
        game.particles.push_back(cm);
    }
    saveFrame("screenshot_pig_graviton.bmp");
    game.particles.clear();

    // Export Rusty the Drill Hog executing supersonic rotary bore
    game.loadLevel(15); // Level 16 (Canopy Skyway)
    game.activePigs.clear();
    PigBody rusty = game.createPigInstance(PigType::DRILL, {680.0f, 420.0f});
    rusty.launched = true;
    rusty.abilityUsed = true;
    rusty.drillTimer = 1.5f;
    rusty.vel = {780.0f, 0.0f};
    game.activePigs.push_back(rusty);
    game.spawnSplinters({700.0f, 420.0f}, MaterialTier::TIMBER, 16, 260.0f);
    saveFrame("screenshot_pig_drill.bmp");
    game.particles.clear();

    // Export Commander Falcon with Directional Tower Shield defending the fortress
    game.loadLevel(15); // Level 16
    saveFrame("screenshot_bird_falcon.bmp");

    // Export Baron Von Condor in Suspended Steampunk Zeppelin hovering above fortress
    game.loadLevel(31); // Level 32 (Zeppelin Spire)
    saveFrame("screenshot_bird_condor.bmp");

    // Export Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
    game.sloMoActive = true;
    game.sloMoTimer = 1.0f;
    game.letterboxY = 46.0f;
    game.sloMoFocusPos = {800.0f, 380.0f};
    saveFrame("screenshot_slomo_finish.bmp");
    game.sloMoActive = false;
    game.letterboxY = 0.0f;
    game.aerialBombs.clear();

    // Export gameplay juice screenshot demonstrating aiming trajectory, ghost arc, splinters & feathers
    game.loadLevel(0);
    game.ghostTrajectory = {
        {SLING_X, SLING_Y}, {340.0f, 380.0f}, {480.0f, 310.0f}, {620.0f, 290.0f},
        {760.0f, 330.0f}, {880.0f, 420.0f}, {960.0f, 520.0f}
    };
    game.draggingSling = true;
    game.slingPullPos = Vec2(SLING_X - 65.0f, SLING_Y + 45.0f);
    if (!game.activePigs.empty()) {
        game.activePigs[0].pos = game.slingPullPos;
        game.activePigs[0].angle = std::atan2(-45.0f, 65.0f);
    }
    game.spawnSplinters({820.0f, 460.0f}, MaterialTier::TIMBER, 14, 200.0f);
    game.spawnFeatherBurst({860.0f, 420.0f}, BirdClass::TOUCAN, 18, 220.0f);
    game.floatScores.push_back({{860.0f, 390.0f}, "+5000", {255, 230, 90, 255}, 1.1f, 1.3f});
    saveFrame("screenshot_gameplay_juice.bmp");

    // Test standard level complete modal
    game.loadLevel(0);
    game.bestScores[0] = 50000;
    for (auto& t : game.birds) t.defeated = true;
    game.score = 18500;
    game.stepPhysics(1.0f / 60.0f);
    saveFrame("screenshot_level_complete.bmp");

    // Test NEW HIGH SCORE celebration banner & animation
    game.loadLevel(0);
    game.bestScores[0] = 20000;
    game.prevLevelBest = 20000;
    for (auto& t : game.birds) t.defeated = true;
    game.score = 46500;
    game.stepPhysics(1.0f / 60.0f);
    saveFrame("screenshot_new_highscore.bmp");

    // Test Milestone Pig Unlock modal (Level 5 cleared -> Bramble unlocked!)
    game.loadLevel(4);
    for (auto& t : game.birds) t.defeated = true;
    game.score = 32000;
    game.stepPhysics(1.0f / 60.0f);
    saveFrame("screenshot_milestone_unlock.bmp");

    // Test Level 7 (The Powder Viaduct with TNT Kegs)
    game.loadLevel(6);
    saveFrame("screenshot_tnt_level.bmp");

    // Test Level 31 (Glacial Ziggurat with Brittle Ice)
    game.loadLevel(30);
    saveFrame("screenshot_ice_level.bmp");

    // Test Level 8 (The Bouncing Keep with Trampoline Deflector)
    game.loadLevel(7);
    saveFrame("screenshot_trampoline_level.bmp");

    // Test Level 12 (Cauldron Barracks with Cauldron-Helmed Birds)
    game.loadLevel(11);
    saveFrame("screenshot_helmed_bird.bmp");

    // Test Level 18 (Pendulum Citadel with Swinging Wrecking Ball)
    game.loadLevel(17);
    saveFrame("screenshot_pendulum_level.bmp");

    // Test Level 14 (Crossbeam Stockade with Headwind & Wind Gauge)
    game.loadLevel(13);
    saveFrame("screenshot_wind_level.bmp");

    // Test Level 39 (Thunder Peak Sanctuary with Cumulonimbus Storm & Lightning)
    game.loadLevel(38);
    game.triggerLightningStrike();
    saveFrame("screenshot_storm_level.bmp");

    // Test Level 44 (Volcanic Casemate with Magma Vents)
    game.loadLevel(43);
    for (auto& b : game.blocks) {
        if (b.mat == MaterialTier::MAGMA_VENT) {
            game.eruptMagmaVent(b);
            break;
        }
    }
    saveFrame("screenshot_magma_level.bmp");

    // Verify Simulation Fast-Forward toggle
    game.simSpeed = 2.5f;
    game.updateGame(0.016f);
    if (game.simSpeed != 2.5f) {
        std::cerr << "[SELF-TEST] simSpeed failed to maintain 2.5x during game update!\n";
        return 4;
    }
    game.spawnNextPigInSlingshot();
    if (game.simSpeed != 1.0f) {
        std::cerr << "[SELF-TEST] simSpeed failed to auto-reset to 1.0x on pig reload!\n";
        return 5;
    }

    // Verify TNT Detonation and Ice destruction mechanics
    game.loadLevel(6);
    bool foundTnt = false;
    for (auto& b : game.blocks) {
        if (b.mat == MaterialTier::TNT) {
            foundTnt = true;
            game.triggerTntDetonation(b);
            break;
        }
    }
    if (!foundTnt) {
        std::cerr << "[SELF-TEST] Level 7 missing TNT keg!\n";
        return 6;
    }
    game.stepPhysics(1.0f / 60.0f);

    // Verify Cauldron Helmet absorption & shattering mechanics
    {
        BirdTarget bt{};
        bt.kind = BirdClass::OWL;
        bt.pos = {800.0f, 500.0f};
        bt.radius = 23.0f;
        bt.hp = 140.0f;
        bt.maxHp = 140.0f;
        bt.hasHelmet = true;
        bt.helmetHp = 100.0f;
        bt.maxHelmetHp = 100.0f;
        game.damageBird(bt, 40.0f);
        if (bt.helmetHp != 60.0f || bt.hp != 140.0f || !bt.hasHelmet) {
            std::cerr << "[SELF-TEST] Helmet failed to absorb 40 damage properly!\n";
            return 7;
        }
        int scoreBefore = game.score;
        game.damageBird(bt, 80.0f); // 60 absorbed by helmet (shatters!), 20 into bird hp (140 -> 120)
        if (bt.hasHelmet || bt.helmetHp != 0.0f || bt.hp != 120.0f || (game.score - scoreBefore) != 1000) {
            std::cerr << "[SELF-TEST] Helmet shattering failed!\n";
            return 8;
        }
    }

    // Verify Trampoline elastic bounce mechanics
    {
        PigBody pb{};
        pb.active = true;
        pb.launched = true;
        pb.radius = 18.0f;
        pb.pos = {600.0f, 595.0f};
        pb.vel = {50.0f, 200.0f};
        BlockBody tb{};
        tb.mat = MaterialTier::TRAMPOLINE;
        tb.pos = {600.0f, 618.0f};
        tb.w = 100.0f;
        tb.h = 24.0f;
        game.resolveCircleOBB(pb, tb);
        if (pb.vel.y >= 0.0f) {
            std::cerr << "[SELF-TEST] Trampoline failed to bounce pig upward! vy=" << pb.vel.y << "\n";
            return 9;
        }
    }

    // Verify Pendulum harmonic gravity physics
    {
        PendulumBody pend{};
        pend.anchor = {800.0f, 200.0f};
        pend.length = 150.0f;
        pend.angle = 0.35f;
        pend.angVel = 0.0f;
        pend.radius = 25.0f;
        pend.mass = 8.0f;
        game.pendulums = {pend};
        game.fortressAwake = true;
        game.stepPhysics(1.0f / 60.0f);
        if (game.pendulums[0].angVel >= 0.0f) {
            std::cerr << "[SELF-TEST] Pendulum harmonic gravity failed to accelerate toward equilibrium!\n";
            return 10;
        }
    }

    // Verify Wind acceleration on airborne pig
    {
        game.loadLevel(21); // Level 22 has canyon tailwind (wind = +110.0f)
        PigBody pb{};
        pb.active = true;
        pb.launched = true;
        pb.radius = 18.0f;
        pb.pos = {400.0f, 400.0f};
        pb.vel = {0.0f, 0.0f};
        game.activePigs = {pb};
        game.stepPhysics(1.0f / 60.0f);
        if (game.activePigs[0].vel.x <= 0.0f) {
            std::cerr << "[SELF-TEST] Wind failed to accelerate airborne pig downwind!\n";
            return 11;
        }
    }

    // Verify Magma Vent blast damage and thermal effects (ice melt & TNT ignition)
    {
        BlockBody vent = SnoutAndSiegeGame::makeBlock(MaterialTier::MAGMA_VENT, 500.0f, 600.0f, 40.0f, 28.0f);
        BlockBody ice = SnoutAndSiegeGame::makeBlock(MaterialTier::ICE, 530.0f, 600.0f, 30.0f, 30.0f);
        BlockBody tnt = SnoutAndSiegeGame::makeBlock(MaterialTier::TNT, 470.0f, 600.0f, 30.0f, 30.0f);
        game.blocks = {vent, ice, tnt};
        float iceHpBefore = game.blocks[1].hp;
        game.damageBlock(game.blocks[0], 50.0f);
        if (game.blocks[1].hp >= iceHpBefore || !game.blocks[2].ignited) {
            std::cerr << "[SELF-TEST] Magma vent blast failed to melt ice or ignite adjacent TNT!\n";
            return 12;
        }
    }

    // Verify Lightning strike generation and sky flash
    {
        game.loadLevel(38); // Thunder Peak Sanctuary
        game.triggerLightningStrike();
        if (game.lightningBolts.empty() || game.skyFlashAlpha <= 0.0f) {
            std::cerr << "[SELF-TEST] Lightning strike generation failed!\n";
            return 13;
        }
    }

    // Unit Test 14: Golden Truffle collection, scoring, particle burst & TRUFFLE_SNIFFER achievement
    {
        game.loadLevel(1); // Level 2 has hidden golden truffle
        if (!game.levelTruffle.exists || game.levelTruffle.collected) {
            std::cerr << "[SELF-TEST] Level 2 golden truffle not properly initialized!\n";
            return 14;
        }
        int scoreBefore = game.score;
        game.collectGoldenTruffle();
        if (!game.levelTruffle.collected || (game.score - scoreBefore) != 2500 || !game.trufflesFound[1]) {
            std::cerr << "[SELF-TEST] Truffle collection logic or +2,500 score award failed!\n";
            return 15;
        }
        int sparkleCount = 0;
        for (const auto& p : game.particles) {
            if (p.kind == ParticleKind::GOLD_SPARKLE) sparkleCount++;
        }
        if (sparkleCount < 24) {
            std::cerr << "[SELF-TEST] Truffle collection failed to spawn >= 24 GOLD_SPARKLE particles!\n";
            return 16;
        }
        if (!game.achievementsUnlocked[static_cast<int>(AchievementId::TRUFFLE_SNIFFER)]) {
            std::cerr << "[SELF-TEST] Collecting first truffle failed to unlock TRUFFLE_SNIFFER achievement!\n";
            return 17;
        }
    }

    // Unit Test 15: Secret Vault unlock progression logic (2, 5, 8, 11, 14, 18, 22, 26, 30, 35 truffles)
    {
        // Reset truffles found
        game.trufflesFound.fill(false);
        // With 0 truffles, all vaults should be locked
        for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) {
            if (game.isLevelUnlocked(v)) {
                std::cerr << "[SELF-TEST] Vault level " << v << " should be locked with 0 truffles!\n";
                return 18;
            }
        }
        // Found 2 truffles -> Vault 01 (index NUM_CAMPAIGN_LEVELS) unlocked, Vault 02 (index NUM_CAMPAIGN_LEVELS + 1) locked
        game.trufflesFound[1] = true;
        game.trufflesFound[4] = true;
        if (!game.isLevelUnlocked(NUM_CAMPAIGN_LEVELS) || game.isLevelUnlocked(NUM_CAMPAIGN_LEVELS + 1)) {
            std::cerr << "[SELF-TEST] Vault 01 failed to unlock with 2 truffles, or Vault 02 unlocked prematurely!\n";
            return 19;
        }
        // Found 35 truffles -> All 10 vaults unlocked
        for (int i = 0; i < 35; ++i) game.trufflesFound[i] = true;
        for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) {
            if (!game.isLevelUnlocked(v)) {
                std::cerr << "[SELF-TEST] Vault level " << v << " failed to unlock with 35 truffles!\n";
                return 20;
            }
        }
    }

    // Unit Test 16: Achievement unlocking, toast animation triggering, and mask serialization
    {
        // Reset achievements
        game.achievementsUnlocked.fill(false);
        game.toast.active = false;
        game.unlockAchievement(AchievementId::FIRST_BLOOD);
        if (!game.achievementsUnlocked[static_cast<int>(AchievementId::FIRST_BLOOD)]) {
            std::cerr << "[SELF-TEST] unlockAchievement failed to mark FIRST_BLOOD unlocked!\n";
            return 21;
        }
        if (!game.toast.active || game.toast.title != "First Strike") {
            std::cerr << "[SELF-TEST] Toast notification failed to activate for unlocked achievement!\n";
            return 22;
        }
        uint32_t mask = game.getAchievementsMask();
        if ((mask & (1u << static_cast<int>(AchievementId::FIRST_BLOOD))) == 0) {
            std::cerr << "[SELF-TEST] getAchievementsMask failed to include FIRST_BLOOD bit!\n";
            return 23;
        }
        // Test save and load persistence
        game.saveProgress();
        SnoutAndSiegeGame testLoader;
        testLoader.initAchievements();
        testLoader.loadProgress();
        if (!testLoader.achievementsUnlocked[static_cast<int>(AchievementId::FIRST_BLOOD)]) {
            std::cerr << "[SELF-TEST] Achievement mask failed to persist through saveProgress/loadProgress!\n";
            return 24;
        }
    }

    // Unit Test 17: Gale the Aeronaut (Glider Pig) aerodynamic lift & twin aerial bomb detonation
    {
        game.loadLevel(0);
        game.activePigs.clear();
        game.aerialBombs.clear();
        PigBody gale = game.createPigInstance(PigType::GLIDER, {450.0f, 300.0f});
        gale.launched = true;
        gale.vel = {200.0f, 50.0f}; // Moving down and forward
        game.activePigs.push_back(gale);

        game.activateMidAirAbility();
        if (!game.activePigs[0].abilityUsed || game.activePigs[0].vel.y >= 0.0f || game.activePigs[0].vel.x <= 200.0f) {
            std::cerr << "[SELF-TEST] Gale mid-air aerodynamic surge failed to lift swine upward and accelerate forward!\n";
            return 25;
        }
        if (game.aerialBombs.size() != 2) {
            std::cerr << "[SELF-TEST] Gale ability failed to drop exactly 2 aerial bombs! count=" << game.aerialBombs.size() << "\n";
            return 26;
        }
        // Place a test target block and test bomb detonation
        BlockBody targetBlock = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 450.0f, 350.0f, 40.0f, 40.0f);
        game.blocks = {targetBlock};
        float blockHpBefore = game.blocks[0].hp;
        game.aerialBombs[0].pos = {450.0f, 340.0f}; // Right above target block
        game.detonateAerialBomb(game.aerialBombs[0]);
        if (game.blocks[0].hp >= blockHpBefore) {
            std::cerr << "[SELF-TEST] Aerial bomb detonation failed to inflict radial damage on target block!\n";
            return 27;
        }
    }

    // Unit Test 18: Fortress Architect Level Serialization & Test Play Snapshot Restoration
    {
        game.screen = ScreenState::EDITOR;
        game.isTestPlaying = false;
        game.initEditor();
        game.customSlot = 2;
        game.customLevel.name = "SIEGE_TEST_SLOT_2";
        game.customLevel.blocks = {
            SnoutAndSiegeGame::makeBlock(MaterialTier::STONE, 750.0f, 500.0f, 30.0f, 60.0f),
            SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 820.0f, 500.0f, 20.0f, 70.0f),
            SnoutAndSiegeGame::makeBlock(MaterialTier::ICE, 785.0f, 450.0f, 90.0f, 18.0f)
        };
        BirdTarget t1{}; t1.kind = BirdClass::OWL; t1.pos = {785.0f, 420.0f}; t1.radius = 24.0f; t1.hp = 120.0f; t1.maxHp = 120.0f; t1.hasHelmet = true; t1.helmetHp = 100.0f; t1.maxHelmetHp = 100.0f;
        BirdTarget t2{}; t2.kind = BirdClass::HERON; t2.pos = {880.0f, 500.0f}; t2.radius = 28.0f; t2.hp = 180.0f; t2.maxHp = 180.0f;
        game.customLevel.birds = {t1, t2};
        PendulumBody pb{}; pb.anchor = {785.0f, 250.0f}; pb.length = 90.0f; pb.angle = 0.35f; pb.radius = 20.0f; pb.spiked = true;
        game.customLevel.pendulums = {pb};
        game.customLevel.hasTruffle = true;
        game.customLevel.trufflePos = {785.0f, 380.0f};

        game.saveCustomLevel(2);
        std::ifstream in("custom_siege_2.cfg");
        if (!in.good()) {
            std::cerr << "[SELF-TEST] saveCustomLevel(2) failed to write custom_siege_2.cfg!\n";
            return 28;
        }
        in.close();

        // Clear in-memory and reload
        game.customLevel.blocks.clear();
        game.customLevel.birds.clear();
        game.customLevel.pendulums.clear();
        game.customLevel.hasTruffle = false;
        game.loadCustomLevel(2);

        if (game.customLevel.blocks.size() != 3 || game.customLevel.birds.size() != 2 ||
            game.customLevel.pendulums.size() != 1 || !game.customLevel.hasTruffle) {
            std::cerr << "[SELF-TEST] loadCustomLevel(2) failed to deserialize exact custom level specification!\n";
            return 29;
        }

        // Test play snapshot and return
        game.startEditorTestPlay();
        if (!game.isTestPlaying || game.blocks.size() != 3 || game.birds.size() != 2 || game.activePigs.empty()) {
            std::cerr << "[SELF-TEST] startEditorTestPlay failed to initialize playable test session!\n";
            return 30;
        }
        // Destroy a block during test play
        game.blocks[0].destroyed = true;
        game.returnToEditor();
        if (game.isTestPlaying || game.customLevel.blocks.size() != 3 || game.customLevel.blocks[0].destroyed) {
            std::cerr << "[SELF-TEST] returnToEditor failed to restore pristine architectural snapshot!\n";
            return 31;
        }
        std::remove("custom_siege_2.cfg");
    }

    // Unit Test 19: Endless Siege Gauntlet Mode (procedural scaling & combo multiplier)
    {
        game.startEndlessMode();
        if (game.screen != ScreenState::ENDLESS || game.endlessWave != 1 || game.endlessScore != 0 ||
            game.blocks.empty() || game.birds.empty() || game.activePigs.empty()) {
            std::cerr << "[SELF-TEST] startEndlessMode failed to initialize Wave 1 arena and swine arsenal!\n";
            return 32;
        }
        // Verify combo multiplier increases on hit
        float multBefore = game.endlessComboMultiplier;
        game.damageBird(game.birds[0], 25.0f);
        if (game.endlessScore <= 0 || game.endlessComboMultiplier <= multBefore) {
            std::cerr << "[SELF-TEST] Endless mode score or combo multiplier failed to advance on bird damage!\n";
            return 33;
        }
        // Advance wave by defeating all birds
        for (auto& b : game.birds) b.defeated = true;
        game.stepPhysics(1.0f / 60.0f);
        if (game.endlessWave != 2) {
            std::cerr << "[SELF-TEST] Defeating all birds in wave 1 failed to advance to Wave 2!\n";
            return 34;
        }
        if (game.endlessBestWave < 2) {
            std::cerr << "[SELF-TEST] endlessBestWave failed to record highest wave achieved!\n";
            return 35;
        }
    }

    // Unit Test 20: Atticus the Graviton Boar vortex pull, cosmic mote emission & singularity implosion
    {
        game.loadLevel(0);
        game.activePigs.clear();
        game.particles.clear();
        PigBody atticus = game.createPigInstance(PigType::GRAVITON, {500.0f, 300.0f});
        atticus.launched = true;
        atticus.vel = {140.0f, -20.0f};
        game.activePigs.push_back(atticus);

        game.activateMidAirAbility();
        if (!game.activePigs[0].abilityUsed || game.activePigs[0].vortexTimer <= 0.0f) {
            std::cerr << "[SELF-TEST] Graviton Boar ability activation failed!\n";
            return 36;
        }

        // Test vortex pull forces on nearby blocks & birds
        BlockBody testBlock = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 620.0f, 300.0f, 30.0f, 30.0f);
        testBlock.vel = {0.0f, 0.0f};
        game.blocks = {testBlock};
        game.stepPhysics(1.0f / 60.0f);
        // Block was to the right of Atticus (x=620 vs x=500), so pull force must accelerate it leftwards (vel.x < 0)
        if (game.blocks[0].vel.x >= 0.0f) {
            std::cerr << "[SELF-TEST] Graviton vortex failed to exert inward pull force on adjacent block! vx="
                      << game.blocks[0].vel.x << "\n";
            return 37;
        }

        // Test singularity implosion shockwave, radial damage & VORTEX_MAESTRO achievement
        BirdTarget b1{}; b1.kind = BirdClass::TOUCAN; b1.pos = {470.0f, 300.0f}; b1.radius = 18.0f; b1.hp = 80.0f; b1.maxHp = 80.0f;
        BirdTarget b2{}; b2.kind = BirdClass::OWL;    b2.pos = {530.0f, 300.0f}; b2.radius = 21.0f; b2.hp = 120.0f; b2.maxHp = 120.0f;
        BirdTarget b3{}; b3.kind = BirdClass::HERON;  b3.pos = {500.0f, 260.0f}; b3.radius = 24.0f; b3.hp = 180.0f; b3.maxHp = 180.0f;
        game.birds = {b1, b2, b3};
        float b1HpBefore = b1.hp;
        game.triggerGravitonImplosion(game.activePigs[0].pos);
        if (game.birds[0].hp >= b1HpBefore || !game.achievementsUnlocked[static_cast<int>(AchievementId::VORTEX_MAESTRO)]) {
            std::cerr << "[SELF-TEST] Graviton singularity implosion failed to damage birds or unlock VORTEX_MAESTRO!\n";
            return 38;
        }
    }

    // Unit Test 21: Rusty the Drill Hog supersonic rotary bore, multi-block penetration & foundation subsidence
    {
        game.loadLevel(0);
        game.activePigs.clear();
        PigBody rusty = game.createPigInstance(PigType::DRILL, {400.0f, 400.0f});
        rusty.launched = true;
        rusty.vel = {160.0f, 0.0f};
        game.activePigs.push_back(rusty);

        game.activateMidAirAbility();
        if (!game.activePigs[0].abilityUsed || game.activePigs[0].drillTimer <= 0.0f || game.activePigs[0].vel.x < 750.0f) {
            std::cerr << "[SELF-TEST] Drill Hog supersonic bore activation failed! vx=" << game.activePigs[0].vel.x << "\n";
            return 39;
        }

        // Test multi-block piercing through 3 consecutive blocks in a single shot & DEEP_BORE achievement
        game.shotDrillPierces = 0;
        BlockBody b1 = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 415.0f, 400.0f, 20.0f, 50.0f);
        game.activePigs[0].pos = {400.0f, 400.0f};
        game.resolveCircleOBB(game.activePigs[0], b1);

        BlockBody b2 = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 445.0f, 400.0f, 20.0f, 50.0f);
        game.activePigs[0].pos = {430.0f, 400.0f};
        game.resolveCircleOBB(game.activePigs[0], b2);

        BlockBody b3 = SnoutAndSiegeGame::makeBlock(MaterialTier::STONE,  475.0f, 400.0f, 20.0f, 50.0f);
        game.activePigs[0].pos = {460.0f, 400.0f};
        game.resolveCircleOBB(game.activePigs[0], b3);

        if (game.shotDrillPierces < 3 || !game.achievementsUnlocked[static_cast<int>(AchievementId::DEEP_BORE)] || game.activePigs[0].vel.x < 350.0f) {
            std::cerr << "[SELF-TEST] Drill Hog failed to pierce 3 blocks or award DEEP_BORE! pierces="
                      << game.shotDrillPierces << " vx=" << game.activePigs[0].vel.x << "\n";
            return 40;
        }

        // Test foundation burrowing subsidence cave-in when contacting the ground
        game.activePigs[0].pos = {700.0f, GROUND_Y - game.activePigs[0].radius - 1.0f};
        BlockBody foundation = SnoutAndSiegeGame::makeBlock(MaterialTier::STONE, 720.0f, GROUND_Y - 30.0f, 30.0f, 40.0f);
        game.blocks = {foundation};
        float foundHpBefore = foundation.hp;
        game.stepPhysics(1.0f / 60.0f);
        if (game.blocks[0].vel.y <= 0.0f || game.blocks[0].hp >= foundHpBefore) {
            std::cerr << "[SELF-TEST] Drill Hog ground burrowing failed to trigger subsidence on foundation block!\n";
            return 41;
        }
    }

    // Unit Test 22: Commander Falcon directional shield deflection, damage absorption & shattering
    {
        BirdTarget falcon{};
        falcon.kind = BirdClass::FALCON;
        falcon.pos = {800.0f, 500.0f};
        falcon.radius = 23.0f;
        falcon.hp = 150.0f;
        falcon.maxHp = 150.0f;
        falcon.hasShield = true;
        falcon.shieldHp = 120.0f;
        falcon.maxShieldHp = 120.0f;
        falcon.mass = 2.5f;
        falcon.invMass = 1.0f / 2.5f;

        // Frontal pig collision bounce-back (p.pos.x <= falcon.pos.x) with high restitution (e=0.75)
        PigBody p{};
        p.active = true;
        p.launched = true;
        p.radius = 20.0f;
        p.mass = 1.0f;
        p.invMass = 1.0f;
        p.pos = {762.0f, 500.0f}; // distance 38 < radius sum 43
        p.vel = {140.0f, 0.0f};

        game.birds = {falcon};
        game.activePigs = {p};
        game.stepPhysics(1.0f / 60.0f);

        // Pig must bounce backward due to shield deflection (vel.x < 0) and shield absorbs damage
        if (game.activePigs[0].vel.x >= 0.0f || game.birds[0].shieldHp >= 120.0f || game.birds[0].shieldHp <= 0.0f || game.birds[0].hp != 150.0f) {
            std::cerr << "[SELF-TEST] Falcon directional shield failed to deflect swine backward or absorb hit! vx="
                      << game.activePigs[0].vel.x << " shieldHp=" << game.birds[0].shieldHp << "\n";
            return 42;
        }

        // Test shield shattering when cumulative damage exceeds shield HP
        int scoreBefore = game.score;
        game.damageBird(game.birds[0], 90.0f);
        if (game.birds[0].hasShield || game.birds[0].shieldHp != 0.0f || (game.score - scoreBefore) != 1200) {
            std::cerr << "[SELF-TEST] Falcon shield failed to shatter or award +1200 bonus score!\n";
            return 43;
        }

        // Verify SHIELD_DEBRIS particles spawned
        bool foundShieldDebris = false;
        for (const auto& part : game.particles) {
            if (part.kind == ParticleKind::SHIELD_DEBRIS) { foundShieldDebris = true; break; }
        }
        if (!foundShieldDebris) {
            std::cerr << "[SELF-TEST] Falcon shield shattering failed to spawn SHIELD_DEBRIS particles!\n";
            return 44;
        }
    }

    // Unit Test 23: Baron Von Condor in Steampunk Zeppelin buoyant lift, balloon pop & plummet crushing blocks
    {
        BirdTarget condor{};
        condor.kind = BirdClass::CONDOR;
        condor.pos = {800.0f, 350.0f};
        condor.radius = 28.0f;
        condor.hp = 240.0f;
        condor.maxHp = 240.0f;
        condor.inZeppelin = true;
        condor.balloonHp = 70.0f;
        condor.maxBalloonHp = 70.0f;
        condor.mass = 2.8f;
        condor.invMass = 1.0f / 2.8f;

        // Test buoyant lift: zeppelin experiences only 15% downward gravity
        game.birds = {condor};
        game.blocks.clear();
        game.activePigs.clear();
        game.stepPhysics(1.0f / 60.0f);
        float expectedMaxVy = (GRAVITY * 0.15f) * (1.0f / 60.0f) * 1.5f;
        if (game.birds[0].vel.y > expectedMaxVy || !game.birds[0].inZeppelin) {
            std::cerr << "[SELF-TEST] Steampunk Zeppelin buoyant lift failed! vy=" << game.birds[0].vel.y << "\n";
            return 45;
        }

        // Test partial damage to balloon
        game.damageBird(game.birds[0], 35.0f);
        if (game.birds[0].balloonHp != 35.0f || !game.birds[0].inZeppelin || game.birds[0].hp != 240.0f) {
            std::cerr << "[SELF-TEST] Zeppelin balloon failed to absorb partial damage!\n";
            return 46;
        }

        // Test popping the balloon -> gondola plummet, BALLOON_SCRAP debris, crushing block beneath
        int scoreBefore = game.score;
        game.particles.clear();
        game.damageBird(game.birds[0], 50.0f); // 35 absorbed by balloon (pops!), remaining 15 into condor hp (240 -> 225)
        if (game.birds[0].inZeppelin || !game.birds[0].balloonPopped || game.birds[0].balloonHp != 0.0f ||
            game.birds[0].vel.y < 300.0f || game.birds[0].mass < 4.5f || (game.score - scoreBefore) != 1500) {
            std::cerr << "[SELF-TEST] Zeppelin popping failed to trigger gondola plunge (vy="
                      << game.birds[0].vel.y << ", mass=" << game.birds[0].mass << ") or +1500 score!\n";
            return 47;
        }

        // Verify BALLOON_SCRAP particles spawned
        bool foundBalloonScrap = false;
        for (const auto& part : game.particles) {
            if (part.kind == ParticleKind::BALLOON_SCRAP) { foundBalloonScrap = true; break; }
        }
        if (!foundBalloonScrap) {
            std::cerr << "[SELF-TEST] Zeppelin puncture failed to spawn BALLOON_SCRAP particles!\n";
            return 48;
        }

        // Test plummeting gondola crushes fortress blocks beneath
        BlockBody roofBlock = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 800.0f, 375.0f, 80.0f, 20.0f);
        game.blocks = {roofBlock};
        float roofHpBefore = game.blocks[0].hp;
        game.stepPhysics(1.0f / 60.0f);
        if (game.blocks[0].hp >= roofHpBefore) {
            std::cerr << "[SELF-TEST] Plunging Condor gondola failed to crush fortress block beneath!\n";
            return 49;
        }
    }

    // Restore player's original save file if it existed, otherwise remove test save
    if (!existingSave.empty()) {
        std::ofstream out("save_progress.cfg");
        out << existingSave;
    } else {
        std::remove("save_progress.cfg");
    }

    game.assets.shutdown();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    std::cout << "[SELF-TEST] ALL CHECKS PASSED.\n";
    return 0;
}

// ============================================================================
// MAIN ENTRY POINT (Interactive Linux Desktop Loop + CLI Flags)
// ============================================================================
int main(int argc, char** argv) {
    bool selfTest = false;
    bool exportShots = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--self-test") selfTest = true;
        if (arg == "--export-screenshots") { selfTest = true; exportShots = true; }
    }
    if (selfTest) {
        return runSelfTestAndExportScreenshots(exportShots);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Failed to initialize video/audio backend: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Snout & Siege: Rise of the Swine (50-Level Campaign)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Failed to create window (ensure X11/Wayland display is active): "
                  << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer, WINDOW_W, WINDOW_H);

    SnoutAndSiegeGame game;
    game.appWindow = window;
    game.assets.loadAll(renderer);
    game.initLevels();
    game.loadProgress();
    game.loadLevel(0);
    game.screen = ScreenState::TITLE;

    bool running = true;
    uint32_t lastTicks = SDL_GetTicks();

    while (running) {
        uint32_t now = SDL_GetTicks();
        float frameTime = (now - lastTicks) / 1000.0f;
        lastTicks = now;
        if (frameTime > 0.10f) frameTime = 0.10f;
        if (frameTime < 0.0f) frameTime = 0.0f;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            bool isPlayingMode = (game.screen == ScreenState::PLAYING || game.screen == ScreenState::ENDLESS ||
                                 (game.screen == ScreenState::EDITOR && game.isTestPlaying));

            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_MOUSEMOTION) {
                game.mouseX = static_cast<float>(ev.motion.x);
                game.mouseY = static_cast<float>(ev.motion.y);
                if (isPlayingMode && game.isManualPanning) {
                    float dx = game.mouseX - game.lastPanMouseX;
                    game.camTarget.x = clampf(game.camTarget.x - dx, 0.0f, 420.0f);
                    game.lastPanMouseX = game.mouseX;
                }
                if (isPlayingMode && game.draggingSling) {
                    Vec2 mPos(game.mouseX + game.camPos.x, game.mouseY + game.camPos.y);
                    Vec2 pull = mPos - Vec2(SLING_X, SLING_Y);
                    if (pull.length() > MAX_PULL) pull = pull.normalized() * MAX_PULL;
                    game.slingPullPos = Vec2(SLING_X, SLING_Y) + pull;
                    if (!game.activePigs.empty() && !game.activePigs[0].launched) {
                        game.activePigs[0].pos = game.slingPullPos;
                        if (pull.lengthSq() > 36.0f) {
                            game.activePigs[0].angle = std::atan2(-pull.y, -pull.x);
                        }
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                float mx = static_cast<float>(ev.button.x);
                float my = static_cast<float>(ev.button.y);
                game.mouseX = mx;
                game.mouseY = my;

                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    if (isPlayingMode) {
                        game.isManualPanning = true;
                        game.manualPanTimer = 0.0f;
                        game.lastPanMouseX = mx;
                    }
                } else if (ev.button.button == SDL_BUTTON_LEFT) {
                    // First check clickable UI buttons on any screen or modal
                    bool clickedUI = false;
                    auto buttonsCopy = game.activeButtons;
                    for (const auto& btn : buttonsCopy) {
                        if (btn.contains(mx, my)) {
                            clickedUI = game.handleButtonClick(btn.id, running);
                            break;
                        }
                    }

                    // If in playing state and not clicking a UI button, handle slingshot or Pig ability
                    if (!clickedUI && isPlayingMode &&
                        !game.levelComplete && !game.levelFailed && !game.endlessGameOver) {
                        Vec2 worldMouse(mx + game.camPos.x, my + game.camPos.y);
                        if (!game.activePigs.empty() && !game.activePigs[0].launched) {
                            if ((worldMouse - game.activePigs[0].pos).length() < 58.0f) {
                                game.draggingSling = true;
                                game.isManualPanning = false;
                                game.manualPanTimer = 0.0f;
                                game.camTarget = {0.0f, 0.0f};
                                game.simSpeed = 1.0f;
                            }
                        } else if (!game.activePigs.empty() && game.activePigs[0].launched) {
                            game.activateMidAirAbility();
                        }
                    } else if (!clickedUI && game.screen == ScreenState::EDITOR && !game.isTestPlaying) {
                        // Canvas build interaction (clicks inside arena zone)
                        if (mx > 218.0f && my > 72.0f && my < GROUND_Y + 15.0f) {
                            float gx = game.editorSnapGrid ? (std::round(mx / 20.0f) * 20.0f) : mx;
                            float gy = game.editorSnapGrid ? (std::round(my / 20.0f) * 20.0f) : my;

                            if (game.editorTool == 0) { // BLOCK
                                game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(game.editorMat, gx, gy, game.editorBlockW, game.editorBlockH));
                                game.assets.playSound("sound.wood_impact", 0.55f);
                            } else if (game.editorTool == 1) { // BIRD
                                BirdTarget bt{};
                                bt.kind = game.editorBird;
                                bt.pos = {gx, gy};
                                bt.radius = (bt.kind == BirdClass::CONDOR ? 28.0f :
                                            (bt.kind == BirdClass::HERON ? 26.0f :
                                            (bt.kind == BirdClass::FALCON ? 23.0f :
                                            (bt.kind == BirdClass::OWL ? 23.0f : 20.0f))));
                                bt.hp = (bt.kind == BirdClass::CONDOR ? 240.0f :
                                        (bt.kind == BirdClass::HERON ? 210.0f :
                                        (bt.kind == BirdClass::FALCON ? 150.0f :
                                        (bt.kind == BirdClass::OWL ? 140.0f : 78.0f))));
                                bt.maxHp = bt.hp;
                                bt.hasHelmet = game.editorBirdHelmet;
                                bt.helmetHp = game.editorBirdHelmet ? 100.0f : 0.0f;
                                bt.maxHelmetHp = bt.helmetHp;
                                if (bt.kind == BirdClass::FALCON) {
                                    bt.hasShield = true;
                                    bt.shieldHp = 120.0f;
                                    bt.maxShieldHp = 120.0f;
                                }
                                if (bt.kind == BirdClass::CONDOR) {
                                    bt.inZeppelin = true;
                                    bt.balloonHp = 70.0f;
                                    bt.maxBalloonHp = 70.0f;
                                }
                                game.customLevel.birds.push_back(bt);
                                game.assets.playSound("sound.bird_caw", 0.6f);
                            } else if (game.editorTool == 2) { // PENDULUM
                                PendulumBody pb{};
                                pb.anchor = {gx, gy};
                                pb.length = 85.0f;
                                pb.angle = 0.40f;
                                pb.radius = 18.0f;
                                pb.mass = 8.0f;
                                pb.spiked = game.editorSpikedPendulum;
                                game.customLevel.pendulums.push_back(pb);
                                game.assets.playSound("sound.stone_impact", 0.6f);
                            } else if (game.editorTool == 3) { // TRUFFLE
                                game.customLevel.hasTruffle = true;
                                game.customLevel.trufflePos = {gx, gy};
                                game.assets.playSound("sound.truffle_collect", 0.6f);
                            } else if (game.editorTool == 4) { // ERASER
                                bool erased = false;
                                if (game.customLevel.hasTruffle && (game.customLevel.trufflePos - Vec2(mx, my)).length() < 32.0f) {
                                    game.customLevel.hasTruffle = false;
                                    erased = true;
                                }
                                if (!erased) {
                                    for (auto it = game.customLevel.birds.begin(); it != game.customLevel.birds.end(); ++it) {
                                        if ((it->pos - Vec2(mx, my)).length() < it->radius + 14.0f) {
                                            game.customLevel.birds.erase(it);
                                            erased = true;
                                            break;
                                        }
                                    }
                                }
                                if (!erased) {
                                    for (auto it = game.customLevel.pendulums.begin(); it != game.customLevel.pendulums.end(); ++it) {
                                        Vec2 bobPos = it->anchor + Vec2(std::sin(it->angle), std::cos(it->angle)) * it->length;
                                        if ((it->anchor - Vec2(mx, my)).length() < 18.0f || (bobPos - Vec2(mx, my)).length() < it->radius + 10.0f) {
                                            game.customLevel.pendulums.erase(it);
                                            erased = true;
                                            break;
                                        }
                                    }
                                }
                                if (!erased) {
                                    for (auto it = game.customLevel.blocks.begin(); it != game.customLevel.blocks.end(); ++it) {
                                        float hx = it->w * 0.5f + 8.0f, hy = it->h * 0.5f + 8.0f;
                                        if (std::abs(mx - it->pos.x) <= hx && std::abs(my - it->pos.y) <= hy) {
                                            game.customLevel.blocks.erase(it);
                                            erased = true;
                                            break;
                                        }
                                    }
                                }
                                if (erased) {
                                    game.assets.playSound("sound.wood_shatter", 0.5f);
                                }
                            }
                        }
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONUP) {
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    if (game.isManualPanning) {
                        game.manualPanTimer = 2.2f;
                    }
                } else if (ev.button.button == SDL_BUTTON_LEFT) {
                    if (isPlayingMode && game.draggingSling) {
                        game.draggingSling = false;
                        game.launchCurrentPig(game.slingPullPos - Vec2(SLING_X, SLING_Y));
                    }
                }
            } else if (ev.type == SDL_KEYDOWN) {
                int sym = ev.key.keysym.sym;

                if (sym == SDLK_F11 || sym == SDLK_f) {
                    game.toggleFullscreen();
                    continue;
                }

                if (sym == SDLK_c) {
                    game.enableSloMoCam = !game.enableSloMoCam;
                    game.floatScores.push_back({{WINDOW_W * 0.5f, 110.0f},
                                                game.enableSloMoCam ? "CINEMATIC SLO-MO CAM: ENABLED" : "CINEMATIC SLO-MO CAM: DISABLED",
                                                {255, 220, 100, 255}, 1.5f, 1.4f});
                    continue;
                }

                if (sym == SDLK_TAB && isPlayingMode) {
                    game.simSpeed = (game.simSpeed > 1.0f) ? 1.0f : 2.5f;
                    continue;
                }

                if (sym == SDLK_ESCAPE) {
                    if (game.screen == ScreenState::TITLE) running = false;
                    else if (game.screen == ScreenState::EDITOR && game.isTestPlaying) game.returnToEditor();
                    else game.screen = ScreenState::TITLE;
                } else if (sym == SDLK_m) {
                    game.assets.musicMuted = !game.assets.musicMuted;
                    game.saveProgress();
                } else if (sym == SDLK_s && game.screen != ScreenState::EDITOR) {
                    game.assets.sfxMuted = !game.assets.sfxMuted;
                    game.saveProgress();
                } else if (sym == SDLK_v) {
                    game.assets.useSpriteTextures = !game.assets.useSpriteTextures;
                } else if (sym == SDLK_F5) {
                    game.assets.loadAll(renderer);
                    game.floatScores.push_back({{WINDOW_W * 0.5f, 110.0f},
                                                "ASSETS HOT-RELOADED!", {140, 255, 190, 255}, 1.5f, 1.5f});
                } else if (isPlayingMode) {
                    if (sym == SDLK_r) {
                        if (game.screen == ScreenState::EDITOR) game.startEditorTestPlay();
                        else if (game.screen == ScreenState::ENDLESS) game.startEndlessMode();
                        else game.loadLevel(game.currentLevelIdx);
                    } else if (sym == SDLK_n && game.screen == ScreenState::PLAYING) {
                        game.loadLevel((game.currentLevelIdx + 1) % NUM_LEVELS);
                    } else if (sym == SDLK_p && game.screen == ScreenState::PLAYING) {
                        game.loadLevel((game.currentLevelIdx + NUM_LEVELS - 1) % NUM_LEVELS);
                    } else if (sym == SDLK_RIGHT || sym == SDLK_d) {
                        game.isManualPanning = true;
                        game.manualPanTimer = 3.0f;
                        game.camTarget.x = std::min(420.0f, game.camTarget.x + 140.0f);
                    } else if (sym == SDLK_LEFT || sym == SDLK_a) {
                        game.isManualPanning = true;
                        game.manualPanTimer = 3.0f;
                        game.camTarget.x = std::max(0.0f, game.camTarget.x - 140.0f);
                    } else if (sym == SDLK_SPACE && !game.levelComplete && !game.levelFailed && !game.endlessGameOver) {
                        game.activateMidAirAbility();
                    }
                } else if (game.screen == ScreenState::EDITOR && !game.isTestPlaying) {
                    if (sym == SDLK_s) game.saveCustomLevel(game.customSlot);
                    else if (sym == SDLK_l) game.loadCustomLevel(game.customSlot);
                    else if (sym == SDLK_t) game.startEditorTestPlay();
                    else if (sym == SDLK_g) game.editorSnapGrid = !game.editorSnapGrid;
                    else if (sym == SDLK_DELETE || sym == SDLK_BACKSPACE) {
                        game.customLevel.blocks.clear();
                        game.customLevel.birds.clear();
                        game.customLevel.pendulums.clear();
                        game.customLevel.hasTruffle = false;
                        game.floatScores.push_back({{WINDOW_W * 0.5f, 120.0f}, "FORTRESS CLEARED!", {255, 140, 140, 255}, 1.4f, 1.4f});
                    }
                } else if (game.screen == ScreenState::LEVEL_SELECT) {
                    if (sym == SDLK_LEFT || sym == SDLK_a) {
                        if (game.levelSelectPage > 0) game.levelSelectPage--;
                    } else if (sym == SDLK_RIGHT || sym == SDLK_d) {
                        if (game.levelSelectPage < 6) game.levelSelectPage++;
                    }
                }
            }
        }

        game.updateGame(frameTime);
        game.render(renderer);
        SDL_RenderPresent(renderer);
        uint32_t frameElapsed = SDL_GetTicks() - now;
        if (frameElapsed < 15) {
            SDL_Delay(15 - frameElapsed);
        }
    }

    game.assets.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
// Minimal SDL2 ABI declarations plus a dlopen()-based runtime loader.
// The game never links against SDL2: at startup it dlopen()s libSDL2 and,
// if that is missing, falls back to raw X11 with a built-in software renderer.
#pragma once

#include <cstdint>

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
// SDL2 API SURFACE (dispatches to libSDL2 when present, else the fallback)
// ============================================================================
int SDL_Init(uint32_t flags);

void SDL_Quit(void);

const char* SDL_GetError(void);

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, uint32_t flags);

void SDL_DestroyWindow(SDL_Window* window);

int SDL_SetWindowFullscreen(SDL_Window* window, uint32_t flags);

int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h);

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, uint32_t flags);

void SDL_DestroyRenderer(SDL_Renderer* renderer);

int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode);

int SDL_SetRenderDrawColor(SDL_Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

int SDL_RenderClear(SDL_Renderer* renderer);

void SDL_RenderPresent(SDL_Renderer* renderer);

int SDL_RenderFillRectF(SDL_Renderer* renderer, const SDL_FRect* rect);

int SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture,
                       const SDL_Vertex* vertices, int num_vertices,
                       const int* indices, int num_indices);

int SDL_RenderCopyExF(SDL_Renderer* renderer, SDL_Texture* texture,
                      const SDL_Rect* srcrect, const SDL_FRect* dstrect,
                      const double angle, const SDL_FPoint* center, const int flip);

int SDL_RenderCopyF(SDL_Renderer* renderer, SDL_Texture* texture,
                    const SDL_Rect* srcrect, const SDL_FRect* dstrect);

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(uint32_t flags, int width, int height,
                                            int depth, uint32_t format);

void SDL_FreeSurface(SDL_Surface* surface);

SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);

void SDL_DestroyTexture(SDL_Texture* texture);

int SDL_SetTextureBlendMode(SDL_Texture* texture, int blendMode);

// 0 = nearest, 1 = linear. Needs SDL >= 2.0.12; a no-op (nearest) otherwise.
int SDL_SetTextureScaleMode(SDL_Texture* texture, int scaleMode);

SDL_RWops* SDL_RWFromFile(const char* file, const char* mode);

int SDL_SaveBMP_RW(SDL_Surface* surface, SDL_RWops* dst, int freedst);

SDL_Surface* SDL_LoadBMP_RW(SDL_RWops* src, int freesrc);

int SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect,
                         uint32_t format, void* pixels, int pitch);

SDL_AudioSpec* SDL_LoadWAV_RW(SDL_RWops* src, int freesrc, SDL_AudioSpec* spec,
                              uint8_t** audio_buf, uint32_t* audio_len);

void SDL_FreeWAV(uint8_t* audio_buf);

uint32_t SDL_OpenAudioDevice(const char* device, int iscapture,
                             const SDL_AudioSpec* desired, SDL_AudioSpec* obtained,
                             int allowed_changes);

void SDL_CloseAudioDevice(uint32_t dev);

void SDL_PauseAudioDevice(uint32_t dev, int pause_on);

void SDL_LockAudioDevice(uint32_t dev);

void SDL_UnlockAudioDevice(uint32_t dev);

int SDL_PollEvent(SDL_Event* event);

uint32_t SDL_GetTicks(void);

void SDL_Delay(uint32_t ms);

// True when libSDL2 was found and loaded (false = X11/software fallback).
bool sdlRuntimeLoaded();

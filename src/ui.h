// UI kit: rounded panels, chunky cartoon buttons, ribbons and vector icons.
// Everything is drawn with SDL_RenderGeometry so it works on both the SDL2
// renderer and the software fallback.
#pragma once

#include "common.h"
#include "text.h"

enum class UiIcon {
    NONE, PLAY, RETRY, MENU, LEVELS, BACK, NEXT, MUSIC, SOUND, FAST, FULLSCREEN,
    QUIT, LOCK, STAR, TROPHY, HAMMER, ENDLESS, TRUFFLE, CHECK, PAUSE
};

namespace ui {

// Palette shared by every screen.
namespace pal {
constexpr SDL_Color INK{24, 22, 40, 255};          // outlines and text strokes
constexpr SDL_Color PANEL{34, 38, 66, 236};
constexpr SDL_Color PANEL_HI{52, 58, 96, 236};
constexpr SDL_Color PANEL_EDGE{255, 255, 255, 34};
constexpr SDL_Color CREAM{255, 248, 230, 255};
constexpr SDL_Color MUTED{176, 184, 214, 255};
constexpr SDL_Color GOLD{255, 204, 64, 255};
constexpr SDL_Color GOLD_DARK{196, 120, 24, 255};
constexpr SDL_Color GREEN{86, 186, 74, 255};
constexpr SDL_Color BLUE{66, 142, 228, 255};
constexpr SDL_Color ORANGE{244, 150, 46, 255};
constexpr SDL_Color RED{226, 78, 66, 255};
constexpr SDL_Color PURPLE{150, 96, 214, 255};
constexpr SDL_Color TEAL{40, 176, 170, 255};
constexpr SDL_Color SLATE{78, 92, 130, 255};
}  // namespace pal

SDL_Color shade(SDL_Color c, float f);                 // scale rgb (f < 1 darker)
SDL_Color tint(SDL_Color c, float t);                  // blend towards white
SDL_Color mix(SDL_Color a, SDL_Color b, float t);
SDL_Color alpha(SDL_Color c, float a);                 // multiply alpha

void fillRoundRect(SDL_Renderer* ren, SDL_FRect r, float rad, SDL_Color c);
void fillRoundRectGrad(SDL_Renderer* ren, SDL_FRect r, float rad, SDL_Color top, SDL_Color bottom);
void strokeRoundRect(SDL_Renderer* ren, SDL_FRect r, float rad, float thick, SDL_Color c);
void softShadow(SDL_Renderer* ren, SDL_FRect r, float rad, float spread, float strength);
void fillCircle(SDL_Renderer* ren, Vec2 c, float rad, SDL_Color col, int segs = 28);
void fillCircleGrad(SDL_Renderer* ren, Vec2 c, float rad, SDL_Color inner, SDL_Color outer, int segs = 32);
void ring(SDL_Renderer* ren, Vec2 c, float rad, float thick, SDL_Color col, int segs = 32);
void fillRectGrad(SDL_Renderer* ren, SDL_FRect r, SDL_Color top, SDL_Color bottom);

// Dark translucent card with a soft shadow and a faint top highlight.
void panel(SDL_Renderer* ren, SDL_FRect r, float rad = 18.0f, SDL_Color body = pal::PANEL);

// Chunky button face: rounded body, darker lip underneath, glossy top.
// Returns the rect the label should be centred in (shifted when hovered).
SDL_FRect buttonFace(SDL_Renderer* ren, SDL_FRect r, SDL_Color base, bool hovered, bool selected = false,
                     float rad = -1.0f);

// Folded-end banner with centred display text.
void ribbon(SDL_Renderer* ren, float cx, float cy, float w, float h, SDL_Color base, const std::string& text,
            float textSize);

// Rounded "pill" chip with an optional icon and text; returns its width.
float pill(SDL_Renderer* ren, float x, float y, float h, UiIcon icon, SDL_Color iconCol, const std::string& text,
           SDL_Color bg = {16, 18, 34, 170});
float pillWidth(float h, UiIcon icon, const std::string& text);

void icon(SDL_Renderer* ren, UiIcon ic, Vec2 c, float size, SDL_Color col);
void star(SDL_Renderer* ren, Vec2 c, float rOuter, bool earned, float rot = 0.0f);

// Outlined display text in one call (the cartoon title look).
float title(SDL_Renderer* ren, const std::string& s, float x, float y, float size, SDL_Color fill,
            TextAlign align = TextAlign::CENTER, SDL_Color outline = pal::INK);

}  // namespace ui

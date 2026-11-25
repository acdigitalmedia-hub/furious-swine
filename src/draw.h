// Bitmap font, geometric drawing primitives and origami character models.
#pragma once

#include "common.h"

std::string formatNumber(int n);

void drawText(SDL_Renderer* ren, const std::string& text, float x, float y,
              float scale, SDL_Color col, bool center = false, bool shadow = true);


void drawTriangle(SDL_Renderer* ren, Vec2 a, Vec2 b, Vec2 c, SDL_Color col);

void drawConvexPoly(SDL_Renderer* ren, const std::vector<Vec2>& pts, SDL_Color col);

void drawThickLine(SDL_Renderer* ren, Vec2 a, Vec2 b, float thickness, SDL_Color col);

void drawNGon(SDL_Renderer* ren, Vec2 center, float radius, int sides,
              float rotRad, SDL_Color col);

void drawStarIcon(SDL_Renderer* ren, Vec2 center, float outerR, float innerR,
                  SDL_Color fillCol, SDL_Color outlineCol);

// ============================================================================
// HIGH-RESOLUTION MULTI-FACETED ORIGAMI CHARACTER MODELS (PIGS & TOTEM BIRDS)
// ============================================================================
void renderOrigamiPigGeometry(SDL_Renderer* ren, PigType type, Vec2 pos,
                              float r, float angle);

void renderTotemBirdGeometry(SDL_Renderer* ren, BirdClass kind, Vec2 pos,
                             float r, float angle, float hpRatio,
                             bool hasHelmet = false, float helmetHpRatio = 1.0f,
                             bool hasShield = false, float shieldHpRatio = 1.0f,
                             bool inZeppelin = false, float balloonHpRatio = 1.0f);

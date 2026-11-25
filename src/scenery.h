// Chapter backdrops: sky, celestial body, clouds, two parallax landscape
// layers, ground and ambient weather, one look per chapter (14 = vaults).
#pragma once

#include "common.h"

constexpr int NUM_SCENERY_THEMES = 15;

void renderScenery(SDL_Renderer* ren, int theme, Vec2 cam, float timeSec);

#include "game.h"

int SnoutAndSiegeGame::getActiveChapter() const {
    if (screen == ScreenState::EDITOR) return clampf(editorTheme, 0, 5);
    if (screen == ScreenState::ENDLESS) return (endlessWave - 1) % 6;
    if (currentLevelIdx >= NUM_CAMPAIGN_LEVELS) return 14;
    return clampf(currentLevelIdx / 10, 0, 13);
}

void SnoutAndSiegeGame::renderBackgroundBackdrop(SDL_Renderer* ren, Vec2 cam) {
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

void SnoutAndSiegeGame::renderSiegeCatapultBase(SDL_Renderer* ren, float sx, float sy, float groundY) {
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

void SnoutAndSiegeGame::render(SDL_Renderer* ren) {
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

void SnoutAndSiegeGame::renderBlock(SDL_Renderer* ren, const BlockBody& b, Vec2 cam) {
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

void SnoutAndSiegeGame::renderPendulum(SDL_Renderer* ren, const PendulumBody& pend, Vec2 cam) {
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

void SnoutAndSiegeGame::renderPig(SDL_Renderer* ren, const PigBody& p, Vec2 cam) {
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

void SnoutAndSiegeGame::renderBird(SDL_Renderer* ren, const BirdTarget& t, Vec2 cam) {
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

void SnoutAndSiegeGame::renderGoldenTruffle(SDL_Renderer* ren, Vec2 cam) {
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

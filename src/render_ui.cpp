#include "game.h"

void drawButton(SDL_Renderer* ren, const UIButton& btn, float mx, float my) {
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

void SnoutAndSiegeGame::renderTitleScreen(SDL_Renderer* ren) {
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

void SnoutAndSiegeGame::renderLevelSelectScreen(SDL_Renderer* ren) {
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

            // Milestone indicators on each new pig's debut level (6, 11, 21, 31, 41, 45)
            if (idx == 5) {
                drawText(ren, "* Bramble Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {255, 210, 80, 255}, false);
            } else if (idx == 10) {
                drawText(ren, "* Clover Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {140, 255, 200, 255}, false);
            } else if (idx == 20) {
                drawText(ren, "* Vulcan Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {255, 150, 110, 255}, false);
            } else if (idx == 30) {
                drawText(ren, "* Gale Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {140, 220, 255, 255}, false);
            } else if (idx == 40) {
                drawText(ren, "* Atticus Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {210, 140, 255, 255}, false);
            } else if (idx == 44) {
                drawText(ren, "* Rusty Unlocked", cx + 12.0f, cy + 80.0f, 1.20f, {255, 195, 75, 255}, false);
            } else {
                drawText(ren, "CLICK TO PLAY >>", cx + 12.0f, cy + 80.0f, 1.25f, {245, 206, 118, 255}, false);
            }
        }
    }
}

void SnoutAndSiegeGame::renderAchievementToast(SDL_Renderer* ren) {
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

void SnoutAndSiegeGame::renderAchievementsScreen(SDL_Renderer* ren) {
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

void SnoutAndSiegeGame::renderEditorScreen(SDL_Renderer* ren) {
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

void SnoutAndSiegeGame::renderHUD(SDL_Renderer* ren) {
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
            struct PigMilestone {
                int clearedIdx;
                const char* title;
                const char* hint;
                SDL_Color bg, border, titleCol, hintCol;
            };
            static const PigMilestone kMilestones[] = {
                {4, "** MILESTONE UNLOCKED: BRAMBLE (TURBO PIG) AVAILABLE! **",
                 "Tap SPACE in mid-air to trigger Supersonic Dash ability!",
                 {48, 62, 38, 240}, {255, 215, 80, 255}, {255, 235, 110, 255}, {220, 240, 220, 255}},
                {9, "** MILESTONE UNLOCKED: CLOVER (SPLIT PIG) AVAILABLE! **",
                 "Tap SPACE in mid-air to split into 3 aerodynamic swine!",
                 {38, 62, 58, 240}, {120, 245, 195, 255}, {150, 255, 210, 255}, {220, 245, 235, 255}},
                {19, "** MILESTONE UNLOCKED: VULCAN (BOMB PIG) AVAILABLE! **",
                 "Tap SPACE or impact to detonate a devastating seismic shockwave!",
                 {68, 38, 34, 240}, {255, 140, 90, 255}, {255, 180, 120, 255}, {255, 220, 200, 255}},
                {29, "** MILESTONE UNLOCKED: GALE (GLIDER PIG) AVAILABLE! **",
                 "Tap SPACE in mid-air to level out and drop twin iron bombs!",
                 {30, 50, 70, 240}, {110, 190, 255, 255}, {150, 220, 255, 255}, {210, 235, 255, 255}},
                {39, "** MILESTONE UNLOCKED: ATTICUS (GRAVITON PIG) AVAILABLE! **",
                 "Tap SPACE to open a vortex that drags blocks in, then implodes!",
                 {52, 34, 70, 240}, {185, 120, 255, 255}, {210, 150, 255, 255}, {230, 215, 250, 255}},
                {43, "** MILESTONE UNLOCKED: RUSTY (DRILL PIG) AVAILABLE! **",
                 "Tap SPACE to bore through blocks. Stone wears the drill out fast!",
                 {66, 46, 26, 240}, {240, 165, 60, 255}, {255, 200, 90, 255}, {250, 230, 200, 255}},
            };
            const PigMilestone* milestone = nullptr;
            for (const auto& m : kMilestones) {
                if (m.clearedIdx == currentLevelIdx) milestone = &m;
            }

            if (milestone) {
                SDL_FRect mBox{card.x + 30.0f, 342.0f, card.w - 60.0f, 48.0f};
                SDL_SetRenderDrawColor(ren, milestone->bg.r, milestone->bg.g, milestone->bg.b, milestone->bg.a);
                SDL_RenderFillRectF(ren, &mBox);
                drawThickLine(ren, {mBox.x, mBox.y}, {mBox.x + mBox.w, mBox.y}, 2.0f, milestone->border);
                drawThickLine(ren, {mBox.x, mBox.y + mBox.h}, {mBox.x + mBox.w, mBox.y + mBox.h}, 2.0f, milestone->border);
                drawText(ren, milestone->title, WINDOW_W * 0.5f, 350.0f, 1.55f, milestone->titleCol, true);
                drawText(ren, milestone->hint, WINDOW_W * 0.5f, 370.0f, 1.30f, milestone->hintCol, true);
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
            if (currentLevelIdx + 1 < NUM_LEVELS && isLevelUnlocked(currentLevelIdx + 1)) {
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

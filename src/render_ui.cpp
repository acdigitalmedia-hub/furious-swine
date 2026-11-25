#include "game.h"

namespace {

using namespace ui;

// "LEVEL 12: TWIN TIMBER WATCHTOWER" -> "Twin Timber Watchtower"
std::string displayLevelName(const std::string& full) {
    std::string s = full;
    auto colon = s.find(':');
    if (colon != std::string::npos) s = s.substr(colon + 1);
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    bool start = true;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (std::isalpha(c)) {
            s[i] = static_cast<char>(start ? std::toupper(c) : std::tolower(c));
            start = false;
        } else {
            start = (c == ' ' || c == '-' || c == '(' || c == '/');
        }
    }
    for (const char* keep : {"Tnt", "Ii", "Iii"}) {
        size_t p = 0;
        std::string k = keep;
        while ((p = s.find(k, p)) != std::string::npos) {
            bool leftOk = (p == 0 || s[p - 1] == ' ');
            bool rightOk = (p + k.size() == s.size() || s[p + k.size()] == ' ');
            if (leftOk && rightOk) {
                for (size_t j = 0; j < k.size(); ++j) s[p + j] = static_cast<char>(std::toupper(s[p + j]));
            }
            p += k.size();
        }
    }
    return s;
}

// One accent colour per chapter (index 14 = vaults).
SDL_Color chapterAccent(int ch) {
    static const SDL_Color k[] = {
        {86, 186, 74, 255},  {222, 120, 52, 255}, {214, 92, 70, 255},  {84, 170, 226, 255},
        {170, 70, 92, 255},  {150, 178, 60, 255}, {44, 160, 140, 255}, {92, 150, 232, 255},
        {110, 196, 230, 255}, {232, 96, 48, 255}, {120, 104, 210, 255}, {236, 168, 48, 255},
        {200, 120, 70, 255}, {92, 104, 220, 255}, {176, 104, 220, 255}};
    return k[std::clamp(ch, 0, 14)];
}

int totalTruffles(const std::vector<LevelSpec>& levels) {
    int n = 0;
    for (const auto& L : levels) n += L.hasTruffle ? 1 : 0;
    return n;
}

void centredLabel(SDL_Renderer* ren, const std::string& s, SDL_FRect box, float size, SDL_Color col,
                  SDL_Color shadow, FontFace face = FontFace::BOLD) {
    TextStyle st = textStyle(face, size, col, TextAlign::CENTER);
    st.shadow = shadow;
    st.shadowDy = std::max(1.5f, size * 0.08f);
    float y = box.y + box.h * 0.5f - textCapTop(face, size) - textCapHeight(face, size) * 0.5f;
    drawTextStyled(ren, s, box.x + box.w * 0.5f, y, st);
}

}  // namespace

void drawButton(SDL_Renderer* ren, const UIButton& btn, float mx, float my) {
    bool hovered = btn.contains(mx, my);
    SDL_Color base = btn.kitColor ? btn.bgCol : shade(btn.bgCol, 1.6f);
    base.a = 255;
    SDL_FRect face = buttonFace(ren, btn.rect, base, hovered, btn.selected);
    SDL_Color shadow = shade(base, 0.32f);
    shadow.a = 200;

    float size = std::min(face.h * 0.48f, 24.0f);
    bool hasIcon = btn.icon != UiIcon::NONE;
    float iconSize = std::min(face.h * 0.62f, 30.0f);
    float gap = (hasIcon && !btn.label.empty()) ? iconSize * 0.35f : 0.0f;
    float avail = face.w - 18.0f - (hasIcon ? iconSize + gap : 0.0f);
    if (!btn.label.empty()) {
        float w = textWidth(btn.label, FontFace::BOLD, size);
        if (w > avail && avail > 10.0f) size *= avail / w;
    }
    float labelW = btn.label.empty() ? 0.0f : textWidth(btn.label, FontFace::BOLD, size);
    float total = (hasIcon ? iconSize + gap : 0.0f) + labelW;
    float x = face.x + (face.w - total) * 0.5f;
    float cy = face.y + face.h * 0.5f;
    if (hasIcon) {
        Vec2 ic(x + iconSize * 0.5f, cy);
        icon(ren, btn.icon, ic + Vec2(0, 1.5f), iconSize, shadow);
        icon(ren, btn.icon, ic, iconSize, pal::CREAM);
        if (btn.off) {
            drawThickLine(ren, ic + Vec2(-iconSize * 0.45f, iconSize * 0.45f), ic + Vec2(iconSize * 0.45f, -iconSize * 0.45f),
                          std::max(4.0f, iconSize * 0.2f), shade(base, 0.3f));
            drawThickLine(ren, ic + Vec2(-iconSize * 0.45f, iconSize * 0.45f), ic + Vec2(iconSize * 0.45f, -iconSize * 0.45f),
                          std::max(2.0f, iconSize * 0.1f), {255, 120, 110, 255});
        }
        x += iconSize + gap;
    }
    if (!btn.label.empty()) {
        TextStyle st = textStyle(FontFace::BOLD, size, pal::CREAM);
        st.shadow = shadow;
        st.shadowDy = std::max(1.5f, size * 0.08f);
        drawTextStyled(ren, btn.label, x, cy - textCapTop(st.face, size) - textCapHeight(st.face, size) * 0.5f, st);
    }
    if (hovered && !btn.tooltip.empty()) {
        float tw = textWidth(btn.tooltip, FontFace::BOLD, 15.0f) + 20.0f;
        float tx = std::clamp(btn.rect.x + btn.rect.w * 0.5f - tw * 0.5f, 6.0f, WINDOW_W - tw - 6.0f);
        SDL_FRect tip{tx, btn.rect.y + btn.rect.h + 8.0f, tw, 28.0f};
        fillRoundRect(ren, tip, 8.0f, {16, 14, 30, 230});
        centredLabel(ren, btn.tooltip, tip, 15.0f, pal::CREAM, {0, 0, 0, 0});
    }
}

void SnoutAndSiegeGame::uiButton(SDL_Renderer* ren, const std::string& id, SDL_FRect r, const std::string& label,
                                 SDL_Color base, UiIcon ic, bool selected, bool off, const std::string& tooltip) {
    UIButton b;
    b.id = id;
    b.rect = r;
    b.label = label;
    b.bgCol = base;
    b.kitColor = true;
    b.icon = ic;
    b.selected = selected;
    b.off = off;
    b.tooltip = tooltip;
    activeButtons.push_back(b);
    drawButton(ren, b, mouseX, mouseY);
}

void SnoutAndSiegeGame::renderTitleScreen(SDL_Renderer* ren) {
    float t = SDL_GetTicks() * 0.001f;

    // Cast of characters on the meadow.
    renderSiegeCatapultBase(ren, 180.0f, SLING_Y);
    renderOrigamiPigGeometry(ren, PigType::TROTTER,  {180, SLING_Y - 4.0f + std::sin(t * 2.0f) * 2.0f}, 25.0f, -0.12f);
    drawCatapultFrontArm(ren, 180.0f, SLING_Y);
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

    // Soft darkening behind the menu so it reads on every backdrop.
    fillRectGrad(ren, {0, 0, WINDOW_W, 330}, {10, 8, 24, 120}, {10, 8, 24, 0});

    // Logo.
    float bob = std::sin(t * 1.6f) * 3.0f;
    title(ren, "Snout & Siege", WINDOW_W * 0.5f, 34.0f + bob, 96.0f, pal::GOLD);
    ribbon(ren, WINDOW_W * 0.5f, 176.0f, 330.0f, 46.0f, pal::RED, "Rise of the Swine", 28.0f);

    // Main actions.
    float cx = WINDOW_W * 0.5f;
    int nextLvl = getHighestUnlockedLevel();
    std::string playLabel = (getLevelsCleared() > 0) ? "Continue  ·  Level " + std::to_string(nextLvl + 1)
                                                     : "Start Campaign";
    uiButton(ren, "START_CAMPAIGN", {cx - 200, 236, 400, 74}, playLabel, pal::GREEN, UiIcon::PLAY);

    float bw = 195.0f, bh = 60.0f, gap = 10.0f;
    uiButton(ren, "OPEN_LEVEL_SELECT", {cx - bw - gap * 0.5f, 324, bw, bh}, "Levels", pal::BLUE, UiIcon::LEVELS);
    uiButton(ren, "OPEN_ACHIEVEMENTS", {cx + gap * 0.5f, 324, bw, bh}, "Trophies", pal::PURPLE, UiIcon::TROPHY);
    uiButton(ren, "START_ENDLESS", {cx - bw - gap * 0.5f, 394, bw, bh}, "Endless", pal::ORANGE, UiIcon::ENDLESS);
    uiButton(ren, "OPEN_EDITOR", {cx + gap * 0.5f, 394, bw, bh}, "Architect", pal::TEAL, UiIcon::HAMMER);

    // Progress chips.
    std::string chips[4] = {
        std::to_string(getTotalStars()) + " / " + std::to_string(NUM_LEVELS * 3),
        std::to_string(getTrufflesFoundCount()) + " / " + std::to_string(totalTruffles(levels)),
        std::to_string(getAchievementsCount()) + " / " + std::to_string(achievementDefs.size()),
        "Wave " + std::to_string(endlessBestWave)};
    UiIcon chipIcons[4] = {UiIcon::STAR, UiIcon::TRUFFLE, UiIcon::TROPHY, UiIcon::ENDLESS};
    SDL_Color chipCols[4] = {pal::GOLD, {214, 160, 72, 255}, {222, 180, 255, 255}, {255, 170, 90, 255}};
    float ph = 34.0f, total = 0.0f;
    for (int i = 0; i < 4; ++i) total += pillWidth(ph, chipIcons[i], chips[i]) + (i ? 10.0f : 0.0f);
    float px = cx - total * 0.5f;
    for (int i = 0; i < 4; ++i) px += pill(ren, px, 476, ph, chipIcons[i], chipCols[i], chips[i]) + 10.0f;

    TextStyle hint = textStyle(FontFace::BOLD, 15.0f, {236, 236, 250, 220}, TextAlign::CENTER);
    hint.shadow = {8, 6, 20, 160};
    drawTextStyled(ren, "Drag to aim  ·  Space: special ability  ·  Tab: fast-forward  ·  Right-drag: look around",
                   cx, 528.0f, hint);

    // Settings, top-right.
    float s = 50.0f, x = WINDOW_W - 20.0f - s;
    uiButton(ren, "QUIT_GAME", {x, 18, s, s}, "", pal::RED, UiIcon::QUIT, false, false, "Quit");
    x -= s + 10.0f;
    uiButton(ren, "TOGGLE_FULLSCREEN", {x, 18, s, s}, "", pal::SLATE, UiIcon::FULLSCREEN, isFullscreen, false,
             "Fullscreen (F11)");
    x -= s + 10.0f;
    uiButton(ren, "TOGGLE_SFX", {x, 18, s, s}, "", pal::SLATE, UiIcon::SOUND, false, assets.sfxMuted, "Sound effects (S)");
    x -= s + 10.0f;
    uiButton(ren, "TOGGLE_MUSIC", {x, 18, s, s}, "", pal::SLATE, UiIcon::MUSIC, false, assets.musicMuted, "Music (M)");
}

void SnoutAndSiegeGame::renderLevelSelectScreen(SDL_Renderer* ren) {
    float t = SDL_GetTicks() * 0.001f;
    bool vaultPage = (levelSelectPage >= LEVEL_SELECT_VAULT_PAGE);
    int chapter = vaultPage ? NUM_CHAPTERS : levelSelectPage;
    SDL_Color accent = chapterAccent(chapter);

    fillRectGrad(ren, {0, 0, WINDOW_W, 140}, {10, 8, 24, 170}, {10, 8, 24, 0});

    uiButton(ren, "BACK_TITLE", {24, 22, 124, 54}, "Menu", pal::SLATE, UiIcon::BACK);

    std::string kicker = vaultPage ? "Bonus levels" : "Chapter " + std::to_string(chapter + 1) + " of " +
                                                          std::to_string(NUM_CHAPTERS);
    TextStyle ks = textStyle(FontFace::BOLD, 18.0f, tint(accent, 0.45f), TextAlign::CENTER);
    ks.shadow = {8, 6, 20, 180};
    drawTextStyled(ren, kicker, WINDOW_W * 0.5f, 18.0f, ks);
    title(ren, chapterName(chapter), WINDOW_W * 0.5f, 40.0f, 46.0f, pal::CREAM);

    // Star / truffle totals, top-right.
    std::string starStr = std::to_string(getTotalStars()) + " / " + std::to_string(NUM_LEVELS * 3);
    std::string trufStr = std::to_string(getTrufflesFoundCount()) + " / " + std::to_string(totalTruffles(levels));
    float ph = 36.0f;
    float w2 = pillWidth(ph, UiIcon::TRUFFLE, trufStr);
    float w1 = pillWidth(ph, UiIcon::STAR, starStr);
    pill(ren, WINDOW_W - 24 - w2, 31, ph, UiIcon::TRUFFLE, {214, 160, 72, 255}, trufStr);
    pill(ren, WINDOW_W - 24 - w2 - 10 - w1, 31, ph, UiIcon::STAR, pal::GOLD, starStr);

    // Page arrows.
    if (levelSelectPage > 0) {
        uiButton(ren, "PAGE_PREV", {24, 316, 58, 72}, "", pal::SLATE, UiIcon::BACK, false, false, "Previous chapter");
    }
    if (levelSelectPage < LEVEL_SELECT_VAULT_PAGE) {
        uiButton(ren, "PAGE_NEXT", {WINDOW_W - 82, 316, 58, 72}, "", pal::SLATE, UiIcon::NEXT, false, false,
                 vaultPage ? "" : (levelSelectPage + 1 == LEVEL_SELECT_VAULT_PAGE ? "Secret vaults" : "Next chapter"));
    }

    // Level tiles: 5 x 2.
    const float tileW = 186.0f, tileH = 196.0f, gapX = 24.0f, gapY = 24.0f;
    const float startX = (WINDOW_W - (5 * tileW + 4 * gapX)) * 0.5f;
    const float startY = 134.0f;
    int nextPlayable = getHighestUnlockedLevel();
    int base = vaultPage ? NUM_CAMPAIGN_LEVELS : chapter * LEVELS_PER_CHAPTER;

    for (int i = 0; i < 10; ++i) {
        int idx = base + i;
        if (idx >= NUM_LEVELS) break;
        float x = startX + (i % 5) * (tileW + gapX);
        float y = startY + (i / 5) * (tileH + gapY);
        SDL_FRect r{x, y, tileW, tileH};
        bool unlocked = isLevelUnlocked(idx);
        bool isNext = (idx == nextPlayable) && unlocked && bestStars[idx] == 0;
        bool hovered = r.x <= mouseX && mouseX <= r.x + r.w && r.y <= mouseY && mouseY <= r.y + r.h;

        UIButton hit;
        hit.id = unlocked ? ("PLAY_LEVEL_" + std::to_string(idx)) : (vaultPage ? "LOCKED_VAULT" : "LOCKED_CARD");
        hit.rect = r;
        activeButtons.push_back(hit);

        std::string number = vaultPage ? "V" + std::to_string(i + 1) : std::to_string(idx + 1);
        if (!unlocked) {
            SDL_FRect face = buttonFace(ren, r, {58, 62, 92, 255}, false, false, 22.0f);
            icon(ren, UiIcon::LOCK, {face.x + face.w * 0.5f, face.y + 62}, 46.0f, {150, 156, 196, 255});
            TextStyle ns = textStyle(FontFace::DISPLAY, 30.0f, {150, 156, 196, 255}, TextAlign::CENTER);
            drawTextStyled(ren, number, face.x + face.w * 0.5f, face.y + 104, ns);
            std::string why = vaultPage ? "Needs " + std::to_string(getVaultTruffleReq(i)) + " truffles"
                                        : "Beat level " + std::to_string(idx);
            TextStyle ws = textStyle(FontFace::BOLD, 15.0f, {150, 156, 196, 255}, TextAlign::CENTER);
            drawTextStyled(ren, why, face.x + face.w * 0.5f, face.y + 146, ws);
            continue;
        }

        SDL_FRect face = buttonFace(ren, r, accent, hovered, isNext, 22.0f);
        if (isNext) {
            float pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
            strokeRoundRect(ren, {r.x - 8, r.y - 8, r.w + 16, r.h + 16}, 28.0f, 2.0f,
                            {255, 230, 120, static_cast<uint8_t>(90 + 120 * pulse)});
        }
        float cxT = face.x + face.w * 0.5f;
        title(ren, number, cxT, face.y + 12, 56.0f, pal::CREAM, TextAlign::CENTER, shade(accent, 0.3f));

        // Name (up to two lines).
        TextStyle nameSt = textStyle(FontFace::BOLD, 15.0f, pal::CREAM, TextAlign::CENTER);
        nameSt.shadow = shade(accent, 0.3f);
        nameSt.shadowDy = 1.5f;
        drawTextWrapped(ren, displayLevelName(levels[idx].name), cxT, face.y + 84, face.w - 24, nameSt, 1.0f);

        // Stars along the bottom.
        for (int s = 0; s < 3; ++s) {
            Vec2 sc(cxT + (s - 1) * 38.0f, face.y + face.h - 30.0f + (s == 1 ? -5.0f : 0.0f));
            star(ren, sc, s == 1 ? 16.0f : 14.0f, s < bestStars[idx]);
        }
        if (levels[idx].hasTruffle) {
            bool found = trufflesFound[idx];
            Vec2 tc(face.x + face.w - 22, face.y + 22);
            fillCircle(ren, tc, 15.0f, found ? SDL_Color{255, 214, 90, 255} : SDL_Color{20, 18, 36, 140}, 20);
            icon(ren, UiIcon::TRUFFLE, tc, 20.0f, found ? SDL_Color{150, 92, 30, 255} : SDL_Color{230, 210, 170, 200});
        }
    }

    // Chapter dots.
    int pages = LEVEL_SELECT_VAULT_PAGE + 1;
    float dotGap = 30.0f;
    float dx = WINDOW_W * 0.5f - (pages - 1) * dotGap * 0.5f;
    float dy = 668.0f;
    for (int p = 0; p < pages; ++p) {
        Vec2 c(dx + p * dotGap, dy);
        bool cur = (p == levelSelectPage);
        bool hov = std::abs(mouseX - c.x) < 13 && std::abs(mouseY - c.y) < 13;
        SDL_Color col = (p == LEVEL_SELECT_VAULT_PAGE) ? chapterAccent(14) : chapterAccent(p);
        fillCircle(ren, c + Vec2(0, 2), cur ? 11.0f : 8.0f, {8, 6, 20, 120}, 18);
        fillCircle(ren, c, cur ? 11.0f : 8.0f, cur ? col : (hov ? tint(col, 0.2f) : shade(col, 0.55f)), 18);
        if (cur) ring(ren, c, 13.0f, 2.5f, pal::CREAM, 24);
        UIButton dot;
        dot.id = "GOTO_PAGE_" + std::to_string(p);
        dot.rect = {c.x - 13, c.y - 13, 26, 26};
        activeButtons.push_back(dot);
    }
    if (vaultPage) {
        TextStyle vs = textStyle(FontFace::BOLD, 15.0f, {236, 226, 255, 230}, TextAlign::CENTER);
        vs.shadow = {8, 6, 20, 180};
        drawTextStyled(ren, "Find golden truffles hidden in the campaign to open the vaults.", WINDOW_W * 0.5f,
                       612.0f, vs);
    }
}

void SnoutAndSiegeGame::renderAchievementToast(SDL_Renderer* ren) {
    if (!toast.active) return;
    float tw = 500.0f, th = 76.0f;
    float tx = WINDOW_W * 0.5f - tw * 0.5f;
    float ty = toast.slideY;
    panel(ren, {tx, ty, tw, th}, 18.0f, {44, 34, 78, 245});
    Vec2 ic(tx + 42, ty + th * 0.5f);
    fillCircleGrad(ren, ic, 26.0f, tint(pal::GOLD, 0.3f), pal::GOLD_DARK, 28);
    icon(ren, UiIcon::TROPHY, ic, 30.0f, {110, 60, 10, 255});
    TextStyle hs = textStyle(FontFace::BOLD, 14.0f, pal::GOLD);
    drawTextStyled(ren, "Achievement unlocked!", tx + 82, ty + 12, hs);
    TextStyle ts = textStyle(FontFace::DISPLAY, 22.0f, pal::CREAM);
    drawTextStyled(ren, toast.title, tx + 82, ty + 28, ts);
    TextStyle ds = textStyle(FontFace::BODY, 13.0f, {214, 206, 240, 255});
    std::string desc = toast.desc;
    while (desc.size() > 4 && textWidth(desc, FontFace::BODY, 13.0f) > tw - 100) desc = desc.substr(0, desc.size() - 4) + "…";
    drawTextStyled(ren, desc, tx + 82, ty + 54, ds);
}

void SnoutAndSiegeGame::renderAchievementsScreen(SDL_Renderer* ren) {
    fillRectGrad(ren, {0, 0, WINDOW_W, WINDOW_H}, {10, 8, 24, 120}, {10, 8, 24, 60});
    uiButton(ren, "BACK_TITLE", {24, 22, 124, 54}, "Menu", pal::SLATE, UiIcon::BACK);
    title(ren, "Trophy Hall", WINDOW_W * 0.5f, 22.0f, 50.0f, pal::GOLD);

    std::string a = std::to_string(getAchievementsCount()) + " / " + std::to_string(achievementDefs.size());
    std::string tr = std::to_string(getTrufflesFoundCount()) + " / " + std::to_string(totalTruffles(levels));
    float ph = 36.0f;
    float w2 = pillWidth(ph, UiIcon::TRUFFLE, tr), w1 = pillWidth(ph, UiIcon::TROPHY, a);
    pill(ren, WINDOW_W - 24 - w2, 31, ph, UiIcon::TRUFFLE, {214, 160, 72, 255}, tr);
    pill(ren, WINDOW_W - 24 - w2 - 10 - w1, 31, ph, UiIcon::TROPHY, {222, 180, 255, 255}, a);

    const float cardW = 580.0f, cardH = 84.0f, gapX = 20.0f, gapY = 11.0f;
    const float startX = (WINDOW_W - (2 * cardW + gapX)) * 0.5f, startY = 98.0f;
    for (size_t i = 0; i < achievementDefs.size() && i < 12; ++i) {
        const auto& def = achievementDefs[i];
        bool unlocked = achievementsUnlocked[i];
        float x = startX + (i % 2) * (cardW + gapX);
        float y = startY + (i / 2) * (cardH + gapY);
        panel(ren, {x, y, cardW, cardH}, 18.0f, unlocked ? SDL_Color{60, 48, 98, 240} : SDL_Color{30, 32, 54, 225});
        Vec2 ic(x + 48, y + cardH * 0.5f);
        if (unlocked) {
            fillCircleGrad(ren, ic, 28.0f, tint(pal::GOLD, 0.35f), pal::GOLD_DARK, 32);
            icon(ren, UiIcon::TROPHY, ic, 32.0f, {110, 60, 10, 255});
        } else {
            fillCircle(ren, ic, 28.0f, {50, 54, 84, 255}, 32);
            icon(ren, UiIcon::LOCK, ic, 30.0f, {120, 126, 170, 255});
        }
        TextStyle ts = textStyle(FontFace::DISPLAY, 22.0f, unlocked ? pal::CREAM : SDL_Color{170, 176, 210, 255});
        drawTextStyled(ren, displayLevelName(def.title), x + 92, y + 12, ts);
        TextStyle ds = textStyle(FontFace::BODY, 14.0f, unlocked ? SDL_Color{222, 216, 246, 255} : SDL_Color{140, 146, 180, 255});
        drawTextWrapped(ren, def.desc, x + 92, y + 42, cardW - 150, ds, 1.0f);

        std::string progress;
        if (!unlocked) {
            if (def.id == AchievementId::CAULDRON_CRACKER) {
                progress = std::to_string(achievementProgress[static_cast<int>(AchievementId::CAULDRON_CRACKER)]) + " / 10";
            } else if (def.id == AchievementId::TRUFFLE_CONNOISSEUR) {
                progress = std::to_string(getTrufflesFoundCount()) + " / 20";
            } else if (def.id == AchievementId::VAULT_CONQUEROR) {
                int vaultsCleared = 0;
                for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) vaultsCleared += bestStars[v] > 0 ? 1 : 0;
                progress = std::to_string(vaultsCleared) + " / " + std::to_string(NUM_VAULT_LEVELS);
            }
        }
        if (unlocked) {
            Vec2 cc(x + cardW - 30, y + 26);
            fillCircle(ren, cc, 14.0f, pal::GREEN, 20);
            icon(ren, UiIcon::CHECK, cc, 18.0f, pal::CREAM);
        } else if (!progress.empty()) {
            TextStyle ps = textStyle(FontFace::BOLD, 15.0f, {170, 176, 210, 255}, TextAlign::RIGHT);
            drawTextStyled(ren, progress, x + cardW - 20, y + 16, ps);
        }
    }
}

void SnoutAndSiegeGame::renderEditorScreen(SDL_Renderer* ren) {
    // 1. Slingshot Launchpad in Editor
    renderSiegeCatapultBase(ren, 180.0f, SLING_Y);
    renderOrigamiPigGeometry(ren, PigType::TROTTER, {180, SLING_Y - 4.0f}, 24.0f, -0.08f);
    drawCatapultFrontArm(ren, 180.0f, SLING_Y);
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
    ui::fillRectGrad(ren, leftDock, {38, 42, 72, 242}, {26, 28, 52, 242});
    drawThickLine(ren, {215.0f, 70.0f}, {215.0f, WINDOW_H}, 3.0f, {24, 22, 40, 255});

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

    std::string themeText = std::string("Theme: ") + chapterName(editorTheme);
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
    ui::fillRectGrad(ren, topBar, {44, 48, 82, 248}, {32, 34, 62, 248});
    drawThickLine(ren, {0, 70.0f}, {WINDOW_W, 70.0f}, 3.0f, {24, 22, 40, 255});

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

namespace {

const char* pigName(PigType t) {
    switch (t) {
    case PigType::TROTTER: return "Trotter";
    case PigType::TURBO: return "Bramble";
    case PigType::SPLIT: return "Clover";
    case PigType::BOMB: return "Vulcan";
    case PigType::GLIDER: return "Gale";
    case PigType::GRAVITON: return "Atticus";
    case PigType::DRILL: return "Rusty";
    }
    return "Pig";
}

const char* pigAbility(PigType t) {
    switch (t) {
    case PigType::TROTTER: return "Anvil Slam";
    case PigType::TURBO: return "Supersonic Dash";
    case PigType::SPLIT: return "Three-Way Split";
    case PigType::BOMB: return "Seismic Blast";
    case PigType::GLIDER: return "Glide & Drop Bombs";
    case PigType::GRAVITON: return "Graviton Vortex";
    case PigType::DRILL: return "Drill Bore";
    }
    return "";
}

// Score readout in the top-centre: big number, small caption underneath.
void drawScore(SDL_Renderer* ren, int value, const std::string& caption) {
    ui::title(ren, formatNumber(value), WINDOW_W * 0.5f, 10.0f, 44.0f, ui::pal::CREAM);
    if (!caption.empty()) {
        TextStyle cs = textStyle(FontFace::BOLD, 15.0f, {236, 236, 250, 230}, TextAlign::CENTER);
        cs.shadow = {8, 6, 20, 200};
        drawTextStyled(ren, caption, WINDOW_W * 0.5f, 62.0f, cs);
    }
}

// Two-line level label next to the top-left buttons.
void drawLevelLabel(SDL_Renderer* ren, float x, const std::string& kicker, const std::string& name,
                    const std::string& sub, SDL_Color accent) {
    TextStyle ks = textStyle(FontFace::BOLD, 14.0f, accent);
    ks.shadow = {8, 6, 20, 200};
    drawTextStyled(ren, kicker, x, 14.0f, ks);
    float maxW = WINDOW_W * 0.5f - 110.0f - x;
    float size = 24.0f;
    float w = textWidth(name, FontFace::DISPLAY, size);
    if (w > maxW) size *= maxW / w;
    ui::title(ren, name, x, 30.0f, size, ui::pal::CREAM, TextAlign::LEFT);
    if (!sub.empty()) {
        std::string s = sub;
        while (s.size() > 4 && textWidth(s, FontFace::BODY, 13.0f) > maxW) s = s.substr(0, s.size() - 4) + "…";
        TextStyle ss = textStyle(FontFace::BODY, 13.0f, {226, 230, 246, 230});
        ss.shadow = {8, 6, 20, 200};
        drawTextStyled(ren, s, x, 60.0f, ss);
    }
}

// "<- 25 mph" style wind gauge.
float drawWind(SDL_Renderer* ren, float rightX, float wind, bool storm) {
    std::string label;
    if (std::abs(wind) < 10.0f) label = storm ? "Storm · calm" : "Calm";
    else label = std::string(storm ? "Storm · " : "") + std::to_string(static_cast<int>(std::abs(wind))) + " mph";
    float h = 38.0f;
    float arrowW = std::abs(wind) < 10.0f ? 0.0f : 30.0f;
    float w = ui::pillWidth(h, UiIcon::NONE, label) + arrowW;
    float x = rightX - w;
    ui::fillRoundRect(ren, {x, 24, w, h}, h * 0.5f, {16, 18, 34, 170});
    float tx = x + h * 0.45f;
    if (arrowW > 0.0f) {
        float dir = wind > 0 ? 1.0f : -1.0f;
        Vec2 c(tx + 10, 24 + h * 0.5f);
        SDL_Color ac = std::abs(wind) > 80.0f ? SDL_Color{255, 196, 90, 255} : SDL_Color{150, 210, 255, 255};
        drawThickLine(ren, {c.x - 9 * dir, c.y}, {c.x + 5 * dir, c.y}, 4.0f, ac);
        drawTriangle(ren, {c.x + 11 * dir, c.y}, {c.x + 2 * dir, c.y - 7}, {c.x + 2 * dir, c.y + 7}, ac);
        tx += arrowW;
    }
    TextStyle st = textStyle(FontFace::BOLD, h * 0.5f, ui::pal::CREAM);
    drawTextStyled(ren, label, tx, 24 + h * 0.5f - textCapTop(st.face, st.size) - textCapHeight(st.face, st.size) * 0.5f, st);
    return w;
}

}  // namespace

void SnoutAndSiegeGame::renderHUD(SDL_Renderer* ren) {
    using namespace ui;
    // Gentle top shade so HUD text reads on bright skies.
    fillRectGrad(ren, {0, 0, WINDOW_W, 110}, {10, 8, 24, 110}, {10, 8, 24, 0});

    const float s = 50.0f, top = 18.0f;
    bool endless = (screen == ScreenState::ENDLESS);
    bool testPlay = (screen == ScreenState::EDITOR && isTestPlaying);

    // Left: navigation.
    float x = 20.0f;
    if (testPlay) {
        uiButton(ren, "PAUSE_GAME", {x, top, s, s}, "", pal::SLATE, UiIcon::PAUSE, false, false, "Pause (Esc)");
        x += s + 10;
        uiButton(ren, "RETRY_LEVEL", {x, top, s, s}, "", pal::ORANGE, UiIcon::RETRY, false, false, "Reset (R)");
        x += s + 18;
        drawLevelLabel(ren, x, "Architect test", customLevel.name, "Slot " + std::to_string(customSlot), pal::TEAL);
    } else if (endless) {
        uiButton(ren, "PAUSE_GAME", {x, top, s, s}, "", pal::SLATE, UiIcon::PAUSE, false, false, "Pause (Esc)");
        x += s + 10;
        uiButton(ren, "RETRY_ENDLESS", {x, top, s, s}, "", pal::ORANGE, UiIcon::RETRY, false, false, "Restart run (R)");
        x += s + 18;
        drawLevelLabel(ren, x, "Endless siege", "Wave " + std::to_string(endlessWave),
                       "Best: wave " + std::to_string(endlessBestWave), pal::ORANGE);
    } else {
        const LevelSpec& L = levels[currentLevelIdx];
        uiButton(ren, "PAUSE_GAME", {x, top, s, s}, "", pal::SLATE, UiIcon::PAUSE, false, false, "Pause (Esc)");
        x += s + 10;
        uiButton(ren, "RETRY_LEVEL", {x, top, s, s}, "", pal::ORANGE, UiIcon::RETRY, false, false, "Retry (R)");
        x += s + 18;
        bool vault = currentLevelIdx >= NUM_CAMPAIGN_LEVELS;
        std::string kicker = vault ? "Secret vault " + std::to_string(currentLevelIdx - NUM_CAMPAIGN_LEVELS + 1)
                                   : "Level " + std::to_string(currentLevelIdx + 1) + "  ·  " +
                                         chapterName(currentLevelIdx / LEVELS_PER_CHAPTER);
        std::string name = L.name;
        auto colon = name.find(':');
        if (colon != std::string::npos) name = name.substr(colon + 2);
        // Title-case the all-caps level name.
        bool start = true;
        for (auto& ch : name) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (std::isalpha(c)) { ch = static_cast<char>(start ? std::toupper(c) : std::tolower(c)); start = false; }
            else start = (c == ' ' || c == '-');
        }
        drawLevelLabel(ren, x, kicker, name, L.subtitle, pal::GOLD);
    }

    // Centre: score.
    if (endless) {
        drawScore(ren, endlessScore, "Combo ×" + std::to_string(static_cast<int>(endlessComboMultiplier)));
    } else if (testPlay) {
        drawScore(ren, score, "Test play");
    } else {
        int best = bestScores[currentLevelIdx];
        drawScore(ren, score, best > 0 ? "Best " + formatNumber(best) : "");
        const LevelSpec& L = levels[currentLevelIdx];
        if (L.hasTruffle) {
            std::string ts = levelTruffle.collected ? "Truffle found!" : "A truffle is hidden here";
            float pw = pillWidth(30.0f, UiIcon::TRUFFLE, ts);
            SDL_Color ic = levelTruffle.collected ? pal::GOLD : SDL_Color{214, 160, 72, 255};
            pill(ren, WINDOW_W * 0.5f - pw * 0.5f, 86.0f, 30.0f, UiIcon::TRUFFLE, ic, ts);
        }
    }

    // Right: settings + wind.
    float rx = WINDOW_W - 20.0f - s;
    uiButton(ren, "TOGGLE_SFX", {rx, top, s, s}, "", pal::SLATE, UiIcon::SOUND, false, assets.sfxMuted, "Sound (S)");
    rx -= s + 10;
    uiButton(ren, "TOGGLE_MUSIC", {rx, top, s, s}, "", pal::SLATE, UiIcon::MUSIC, false, assets.musicMuted, "Music (M)");
    rx -= s + 10;
    bool fast = simSpeed > 1.0f;
    uiButton(ren, "TOGGLE_SPEED", {rx, top, s, s}, "", fast ? pal::ORANGE : pal::SLATE, UiIcon::FAST, fast, false,
             fast ? "Speed 2.5× (Tab)" : "Fast-forward (Tab)");
    if (!endless && !testPlay) {
        drawWind(ren, rx - 14, currentWind, levels[currentLevelIdx].hasStorm);
    } else if (std::abs(currentWind) >= 10.0f) {
        drawWind(ren, rx - 14, currentWind, false);
    }

    // Bottom-left: who is in the sling.
    if (!activePigs.empty()) {
        PigType pt = activePigs[0].type;
        std::string who = std::string(pigName(pt)) + "  ·  Space: " + pigAbility(pt);
        pill(ren, 20.0f, WINDOW_H - 50.0f, 34.0f, UiIcon::NONE, pal::CREAM, who, {16, 18, 34, 180});
    }

    if ((levelComplete || levelFailed) && !endless) renderResultsCard(ren);
    if (endless && endlessGameOver) renderResultsCard(ren);
}

void SnoutAndSiegeGame::renderResultsCard(SDL_Renderer* ren) {
    using namespace ui;
    bool endless = (screen == ScreenState::ENDLESS);
    bool testPlay = (screen == ScreenState::EDITOR && isTestPlaying);
    bool win = !endless && levelComplete;

    SDL_SetRenderDrawColor(ren, 8, 6, 20, 120);
    SDL_FRect dim{0, 0, WINDOW_W, WINDOW_H};
    SDL_RenderFillRectF(ren, &dim);

    const float w = 600.0f;
    const LevelSpec& Lc = levels[std::min(currentLevelIdx, NUM_LEVELS - 1)];
    static const int kMilestoneIdx[] = {4, 9, 19, 29, 39, 43};
    bool hasNote = !endless && !testPlay && (Lc.hasTruffle || currentLevelIdx >= NUM_CAMPAIGN_LEVELS ||
                   std::find(std::begin(kMilestoneIdx), std::end(kMilestoneIdx), currentLevelIdx) != std::end(kMilestoneIdx));
    float h = win && !testPlay ? (hasNote ? 470.0f : 400.0f) : 380.0f;
    SDL_FRect card{WINDOW_W * 0.5f - w * 0.5f, 360.0f - h * 0.5f + 10.0f, w, h};
    panel(ren, card, 26.0f, {40, 44, 78, 245});
    float cx = WINDOW_W * 0.5f;

    std::string head = endless ? "Overwhelmed!" : (win ? (testPlay ? "Fortress Demolished!" : "Level Complete!")
                                                       : (testPlay ? "Fortress Stood!" : "Level Failed"));
    ribbon(ren, cx, card.y + 4.0f, 380.0f, 58.0f, win ? pal::GREEN : pal::RED, head, 34.0f);

    float y = card.y + 56.0f;
    if (endless) {
        TextStyle ls = textStyle(FontFace::BOLD, 18.0f, pal::MUTED, TextAlign::CENTER);
        drawTextStyled(ren, "Waves survived", cx, y + 10, ls);
        title(ren, std::to_string(std::max(0, endlessWave - 1)), cx, y + 34, 64.0f, pal::GOLD);
        drawTextStyled(ren, "Score " + formatNumber(endlessScore) + "   ·   Best: wave " + std::to_string(endlessBestWave),
                       cx, y + 120, ls);
        float by = card.y + card.h - 92.0f;
        uiButton(ren, "BACK_TITLE", {cx - 230, by, 200, 64}, "Menu", pal::SLATE, UiIcon::MENU);
        uiButton(ren, "RETRY_ENDLESS", {cx + 30, by, 200, 64}, "Play again", pal::GREEN, UiIcon::RETRY);
        return;
    }

    if (testPlay) {
        TextStyle ls = textStyle(FontFace::BOLD, 18.0f, pal::MUTED, TextAlign::CENTER);
        drawTextStyled(ren, win ? "Score" : "The birds survived your barrage.", cx, y + 22, ls);
        if (win) title(ren, formatNumber(score), cx, y + 46, 60.0f, pal::CREAM);
        float by = card.y + card.h - 92.0f;
        uiButton(ren, "RETURN_TO_EDITOR", {cx - 230, by, 200, 64}, "Architect", pal::SLATE, UiIcon::BACK);
        uiButton(ren, "RETRY_LEVEL", {cx + 30, by, 200, 64}, "Test again", pal::GREEN, UiIcon::RETRY);
        return;
    }

    const LevelSpec& L = levels[currentLevelIdx];
    if (win) {
        for (int s = 0; s < 3; ++s) {
            bool earned = s < earnedStars;
            float pulse = (earned && isNewRecordThisRun) ? 1.0f + 0.08f * std::sin(recordAnimTimer * 5.0f + s * 1.5f) : 1.0f;
            float r = (s == 1 ? 44.0f : 36.0f) * pulse;
            star(ren, {cx + (s - 1) * 96.0f, y + 52.0f + (s == 1 ? -8.0f : 6.0f)}, r, earned, (s - 1) * 0.18f);
        }
        title(ren, formatNumber(score), cx, y + 104, 56.0f, pal::CREAM);

        std::string recordLine;
        SDL_Color recordCol = pal::MUTED;
        if (isNewRecordThisRun) {
            recordLine = prevLevelBest > 0 ? "New best!  +" + formatNumber(score - prevLevelBest) : "First clear!";
            recordCol = pal::GOLD;
        } else {
            recordLine = "Best " + formatNumber(bestScores[currentLevelIdx]);
        }
        TextStyle rs = textStyle(FontFace::BOLD, 20.0f, recordCol, TextAlign::CENTER);
        drawTextStyled(ren, recordLine, cx, y + 174, rs);

        // Milestone / truffle / vault note.
        struct Note { int clearedIdx; const char* title; const char* hint; };
        static const Note kMilestones[] = {
            {4, "New pig: Bramble", "Tap Space in mid-air for a Supersonic Dash."},
            {9, "New pig: Clover", "Tap Space in mid-air to split into three."},
            {19, "New pig: Vulcan", "Tap Space, or hit something, to detonate."},
            {29, "New pig: Gale", "Tap Space to level out and drop twin bombs."},
            {39, "New pig: Atticus", "Tap Space to open a vortex that drags blocks in."},
            {43, "New pig: Rusty", "Tap Space to drill through blocks. Stone wears it out fast!"},
        };
        std::string noteTitle, noteHint;
        UiIcon noteIcon = UiIcon::STAR;
        for (const auto& m : kMilestones) {
            if (m.clearedIdx == currentLevelIdx) { noteTitle = m.title; noteHint = m.hint; noteIcon = UiIcon::PLAY; }
        }
        if (noteTitle.empty() && L.hasTruffle) {
            noteIcon = UiIcon::TRUFFLE;
            if (levelTruffle.collected) {
                noteTitle = "Golden truffle found!  +2,500";
                noteHint = std::to_string(getTrufflesFoundCount()) + " of " + std::to_string([&] {
                    int n = 0; for (const auto& lv : levels) n += lv.hasTruffle; return n; }()) + " truffles recovered.";
            } else {
                noteTitle = "A golden truffle is still hidden here";
                noteHint = "Look in hidden cavities, or smash barriers to find it.";
            }
        } else if (noteTitle.empty() && currentLevelIdx >= NUM_CAMPAIGN_LEVELS) {
            noteIcon = UiIcon::TROPHY;
            noteTitle = "Vault conquered!";
            noteHint = "Royal swine treasure, plundered with precision.";
        }
        if (!noteTitle.empty()) {
            SDL_FRect nb{card.x + 40, y + 210, card.w - 80, 66};
            fillRoundRect(ren, nb, 16.0f, {24, 26, 50, 200});
            Vec2 ic(nb.x + 36, nb.y + nb.h * 0.5f);
            fillCircle(ren, ic, 22.0f, noteIcon == UiIcon::TRUFFLE ? SDL_Color{255, 214, 90, 255} : pal::BLUE, 24);
            icon(ren, noteIcon, ic, 24.0f, noteIcon == UiIcon::TRUFFLE ? SDL_Color{140, 84, 24, 255} : pal::CREAM);
            TextStyle nt = textStyle(FontFace::BOLD, 18.0f, pal::CREAM);
            drawTextStyled(ren, noteTitle, nb.x + 70, nb.y + 10, nt);
            TextStyle nh = textStyle(FontFace::BODY, 14.0f, {200, 204, 232, 255});
            drawTextStyled(ren, noteHint, nb.x + 70, nb.y + 36, nh);
        }

        float by = card.y + card.h - 92.0f;
        bool hasNext = currentLevelIdx + 1 < NUM_LEVELS && isLevelUnlocked(currentLevelIdx + 1);
        float bx = hasNext ? cx - 236 : cx - 140;
        uiButton(ren, "OPEN_LEVEL_SELECT", {bx, by, 64, 64}, "", pal::BLUE, UiIcon::LEVELS, false, false, "Level select");
        uiButton(ren, "RETRY_LEVEL", {bx + 80, by, 64, 64}, "", pal::ORANGE, UiIcon::RETRY, false, false, "Retry");
        if (hasNext) {
            uiButton(ren, "NEXT_LEVEL_BTN", {bx + 160, by, 252, 64}, "Next level", pal::GREEN, UiIcon::PLAY);
        } else {
            uiButton(ren, "BACK_TITLE", {bx + 160, by, 120, 64}, "Menu", pal::SLATE, UiIcon::MENU);
        }
    } else {
        TextStyle ls = textStyle(FontFace::BOLD, 20.0f, {214, 218, 240, 255}, TextAlign::CENTER);
        drawTextStyled(ren, "The birds are still perched.", cx, y + 30, ls);
        std::string best = bestScores[currentLevelIdx] > 0 ? "Your best: " + formatNumber(bestScores[currentLevelIdx])
                                                           : "Try a different angle, or save your strongest pig.";
        TextStyle bs = textStyle(FontFace::BODY, 17.0f, pal::MUTED, TextAlign::CENTER);
        drawTextStyled(ren, best, cx, y + 70, bs);
        float by = card.y + card.h - 100.0f;
        uiButton(ren, "OPEN_LEVEL_SELECT", {cx - 196, by, 64, 64}, "", pal::BLUE, UiIcon::LEVELS, false, false, "Level select");
        uiButton(ren, "RETRY_LEVEL", {cx - 116, by, 312, 64}, "Try again", pal::GREEN, UiIcon::RETRY);
    }
}

void SnoutAndSiegeGame::renderPauseMenu(SDL_Renderer* ren) {
    using namespace ui;
    activeButtons.clear();  // the HUD underneath is inert while paused
    SDL_SetRenderDrawColor(ren, 8, 6, 20, 140);
    SDL_FRect dim{0, 0, WINDOW_W, WINDOW_H};
    SDL_RenderFillRectF(ren, &dim);

    bool endless = (screen == ScreenState::ENDLESS);
    bool testPlay = (screen == ScreenState::EDITOR && isTestPlaying);
    const float w = 420.0f, h = endless || testPlay ? 380.0f : 440.0f;
    SDL_FRect card{WINDOW_W * 0.5f - w * 0.5f, 360.0f - h * 0.5f, w, h};
    panel(ren, card, 26.0f, {40, 44, 78, 245});
    ribbon(ren, WINDOW_W * 0.5f, card.y + 4.0f, 300.0f, 56.0f, pal::BLUE, "Paused", 34.0f);

    float bx = card.x + 50.0f, bw = w - 100.0f, y = card.y + 62.0f;
    uiButton(ren, "RESUME_GAME", {bx, y, bw, 66.0f}, "Resume", pal::GREEN, UiIcon::PLAY);
    y += 82.0f;
    if (endless) {
        uiButton(ren, "RETRY_ENDLESS", {bx, y, bw, 56.0f}, "Restart run", pal::ORANGE, UiIcon::RETRY);
        y += 70.0f;
    } else {
        uiButton(ren, "RETRY_LEVEL", {bx, y, bw, 56.0f}, testPlay ? "Reset test" : "Retry level", pal::ORANGE,
                 UiIcon::RETRY);
        y += 70.0f;
    }
    if (testPlay) {
        uiButton(ren, "RETURN_TO_EDITOR", {bx, y, bw, 56.0f}, "Back to Architect", pal::TEAL, UiIcon::HAMMER);
        y += 70.0f;
    } else {
        if (!endless) {
            uiButton(ren, "OPEN_LEVEL_SELECT", {bx, y, bw, 56.0f}, "Level select", pal::BLUE, UiIcon::LEVELS);
            y += 70.0f;
        }
        uiButton(ren, "BACK_TITLE", {bx, y, bw, 56.0f}, "Main menu", pal::SLATE, UiIcon::MENU);
        y += 70.0f;
    }
    float s = 50.0f, gx = WINDOW_W * 0.5f - s - 8.0f;
    uiButton(ren, "TOGGLE_MUSIC", {gx, y + 4, s, s}, "", pal::SLATE, UiIcon::MUSIC, false, assets.musicMuted, "Music (M)");
    uiButton(ren, "TOGGLE_SFX", {gx + s + 16, y + 4, s, s}, "", pal::SLATE, UiIcon::SOUND, false, assets.sfxMuted,
             "Sound (S)");
}

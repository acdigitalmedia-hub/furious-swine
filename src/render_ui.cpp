#include "game.h"

#include <functional>

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
    using namespace ui;
    const float dockW = 215.0f, topH = 70.0f;

    // --- Canvas: build zone, grid, placed pieces.
    fillRectGrad(ren, {dockW, topH, WINDOW_W - dockW, 60}, {10, 8, 24, 70}, {10, 8, 24, 0});
    if (editorSnapGrid) {
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 38);
        for (float gx = 240.0f; gx <= WINDOW_W - 10.0f; gx += 20.0f) {
            for (float gy = 80.0f; gy <= GROUND_Y; gy += 20.0f) {
                SDL_FRect d{gx - 1.0f, gy - 1.0f, 2.0f, 2.0f};
                SDL_RenderFillRectF(ren, &d);
            }
        }
    }
    for (const auto& b : customLevel.blocks) renderBlock(ren, b, {0.0f, 0.0f});
    for (const auto& p : customLevel.pendulums) renderPendulum(ren, p, {0.0f, 0.0f});
    for (const auto& bt : customLevel.birds) renderBird(ren, bt, {0.0f, 0.0f});
    if (customLevel.hasTruffle) {
        GoldenTruffle bk = levelTruffle;
        levelTruffle.exists = true;
        levelTruffle.collected = false;
        levelTruffle.pos = customLevel.trufflePos;
        levelTruffle.radius = 16.0f;
        renderGoldenTruffle(ren, {0.0f, 0.0f});
        levelTruffle = bk;
    }

    // --- Placement preview under the cursor.
    if (mouseX > 218.0f && mouseY > 72.0f && mouseY < GROUND_Y + 10.0f) {
        float gx = editorSnapGrid ? (std::round(mouseX / 20.0f) * 20.0f) : mouseX;
        float gy = editorSnapGrid ? (std::round(mouseY / 20.0f) * 20.0f) : mouseY;
        if (editorTool == 0) {
            BlockBody ghost = makeBlock(editorMat, gx, gy, editorBlockW, editorBlockH);
            renderBlock(ren, ghost, {0, 0});
            strokeRoundRect(ren, {gx - editorBlockW * 0.5f - 4, gy - editorBlockH * 0.5f - 4, editorBlockW + 8, editorBlockH + 8},
                            6.0f, 2.0f, {255, 236, 140, 230});
        } else if (editorTool == 1) {
            BirdTarget ghost = makeBird(editorBird, gx, gy, editorBirdHelmet);
            renderBird(ren, ghost, {0, 0});
            ring(ren, {gx, gy}, ghost.radius + 6, 2.0f, {255, 236, 140, 230});
        } else if (editorTool == 2) {
            drawThickLine(ren, {gx, gy}, {gx + 30.0f, gy + 80.0f}, 2.5f, {230, 230, 245, 200});
            fillCircle(ren, {gx, gy}, 6.0f, {255, 214, 90, 255}, 14);
            fillCircleGrad(ren, {gx + 30.0f, gy + 80.0f}, 18.0f, {170, 176, 196, 230}, {80, 86, 104, 230}, 22);
        } else if (editorTool == 3) {
            GoldenTruffle bk = levelTruffle;
            levelTruffle.exists = true;
            levelTruffle.collected = false;
            levelTruffle.pos = {gx, gy};
            levelTruffle.radius = 16.0f;
            renderGoldenTruffle(ren, {0, 0});
            levelTruffle = bk;
        } else if (editorTool == 4) {
            fillCircle(ren, {mouseX, mouseY}, 24.0f, {226, 78, 66, 120}, 24);
            ring(ren, {mouseX, mouseY}, 24.0f, 2.5f, {255, 140, 130, 255});
            icon(ren, UiIcon::QUIT, {mouseX, mouseY}, 22.0f, pal::CREAM);
        }
    }

    // --- Picture tile helper: a selectable button showing a live preview.
    auto tile = [&](const std::string& id, SDL_FRect r, bool selected, const std::string& tip,
                    const std::function<void(Vec2)>& art) {
        UIButton hit;
        hit.id = id;
        hit.rect = r;
        hit.tooltip = tip;
        activeButtons.push_back(hit);
        bool hov = hit.contains(mouseX, mouseY);
        SDL_FRect face = buttonFace(ren, r, selected ? SDL_Color{92, 124, 200, 255} : SDL_Color{62, 70, 104, 255},
                                    hov, selected, 10.0f);
        art({face.x + face.w * 0.5f, face.y + face.h * 0.5f});
        if (hov && !tip.empty()) {
            float tw = textWidth(tip, FontFace::BOLD, 14.0f) + 18.0f;
            SDL_FRect tr{std::min(r.x, WINDOW_W - tw - 6), r.y + r.h + 6, tw, 26};
            fillRoundRect(ren, tr, 8.0f, {16, 14, 30, 235});
            TextStyle ts = textStyle(FontFace::BOLD, 14.0f, pal::CREAM, TextAlign::CENTER);
            drawTextStyled(ren, tip, tr.x + tw * 0.5f, tr.y + 13 - textCapTop(ts.face, 14) - textCapHeight(ts.face, 14) * 0.5f, ts);
        }
    };
    auto header = [&](const std::string& s, float y) {
        TextStyle hs = textStyle(FontFace::BOLD, 14.0f, {170, 178, 214, 255});
        drawTextStyled(ren, s, 14.0f, y, hs);
    };
    auto note = [&](const std::string& s, float y) {
        TextStyle ns = textStyle(FontFace::BODY, 14.0f, {214, 218, 240, 255});
        return drawTextWrapped(ren, s, 14.0f, y, dockW - 28.0f, ns, 1.1f);
    };

    // --- Left dock.
    fillRectGrad(ren, {0, topH, dockW, WINDOW_H - topH}, {40, 44, 76, 246}, {28, 30, 56, 246});
    drawThickLine(ren, {dockW, topH}, {dockW, WINDOW_H}, 3.0f, pal::INK);

    header("Tool", 80.0f);
    const char* toolIds[5] = {"TOOL_BLOCK", "TOOL_BIRD", "TOOL_PENDULUM", "TOOL_TRUFFLE", "TOOL_ERASER"};
    const char* toolTips[5] = {"Blocks", "Birds", "Wrecking ball", "Golden truffle", "Eraser"};
    for (int i = 0; i < 5; ++i) {
        SDL_FRect r{12.0f + i * 39.0f, 100.0f, 35.0f, 40.0f};
        tile(toolIds[i], r, editorTool == i, toolTips[i], [&, i](Vec2 c) {
            switch (i) {
            case 0: renderBlock(ren, makeBlock(MaterialTier::TIMBER, c.x, c.y, 20, 20), {0, 0}); break;
            case 1: renderTotemBirdGeometry(ren, BirdClass::TOUCAN, c + Vec2(2, 1), 10.0f, 0.0f, 1.0f); break;
            case 2:
                drawThickLine(ren, c + Vec2(-5, -11), c + Vec2(2, 3), 1.5f, pal::CREAM);
                fillCircleGrad(ren, c + Vec2(3, 5), 7.0f, {190, 196, 214, 255}, {90, 96, 116, 255}, 14);
                break;
            case 3: fillCircleGrad(ren, c, 8.0f, {255, 236, 150, 255}, {214, 150, 34, 255}, 16); break;
            case 4: icon(ren, UiIcon::QUIT, c, 16.0f, {255, 150, 140, 255}); break;
            }
        });
    }

    float y = 156.0f;
    if (editorTool == 0) {
        header("Material", y);
        y += 20.0f;
        struct M { const char* id; MaterialTier m; const char* name; };
        const M mats[7] = {{"MAT_TWIG", MaterialTier::TWIG, "Bamboo"}, {"MAT_WOOD", MaterialTier::TIMBER, "Timber"},
                           {"MAT_STONE", MaterialTier::STONE, "Stone"}, {"MAT_ICE", MaterialTier::ICE, "Ice"},
                           {"MAT_TNT", MaterialTier::TNT, "TNT"}, {"MAT_BOUNCE", MaterialTier::TRAMPOLINE, "Trampoline"},
                           {"MAT_MAGMA", MaterialTier::MAGMA_VENT, "Magma vent"}};
        for (int i = 0; i < 7; ++i) {
            SDL_FRect r{12.0f + (i % 4) * 49.0f, y + (i / 4) * 46.0f, 45.0f, 40.0f};
            tile(mats[i].id, r, editorMat == mats[i].m, mats[i].name,
                 [&, i](Vec2 c) { renderBlock(ren, makeBlock(mats[i].m, c.x, c.y, 28, 20), {0, 0}); });
        }
        y += 104.0f;
        header("Size", y);
        y += 20.0f;
        struct Sz { const char* id; float w, h; const char* name; };
        const Sz sizes[7] = {{"BLOCK_SIZE_SM", 16, 50, "Post 16 x 50"}, {"BLOCK_SIZE_MED", 20, 70, "Beam 20 x 70"},
                             {"BLOCK_SIZE_LG", 24, 90, "Pillar 24 x 90"}, {"BLOCK_SIZE_SQR", 36, 36, "Square 36 x 36"},
                             {"BLOCK_SIZE_SLAB", 60, 22, "Slab 60 x 22"}, {"BLOCK_SIZE_HORIZ", 80, 18, "Plank 80 x 18"},
                             {"BLOCK_SIZE_LINTEL", 120, 22, "Lintel 120 x 22"}};
        for (int i = 0; i < 7; ++i) {
            SDL_FRect r{12.0f + (i % 4) * 49.0f, y + (i / 4) * 52.0f, 45.0f, 46.0f};
            bool sel = editorBlockW == sizes[i].w && editorBlockH == sizes[i].h;
            tile(sizes[i].id, r, sel, sizes[i].name, [&, i](Vec2 c) {
                float k = 30.0f / std::max(sizes[i].w, sizes[i].h);
                renderBlock(ren, makeBlock(editorMat, c.x, c.y, sizes[i].w * k, sizes[i].h * k), {0, 0});
            });
        }
        y += 110.0f;
    } else if (editorTool == 1) {
        header("Bird", y);
        y += 20.0f;
        struct B { const char* id; BirdClass k; const char* name; };
        const B birdsList[5] = {{"BIRD_TOUCAN", BirdClass::TOUCAN, "Toucan  (78 HP)"}, {"BIRD_OWL", BirdClass::OWL, "Owl  (140 HP)"},
                                {"BIRD_HERON", BirdClass::HERON, "Heron  (210 HP)"},
                                {"BIRD_FALCON", BirdClass::FALCON, "Falcon  (shield)"},
                                {"BIRD_CONDOR", BirdClass::CONDOR, "Condor  (zeppelin)"}};
        for (int i = 0; i < 5; ++i) {
            SDL_FRect r{12.0f + (i % 3) * 65.0f, y + (i / 3) * 62.0f, 60.0f, 56.0f};
            tile(birdsList[i].id, r, editorBird == birdsList[i].k, birdsList[i].name, [&, i](Vec2 c) {
                BirdClass k = birdsList[i].k;
                renderTotemBirdGeometry(ren, k, c + Vec2(k == BirdClass::FALCON ? 3 : 0, k == BirdClass::CONDOR ? 9 : 1),
                                        k == BirdClass::CONDOR ? 10.0f : 14.0f, 0.0f, 1.0f, editorBirdHelmet, 1.0f,
                                        k == BirdClass::FALCON, 1.0f, k == BirdClass::CONDOR, 1.0f);
            });
        }
        y += 132.0f;
        uiButton(ren, "TOGGLE_HELMET", {12, y, dockW - 24, 40}, editorBirdHelmet ? "Helmet: on" : "Helmet: off",
                 editorBirdHelmet ? pal::ORANGE : pal::SLATE, UiIcon::NONE, editorBirdHelmet);
        y += 56.0f;
    } else if (editorTool == 2) {
        header("Wrecking ball", y);
        y += 22.0f;
        uiButton(ren, "TOGGLE_SPIKE", {12, y, dockW - 24, 40}, editorSpikedPendulum ? "Spiked" : "Blunt",
                 editorSpikedPendulum ? pal::RED : pal::SLATE, UiIcon::NONE, editorSpikedPendulum);
        y += 56.0f;
        y += note("Click to place the anchor. The ball hangs below it and swings when hit.", y) + 10.0f;
    } else if (editorTool == 3) {
        header("Golden truffle", y);
        y += 22.0f;
        y += note("Click to hide the level's truffle (+2,500 points). Each level holds one.", y) + 10.0f;
    } else {
        header("Eraser", y);
        y += 22.0f;
        y += note("Click any block, bird, wrecking ball or truffle to remove it.", y) + 10.0f;
    }

    // Level settings at the bottom of the dock.
    float by = 470.0f;
    header("Level", by);
    by += 20.0f;
    uiButton(ren, "TOGGLE_GRID", {12, by, 92, 36}, "Grid", editorSnapGrid ? pal::BLUE : pal::SLATE, UiIcon::LEVELS,
             editorSnapGrid, false, "Snap to a 20 px grid (G)");
    uiButton(ren, "TOGGLE_STORM", {111, by, 92, 36}, "Storm", editorStorm ? pal::PURPLE : pal::SLATE, UiIcon::NONE,
             editorStorm, false, "Storm clouds with lightning");
    by += 46.0f;
    uiButton(ren, "CYCLE_THEME", {12, by, dockW - 24, 36}, chapterName(editorTheme), pal::TEAL, UiIcon::NONE, false, false,
             "Backdrop (click to cycle)");
    by += 46.0f;
    uiButton(ren, "WIND_DEC", {12, by, 40, 36}, "", pal::SLATE, UiIcon::BACK, false, false, "Wind left");
    uiButton(ren, "WIND_INC", {dockW - 52, by, 40, 36}, "", pal::SLATE, UiIcon::NEXT, false, false, "Wind right");
    {
        int w = static_cast<int>(editorWind);
        std::string ws = w == 0 ? "No wind" : std::to_string(std::abs(w)) + " mph " + (w < 0 ? "left" : "right");
        TextStyle st = textStyle(FontFace::BOLD, 15.0f, pal::CREAM, TextAlign::CENTER);
        drawTextStyled(ren, ws, dockW * 0.5f, by + 18 - textCapTop(st.face, 15) - textCapHeight(st.face, 15) * 0.5f, st);
    }
    by += 50.0f;
    TextStyle hs = textStyle(FontFace::BODY, 12.5f, {150, 158, 196, 255});
    drawTextStyled(ren, "Click: place  ·  Del: clear all", 14.0f, by, hs);
    drawTextStyled(ren, "T: test  ·  S: save  ·  L: load", 14.0f, by + 18, hs);

    // --- Top bar.
    fillRectGrad(ren, {0, 0, WINDOW_W, topH}, {48, 52, 88, 248}, {34, 36, 66, 248});
    drawThickLine(ren, {0, topH}, {WINDOW_W, topH}, 3.0f, pal::INK);
    uiButton(ren, "BACK_TITLE", {14, 12, 110, 46}, "Menu", pal::SLATE, UiIcon::BACK);
    // Slot selector.
    TextStyle sl = textStyle(FontFace::BOLD, 13.0f, {170, 178, 214, 255});
    drawTextStyled(ren, "Slot", 138.0f, 35.0f - textCapTop(sl.face, 13.0f) - textCapHeight(sl.face, 13.0f) * 0.5f, sl);
    for (int sIdx = 1; sIdx <= 3; ++sIdx) {
        uiButton(ren, "EDITOR_SLOT_" + std::to_string(sIdx), {172.0f + (sIdx - 1) * 46.0f, 14, 40, 42},
                 std::to_string(sIdx), customSlot == sIdx ? pal::BLUE : pal::SLATE, UiIcon::NONE, customSlot == sIdx);
    }
    uiButton(ren, "EDITOR_SAVE", {324, 12, 96, 46}, "Save", pal::BLUE, UiIcon::CHECK, false, false, "Save to this slot (S)");
    uiButton(ren, "EDITOR_LOAD", {428, 12, 96, 46}, "Load", pal::SLATE, UiIcon::RETRY, false, false, "Reload this slot (L)");
    uiButton(ren, "EDITOR_CLEAR", {532, 12, 96, 46}, "Clear", pal::RED, UiIcon::QUIT, false, false, "Clear everything (Del)");

    title(ren, "Architect", 650.0f, 10.0f, 28.0f, pal::GOLD, TextAlign::LEFT);
    std::string counts = std::to_string(customLevel.blocks.size()) + " blocks  ·  " +
                         std::to_string(customLevel.birds.size()) + " birds  ·  " +
                         std::to_string(customLevel.pendulums.size()) + " wrecking balls" +
                         (customLevel.hasTruffle ? "  ·  truffle" : "");
    TextStyle cs = textStyle(FontFace::BODY, 14.0f, {200, 206, 232, 255});
    drawTextStyled(ren, counts, 650.0f, 44.0f, cs);

    uiButton(ren, "EDITOR_TEST_PLAY", {WINDOW_W - 170.0f, 12, 156, 46}, "Test", pal::GREEN, UiIcon::PLAY, false, false,
             customLevel.birds.empty() ? "Add some birds first!" : "Play this fortress (T)");
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

#include "scenery.h"

#include "draw.h"
#include "ui.h"

namespace {

using ui::fillCircle;
using ui::fillCircleGrad;
using ui::fillRectGrad;
using ui::mix;
using ui::shade;
using ui::tint;

constexpr float PAD = 140.0f;  // overdraw for camera shake

// ---------------------------------------------------------------- helpers

float hash01(int i, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(i) * 374761393u + seed * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFF) / static_cast<float>(0x1000000);
}

float tri(float x) {  // triangle wave in [0, 1]
    float f = x - std::floor(x);
    return 1.0f - std::abs(f * 2.0f - 1.0f);
}

float smooth01(float a, float b, float x) {
    float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

SDL_Color rgba(int r, int g, int b, int a = 255) {
    return {static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a)};
}

void rect(SDL_Renderer* ren, float x, float y, float w, float h, SDL_Color c) {
    drawConvexPoly(ren, {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}, c);
}

// ---------------------------------------------------------------- theme data

enum class Ridge { NONE, HILLS, PEAKS, MESAS, VOLCANO, FLAT };
enum class Item {
    NONE, PINES, SNOWY_PINES, ROUND_TREES, BAMBOO, PAGODAS, COLUMNS, CHIMNEYS, ZIGGURATS,
    ARCADE, CASTLE, OBELISKS, ISLANDS, WINDMILL
};
enum class Sky { SUN, MOON, CRESCENT, TWIN_MOONS, ECLIPSE, GOLD_ECLIPSE, NONE };
enum class Weather { NONE, POLLEN, LEAVES, DUST, SNOW, EMBERS, PETALS, FIREFLIES, RAIN, SPARKLES, WIND };
enum class Turf { GRASS, AUTUMN, SAND, SNOW, OBSIDIAN, MOSS, IRON, ROCK, MARBLE, VAULT };

struct Layer {
    Ridge ridge = Ridge::NONE;
    float height = 200.0f;      // tallest ridge point above the ground line
    float parallax = 0.2f;
    SDL_Color top{}, bottom{};
    Item item = Item::NONE;
    float spacing = 160.0f;
    SDL_Color itemCol{};
    uint32_t seed = 1;
    float snowLine = -1.0f;     // fraction of height above which peaks get snow (-1 = none)
};

struct Theme {
    SDL_Color skyTop, skyMid, skyBot;
    Sky sky;
    SDL_Color celestial;
    Vec2 celPos;
    float celR;
    bool stars;
    bool aurora;
    int clouds;
    SDL_Color cloudCol;
    Layer far, mid;
    Turf turf;
    SDL_Color turfTop, soilTop, soilBot;
    Weather weather;
};

const Theme& theme(int i) {
    static const Theme T[NUM_SCENERY_THEMES] = {
        // 0 Dawn Meadow
        {rgba(92, 156, 224), rgba(156, 204, 238), rgba(255, 218, 168), Sky::SUN, rgba(255, 244, 196), {1030, 150}, 44,
         false, false, 5, rgba(255, 255, 255, 235),
         {Ridge::HILLS, 250, 0.10f, rgba(140, 186, 170), rgba(170, 206, 186), Item::NONE, 0, {}, 11},
         {Ridge::HILLS, 150, 0.30f, rgba(92, 164, 96), rgba(66, 134, 78), Item::PINES, 150, rgba(46, 108, 70), 7},
         Turf::GRASS, rgba(112, 196, 78), rgba(118, 84, 52), rgba(74, 50, 32), Weather::POLLEN},
        // 1 Autumn Woodlands
        {rgba(84, 96, 170), rgba(232, 150, 128), rgba(255, 208, 146), Sky::SUN, rgba(255, 214, 150), {1060, 250}, 62,
         false, false, 3, rgba(255, 226, 210, 220),
         {Ridge::HILLS, 230, 0.10f, rgba(178, 120, 138), rgba(206, 150, 150), Item::NONE, 0, {}, 21},
         {Ridge::HILLS, 120, 0.30f, rgba(150, 86, 64), rgba(118, 62, 50), Item::ROUND_TREES, 120, rgba(214, 104, 46), 5},
         Turf::AUTUMN, rgba(206, 146, 62), rgba(112, 70, 42), rgba(70, 42, 28), Weather::LEAVES},
        // 2 Basalt Canyon
        {rgba(76, 120, 200), rgba(236, 170, 126), rgba(255, 206, 150), Sky::SUN, rgba(255, 250, 226), {1080, 130}, 36,
         false, false, 2, rgba(255, 240, 228, 200),
         {Ridge::MESAS, 260, 0.10f, rgba(218, 148, 118), rgba(236, 180, 146), Item::NONE, 0, {}, 31},
         {Ridge::MESAS, 170, 0.30f, rgba(180, 90, 66), rgba(140, 66, 52), Item::NONE, 0, {}, 13},
         Turf::SAND, rgba(232, 172, 108), rgba(178, 108, 68), rgba(120, 66, 44), Weather::DUST},
        // 3 Frostpeak Highlands (aurora night)
        {rgba(12, 20, 52), rgba(30, 58, 108), rgba(86, 138, 180), Sky::CRESCENT, rgba(236, 246, 255), {1090, 140}, 30,
         true, true, 0, rgba(0, 0, 0, 0),
         {Ridge::PEAKS, 330, 0.10f, rgba(68, 96, 142), rgba(92, 128, 170), Item::NONE, 0, {}, 41, 0.6f},
         {Ridge::HILLS, 120, 0.30f, rgba(40, 62, 96), rgba(30, 48, 76), Item::SNOWY_PINES, 110, rgba(26, 46, 72), 9},
         Turf::SNOW, rgba(236, 246, 255), rgba(126, 146, 180), rgba(76, 92, 124), Weather::SNOW},
        // 4 Obsidian Citadel (volcanic eclipse)
        {rgba(26, 14, 30), rgba(98, 30, 42), rgba(232, 98, 52), Sky::ECLIPSE, rgba(255, 140, 60), {1070, 160}, 34,
         true, false, 0, rgba(0, 0, 0, 0),
         {Ridge::VOLCANO, 330, 0.10f, rgba(72, 36, 52), rgba(108, 46, 56), Item::NONE, 0, {}, 51},
         {Ridge::HILLS, 90, 0.30f, rgba(44, 24, 36), rgba(34, 18, 28), Item::CASTLE, 260, rgba(40, 22, 34), 15},
         Turf::OBSIDIAN, rgba(96, 62, 74), rgba(52, 38, 50), rgba(30, 22, 32), Weather::EMBERS},
        // 5 Bamboo Steppes
        {rgba(116, 170, 214), rgba(240, 206, 148), rgba(255, 230, 176), Sky::SUN, rgba(255, 238, 184), {1040, 200}, 50,
         false, false, 3, rgba(255, 250, 236, 210),
         {Ridge::HILLS, 220, 0.10f, rgba(170, 186, 136), rgba(196, 204, 156), Item::PAGODAS, 420, rgba(150, 166, 120), 61},
         {Ridge::HILLS, 100, 0.30f, rgba(90, 140, 80), rgba(70, 116, 66), Item::BAMBOO, 70, rgba(58, 112, 62), 17},
         Turf::GRASS, rgba(158, 196, 82), rgba(122, 96, 56), rgba(82, 62, 38), Weather::LEAVES},
        // 6 Sunken Ruins (misty jungle)
        {rgba(46, 96, 124), rgba(100, 166, 156), rgba(206, 232, 196), Sky::NONE, rgba(0, 0, 0), {0, 0}, 0,
         false, false, 2, rgba(232, 248, 240, 150),
         {Ridge::HILLS, 260, 0.10f, rgba(92, 150, 132), rgba(132, 186, 160), Item::NONE, 0, {}, 71},
         {Ridge::HILLS, 80, 0.30f, rgba(62, 108, 92), rgba(48, 88, 74), Item::COLUMNS, 150, rgba(70, 110, 98), 19},
         Turf::MOSS, rgba(96, 160, 84), rgba(84, 74, 52), rgba(52, 46, 34), Weather::FIREFLIES},
        // 7 Zephyr Highlands
        {rgba(64, 146, 240), rgba(136, 200, 252), rgba(222, 242, 255), Sky::SUN, rgba(255, 252, 230), {1100, 120}, 38,
         false, false, 8, rgba(255, 255, 255, 240),
         {Ridge::NONE, 0, 0.08f, rgba(156, 196, 226), rgba(156, 196, 226), Item::ISLANDS, 330, rgba(146, 186, 220), 81},
         {Ridge::HILLS, 140, 0.30f, rgba(104, 178, 112), rgba(80, 150, 92), Item::NONE, 0, {}, 23},
         Turf::GRASS, rgba(120, 204, 96), rgba(110, 86, 60), rgba(70, 54, 38), Weather::WIND},
        // 8 Glacial Bastion (bright snow)
        {rgba(104, 166, 230), rgba(178, 218, 246), rgba(236, 248, 255), Sky::SUN, rgba(255, 255, 240), {1050, 140}, 34,
         false, false, 3, rgba(255, 255, 255, 220),
         {Ridge::PEAKS, 340, 0.10f, rgba(160, 190, 228), rgba(196, 218, 240), Item::NONE, 0, {}, 91, 0.45f},
         {Ridge::PEAKS, 190, 0.30f, rgba(112, 152, 204), rgba(96, 132, 184), Item::NONE, 0, {}, 29, 0.5f},
         Turf::SNOW, rgba(240, 250, 255), rgba(150, 176, 210), rgba(100, 124, 160), Weather::SNOW},
        // 9 Magma Foundry
        {rgba(34, 22, 26), rgba(84, 36, 32), rgba(204, 84, 42), Sky::NONE, rgba(0, 0, 0), {0, 0}, 0,
         false, false, 0, rgba(0, 0, 0, 0),
         {Ridge::HILLS, 200, 0.10f, rgba(64, 38, 38), rgba(92, 46, 40), Item::NONE, 0, {}, 101},
         {Ridge::FLAT, 0, 0.30f, rgba(36, 26, 28), rgba(36, 26, 28), Item::CHIMNEYS, 190, rgba(40, 30, 32), 33},
         Turf::IRON, rgba(98, 84, 86), rgba(58, 48, 50), rgba(34, 28, 30), Weather::EMBERS},
        // 10 Thunderhead Peaks
        {rgba(40, 44, 72), rgba(82, 88, 122), rgba(150, 152, 176), Sky::NONE, rgba(0, 0, 0), {0, 0}, 0,
         false, false, 6, rgba(92, 96, 126, 230),
         {Ridge::PEAKS, 320, 0.10f, rgba(74, 78, 110), rgba(100, 104, 136), Item::NONE, 0, {}, 111},
         {Ridge::PEAKS, 170, 0.30f, rgba(50, 52, 78), rgba(40, 42, 64), Item::NONE, 0, {}, 37},
         Turf::ROCK, rgba(110, 118, 132), rgba(74, 78, 92), rgba(46, 48, 60), Weather::RAIN},
        // 11 Solstice Sanctuary (sunset)
        {rgba(84, 62, 150), rgba(238, 132, 116), rgba(255, 204, 126), Sky::SUN, rgba(255, 226, 150), {990, 330}, 78,
         false, false, 3, rgba(255, 200, 190, 200),
         {Ridge::HILLS, 200, 0.10f, rgba(188, 118, 132), rgba(222, 150, 140), Item::NONE, 0, {}, 121},
         {Ridge::FLAT, 0, 0.30f, rgba(120, 70, 92), rgba(120, 70, 92), Item::ZIGGURATS, 330, rgba(124, 72, 94), 41},
         Turf::SAND, rgba(232, 186, 112), rgba(170, 116, 76), rgba(114, 74, 52), Weather::PETALS},
        // 12 Emperor's Colosseum
        {rgba(98, 158, 222), rgba(196, 214, 230), rgba(252, 232, 200), Sky::SUN, rgba(255, 250, 230), {1080, 110}, 36,
         false, false, 4, rgba(255, 255, 255, 230),
         {Ridge::HILLS, 180, 0.10f, rgba(166, 182, 176), rgba(196, 206, 196), Item::NONE, 0, {}, 131},
         {Ridge::FLAT, 0, 0.18f, rgba(206, 164, 120), rgba(206, 164, 120), Item::ARCADE, 0, rgba(222, 196, 164), 43},
         Turf::SAND, rgba(232, 202, 146), rgba(186, 146, 98), rgba(130, 98, 64), Weather::DUST},
        // 13 Sovereign Sanctum (royal night)
        {rgba(14, 16, 46), rgba(46, 46, 116), rgba(138, 108, 180), Sky::MOON, rgba(244, 240, 220), {1080, 150}, 40,
         true, false, 0, rgba(0, 0, 0, 0),
         {Ridge::PEAKS, 280, 0.10f, rgba(58, 58, 108), rgba(82, 76, 136), Item::NONE, 0, {}, 141, 0.7f},
         {Ridge::HILLS, 70, 0.30f, rgba(30, 30, 64), rgba(24, 24, 52), Item::CASTLE, 210, rgba(34, 32, 70), 47},
         Turf::MARBLE, rgba(150, 146, 196), rgba(70, 68, 110), rgba(42, 40, 72), Weather::SPARKLES},
        // 14 Secret Truffle Vaults
        {rgba(24, 14, 42), rgba(70, 34, 96), rgba(190, 124, 58), Sky::GOLD_ECLIPSE, rgba(255, 215, 0), {1080, 165}, 32,
         true, false, 0, rgba(0, 0, 0, 0),
         {Ridge::HILLS, 160, 0.10f, rgba(56, 34, 76), rgba(78, 46, 96), Item::OBELISKS, 300, rgba(64, 38, 86), 151},
         {Ridge::HILLS, 70, 0.30f, rgba(40, 24, 58), rgba(32, 20, 46), Item::COLUMNS, 170, rgba(48, 30, 66), 53},
         Turf::VAULT, rgba(224, 178, 65), rgba(46, 32, 66), rgba(26, 18, 40), Weather::SPARKLES},
    };
    return T[std::clamp(i, 0, NUM_SCENERY_THEMES - 1)];
}

// ---------------------------------------------------------------- ridges

float ridgeHeight(const Layer& L, float u) {
    float s = static_cast<float>(L.seed);
    switch (L.ridge) {
    case Ridge::NONE:
    case Ridge::FLAT:
        return 0.0f;
    case Ridge::HILLS:
        return L.height * (0.55f + 0.25f * std::sin(u * 0.0042f + s) + 0.14f * std::sin(u * 0.0107f + s * 2.1f) +
                           0.06f * std::sin(u * 0.027f + s * 3.7f));
    case Ridge::PEAKS: {
        float v = 0.55f * tri(u / 520.0f + s * 0.13f) + 0.3f * tri(u / 210.0f + s * 0.37f) + 0.15f * tri(u / 77.0f + s);
        return L.height * (0.18f + 0.82f * v * v * 1.25f);
    }
    case Ridge::MESAS: {
        float v = 0.5f + 0.32f * std::sin(u * 0.0071f + s) + 0.18f * std::sin(u * 0.0173f + s * 1.7f);
        float plateau = smooth01(0.48f, 0.56f, v);
        float lower = smooth01(0.30f, 0.36f, v) * 0.35f;
        return L.height * (0.08f + std::max(plateau * 0.92f, lower)) +
               3.0f * std::sin(u * 0.09f + s);  // rubble texture
    }
    case Ridge::VOLCANO: {
        float base = L.height * (0.25f + 0.1f * std::sin(u * 0.006f + s));
        float cone = L.height * std::max(0.0f, 1.0f - std::abs(u - 760.0f) / 360.0f);
        cone = std::min(cone, L.height * 0.92f);  // crater rim
        return std::max(base, cone);
    }
    }
    return 0.0f;
}

void drawRidge(SDL_Renderer* ren, const Layer& L, float offset, float gy) {
    if (L.ridge == Ridge::NONE || L.ridge == Ridge::FLAT) return;
    const float step = 8.0f;
    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    std::vector<SDL_Vertex> snow;
    std::vector<int> sidx;
    float horizonTop = gy - L.height;
    SDL_Color snowCol{240, 248, 255, 235};
    for (float x = -PAD; x <= WINDOW_W + PAD + step; x += step) {
        float h = ridgeHeight(L, x + offset);
        float yTop = gy - h;
        // Colour by absolute height so the ridge reads as lit from above.
        float t = std::clamp((yTop - horizonTop) / std::max(1.0f, L.height), 0.0f, 1.0f);
        SDL_Color topCol = mix(L.top, L.bottom, t * 0.5f);
        int base = static_cast<int>(v.size());
        v.push_back({{x, yTop}, topCol, {0, 0}});
        v.push_back({{x, gy + 2}, L.bottom, {0, 0}});
        if (base >= 2) {
            for (int k : {base - 2, base, base - 1, base - 1, base, base + 1}) idx.push_back(k);
        }
        if (L.snowLine > 0.0f) {
            float snowH = L.height * L.snowLine;
            float depth = std::max(0.0f, h - snowH) * 0.55f;
            int sb = static_cast<int>(snow.size());
            snow.push_back({{x, yTop}, snowCol, {0, 0}});
            snow.push_back({{x, yTop + depth}, ui::alpha(snowCol, depth > 0.5f ? 0.85f : 0.0f), {0, 0}});
            if (sb >= 2) {
                for (int k : {sb - 2, sb, sb - 1, sb - 1, sb, sb + 1}) sidx.push_back(k);
            }
        }
    }
    SDL_RenderGeometry(ren, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
    if (!snow.empty()) {
        SDL_RenderGeometry(ren, nullptr, snow.data(), static_cast<int>(snow.size()), sidx.data(),
                           static_cast<int>(sidx.size()));
    }
}

// ---------------------------------------------------------------- items

void drawPine(SDL_Renderer* ren, float x, float by, float s, SDL_Color c, bool snowy) {
    rect(ren, x - 3 * s, by - 16 * s, 6 * s, 18 * s, shade(c, 0.6f));
    for (int k = 0; k < 3; ++k) {
        float w = (30 - k * 7) * s, y0 = by - (12 + k * 20) * s, y1 = y0 - 34 * s;
        drawTriangle(ren, {x - w, y0}, {x, y1}, {x + w, y0}, k == 0 ? shade(c, 0.9f) : c);
        if (snowy) drawTriangle(ren, {x - w * 0.42f, y1 + 14 * s}, {x, y1}, {x + w * 0.42f, y1 + 14 * s}, {236, 246, 255, 230});
    }
}

void drawRoundTree(SDL_Renderer* ren, float x, float by, float s, SDL_Color c, float h) {
    rect(ren, x - 3.5f * s, by - 30 * s, 7 * s, 32 * s, rgba(84, 52, 38));
    SDL_Color c2 = h < 0.33f ? c : (h < 0.66f ? mix(c, rgba(236, 170, 60), 0.6f) : mix(c, rgba(170, 52, 40), 0.6f));
    fillCircle(ren, {x - 14 * s, by - 40 * s}, 18 * s, shade(c2, 0.85f), 16);
    fillCircle(ren, {x + 14 * s, by - 42 * s}, 18 * s, shade(c2, 0.9f), 16);
    fillCircle(ren, {x, by - 58 * s}, 22 * s, c2, 18);
    fillCircle(ren, {x - 6 * s, by - 64 * s}, 9 * s, tint(c2, 0.2f), 12);
}

void drawBamboo(SDL_Renderer* ren, float x, float by, float s, SDL_Color c, float h, float t) {
    for (int k = 0; k < 3; ++k) {
        float sx = x + (k - 1) * 11 * s;
        float height = (150 + 70 * hash01(k, static_cast<uint32_t>(h * 1000)) ) * s;
        float sway = std::sin(t * 0.8f + x * 0.01f + k) * 4.0f * s;
        SDL_Color sc = k == 1 ? c : shade(c, 0.85f);
        drawConvexPoly(ren, {{sx - 3 * s, by}, {sx + 3 * s, by}, {sx + 3 * s + sway, by - height}, {sx - 3 * s + sway, by - height}}, sc);
        for (float ny = 30 * s; ny < height; ny += 34 * s) {
            float f = ny / height;
            drawThickLine(ren, {sx - 4 * s + sway * f, by - ny}, {sx + 4 * s + sway * f, by - ny}, 2.0f, shade(sc, 0.7f));
        }
        Vec2 tip(sx + sway, by - height);
        drawTriangle(ren, tip, tip + Vec2(-22 * s, 8 * s), tip + Vec2(-4 * s, 14 * s), tint(sc, 0.1f));
        drawTriangle(ren, tip, tip + Vec2(22 * s, 4 * s), tip + Vec2(4 * s, 12 * s), tint(sc, 0.1f));
    }
}

void drawPagoda(SDL_Renderer* ren, float x, float by, float s, SDL_Color c) {
    float y = by;
    for (int k = 0; k < 4; ++k) {
        float w = (34 - k * 6) * s, bh = 18 * s;
        rect(ren, x - w * 0.6f, y - bh, w * 1.2f, bh, shade(c, 0.85f));
        y -= bh;
        float rw = w + 14 * s;
        drawConvexPoly(ren, {{x - rw, y + 2 * s}, {x + rw, y + 2 * s}, {x + w * 0.7f, y - 9 * s}, {x - w * 0.7f, y - 9 * s}}, c);
        y -= 9 * s;
    }
    drawThickLine(ren, {x, y}, {x, y - 22 * s}, 2.5f * s, c);
}

void drawColumns(SDL_Renderer* ren, float x, float by, float s, SDL_Color c, float h) {
    int n = 1 + static_cast<int>(h * 3);
    float hh[3];
    for (int k = 0; k < n; ++k) {
        float cx = x + k * 40 * s;
        hh[k] = (90 + 80 * hash01(k + 7, static_cast<uint32_t>(h * 977))) * s;
        rect(ren, cx - 10 * s, by - hh[k], 20 * s, hh[k], c);
        rect(ren, cx - 14 * s, by - hh[k] - 8 * s, 28 * s, 8 * s, tint(c, 0.08f));
        for (int f = -1; f <= 1; ++f) rect(ren, cx + f * 6 * s - 1, by - hh[k] + 4, 2, hh[k] - 8, shade(c, 0.82f));
        if (hash01(k, static_cast<uint32_t>(h * 313)) < 0.35f) {  // broken top
            drawTriangle(ren, {cx - 14 * s, by - hh[k] - 8 * s}, {cx + 14 * s, by - hh[k] - 8 * s},
                         {cx + 6 * s, by - hh[k] - 20 * s}, tint(c, 0.08f));
        }
    }
    if (n >= 2) {
        float top = std::min(hh[0], hh[1]);
        rect(ren, x - 16 * s, by - top - 20 * s, 72 * s, 12 * s, shade(c, 0.95f));
    }
}

void drawChimney(SDL_Renderer* ren, float x, float by, float s, SDL_Color c, float h, float t) {
    float H = (160 + 110 * h) * s, W = (30 + 16 * h) * s;
    // Factory hall at the base.
    rect(ren, x - 60 * s, by - 70 * s, 120 * s, 70 * s, shade(c, 0.95f));
    drawConvexPoly(ren, {{x - 64 * s, by - 70 * s}, {x - 30 * s, by - 92 * s}, {x + 4 * s, by - 70 * s}}, c);
    drawConvexPoly(ren, {{x + 4 * s, by - 70 * s}, {x + 34 * s, by - 92 * s}, {x + 64 * s, by - 70 * s}}, c);
    for (int k = 0; k < 4; ++k) {
        float glow = 0.75f + 0.25f * std::sin(t * 3.0f + k + x * 0.1f);
        rect(ren, x - 48 * s + k * 26 * s, by - 50 * s, 14 * s, 18 * s, rgba(255, static_cast<int>(140 * glow), 50, 230));
    }
    // Stack.
    drawConvexPoly(ren, {{x + 20 * s, by - 60 * s}, {x + 20 * s + W, by - 60 * s}, {x + 16 * s + W, by - H}, {x + 24 * s, by - H}}, c);
    rect(ren, x + 18 * s, by - H - 8 * s, W + 4 * s, 8 * s, tint(c, 0.1f));
    rect(ren, x + 22 * s, by - H + 18 * s, W - 4 * s, 5 * s, rgba(200, 70, 40, 200));
    // Smoke.
    for (int k = 0; k < 5; ++k) {
        float ph = std::fmod(t * 0.25f + k * 0.2f + h, 1.0f);
        Vec2 p(x + 20 * s + W * 0.5f + ph * 70 * s, by - H - 14 * s - ph * 120 * s);
        fillCircle(ren, p, (10 + ph * 26) * s, rgba(90, 76, 78, static_cast<int>(130 * (1.0f - ph))), 16);
    }
}

void drawZiggurat(SDL_Renderer* ren, float x, float by, float s, SDL_Color c) {
    float y = by;
    for (int k = 0; k < 5; ++k) {
        float w = (130 - k * 22) * s, h = 22 * s;
        rect(ren, x - w * 0.5f, y - h, w, h, k % 2 ? c : shade(c, 0.92f));
        y -= h;
    }
    rect(ren, x - 14 * s, y - 26 * s, 28 * s, 26 * s, shade(c, 0.9f));
    rect(ren, x - 6 * s, y - 18 * s, 12 * s, 18 * s, rgba(255, 210, 130, 200));
    rect(ren, x - 4 * s, by - 100 * s, 8 * s, 100 * s, shade(c, 0.85f));  // stair
}

void drawCastle(SDL_Renderer* ren, float x, float by, float s, SDL_Color c, float h, SDL_Color windowCol) {
    float H = (110 + 120 * h) * s, W = 28 * s;
    rect(ren, x - W * 0.5f, by - H, W, H, c);
    drawTriangle(ren, {x - W * 0.75f, by - H}, {x, by - H - 46 * s}, {x + W * 0.75f, by - H}, shade(c, 0.85f));
    drawThickLine(ren, {x, by - H - 46 * s}, {x, by - H - 62 * s}, 1.5f, c);
    drawTriangle(ren, {x, by - H - 62 * s}, {x + 12 * s, by - H - 58 * s}, {x, by - H - 54 * s}, rgba(220, 60, 60));
    for (int k = 0; k < 3; ++k) {
        if (hash01(k, static_cast<uint32_t>(h * 2000)) < 0.6f) {
            rect(ren, x - 3 * s, by - H + (20 + k * 30) * s, 6 * s, 10 * s, windowCol);
        }
    }
    // Curtain wall to the right with crenellations.
    float wallH = 60 * s;
    rect(ren, x + W * 0.5f, by - wallH, 90 * s, wallH, shade(c, 0.95f));
    for (int k = 0; k < 6; ++k) rect(ren, x + W * 0.5f + k * 16 * s, by - wallH - 8 * s, 9 * s, 8 * s, shade(c, 0.95f));
}

void drawObelisk(SDL_Renderer* ren, float x, float by, float s, SDL_Color c, float t, float h) {
    float H = (150 + 70 * h) * s;
    drawConvexPoly(ren, {{x - 15 * s, by}, {x + 15 * s, by}, {x + 10 * s, by - H}, {x - 10 * s, by - H}}, c);
    drawTriangle(ren, {x - 10 * s, by - H}, {x, by - H - 22 * s}, {x + 10 * s, by - H}, tint(c, 0.15f));
    float glow = 0.6f + 0.4f * std::sin(t * 2.0f + h * 10.0f);
    fillCircle(ren, {x, by - H * 0.6f}, 5 * s, rgba(255, 215, 80, static_cast<int>(220 * glow)), 10);
    fillCircle(ren, {x, by - H - 6 * s}, 14 * s, rgba(255, 215, 80, static_cast<int>(60 * glow)), 14);
}

void drawIsland(SDL_Renderer* ren, float x, float y, float s, SDL_Color c, SDL_Color grass) {
    drawConvexPoly(ren, {{x - 70 * s, y}, {x + 70 * s, y}, {x + 30 * s, y + 46 * s}, {x - 10 * s, y + 70 * s}, {x - 40 * s, y + 40 * s}}, c);
    ui::fillRoundRect(ren, {x - 74 * s, y - 8 * s, 148 * s, 14 * s}, 7 * s, grass);
    drawPine(ren, x - 30 * s, y - 4 * s, 0.7f * s, shade(grass, 0.7f), false);
    drawPine(ren, x + 22 * s, y - 4 * s, 0.55f * s, shade(grass, 0.75f), false);
}

void drawArcade(SDL_Renderer* ren, float offset, float gy, SDL_Color c) {
    // Colosseum: two tiers of arches running across the horizon.
    const float tierH = 58.0f, archW = 48.0f;
    float top = gy - tierH * 2 - 24;
    rect(ren, -PAD, top, WINDOW_W + 2 * PAD, gy - top, c);
    rect(ren, -PAD, top - 10, WINDOW_W + 2 * PAD, 12, tint(c, 0.12f));
    SDL_Color hole = shade(c, 0.62f), rim = tint(c, 0.1f);
    float start = -std::fmod(offset, archW) - archW;
    for (int tier = 0; tier < 2; ++tier) {
        float baseY = gy - tier * (tierH + 12);
        rect(ren, -PAD, baseY - tierH - 12, WINDOW_W + 2 * PAD, 8, rim);
        for (float ax = start; ax < WINDOW_W + archW; ax += archW) {
            float w = archW * 0.56f, cx = ax + archW * 0.5f;
            float openTop = baseY - tierH + w * 0.5f;
            rect(ren, cx - w * 0.5f, openTop, w, baseY - openTop - 2, hole);
            std::vector<Vec2> arch;
            for (int k = 0; k <= 8; ++k) {
                float a = PI + PI * k / 8;
                arch.push_back({cx + std::cos(a) * w * 0.5f, openTop + std::sin(a) * w * 0.5f});
            }
            drawConvexPoly(ren, arch, hole);
        }
    }
    // Pennants along the top.
    for (float ax = start; ax < WINDOW_W + archW; ax += archW * 2) {
        drawThickLine(ren, {ax, top - 10}, {ax, top - 40}, 2.0f, shade(c, 0.6f));
        drawTriangle(ren, {ax, top - 40}, {ax + 18, top - 34}, {ax, top - 28}, rgba(200, 60, 60));
    }
}

void drawItems(SDL_Renderer* ren, const Layer& L, float offset, float gy, float t) {
    if (L.item == Item::NONE) return;
    if (L.item == Item::ARCADE) {
        drawArcade(ren, offset, gy, L.itemCol);
        return;
    }
    float sp = L.spacing;
    int i0 = static_cast<int>(std::floor((offset - PAD - 200) / sp));
    int i1 = static_cast<int>(std::ceil((offset + WINDOW_W + PAD + 200) / sp));
    for (int i = i0; i <= i1; ++i) {
        float h = hash01(i, L.seed);
        float u = i * sp + (hash01(i, L.seed + 99) - 0.5f) * sp * 0.6f;
        float x = u - offset;
        float by = gy - ridgeHeight(L, u) + 4;
        float s = 0.8f + 0.4f * hash01(i, L.seed + 7);
        switch (L.item) {
        case Item::PINES:
            drawPine(ren, x, by, s, L.itemCol, false);
            if (h > 0.5f) drawPine(ren, x + 26 * s, by + 2, s * 0.75f, shade(L.itemCol, 0.9f), false);
            break;
        case Item::SNOWY_PINES:
            drawPine(ren, x, by, s, L.itemCol, true);
            break;
        case Item::ROUND_TREES:
            drawRoundTree(ren, x, by, s, L.itemCol, h);
            break;
        case Item::BAMBOO:
            if (h > 0.25f) drawBamboo(ren, x, by, s, L.itemCol, h, t);
            break;
        case Item::PAGODAS:
            if (h > 0.4f) drawPagoda(ren, x, by, s * 1.1f, L.itemCol);
            break;
        case Item::COLUMNS:
            if (h > 0.3f) drawColumns(ren, x, by, s, L.itemCol, h);
            break;
        case Item::CHIMNEYS:
            drawChimney(ren, x, by, s, L.itemCol, h, t);
            break;
        case Item::ZIGGURATS:
            if (h > 0.25f) drawZiggurat(ren, x, by, s, L.itemCol);
            break;
        case Item::CASTLE:
            drawCastle(ren, x, by, s, L.itemCol, h, rgba(255, 206, 110, 220));
            break;
        case Item::OBELISKS:
            if (h > 0.3f) drawObelisk(ren, x, by, s, L.itemCol, t, h);
            break;
        case Item::ISLANDS: {
            float y = 220 + 120 * hash01(i, L.seed + 3) + std::sin(t * 0.6f + i) * 6.0f;
            drawIsland(ren, x, y, s, L.itemCol, rgba(132, 200, 120));
            break;
        }
        case Item::WINDMILL:
        case Item::NONE:
        case Item::ARCADE:
            break;
        }
    }
}

// ---------------------------------------------------------------- sky

void drawCloud(SDL_Renderer* ren, Vec2 p, float s, SDL_Color c) {
    SDL_Color shadeC = shade(c, 0.88f);
    shadeC.a = c.a;
    ui::fillRoundRect(ren, {p.x - 70 * s, p.y - 10 * s, 140 * s, 30 * s}, 15 * s, shadeC);
    fillCircle(ren, {p.x - 40 * s, p.y}, 26 * s, shadeC, 20);
    fillCircle(ren, {p.x + 4 * s, p.y - 16 * s}, 36 * s, c, 24);
    fillCircle(ren, {p.x + 44 * s, p.y - 2 * s}, 24 * s, c, 20);
    fillCircle(ren, {p.x - 30 * s, p.y - 8 * s}, 22 * s, c, 20);
    ui::fillRoundRect(ren, {p.x - 62 * s, p.y - 6 * s, 124 * s, 22 * s}, 11 * s, c);
}

void drawCelestial(SDL_Renderer* ren, const Theme& th, Vec2 off, float t) {
    Vec2 c = th.celPos + off;
    float r = th.celR;
    switch (th.sky) {
    case Sky::NONE:
        return;
    case Sky::SUN:
        fillCircleGrad(ren, c, r * 3.2f, ui::alpha(th.celestial, 0.28f), ui::alpha(th.celestial, 0.0f), 40);
        fillCircleGrad(ren, c, r * 1.7f, ui::alpha(th.celestial, 0.45f), ui::alpha(th.celestial, 0.0f), 36);
        fillCircleGrad(ren, c, r, tint(th.celestial, 0.6f), th.celestial, 36);
        break;
    case Sky::MOON:
        fillCircleGrad(ren, c, r * 2.6f, ui::alpha(th.celestial, 0.18f), ui::alpha(th.celestial, 0.0f), 36);
        fillCircle(ren, c, r, th.celestial, 36);
        fillCircle(ren, c + Vec2(-r * 0.3f, -r * 0.2f), r * 0.22f, shade(th.celestial, 0.88f), 16);
        fillCircle(ren, c + Vec2(r * 0.25f, r * 0.3f), r * 0.16f, shade(th.celestial, 0.9f), 14);
        break;
    case Sky::CRESCENT:
        fillCircleGrad(ren, c, r * 2.6f, ui::alpha(th.celestial, 0.16f), ui::alpha(th.celestial, 0.0f), 36);
        fillCircle(ren, c, r, th.celestial, 36);
        fillCircle(ren, c + Vec2(r * 0.42f, -r * 0.22f), r * 0.86f, th.skyTop, 36);
        break;
    case Sky::TWIN_MOONS:
        fillCircle(ren, c, r, th.celestial, 30);
        fillCircle(ren, c + Vec2(80, 50), r * 0.5f, shade(th.celestial, 0.9f), 24);
        break;
    case Sky::ECLIPSE:
    case Sky::GOLD_ECLIPSE: {
        SDL_Color corona = th.celestial;
        fillCircleGrad(ren, c, r * 3.4f, ui::alpha(corona, 0.35f), ui::alpha(corona, 0.0f), 40);
        for (int k = 0; k < 12; ++k) {
            float a = t * 0.15f + k * PI / 6.0f;
            float len = r * (1.6f + 0.35f * std::sin(t * 1.3f + k * 1.7f));
            drawTriangle(ren, c + Vec2(std::cos(a - 0.12f), std::sin(a - 0.12f)) * r,
                         c + Vec2(std::cos(a), std::sin(a)) * len,
                         c + Vec2(std::cos(a + 0.12f), std::sin(a + 0.12f)) * r, ui::alpha(corona, 0.55f));
        }
        fillCircle(ren, c, r * 1.08f, tint(corona, 0.4f), 36);
        fillCircle(ren, c, r, th.sky == Sky::GOLD_ECLIPSE ? rgba(30, 16, 46) : rgba(24, 14, 24), 36);
        break;
    }
    }
}

void drawAurora(SDL_Renderer* ren, Vec2 off, float t) {
    const SDL_Color cols[3] = {rgba(70, 240, 180, 90), rgba(80, 190, 250, 70), rgba(160, 110, 250, 60)};
    for (int band = 0; band < 3; ++band) {
        std::vector<SDL_Vertex> v;
        std::vector<int> idx;
        for (float x = -PAD; x <= WINDOW_W + PAD; x += 24.0f) {
            float y = 90 + band * 40 + off.y + std::sin(t * 0.7f + x * 0.006f + band * 1.3f) * 24.0f +
                      std::sin(x * 0.017f + band) * 10.0f;
            float hgt = 60 + 30 * std::sin(x * 0.01f + t * 0.5f + band);
            SDL_Color c = cols[band];
            int b = static_cast<int>(v.size());
            v.push_back({{x + off.x, y - hgt}, ui::alpha(c, 0.0f), {0, 0}});
            v.push_back({{x + off.x, y}, c, {0, 0}});
            if (b >= 2) {
                for (int k : {b - 2, b, b - 1, b - 1, b, b + 1}) idx.push_back(k);
            }
        }
        SDL_RenderGeometry(ren, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
    }
}

// ---------------------------------------------------------------- ground

void drawTurf(SDL_Renderer* ren, const Theme& th, float gy, float camX, float t) {
    float bottom = WINDOW_H + PAD;
    fillRectGrad(ren, {-PAD, gy, WINDOW_W + 2 * PAD, bottom - gy}, th.soilTop, th.soilBot);
    // Speckled soil texture (scrolls with the camera).
    for (int i = 0; i < 60; ++i) {
        float u = i * 53.0f;
        float x = std::fmod(u - camX + 10000.0f * 53.0f, 60 * 53.0f) - PAD;
        if (x > WINDOW_W + PAD) continue;
        float y = gy + 22 + hash01(i, 5) * 70;
        float r = 2 + hash01(i, 9) * 3;
        fillCircle(ren, {x, y}, r, shade(th.soilTop, 0.8f), 8);
    }
    SDL_Color lip = th.turfTop;
    SDL_Color lipDark = shade(lip, 0.72f);
    rect(ren, -PAD, gy, WINDOW_W + 2 * PAD, 12, lip);
    rect(ren, -PAD, gy + 12, WINDOW_W + 2 * PAD, 5, lipDark);
    // Tufts / drifts / tiles on top of the lip.
    float period = 26.0f;
    float start = -std::fmod(camX, period) - PAD;
    int n = 0;
    for (float x = start; x < WINDOW_W + PAD; x += period, ++n) {
        int id = static_cast<int>(std::floor((x + camX) / period));
        float h = hash01(id, 17);
        switch (th.turf) {
        case Turf::GRASS:
        case Turf::AUTUMN:
        case Turf::MOSS:
            drawTriangle(ren, {x, gy + 2}, {x + 5 + h * 4, gy - 6 - h * 6}, {x + 11, gy + 2}, tint(lip, 0.1f));
            if (h > 0.6f) fillCircle(ren, {x + 16, gy - 2}, 2.5f, th.turf == Turf::AUTUMN ? rgba(230, 90, 50) : rgba(255, 236, 120), 8);
            break;
        case Turf::SNOW:
            fillCircle(ren, {x + 12, gy + 4}, 10 + h * 6, lip, 14);
            break;
        case Turf::SAND:
            if (h > 0.7f) fillCircle(ren, {x + 10, gy + 3}, 4, shade(lip, 0.85f), 8);
            break;
        case Turf::OBSIDIAN:
        case Turf::IRON: {
            float glow = 0.6f + 0.4f * std::sin(t * 2.0f + id);
            if (h > 0.75f) drawThickLine(ren, {x, gy + 20 + h * 30}, {x + 18, gy + 30 + h * 20}, 2.5f,
                                         rgba(255, 120, 40, static_cast<int>(200 * glow)));
            break;
        }
        case Turf::ROCK:
            if (h > 0.5f) drawTriangle(ren, {x, gy + 2}, {x + 8, gy - 4 - h * 4}, {x + 16, gy + 2}, shade(lip, 0.9f));
            break;
        case Turf::MARBLE:
        case Turf::VAULT:
            if (id % 3 == 0) rect(ren, x, gy, 2, 12, lipDark);
            break;
        }
    }
    if (th.turf == Turf::VAULT || th.turf == Turf::MARBLE) {
        rect(ren, -PAD, gy, WINDOW_W + 2 * PAD, 3, tint(lip, 0.35f));
    }
    (void)n;
}

// ---------------------------------------------------------------- weather

void drawWeather(SDL_Renderer* ren, Weather w, float gy, float t) {
    if (w == Weather::NONE) return;
    int count = (w == Weather::RAIN) ? 90 : (w == Weather::WIND ? 10 : 26);
    for (int i = 0; i < count; ++i) {
        float a = hash01(i, 3), b = hash01(i, 4), c = hash01(i, 5);
        float span = WINDOW_W + 80.0f;
        switch (w) {
        case Weather::RAIN: {
            float x = std::fmod(a * span + t * 260.0f, span) - 40;
            float y = std::fmod(b * gy + t * (700.0f + c * 200), gy);
            drawThickLine(ren, {x, y}, {x - 6, y + 18}, 1.6f, rgba(190, 210, 240, 110));
            break;
        }
        case Weather::SNOW: {
            float x = std::fmod(a * span + std::sin(t * 0.8f + i) * 30 + t * 12, span) - 40;
            float y = std::fmod(b * gy + t * (30 + c * 30), gy);
            fillCircle(ren, {x, y}, 2.0f + c * 2.5f, rgba(255, 255, 255, 220), 8);
            break;
        }
        case Weather::LEAVES:
        case Weather::PETALS: {
            float x = std::fmod(a * span + t * (30 + c * 20), span) - 40;
            float y = std::fmod(b * gy + t * (34 + c * 20), gy);
            float rot = t * 2.0f + i;
            SDL_Color col = (w == Weather::PETALS) ? rgba(255, 190, 210, 210)
                                                   : (i % 2 ? rgba(230, 120, 50, 220) : rgba(240, 180, 60, 220));
            Vec2 d(std::cos(rot) * 5, std::sin(rot) * 2.5f);
            drawTriangle(ren, {x - d.x, y - d.y}, {x + d.y, y - d.x * 0.5f}, {x + d.x, y + d.y}, col);
            drawTriangle(ren, {x - d.x, y - d.y}, {x - d.y, y + d.x * 0.5f}, {x + d.x, y + d.y}, shade(col, 0.85f));
            break;
        }
        case Weather::EMBERS: {
            float x = std::fmod(a * span + std::sin(t + i) * 20, span) - 40;
            float y = gy - std::fmod(b * gy + t * (30 + c * 40), gy);
            fillCircle(ren, {x, y}, 4.5f, rgba(255, 120, 40, 60), 8);
            fillCircle(ren, {x, y}, 2.0f, rgba(255, 200, 90, 230), 6);
            break;
        }
        case Weather::SPARKLES:
        case Weather::FIREFLIES: {
            float x = std::fmod(a * span + std::sin(t * 0.5f + i) * 40, span) - 40;
            float y = gy - 40 - std::fmod(b * (gy - 120) + t * (8 + c * 10), gy - 120);
            float glow = 0.5f + 0.5f * std::sin(t * 3.0f + i * 1.7f);
            SDL_Color col = (w == Weather::FIREFLIES) ? rgba(220, 255, 120) : rgba(255, 220, 110);
            fillCircle(ren, {x, y}, 6.0f, ui::alpha(col, 0.25f * glow), 10);
            fillCircle(ren, {x, y}, 2.0f, ui::alpha(col, glow), 6);
            break;
        }
        case Weather::POLLEN:
        case Weather::DUST: {
            float x = std::fmod(a * span + t * (14 + c * 10), span) - 40;
            float y = 80 + std::fmod(b * (gy - 80) + std::sin(t + i) * 10, gy - 80);
            fillCircle(ren, {x, y}, 1.8f + c, w == Weather::POLLEN ? rgba(255, 250, 200, 170) : rgba(240, 200, 150, 120), 6);
            break;
        }
        case Weather::WIND: {
            float x = std::fmod(a * span * 2 + t * (240 + c * 120), span * 2) - span * 0.5f;
            float y = 120 + b * (gy - 260);
            drawThickLine(ren, {x, y}, {x + 90 + c * 60, y}, 2.0f, rgba(255, 255, 255, 70));
            break;
        }
        case Weather::NONE:
            break;
        }
    }
}

}  // namespace

void renderScenery(SDL_Renderer* ren, int themeIdx, Vec2 cam, float t) {
    const Theme& th = theme(themeIdx);
    float gy = GROUND_Y - cam.y;

    // Sky.
    SDL_SetRenderDrawColor(ren, th.skyTop.r, th.skyTop.g, th.skyTop.b, 255);
    SDL_RenderClear(ren);
    float midY = gy * 0.55f;
    fillRectGrad(ren, {-PAD, -PAD, WINDOW_W + 2 * PAD, midY + PAD}, th.skyTop, th.skyMid);
    fillRectGrad(ren, {-PAD, midY, WINDOW_W + 2 * PAD, gy - midY + 2}, th.skyMid, th.skyBot);

    Vec2 celOff(-cam.x * 0.04f, -cam.y * 0.04f);
    if (th.stars) {
        for (int i = 0; i < 70; ++i) {
            float x = hash01(i, 77) * (WINDOW_W + 200) - 100 + celOff.x;
            float y = hash01(i, 78) * gy * 0.62f + celOff.y;
            float tw = 0.55f + 0.45f * std::sin(t * (1.0f + hash01(i, 79) * 2.0f) + i);
            float r = 0.8f + hash01(i, 80) * 1.6f;
            fillCircle(ren, {x, y}, r, rgba(255, 250, 230, static_cast<int>(200 * tw)), 6);
        }
    }
    if (th.aurora) drawAurora(ren, celOff, t);
    drawCelestial(ren, th, celOff, t);

    // Clouds drift slowly and parallax a little.
    for (int i = 0; i < th.clouds; ++i) {
        float s = 0.6f + hash01(i, 33) * 0.7f;
        float speed = 6.0f + hash01(i, 34) * 10.0f;
        float span = WINDOW_W + 400.0f;
        float x = std::fmod(hash01(i, 35) * span + t * speed - cam.x * 0.06f + span * 10.0f, span) - 200.0f;
        float y = 90.0f + hash01(i, 36) * 200.0f - cam.y * 0.05f;
        drawCloud(ren, {x, y}, s, th.cloudCol);
    }

    // Far layer, then a haze band so it sits back in the distance.
    float farOff = cam.x * th.far.parallax;
    drawRidge(ren, th.far, farOff, gy);
    drawItems(ren, th.far, farOff, gy, t);
    fillRectGrad(ren, {-PAD, gy - 220, WINDOW_W + 2 * PAD, 222}, ui::alpha(th.skyBot, 0.0f), ui::alpha(th.skyBot, 0.45f));

    // Mid layer.
    float midOff = cam.x * th.mid.parallax;
    drawRidge(ren, th.mid, midOff, gy);
    drawItems(ren, th.mid, midOff, gy, t);
    if (themeIdx == 0) {
        // The meadow's windmill on its hill.
        float u = 300.0f;
        float x = u - midOff, by = gy - ridgeHeight(th.mid, u) + 6;
        drawConvexPoly(ren, {{x - 14, by}, {x + 14, by}, {x + 9, by - 70}, {x - 9, by - 70}}, rgba(240, 228, 206));
        drawTriangle(ren, {x - 13, by - 70}, {x, by - 88}, {x + 13, by - 70}, rgba(196, 90, 70));
        for (int b = 0; b < 4; ++b) {
            float a = t * 0.9f + b * PI * 0.5f;
            Vec2 hub(x, by - 74), tip = hub + Vec2(std::cos(a), std::sin(a)) * 46.0f;
            drawThickLine(ren, hub, tip, 3.0f, rgba(250, 244, 228));
            Vec2 n(-std::sin(a), std::cos(a));
            drawConvexPoly(ren, {hub + (tip - hub) * 0.3f, tip, tip + n * 9.0f, hub + (tip - hub) * 0.3f + n * 9.0f},
                           rgba(250, 244, 228, 220));
        }
    }
    fillRectGrad(ren, {-PAD, gy - 70, WINDOW_W + 2 * PAD, 72}, ui::alpha(th.skyBot, 0.0f), ui::alpha(th.skyBot, 0.18f));

    drawWeather(ren, th.weather, gy, t);
    drawTurf(ren, th, gy, cam.x, t);
}

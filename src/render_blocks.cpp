// Fortress blocks and the golden truffle, drawn as outlined, shaded vector
// art to match the characters. Custom BMP textures can still be used with
// the V key (AssetPipeline::useSpriteTextures).
#include "game.h"

namespace {

using ui::mix;
using ui::shade;
using ui::tint;

uint32_t blockSeed(const BlockBody& b) {
    // Stable per block for its lifetime (size and toughness never change).
    uint32_t h = static_cast<uint32_t>(b.w * 131.0f) * 2654435761u ^ static_cast<uint32_t>(b.h * 977.0f) * 40503u ^
                 static_cast<uint32_t>(b.maxHp * 7.0f) * 97u ^ static_cast<uint32_t>(b.mat) * 7919u;
    return h ? h : 1u;
}

float rand01(uint32_t& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (s & 0xFFFFFF) / static_cast<float>(0x1000000);
}

// Local-space drawing helper for one (rotated) block.
struct BlockPainter {
    SDL_Renderer* ren;
    Vec2 c;
    float cs, sn, hw, hh;

    Vec2 P(float x, float y) const { return {c.x + x * cs - y * sn, c.y + x * sn + y * cs}; }

    std::vector<Vec2> rrect(float x0, float y0, float x1, float y1, float rad) const {
        rad = std::max(0.0f, std::min(rad, std::min(x1 - x0, y1 - y0) * 0.5f));
        std::vector<Vec2> pts;
        const Vec2 cen[4] = {{x0 + rad, y0 + rad}, {x1 - rad, y0 + rad}, {x1 - rad, y1 - rad}, {x0 + rad, y1 - rad}};
        int segs = rad > 2.0f ? 3 : 0;
        for (int k = 0; k < 4; ++k) {
            float a0 = PI + k * PI * 0.5f;
            for (int i = 0; i <= segs; ++i) {
                float a = a0 + (PI * 0.5f) * (segs ? static_cast<float>(i) / segs : 0.5f);
                pts.push_back({cen[k].x + std::cos(a) * rad, cen[k].y + std::sin(a) * rad});
            }
        }
        return pts;
    }

    // Vertical (local) gradient fill of a convex local polygon.
    void fillGrad(const std::vector<Vec2>& pts, SDL_Color top, SDL_Color bot) const {
        float y0 = 1e9f, y1 = -1e9f;
        Vec2 ctr(0, 0);
        for (const auto& p : pts) {
            y0 = std::min(y0, p.y);
            y1 = std::max(y1, p.y);
            ctr = ctr + p;
        }
        ctr = ctr * (1.0f / pts.size());
        auto colAt = [&](float y) { return mix(top, bot, (y - y0) / std::max(1.0f, y1 - y0)); };
        std::vector<SDL_Vertex> v;
        std::vector<int> idx;
        Vec2 w = P(ctr.x, ctr.y);
        v.push_back({{w.x, w.y}, colAt(ctr.y), {0, 0}});
        for (const auto& p : pts) {
            Vec2 q = P(p.x, p.y);
            v.push_back({{q.x, q.y}, colAt(p.y), {0, 0}});
        }
        int n = static_cast<int>(pts.size());
        for (int i = 0; i < n; ++i) {
            idx.push_back(0);
            idx.push_back(1 + i);
            idx.push_back(1 + (i + 1) % n);
        }
        SDL_RenderGeometry(ren, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
    }

    void fill(const std::vector<Vec2>& pts, SDL_Color col) const { fillGrad(pts, col, col); }

    void line(float x0, float y0, float x1, float y1, float w, SDL_Color col) const {
        drawThickLine(ren, P(x0, y0), P(x1, y1), w, col);
    }

    void dot(float x, float y, float r, SDL_Color col) const { ui::fillCircle(ren, P(x, y), r, col, 10); }

    void rect(float x0, float y0, float x1, float y1, SDL_Color col) const {
        fill({{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}, col);
    }
};

// A jagged crack from (x0,y0) towards (x1,y1), with a lighter edge.
void crack(const BlockPainter& g, float x0, float y0, float x1, float y1, uint32_t& seed, SDL_Color dark,
           SDL_Color lightEdge, float w) {
    const int segs = 5;
    float px = x0, py = y0;
    float nx = -(y1 - y0), ny = x1 - x0;
    float len = std::max(1.0f, std::sqrt(nx * nx + ny * ny));
    nx /= len;
    ny /= len;
    for (int i = 1; i <= segs; ++i) {
        float t = static_cast<float>(i) / segs;
        float j = (i == segs) ? 0.0f : (rand01(seed) - 0.5f) * len * 0.22f;
        float qx = x0 + (x1 - x0) * t + nx * j, qy = y0 + (y1 - y0) * t + ny * j;
        g.line(px + 0.8f, py + 0.8f, qx + 0.8f, qy + 0.8f, w * 0.7f, lightEdge);
        g.line(px, py, qx, qy, w, dark);
        px = qx;
        py = qy;
    }
}

}  // namespace

void SnoutAndSiegeGame::renderBlock(SDL_Renderer* ren, const BlockBody& b, Vec2 cam) {
    Vec2 sp = b.pos - cam;
    BlockPainter g{ren, sp, std::cos(b.angle), std::sin(b.angle), b.w * 0.5f, b.h * 0.5f};
    const float hw = g.hw, hh = g.hh;
    const float t = SDL_GetTicks() * 0.001f;
    uint32_t seed = blockSeed(b);
    float rad = std::min({4.0f, hw * 0.4f, hh * 0.4f});

    // Soft contact shadow.
    {
        BlockPainter sg = g;
        sg.c = sp + Vec2(3.0f, 4.0f);
        sg.fill(sg.rrect(-hw, -hh, hw, hh, rad + 1), {10, 10, 22, 70});
    }

    SDL_Texture* tex = nullptr;
    if (assets.useSpriteTextures) {
        const char* key = (b.mat == MaterialTier::TWIG) ? "sprite.mat_twig" :
                          (b.mat == MaterialTier::TIMBER) ? "sprite.mat_timber" :
                          (b.mat == MaterialTier::STONE) ? "sprite.mat_stone" :
                          (b.mat == MaterialTier::ICE) ? "sprite.mat_ice" :
                          (b.mat == MaterialTier::TNT) ? "sprite.mat_tnt" :
                          (b.mat == MaterialTier::MAGMA_VENT) ? "sprite.mat_magma" : "sprite.mat_trampoline";
        tex = assets.getTexture(key);
    }
    if (tex) {
        SDL_FRect dst{sp.x - hw, sp.y - hh, b.w, b.h};
        SDL_RenderCopyExF(ren, tex, nullptr, &dst, b.angle * (180.0 / PI), nullptr, SDL_FLIP_NONE);
        return;
    }

    const bool horizontal = b.w >= b.h;
    const float ow = std::clamp(std::min(b.w, b.h) * 0.08f, 1.5f, 2.6f);  // outline width
    auto outlineAndBody = [&](SDL_Color ink, SDL_Color top, SDL_Color bot) {
        g.fill(g.rrect(-hw - ow, -hh - ow, hw + ow, hh + ow, rad + ow), ink);
        g.fillGrad(g.rrect(-hw, -hh, hw, hh, rad), top, bot);
    };
    auto bevel = [&](SDL_Color hi, SDL_Color lo) {
        float in = std::min(2.5f, std::min(hw, hh) * 0.3f);
        g.line(-hw + in, -hh + in, hw - in, -hh + in, std::max(1.2f, in * 0.8f), hi);
        g.line(-hw + in, hh - in, hw - in, hh - in, std::max(1.2f, in * 0.8f), lo);
    };

    switch (b.mat) {
    case MaterialTier::TWIG: {
        // Bamboo: pale cane with nodes across the long axis.
        const SDL_Color ink{94, 66, 24, 255}, top{240, 208, 128, 255}, bot{198, 156, 78, 255};
        outlineAndBody(ink, top, bot);
        float len = horizontal ? b.w : b.h, thick = horizontal ? b.h : b.w;
        int nodes = std::max(1, static_cast<int>(len / 24.0f));
        for (int i = 1; i < nodes; ++i) {
            float u = -len * 0.5f + len * i / nodes;
            if (horizontal) {
                g.line(u, -hh + 1, u, hh - 1, 2.2f, {150, 112, 48, 255});
                g.line(u + 1.6f, -hh + 2, u + 1.6f, hh - 2, 1.0f, {255, 236, 170, 200});
            } else {
                g.line(-hw + 1, u, hw - 1, u, 2.2f, {150, 112, 48, 255});
                g.line(-hw + 2, u + 1.6f, hw - 2, u + 1.6f, 1.0f, {255, 236, 170, 200});
            }
        }
        // Long highlight stripe.
        if (horizontal) g.line(-hw + 3, -hh * 0.35f, hw - 3, -hh * 0.35f, std::max(1.0f, thick * 0.12f), {255, 244, 200, 150});
        else g.line(-hw * 0.35f, -hh + 3, -hw * 0.35f, hh - 3, std::max(1.0f, thick * 0.12f), {255, 244, 200, 150});
        break;
    }
    case MaterialTier::TIMBER: {
        const SDL_Color ink{60, 32, 18, 255}, top{184, 118, 68, 255}, bot{140, 84, 46, 255};
        outlineAndBody(ink, top, bot);
        SDL_Color grain{118, 68, 36, 200};
        bool square = std::abs(b.w - b.h) < std::min(b.w, b.h) * 0.4f && b.w > 28.0f;
        if (square) {
            // Crate: frame and cross brace.
            float in = std::min(hw, hh) * 0.22f;
            g.line(-hw + in, -hh + in, hw - in, hh - in, 3.0f, {116, 66, 34, 255});
            g.line(-hw + in, hh - in, hw - in, -hh + in, 3.0f, {116, 66, 34, 255});
            for (float sx : {-1.0f, 1.0f}) {
                for (float sy : {-1.0f, 1.0f}) g.dot(sx * (hw - in * 0.6f), sy * (hh - in * 0.6f), 1.6f, {60, 54, 52, 255});
            }
        } else {
            // Planks: grain lines along the length, a knot or two, nails at the ends.
            int lines = std::max(1, static_cast<int>((horizontal ? b.h : b.w) / 7.0f));
            for (int i = 1; i <= lines; ++i) {
                float v = -1.0f + 2.0f * i / (lines + 1);
                float wob = (rand01(seed) - 0.5f) * 2.0f;
                if (horizontal) g.line(-hw + 5, v * hh * 0.8f, hw - 5, v * hh * 0.8f + wob, 1.1f, grain);
                else g.line(v * hw * 0.8f, -hh + 5, v * hw * 0.8f + wob, hh - 5, 1.1f, grain);
            }
            float kx = (rand01(seed) - 0.5f) * (horizontal ? b.w * 0.6f : 0.0f);
            float ky = (rand01(seed) - 0.5f) * (horizontal ? 0.0f : b.h * 0.6f);
            float kr = std::min(hw, hh) * 0.35f;
            if (kr > 1.5f) {
                g.dot(kx, ky, kr, {120, 70, 38, 255});
                g.dot(kx, ky, kr * 0.45f, {96, 54, 28, 255});
            }
            if (horizontal && b.w > 40.0f) {
                g.dot(-hw + 6, 0, 1.6f, {62, 58, 56, 255});
                g.dot(hw - 6, 0, 1.6f, {62, 58, 56, 255});
            } else if (!horizontal && b.h > 40.0f) {
                g.dot(0, -hh + 6, 1.6f, {62, 58, 56, 255});
                g.dot(0, hh - 6, 1.6f, {62, 58, 56, 255});
            }
        }
        bevel({255, 214, 160, 90}, {70, 36, 18, 90});
        break;
    }
    case MaterialTier::STONE: {
        const SDL_Color ink{42, 46, 60, 255}, top{164, 174, 192, 255}, bot{112, 122, 142, 255};
        outlineAndBody(ink, top, bot);
        SDL_Color mortar{86, 94, 112, 230}, lit{206, 214, 228, 160};
        if (b.w >= 30.0f && b.h >= 30.0f) {
            // Ashlar courses with staggered joints.
            int rows = std::max(1, static_cast<int>(b.h / 22.0f));
            float rowH = b.h / rows;
            for (int r = 0; r < rows; ++r) {
                float y0 = -hh + r * rowH;
                if (r > 0) g.line(-hw + 1, y0, hw - 1, y0, 1.4f, mortar);
                float brick = std::min(b.w, 34.0f);
                float off = (r % 2) ? brick * 0.5f : 0.0f;
                for (float x = -hw + off + brick; x < hw - 4; x += brick) {
                    g.line(x, y0 + 2, x, y0 + rowH - 2, 1.4f, mortar);
                    g.line(x + 1.4f, y0 + 2, x + 1.4f, y0 + rowH * 0.5f, 0.9f, lit);
                }
            }
        } else {
            // Slim stone: a single groove along the length.
            if (horizontal && b.h > 12) g.line(-hw + 5, 0, hw - 5, 0, 1.3f, mortar);
            else if (!horizontal && b.w > 12) g.line(0, -hh + 5, 0, hh - 5, 1.3f, mortar);
            float seg = 40.0f, len = horizontal ? b.w : b.h;
            for (float u = -len * 0.5f + seg; u < len * 0.5f - 6; u += seg) {
                if (horizontal) g.line(u, -hh + 1, u, hh - 1, 1.4f, mortar);
                else g.line(-hw + 1, u, hw - 1, u, 1.4f, mortar);
            }
        }
        // Speckles.
        int specks = static_cast<int>(b.w * b.h / 220.0f);
        for (int i = 0; i < std::min(specks, 14); ++i) {
            float x = (rand01(seed) - 0.5f) * (b.w - 6), y = (rand01(seed) - 0.5f) * (b.h - 6);
            g.dot(x, y, 1.0f, (i % 2) ? SDL_Color{96, 104, 122, 200} : SDL_Color{196, 204, 218, 180});
        }
        bevel({230, 236, 246, 110}, {60, 66, 84, 110});
        break;
    }
    case MaterialTier::ICE: {
        const SDL_Color ink{48, 120, 172, 255}, top{208, 244, 255, 225}, bot{142, 208, 240, 215};
        outlineAndBody(ink, top, bot);
        // Diagonal shine streaks and a frosty inner rim.
        float d = std::min(hw, hh);
        g.line(-hw * 0.6f, hh * 0.3f, -hw * 0.6f + d * 0.8f, hh * 0.3f - d * 0.8f, std::max(1.5f, d * 0.22f), {255, 255, 255, 120});
        g.line(-hw * 0.1f, hh * 0.55f, -hw * 0.1f + d * 0.5f, hh * 0.55f - d * 0.5f, std::max(1.0f, d * 0.12f), {255, 255, 255, 90});
        {
            float in = 2.5f;
            auto rim = g.rrect(-hw + in, -hh + in, hw - in, hh - in, std::max(0.0f, rad - in));
            for (size_t i = 0; i < rim.size(); ++i) {
                const Vec2& a = rim[i];
                const Vec2& c2 = rim[(i + 1) % rim.size()];
                g.line(a.x, a.y, c2.x, c2.y, 1.2f, {255, 255, 255, 110});
            }
        }
        g.line(-hw + 3, -hh + 2.5f, hw - 3, -hh + 2.5f, 1.6f, {255, 255, 255, 200});
        break;
    }
    case MaterialTier::TNT: {
        bool flash = b.ignited && (static_cast<int>(b.tntFuse * 30.0f) % 2 == 0);
        const SDL_Color ink{70, 18, 16, 255};
        SDL_Color top = flash ? SDL_Color{255, 150, 130, 255} : SDL_Color{226, 66, 52, 255};
        SDL_Color bot = flash ? SDL_Color{230, 90, 70, 255} : SDL_Color{170, 40, 34, 255};
        outlineAndBody(ink, top, bot);
        // Dark bands and a cream label.
        float band = std::max(2.0f, hh * 0.18f);
        g.rect(-hw, -hh * 0.62f - band * 0.5f, hw, -hh * 0.62f + band * 0.5f, {60, 30, 28, 255});
        g.rect(-hw, hh * 0.62f - band * 0.5f, hw, hh * 0.62f + band * 0.5f, {60, 30, 28, 255});
        float lw = hw * 0.82f, lh = hh * 0.34f;
        if (lh > 3.0f) {
            g.fill(g.rrect(-lw, -lh, lw, lh, 2.0f), {252, 236, 196, 255});
            float size = std::min(lh * 2.0f, lw * 0.85f);
            if (size >= 7.0f) {
                TextStyle st = textStyle(FontFace::DISPLAY, size, {190, 36, 30, 255}, TextAlign::CENTER);
                float capT = textCapTop(st.face, size), capH = textCapHeight(st.face, size);
                // The label is drawn upright; skip it while the crate is tumbling far over.
                if (std::abs(std::sin(b.angle)) < 0.35f) {
                    drawTextStyled(ren, "TNT", sp.x, sp.y - capT - capH * 0.5f, st);
                }
            }
        }
        // Fuse with a spark when lit.
        Vec2 base = g.P(0, -hh), tip = g.P(hw * 0.25f, -hh - 7.0f);
        drawThickLine(ren, base, tip, 2.0f, {70, 56, 40, 255});
        if (b.ignited) {
            float flick = 0.7f + 0.3f * std::sin(t * 50.0f);
            ui::fillCircle(ren, tip, 7.0f * flick, {255, 150, 40, 140}, 14);
            ui::fillCircle(ren, tip, 3.5f, {255, 240, 160, 255}, 10);
        }
        break;
    }
    case MaterialTier::TRAMPOLINE: {
        // Steel frame and a springy green mat with chevrons.
        outlineAndBody({30, 34, 48, 255}, {112, 122, 146, 255}, {72, 80, 100, 255});
        float in = std::min(std::min(hw, hh) * 0.35f, 6.0f);
        auto mat = g.rrect(-hw + in, -hh + in, hw - in, hh - in, 2.0f);
        g.fillGrad(mat, {80, 200, 130, 255}, {40, 140, 96, 255});
        float len = horizontal ? b.w : b.h;
        int n = std::max(2, static_cast<int>(len / 16.0f));
        for (int i = 0; i < n; ++i) {
            float u = -len * 0.5f + in + (len - 2 * in) * (i + 0.5f) / n;
            float a = std::min(4.0f, (horizontal ? hh : hw) - in - 1);
            if (a < 1.5f) break;
            if (horizontal) {
                g.line(u - a, a * 0.5f, u, -a * 0.5f, 1.6f, {255, 232, 110, 230});
                g.line(u, -a * 0.5f, u + a, a * 0.5f, 1.6f, {255, 232, 110, 230});
            } else {
                g.line(a * 0.5f, u - a, -a * 0.5f, u, 1.6f, {255, 232, 110, 230});
                g.line(-a * 0.5f, u, a * 0.5f, u + a, 1.6f, {255, 232, 110, 230});
            }
        }
        for (float sx : {-1.0f, 1.0f}) {
            for (float sy : {-1.0f, 1.0f}) g.dot(sx * (hw - in * 0.5f), sy * (hh - in * 0.5f), 1.6f, {236, 190, 80, 255});
        }
        bevel({200, 214, 236, 140}, {30, 34, 48, 120});
        break;
    }
    case MaterialTier::MAGMA_VENT: {
        float glow = 0.75f + 0.25f * std::sin(t * 3.0f + b.pos.x * 0.05f);
        outlineAndBody({20, 14, 16, 255}, {78, 62, 66, 255}, {42, 32, 36, 255});
        SDL_Color lava{255, static_cast<uint8_t>(120 + 80 * glow), 40, 255};
        crack(g, -hw * 0.5f, -hh * 0.1f, hw * 0.4f, hh * 0.5f, seed, lava, {255, 230, 140, 160}, 2.4f);
        crack(g, hw * 0.3f, -hh * 0.5f, -hw * 0.1f, hh * 0.6f, seed, lava, {255, 230, 140, 160}, 2.0f);
        // Molten mouth on top.
        g.fill(g.rrect(-hw * 0.45f, -hh - 1, hw * 0.45f, -hh + std::min(5.0f, hh * 0.5f), 2.0f), lava);
        break;
    }
    }

    // Damage: jagged cracks that grow as the block weakens.
    if (b.mat != MaterialTier::TRAMPOLINE && b.mat != MaterialTier::MAGMA_VENT) {
        float hp = b.hp / std::max(1.0f, b.maxHp);
        SDL_Color dark = (b.mat == MaterialTier::ICE) ? SDL_Color{255, 255, 255, 235} : SDL_Color{30, 22, 20, 230};
        SDL_Color edge = (b.mat == MaterialTier::ICE) ? SDL_Color{80, 160, 210, 160} : SDL_Color{255, 255, 255, 60};
        uint32_t cs = seed * 31u + 7u;
        float w = std::clamp(std::min(b.w, b.h) * 0.09f, 1.3f, 2.4f);
        if (hp < 0.7f) crack(g, -hw * 0.75f, -hh * 0.7f, hw * 0.15f, hh * 0.45f, cs, dark, edge, w);
        if (hp < 0.4f) {
            crack(g, hw * 0.7f, -hh * 0.55f, -hw * 0.25f, hh * 0.75f, cs, dark, edge, w);
            crack(g, hw * 0.15f, hh * 0.45f, hw * 0.6f, hh * 0.85f, cs, dark, edge, w * 0.8f);
        }
    }
}

void SnoutAndSiegeGame::renderGoldenTruffle(SDL_Renderer* ren, Vec2 cam) {
    if (!levelTruffle.exists || levelTruffle.collected) return;
    Vec2 p = levelTruffle.pos - cam;
    float t = levelTruffle.pulseTime;
    float pulse = std::sin(t * 4.5f);
    float r = levelTruffle.radius;
    p.y += std::sin(t * 2.2f) * 2.5f;  // gentle hover

    ui::fillCircleGrad(ren, p, r * (2.1f + 0.2f * pulse), {255, 220, 90, 90}, {255, 220, 90, 0}, 28);
    SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture("sprite.item_truffle") : nullptr;
    if (tex) {
        float s = r * 2.4f;
        SDL_FRect dst{p.x - s * 0.5f, p.y - s * 0.5f, s, s};
        SDL_RenderCopyF(ren, tex, nullptr, &dst);
    } else {
        // Knobbly golden truffle with an outline and a shine.
        const SDL_Color ink{96, 56, 12, 255};
        for (int i = 0; i < 7; ++i) {
            float a = i * 2.0f * PI / 7.0f;
            ui::fillCircle(ren, p + Vec2(std::cos(a), std::sin(a)) * r * 0.62f, r * 0.5f, ink, 14);
        }
        for (int i = 0; i < 7; ++i) {
            float a = i * 2.0f * PI / 7.0f;
            ui::fillCircleGrad(ren, p + Vec2(std::cos(a), std::sin(a)) * r * 0.62f, r * 0.42f, {255, 222, 110, 255},
                               {214, 150, 34, 255}, 14);
        }
        ui::fillCircleGrad(ren, p, r * 0.78f, {255, 236, 150, 255}, {222, 160, 40, 255}, 20);
        ui::fillCircle(ren, p + Vec2(-r * 0.3f, -r * 0.35f), r * 0.22f, {255, 255, 235, 220}, 12);
    }
    // Twinkle.
    float g = 7.0f + 4.0f * pulse;
    Vec2 tw = p + Vec2(r * 0.8f, -r * 0.8f);
    drawThickLine(ren, {tw.x - g, tw.y}, {tw.x + g, tw.y}, 2.0f, {255, 250, 210, 230});
    drawThickLine(ren, {tw.x, tw.y - g}, {tw.x, tw.y + g}, 2.0f, {255, 250, 210, 230});
}

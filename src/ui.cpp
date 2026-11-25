#include "ui.h"

#include "draw.h"

#include <functional>

namespace ui {

namespace {

uint8_t clamp8(float v) { return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f)); }

// Outline of a rounded rect, clockwise from the top-left corner's arc.
void roundRectPath(SDL_FRect r, float rad, std::vector<Vec2>& out, int segs = 7) {
    rad = std::max(0.0f, std::min(rad, std::min(r.w, r.h) * 0.5f));
    out.clear();
    const Vec2 centres[4] = {{r.x + rad, r.y + rad}, {r.x + r.w - rad, r.y + rad},
                             {r.x + r.w - rad, r.y + r.h - rad}, {r.x + rad, r.y + r.h - rad}};
    for (int c = 0; c < 4; ++c) {
        float a0 = PI + c * (PI * 0.5f);
        for (int i = 0; i <= segs; ++i) {
            float a = a0 + (PI * 0.5f) * (static_cast<float>(i) / segs);
            out.push_back({centres[c].x + std::cos(a) * rad, centres[c].y + std::sin(a) * rad});
        }
    }
}

void fanFill(SDL_Renderer* ren, const std::vector<Vec2>& pts, Vec2 centre, SDL_Color cc,
             const std::function<SDL_Color(Vec2)>& colourAt) {
    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    v.reserve(pts.size() + 1);
    v.push_back({{centre.x, centre.y}, cc, {0, 0}});
    for (const auto& p : pts) v.push_back({{p.x, p.y}, colourAt(p), {0, 0}});
    int n = static_cast<int>(pts.size());
    for (int i = 0; i < n; ++i) {
        idx.push_back(0);
        idx.push_back(1 + i);
        idx.push_back(1 + (i + 1) % n);
    }
    SDL_RenderGeometry(ren, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
}

}  // namespace

SDL_Color shade(SDL_Color c, float f) { return {clamp8(c.r * f), clamp8(c.g * f), clamp8(c.b * f), c.a}; }

SDL_Color tint(SDL_Color c, float t) {
    return {clamp8(c.r + (255 - c.r) * t), clamp8(c.g + (255 - c.g) * t), clamp8(c.b + (255 - c.b) * t), c.a};
}

SDL_Color mix(SDL_Color a, SDL_Color b, float t) {
    return {clamp8(a.r + (b.r - a.r) * t), clamp8(a.g + (b.g - a.g) * t), clamp8(a.b + (b.b - a.b) * t),
            clamp8(a.a + (b.a - a.a) * t)};
}

SDL_Color alpha(SDL_Color c, float a) { return {c.r, c.g, c.b, clamp8(c.a * a)}; }

void fillRoundRect(SDL_Renderer* ren, SDL_FRect r, float rad, SDL_Color c) {
    fillRoundRectGrad(ren, r, rad, c, c);
}

void fillRoundRectGrad(SDL_Renderer* ren, SDL_FRect r, float rad, SDL_Color top, SDL_Color bottom) {
    if (r.w <= 0.0f || r.h <= 0.0f) return;
    std::vector<Vec2> pts;
    roundRectPath(r, rad, pts);
    Vec2 centre(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
    fanFill(ren, pts, centre, mix(top, bottom, 0.5f),
            [&](Vec2 p) { return mix(top, bottom, (p.y - r.y) / r.h); });
}

void strokeRoundRect(SDL_Renderer* ren, SDL_FRect r, float rad, float thick, SDL_Color c) {
    std::vector<Vec2> outer, inner;
    roundRectPath(r, rad, outer);
    SDL_FRect ri{r.x + thick, r.y + thick, r.w - 2 * thick, r.h - 2 * thick};
    roundRectPath(ri, std::max(0.0f, rad - thick), inner);
    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    int n = static_cast<int>(outer.size());
    for (int i = 0; i < n; ++i) {
        v.push_back({{outer[i].x, outer[i].y}, c, {0, 0}});
        v.push_back({{inner[i].x, inner[i].y}, c, {0, 0}});
    }
    for (int i = 0; i < n; ++i) {
        int a = 2 * i, b = 2 * ((i + 1) % n);
        for (int k : {a, a + 1, b, b, a + 1, b + 1}) idx.push_back(k);
    }
    SDL_RenderGeometry(ren, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
}

void softShadow(SDL_Renderer* ren, SDL_FRect r, float rad, float spread, float strength) {
    const int layers = 5;
    for (int i = layers; i >= 1; --i) {
        float g = spread * i / layers;
        SDL_FRect s{r.x - g * 0.5f, r.y - g * 0.2f + spread * 0.45f, r.w + g, r.h + g * 0.7f};
        fillRoundRect(ren, s, rad + g * 0.5f, {8, 6, 20, clamp8(255.0f * strength / layers)});
    }
}

void fillCircle(SDL_Renderer* ren, Vec2 c, float rad, SDL_Color col, int segs) {
    fillCircleGrad(ren, c, rad, col, col, segs);
}

void fillCircleGrad(SDL_Renderer* ren, Vec2 c, float rad, SDL_Color inner, SDL_Color outer, int segs) {
    std::vector<Vec2> pts;
    pts.reserve(segs);
    for (int i = 0; i < segs; ++i) {
        float a = 2.0f * PI * i / segs;
        pts.push_back({c.x + std::cos(a) * rad, c.y + std::sin(a) * rad});
    }
    fanFill(ren, pts, c, inner, [&](Vec2) { return outer; });
}

void ring(SDL_Renderer* ren, Vec2 c, float rad, float thick, SDL_Color col, int segs) {
    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    for (int i = 0; i < segs; ++i) {
        float a = 2.0f * PI * i / segs;
        Vec2 d(std::cos(a), std::sin(a));
        v.push_back({{c.x + d.x * (rad + thick * 0.5f), c.y + d.y * (rad + thick * 0.5f)}, col, {0, 0}});
        v.push_back({{c.x + d.x * (rad - thick * 0.5f), c.y + d.y * (rad - thick * 0.5f)}, col, {0, 0}});
    }
    for (int i = 0; i < segs; ++i) {
        int a = 2 * i, b = 2 * ((i + 1) % segs);
        for (int k : {a, a + 1, b, b, a + 1, b + 1}) idx.push_back(k);
    }
    SDL_RenderGeometry(ren, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
}

void fillRectGrad(SDL_Renderer* ren, SDL_FRect r, SDL_Color top, SDL_Color bottom) {
    if (r.h > 24.0f) {
        // Large gradients as 2px solid bands: far cheaper than textured-free
        // geometry on software renderers, and invisible at this step size.
        const float band = 2.0f;
        int n = static_cast<int>(std::ceil(r.h / band));
        for (int i = 0; i < n; ++i) {
            SDL_Color c = mix(top, bottom, (i + 0.5f) / n);
            if (c.a == 0) continue;
            SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
            float y = r.y + i * band;
            SDL_FRect b{r.x, y, r.w, std::min(band, r.y + r.h - y)};
            SDL_RenderFillRectF(ren, &b);
        }
        return;
    }
    SDL_Vertex v[4] = {{{r.x, r.y}, top, {0, 0}},
                       {{r.x + r.w, r.y}, top, {0, 0}},
                       {{r.x + r.w, r.y + r.h}, bottom, {0, 0}},
                       {{r.x, r.y + r.h}, bottom, {0, 0}}};
    int idx[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(ren, nullptr, v, 4, idx, 6);
}

void panel(SDL_Renderer* ren, SDL_FRect r, float rad, SDL_Color body) {
    softShadow(ren, r, rad, 18.0f, 0.55f);
    fillRoundRect(ren, {r.x - 3, r.y - 3, r.w + 6, r.h + 6}, rad + 3, alpha(pal::INK, body.a / 255.0f));
    fillRoundRectGrad(ren, r, rad, tint(body, 0.07f), shade(body, 0.82f));
    strokeRoundRect(ren, {r.x + 1, r.y + 1, r.w - 2, r.h - 2}, rad - 1, 1.5f, pal::PANEL_EDGE);
}

SDL_FRect buttonFace(SDL_Renderer* ren, SDL_FRect r, SDL_Color base, bool hovered, bool selected, float rad) {
    if (rad < 0.0f) rad = std::min(14.0f, r.h * 0.32f);
    float lip = std::clamp(r.h * 0.12f, 3.0f, 6.0f);
    float lift = hovered ? 1.5f : 0.0f;
    SDL_Color body = hovered ? tint(base, 0.12f) : base;
    base.a = 255;
    body.a = 255;

    // Drop shadow, dark outline, lip, then the glossy face.
    fillRoundRect(ren, {r.x + 1, r.y + lip + 2, r.w, r.h}, rad, {8, 6, 20, 90});
    fillRoundRect(ren, {r.x - 2, r.y - 2 - lift, r.w + 4, r.h + 4 + lift}, rad + 2, shade(base, 0.32f));
    fillRoundRect(ren, {r.x, r.y - lift, r.w, r.h + lift}, rad, shade(base, 0.62f));
    SDL_FRect face{r.x, r.y - lift, r.w, r.h - lip};
    fillRoundRectGrad(ren, face, rad, tint(body, 0.22f), body);
    // Gloss band across the upper half.
    fillRoundRect(ren, {face.x + 5, face.y + 3, face.w - 10, std::min(face.h * 0.38f, 20.0f)}, std::min(rad * 0.7f, 8.0f),
                  {255, 255, 255, 46});
    if (selected) {
        strokeRoundRect(ren, {r.x - 5, r.y - 5 - lift, r.w + 10, r.h + 10 + lift}, rad + 5, 3.0f, pal::GOLD);
    }
    return face;
}

void ribbon(SDL_Renderer* ren, float cx, float cy, float w, float h, SDL_Color base, const std::string& text,
            float textSize) {
    float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f, y0 = cy - h * 0.5f, y1 = cy + h * 0.5f;
    float tail = h * 0.9f, notch = h * 0.32f;
    SDL_Color dark = shade(base, 0.62f), darker = shade(base, 0.42f);
    // Tails (behind), with a fold triangle.
    for (int side : {-1, 1}) {
        float ex = side < 0 ? x0 : x1;
        float tx = ex - side * h * 0.35f;
        float ox = ex + side * tail;
        drawConvexPoly(ren, {{tx, y0 + h * 0.28f}, {ox, y0 + h * 0.28f}, {ox - side * notch, cy + h * 0.39f},
                             {ox, y1 + h * 0.5f}, {tx, y1 + h * 0.5f}},
                       dark);
        drawTriangle(ren, {ex, y1}, {tx, y1 + h * 0.5f}, {ex, y1 + h * 0.5f}, darker);
    }
    fillRoundRect(ren, {x0 - 2, y0 - 2, w + 4, h + 4}, 6.0f, shade(base, 0.35f));
    fillRoundRectGrad(ren, {x0, y0, w, h}, 5.0f, tint(base, 0.2f), shade(base, 0.9f));
    fillRectGrad(ren, {x0 + 4, y0 + 3, w - 8, h * 0.3f}, {255, 255, 255, 50}, {255, 255, 255, 0});
    float capTop = textCapTop(FontFace::DISPLAY, textSize), capH = textCapHeight(FontFace::DISPLAY, textSize);
    title(ren, text, cx, cy - capTop - capH * 0.5f, textSize, pal::CREAM, TextAlign::CENTER, shade(base, 0.3f));
}

float pillWidth(float h, UiIcon ic, const std::string& text) {
    float pad = h * 0.45f;
    float w = pad * 2 + textWidth(text, FontFace::BOLD, h * 0.56f);
    if (ic != UiIcon::NONE) w += h * 0.8f;
    return w;
}

float pill(SDL_Renderer* ren, float x, float y, float h, UiIcon ic, SDL_Color iconCol, const std::string& text,
           SDL_Color bg) {
    float w = pillWidth(h, ic, text);
    fillRoundRect(ren, {x, y, w, h}, h * 0.5f, bg);
    float tx = x + h * 0.45f;
    if (ic != UiIcon::NONE) {
        icon(ren, ic, {tx + h * 0.3f, y + h * 0.5f}, h * 0.62f, iconCol);
        tx += h * 0.8f;
    }
    float size = h * 0.56f;
    TextStyle st = textStyle(FontFace::BOLD, size, pal::CREAM);
    drawTextStyled(ren, text, tx, y + h * 0.5f - textCapTop(st.face, size) - textCapHeight(st.face, size) * 0.5f, st);
    return w;
}

void star(SDL_Renderer* ren, Vec2 c, float rOuter, bool earned, float rot) {
    auto pts = [&](float ro, float ri) {
        std::vector<Vec2> p;
        for (int i = 0; i < 10; ++i) {
            float a = -PI * 0.5f + rot + i * PI / 5.0f;
            float rr = (i % 2 == 0) ? ro : ri;
            p.push_back({c.x + std::cos(a) * rr, c.y + std::sin(a) * rr});
        }
        return p;
    };
    // Star polygons are not convex: fan from the centre.
    auto fillStar = [&](float ro, float ri, SDL_Color col) {
        std::vector<Vec2> p = pts(ro, ri);
        for (int i = 0; i < 10; ++i) drawTriangle(ren, c, p[i], p[(i + 1) % 10], col);
    };
    SDL_Color fill = earned ? pal::GOLD : SDL_Color{58, 62, 94, 255};
    SDL_Color edge = earned ? SDL_Color{120, 64, 12, 255} : SDL_Color{26, 26, 46, 255};
    fillStar(rOuter + std::max(2.0f, rOuter * 0.14f), rOuter * 0.5f + std::max(1.5f, rOuter * 0.1f), edge);
    fillStar(rOuter, rOuter * 0.48f, fill);
    if (earned) {
        std::vector<Vec2> p = pts(rOuter * 0.62f, rOuter * 0.3f);
        for (int i = 0; i < 10; ++i) drawTriangle(ren, c - Vec2(0, rOuter * 0.12f), p[i], p[(i + 1) % 10], {255, 240, 160, 120});
    }
}

float title(SDL_Renderer* ren, const std::string& s, float x, float y, float size, SDL_Color fill, TextAlign align,
            SDL_Color outline) {
    TextStyle st = textStyle(FontFace::DISPLAY, size, fill, align);
    st.outline = outline;
    st.shadow = {8, 6, 20, static_cast<uint8_t>(fill.a * 0.45f)};
    st.shadowDy = std::max(2.0f, size * 0.07f);
    return drawTextStyled(ren, s, x, y, st);
}

void icon(SDL_Renderer* ren, UiIcon ic, Vec2 c, float s, SDL_Color col) {
    float h = s * 0.5f;
    float lw = std::max(2.0f, s * 0.14f);
    auto line = [&](Vec2 a, Vec2 b, float w) {
        drawThickLine(ren, a, b, w, col);
        fillCircle(ren, a, w * 0.5f, col, 10);
        fillCircle(ren, b, w * 0.5f, col, 10);
    };
    switch (ic) {
    case UiIcon::NONE:
        break;
    case UiIcon::PLAY:
        drawTriangle(ren, {c.x - h * 0.55f, c.y - h * 0.75f}, {c.x + h * 0.8f, c.y}, {c.x - h * 0.55f, c.y + h * 0.75f}, col);
        break;
    case UiIcon::NEXT:
        line({c.x - h * 0.3f, c.y - h * 0.6f}, {c.x + h * 0.3f, c.y}, lw * 1.2f);
        line({c.x + h * 0.3f, c.y}, {c.x - h * 0.3f, c.y + h * 0.6f}, lw * 1.2f);
        break;
    case UiIcon::BACK:
        line({c.x + h * 0.3f, c.y - h * 0.6f}, {c.x - h * 0.3f, c.y}, lw * 1.2f);
        line({c.x - h * 0.3f, c.y}, {c.x + h * 0.3f, c.y + h * 0.6f}, lw * 1.2f);
        break;
    case UiIcon::RETRY: {
        // Clockwise arc with an arrowhead at its end.
        float r = h * 0.58f;
        float a0 = -PI * 0.15f, a1 = PI * 1.45f;
        int segs = 16;
        for (int i = 0; i < segs; ++i) {
            float t0 = a0 + (a1 - a0) * i / segs, t1 = a0 + (a1 - a0) * (i + 1) / segs;
            drawThickLine(ren, {c.x + std::cos(t0) * r, c.y + std::sin(t0) * r},
                          {c.x + std::cos(t1) * r, c.y + std::sin(t1) * r}, lw, col);
        }
        Vec2 end(c.x + std::cos(a1) * r, c.y + std::sin(a1) * r);
        Vec2 tang(-std::sin(a1), std::cos(a1)), nrm(std::cos(a1), std::sin(a1));
        drawTriangle(ren, end + tang * (h * 0.42f), end + nrm * (h * 0.36f) - tang * (h * 0.1f),
                     end - nrm * (h * 0.36f) - tang * (h * 0.1f), col);
        break;
    }
    case UiIcon::MENU:
        for (int i = -1; i <= 1; ++i) line({c.x - h * 0.6f, c.y + i * h * 0.48f}, {c.x + h * 0.6f, c.y + i * h * 0.48f}, lw);
        break;
    case UiIcon::LEVELS:
        for (int i = 0; i < 4; ++i) {
            float ox = (i % 2 == 0 ? -1.0f : 1.0f) * h * 0.36f, oy = (i < 2 ? -1.0f : 1.0f) * h * 0.36f;
            fillRoundRect(ren, {c.x + ox - h * 0.28f, c.y + oy - h * 0.28f, h * 0.56f, h * 0.56f}, h * 0.12f, col);
        }
        break;
    case UiIcon::MUSIC:
        fillCircle(ren, {c.x - h * 0.38f, c.y + h * 0.45f}, h * 0.26f, col, 14);
        fillCircle(ren, {c.x + h * 0.42f, c.y + h * 0.3f}, h * 0.26f, col, 14);
        drawThickLine(ren, {c.x - h * 0.16f, c.y + h * 0.45f}, {c.x - h * 0.16f, c.y - h * 0.55f}, lw * 0.8f, col);
        drawThickLine(ren, {c.x + h * 0.64f, c.y + h * 0.3f}, {c.x + h * 0.64f, c.y - h * 0.7f}, lw * 0.8f, col);
        drawConvexPoly(ren, {{c.x - h * 0.2f, c.y - h * 0.55f}, {c.x + h * 0.68f, c.y - h * 0.75f},
                             {c.x + h * 0.68f, c.y - h * 0.45f}, {c.x - h * 0.2f, c.y - h * 0.25f}},
                       col);
        break;
    case UiIcon::SOUND:
        drawConvexPoly(ren, {{c.x - h * 0.7f, c.y - h * 0.25f}, {c.x - h * 0.35f, c.y - h * 0.25f},
                             {c.x + h * 0.1f, c.y - h * 0.65f}, {c.x + h * 0.1f, c.y + h * 0.65f},
                             {c.x - h * 0.35f, c.y + h * 0.25f}, {c.x - h * 0.7f, c.y + h * 0.25f}},
                       col);
        for (int k = 0; k < 2; ++k) {
            float r = h * (0.4f + k * 0.3f);
            for (int i = 0; i < 6; ++i) {
                float a0 = -PI * 0.3f + PI * 0.6f * i / 6, a1 = -PI * 0.3f + PI * 0.6f * (i + 1) / 6;
                drawThickLine(ren, {c.x + h * 0.15f + std::cos(a0) * r, c.y + std::sin(a0) * r},
                              {c.x + h * 0.15f + std::cos(a1) * r, c.y + std::sin(a1) * r}, lw * 0.7f, col);
            }
        }
        break;
    case UiIcon::FAST:
        drawTriangle(ren, {c.x - h * 0.75f, c.y - h * 0.55f}, {c.x, c.y}, {c.x - h * 0.75f, c.y + h * 0.55f}, col);
        drawTriangle(ren, {c.x, c.y - h * 0.55f}, {c.x + h * 0.75f, c.y}, {c.x, c.y + h * 0.55f}, col);
        break;
    case UiIcon::FULLSCREEN:
        for (int sx : {-1, 1}) {
            for (int sy : {-1, 1}) {
                Vec2 k(c.x + sx * h * 0.6f, c.y + sy * h * 0.6f);
                line(k, {k.x - sx * h * 0.42f, k.y}, lw);
                line(k, {k.x, k.y - sy * h * 0.42f}, lw);
            }
        }
        break;
    case UiIcon::QUIT:
        line({c.x - h * 0.5f, c.y - h * 0.5f}, {c.x + h * 0.5f, c.y + h * 0.5f}, lw * 1.2f);
        line({c.x - h * 0.5f, c.y + h * 0.5f}, {c.x + h * 0.5f, c.y - h * 0.5f}, lw * 1.2f);
        break;
    case UiIcon::LOCK:
        fillRoundRect(ren, {c.x - h * 0.6f, c.y - h * 0.1f, h * 1.2f, h * 0.85f}, h * 0.15f, col);
        for (int i = 0; i < 10; ++i) {
            float a0 = PI + PI * i / 10, a1 = PI + PI * (i + 1) / 10;
            float r = h * 0.38f;
            drawThickLine(ren, {c.x + std::cos(a0) * r, c.y - h * 0.15f + std::sin(a0) * r},
                          {c.x + std::cos(a1) * r, c.y - h * 0.15f + std::sin(a1) * r}, lw, col);
        }
        drawThickLine(ren, {c.x - h * 0.38f, c.y - h * 0.15f}, {c.x - h * 0.38f, c.y}, lw, col);
        drawThickLine(ren, {c.x + h * 0.38f, c.y - h * 0.15f}, {c.x + h * 0.38f, c.y}, lw, col);
        break;
    case UiIcon::STAR:
        star(ren, c, h * 0.9f, true);
        break;
    case UiIcon::TROPHY:
        drawConvexPoly(ren, {{c.x - h * 0.6f, c.y - h * 0.7f}, {c.x + h * 0.6f, c.y - h * 0.7f},
                             {c.x + h * 0.45f, c.y - h * 0.05f}, {c.x + h * 0.15f, c.y + h * 0.2f},
                             {c.x - h * 0.15f, c.y + h * 0.2f}, {c.x - h * 0.45f, c.y - h * 0.05f}},
                       col);
        drawThickLine(ren, {c.x, c.y + h * 0.1f}, {c.x, c.y + h * 0.5f}, lw, col);
        fillRoundRect(ren, {c.x - h * 0.42f, c.y + h * 0.45f, h * 0.84f, h * 0.28f}, h * 0.08f, col);
        ring(ren, {c.x - h * 0.62f, c.y - h * 0.4f}, h * 0.22f, lw * 0.7f, col, 12);
        ring(ren, {c.x + h * 0.62f, c.y - h * 0.4f}, h * 0.22f, lw * 0.7f, col, 12);
        break;
    case UiIcon::HAMMER:
        line({c.x - h * 0.55f, c.y + h * 0.6f}, {c.x + h * 0.2f, c.y - h * 0.15f}, lw * 1.1f);
        drawConvexPoly(ren, {{c.x - h * 0.05f, c.y - h * 0.55f}, {c.x + h * 0.35f, c.y - h * 0.95f},
                             {c.x + h * 0.85f, c.y - h * 0.45f}, {c.x + h * 0.45f, c.y - h * 0.05f}},
                       col);
        break;
    case UiIcon::ENDLESS: {
        int segs = 28;
        for (int i = 0; i < segs; ++i) {
            float t0 = 2 * PI * i / segs, t1 = 2 * PI * (i + 1) / segs;
            auto lem = [&](float t) {
                float d = 1.0f + std::sin(t) * std::sin(t);
                return Vec2(c.x + h * 0.85f * std::cos(t) / d, c.y + h * 0.85f * std::sin(t) * std::cos(t) / d);
            };
            drawThickLine(ren, lem(t0), lem(t1), lw, col);
        }
        break;
    }
    case UiIcon::TRUFFLE:
        fillCircle(ren, {c.x, c.y + h * 0.1f}, h * 0.62f, col, 18);
        fillCircle(ren, {c.x - h * 0.18f, c.y - h * 0.12f}, h * 0.18f, {255, 255, 255, 120}, 10);
        drawThickLine(ren, {c.x, c.y - h * 0.5f}, {c.x + h * 0.15f, c.y - h * 0.85f}, lw * 0.7f, col);
        break;
    case UiIcon::PAUSE:
        fillRoundRect(ren, {c.x - h * 0.55f, c.y - h * 0.65f, h * 0.38f, h * 1.3f}, h * 0.1f, col);
        fillRoundRect(ren, {c.x + h * 0.17f, c.y - h * 0.65f, h * 0.38f, h * 1.3f}, h * 0.1f, col);
        break;
    case UiIcon::CHECK:
        line({c.x - h * 0.55f, c.y}, {c.x - h * 0.15f, c.y + h * 0.45f}, lw * 1.2f);
        line({c.x - h * 0.15f, c.y + h * 0.45f}, {c.x + h * 0.6f, c.y - h * 0.5f}, lw * 1.2f);
        break;
    }
}

}  // namespace ui

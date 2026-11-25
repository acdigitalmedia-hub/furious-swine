// Cartoon pig and bird models: rounded shaded bodies with bold outlines,
// big expressive eyes, and each character's signature accessories.
// Coordinates are in units of the body radius; +x is "forward" for pigs
// (towards the fortress) and the birds face -x (towards the catapult).
#include "draw.h"
#include "ui.h"

namespace {

using ui::mix;
using ui::shade;
using ui::tint;

const SDL_Color INK{28, 22, 30, 255};
const Vec2 LIGHT_DIR(0.55f, 0.83f);  // world-space direction towards shadow

struct Painter {
    SDL_Renderer* ren;
    Vec2 pos;
    float r, c, s, ow;

    float sqK = 0.0f;
    Vec2 sqD{1.0f, 0.0f};

    Painter(SDL_Renderer* rr, Vec2 p, float radius, float angle, float squash = 0.0f, float squashDir = 0.0f)
        : ren(rr), pos(p), r(radius), c(std::cos(angle)), s(std::sin(angle)),
          ow(std::max(1.4f, radius * 0.085f)), sqK(squash), sqD(std::cos(squashDir), std::sin(squashDir)) {}

    Vec2 P(float lx, float ly) const {
        Vec2 o((lx * c - ly * s) * r, (lx * s + ly * c) * r);
        if (sqK != 0.0f) {
            // Squash along sqD, bulge sideways (roughly volume-preserving).
            float along = o.x * sqD.x + o.y * sqD.y;
            Vec2 perp(o.x - sqD.x * along, o.y - sqD.y * along);
            float ka = 1.0f - sqK, kp = 1.0f + sqK * 0.6f;
            // Keep the contact side planted: shift towards the impact.
            o = Vec2(sqD.x * along * ka + perp.x * kp, sqD.y * along * ka + perp.y * kp) + sqD * (sqK * r * 0.9f);
        }
        return {pos.x + o.x, pos.y + o.y};
    }
    Vec2 P(Vec2 l) const { return P(l.x, l.y); }
    Vec2 toWorldDir(Vec2 d) const { return {d.x * c - d.y * s, d.x * s + d.y * c}; }
    Vec2 toLocalDir(Vec2 d) const { return {d.x * c + d.y * s, -d.x * s + d.y * c}; }

    std::vector<Vec2> ellipsePts(float cx, float cy, float rx, float ry, float rot = 0.0f, int segs = 0) const {
        if (segs <= 0) segs = std::clamp(static_cast<int>(std::max(rx, ry) * r * 0.9f), 12, 40);
        std::vector<Vec2> pts;
        float cr = std::cos(rot), sr = std::sin(rot);
        for (int i = 0; i < segs; ++i) {
            float a = 2.0f * PI * i / segs;
            float x = std::cos(a) * rx, y = std::sin(a) * ry;
            pts.push_back({cx + x * cr - y * sr, cy + x * sr + y * cr});
        }
        return pts;
    }

    // Fan fill of a convex local-space polygon with per-vertex colours.
    void fan(const std::vector<Vec2>& pts, Vec2 centre, SDL_Color cc, const std::vector<SDL_Color>& cols) const {
        std::vector<SDL_Vertex> v;
        std::vector<int> idx;
        Vec2 wc = P(centre);
        v.push_back({{wc.x, wc.y}, cc, {0, 0}});
        for (size_t i = 0; i < pts.size(); ++i) {
            Vec2 w = P(pts[i]);
            v.push_back({{w.x, w.y}, cols[i], {0, 0}});
        }
        int n = static_cast<int>(pts.size());
        for (int i = 0; i < n; ++i) {
            idx.push_back(0);
            idx.push_back(1 + i);
            idx.push_back(1 + (i + 1) % n);
        }
        SDL_RenderGeometry(ren, nullptr, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
    }

    void fill(const std::vector<Vec2>& pts, SDL_Color col) const {
        Vec2 ctr(0, 0);
        for (const auto& p : pts) ctr = ctr + p;
        ctr = ctr * (1.0f / pts.size());
        fan(pts, ctr, col, std::vector<SDL_Color>(pts.size(), col));
    }

    // Bold outline: thick edges plus round joints, in pixels.
    void stroke(const std::vector<Vec2>& pts, float px, SDL_Color col, bool closed = true) const {
        size_t n = pts.size();
        for (size_t i = 0; i + (closed ? 0 : 1) < n; ++i) {
            drawThickLine(ren, P(pts[i]), P(pts[(i + 1) % n]), px * 2.0f, col);
        }
        if (px * r > 0.0f && n <= 12) {
            for (const auto& p : pts) ui::fillCircle(ren, P(p), px, col, 8);
        }
    }

    // Light-to-dark shaded convex shape lit from the top-left in world space.
    void shaded(const std::vector<Vec2>& pts, SDL_Color light, SDL_Color mid, SDL_Color dark, Vec2 centre) const {
        std::vector<SDL_Color> cols;
        for (const auto& p : pts) {
            Vec2 d = p - centre;
            float len = std::max(1e-4f, std::sqrt(d.x * d.x + d.y * d.y));
            Vec2 w = toWorldDir({d.x / len, d.y / len});
            float t = std::clamp(w.x * LIGHT_DIR.x + w.y * LIGHT_DIR.y, -1.0f, 1.0f);
            cols.push_back(t > 0 ? mix(mid, dark, t) : mix(mid, light, -t * 0.6f));
        }
        // Highlight sits up-left in world space.
        Vec2 hl = centre + toLocalDir({-0.28f, -0.34f}) * 0.55f;
        fan(pts, hl, light, cols);
    }

    void ellipse(float cx, float cy, float rx, float ry, SDL_Color col, float rot = 0.0f) const {
        fill(ellipsePts(cx, cy, rx, ry, rot), col);
    }

    void ellipseOutlined(float cx, float cy, float rx, float ry, SDL_Color col, float rot = 0.0f) const {
        float o = ow / r;
        fill(ellipsePts(cx, cy, rx + o, ry + o, rot), INK);
        fill(ellipsePts(cx, cy, rx, ry, rot), col);
    }

    void ellipseShaded(float cx, float cy, float rx, float ry, SDL_Color light, SDL_Color mid, SDL_Color dark,
                       bool outline = true, float rot = 0.0f) const {
        if (outline) {
            float o = ow / r;
            fill(ellipsePts(cx, cy, rx + o, ry + o, rot), INK);
        }
        shaded(ellipsePts(cx, cy, rx, ry, rot), light, mid, dark, {cx, cy});
    }

    void polyOutlined(const std::vector<Vec2>& pts, SDL_Color col) const {
        stroke(pts, ow, INK);
        fill(pts, col);
    }

    void line(Vec2 a, Vec2 b, float wLocal, SDL_Color col) const {
        float w = wLocal * r;
        drawThickLine(ren, P(a), P(b), w, col);
        ui::fillCircle(ren, P(a), w * 0.5f, col, 10);
        ui::fillCircle(ren, P(b), w * 0.5f, col, 10);
    }

    void arc(Vec2 centre, float rad, float a0, float a1, float wLocal, SDL_Color col, int segs = 10) const {
        for (int i = 0; i < segs; ++i) {
            float t0 = a0 + (a1 - a0) * i / segs, t1 = a0 + (a1 - a0) * (i + 1) / segs;
            line(centre + Vec2(std::cos(t0), std::sin(t0)) * rad, centre + Vec2(std::cos(t1), std::sin(t1)) * rad, wLocal, col);
        }
    }

    // Horizontal band across a unit-circle body at height y (clipped to the body).
    void band(float y, float h, SDL_Color col, float bodyRx = 1.0f) const {
        float y0 = y - h * 0.5f, y1 = y + h * 0.5f;
        auto half = [&](float yy) { return bodyRx * std::sqrt(std::max(0.0f, 1.0f - yy * yy)) * 0.99f; };
        std::vector<Vec2> pts;
        const int n = 6;
        for (int i = 0; i <= n; ++i) {
            float yy = y0 + (y1 - y0) * i / n;
            pts.push_back({half(yy), yy});
        }
        for (int i = n; i >= 0; --i) {
            float yy = y0 + (y1 - y0) * i / n;
            pts.push_back({-half(yy), yy});
        }
        fill(pts, col);
    }

    // Cartoon eye: outlined white, iris, pupil and two catchlights.
    void eye(float ex, float ey, float rx, float ry, Vec2 look, SDL_Color iris, float pupil = 1.0f,
             SDL_Color white = {255, 255, 250, 255}) const {
        ellipseOutlined(ex, ey, rx, ry, white);
        float ix = ex + look.x * rx * 0.35f, iy = ey + look.y * ry * 0.3f;
        float ir = std::min(rx, ry) * 0.62f;
        ellipse(ix, iy, ir, ir * 1.08f, iris);
        ellipse(ix, iy, ir * 0.62f * pupil, ir * 0.68f * pupil, {16, 14, 22, 255});
        ellipse(ix - ir * 0.32f, iy - ir * 0.38f, ir * 0.3f, ir * 0.3f, {255, 255, 255, 255});
        ellipse(ix + ir * 0.3f, iy + ir * 0.32f, ir * 0.13f, ir * 0.13f, {255, 255, 255, 230});
    }

    // Squeezed-shut "> <" eye; dir = +1 points the chevron right.
    void squint(float ex, float ey, float w, float dir) const {
        float lw = 0.075f;
        line({ex - w * dir, ey - w * 0.7f}, {ex + w * dir * 0.6f, ey}, lw, INK);
        line({ex + w * dir * 0.6f, ey}, {ex - w * dir, ey + w * 0.7f}, lw, INK);
    }
};

}  // namespace

// ============================================================================
// PIGS
// ============================================================================
void renderOrigamiPigGeometry(SDL_Renderer* ren, PigType type, Vec2 pos, float r, float angle, const CharPose& pose) {
    Painter g(ren, pos, r, angle, pose.squash, pose.squashDir);

    SDL_Color base, light, dark, snout, snoutDark, earIn, iris;
    switch (type) {
    case PigType::TROTTER:
        base = {232, 120, 104, 255}; light = {255, 178, 160, 255}; dark = {170, 70, 62, 255};
        snout = {255, 176, 164, 255}; snoutDark = {214, 116, 106, 255}; earIn = {255, 158, 150, 255};
        iris = {104, 62, 40, 255};
        break;
    case PigType::TURBO:
        base = {246, 184, 58, 255}; light = {255, 226, 128, 255}; dark = {196, 126, 24, 255};
        snout = {255, 220, 140, 255}; snoutDark = {222, 164, 70, 255}; earIn = {250, 140, 80, 255};
        iris = {120, 74, 24, 255};
        break;
    case PigType::SPLIT:
        base = {98, 196, 150, 255}; light = {170, 236, 200, 255}; dark = {50, 138, 104, 255};
        snout = {178, 238, 206, 255}; snoutDark = {112, 190, 156, 255}; earIn = {255, 170, 176, 255};
        iris = {40, 120, 84, 255};
        break;
    case PigType::BOMB:
        base = {84, 90, 108, 255}; light = {140, 148, 168, 255}; dark = {44, 48, 62, 255};
        snout = {150, 158, 176, 255}; snoutDark = {106, 112, 130, 255}; earIn = {236, 110, 60, 255};
        iris = {236, 96, 40, 255};
        break;
    case PigType::GLIDER:
        base = {92, 176, 240, 255}; light = {172, 222, 255, 255}; dark = {42, 118, 190, 255};
        snout = {186, 230, 255, 255}; snoutDark = {120, 186, 236, 255}; earIn = {220, 240, 255, 255};
        iris = {30, 120, 210, 255};
        break;
    case PigType::GRAVITON:
        base = {136, 84, 214, 255}; light = {196, 160, 250, 255}; dark = {82, 40, 150, 255};
        snout = {210, 176, 255, 255}; snoutDark = {156, 112, 226, 255}; earIn = {246, 196, 255, 255};
        iris = {200, 70, 250, 255};
        break;
    case PigType::DRILL:
    default:
        base = {196, 116, 48, 255}; light = {240, 172, 98, 255}; dark = {130, 70, 24, 255};
        snout = {248, 204, 150, 255}; snoutDark = {206, 152, 96, 255}; earIn = {255, 182, 130, 255};
        iris = {200, 120, 30, 255};
        break;
    }

    // --- Behind the body: tail / scarf / wings / gear, feet, ears.
    if (type == PigType::TURBO) {
        // Fluttering red aviator scarf.
        g.polyOutlined({{-0.75f, 0.05f}, {-1.45f, -0.18f}, {-1.3f, 0.08f}, {-1.5f, 0.3f}, {-0.78f, 0.38f}}, {222, 56, 50, 255});
    } else if (type == PigType::SPLIT) {
        g.polyOutlined({{-0.7f, -0.42f}, {-1.4f, -0.68f}, {-1.22f, -0.36f}}, {222, 60, 56, 255});
        g.polyOutlined({{-0.72f, -0.3f}, {-1.36f, -0.12f}, {-1.08f, 0.0f}}, {190, 44, 42, 255});
    } else if (type == PigType::GLIDER) {
        // Paper glider wing on the back.
        g.polyOutlined({{-0.35f, -0.55f}, {-1.15f, -1.25f}, {-0.95f, -0.7f}, {-0.6f, -0.32f}}, {236, 248, 255, 255});
        g.line({-0.42f, -0.52f}, {-1.05f, -1.12f}, 0.05f, {170, 208, 240, 255});
    } else if (type == PigType::DRILL) {
        Vec2 gc(-0.9f, 0.1f);
        std::vector<Vec2> cog;
        for (int i = 0; i < 16; ++i) {
            float a = angle * 3.0f + i * PI / 8.0f;
            float rr = (i % 2 == 0) ? 0.34f : 0.26f;
            cog.push_back(gc + Vec2(std::cos(a), std::sin(a)) * rr);
        }
        g.stroke(cog, g.ow, INK);
        g.fill(cog, {170, 176, 188, 255});
        g.ellipse(gc.x, gc.y, 0.1f, 0.1f, {60, 62, 72, 255});
    } else if (type != PigType::GRAVITON) {
        // Curly tail.
        g.arc({-1.0f, 0.12f}, 0.13f, 0.4f, PI * 1.8f, 0.09f + g.ow / r * 2.0f, INK, 8);
        g.arc({-1.0f, 0.12f}, 0.13f, 0.4f, PI * 1.8f, 0.09f, dark, 8);
    }
    // Stubby feet.
    g.ellipseOutlined(-0.42f, 0.86f, 0.2f, 0.14f, dark);
    g.ellipseOutlined(0.38f, 0.88f, 0.2f, 0.14f, dark);
    // Ears.
    auto ear = [&](float x, float tilt, bool front) {
        float bx = x, by = -0.72f;
        Vec2 tip(x + tilt, -1.22f);
        std::vector<Vec2> e = {{bx - 0.26f, by + 0.06f}, tip, {bx + 0.26f, by - 0.02f}};
        g.polyOutlined(e, front ? base : dark);
        g.fill({{bx - 0.13f, by - 0.0f}, {tip.x * 0.75f + bx * 0.25f, tip.y * 0.8f + by * 0.2f}, {bx + 0.13f, by - 0.04f}}, earIn);
    };
    ear(-0.42f, -0.12f, false);
    ear(0.3f, 0.06f, true);

    // --- Body.
    g.ellipseShaded(0.0f, 0.0f, 1.0f, 0.96f, light, base, dark);
    // Soft belly highlight.
    g.ellipse(-0.05f, 0.42f, 0.55f, 0.34f, ui::alpha(light, 0.35f));

    // --- Headgear sitting on the body.
    switch (type) {
    case PigType::TROTTER:
        g.band(-0.5f, 0.17f, {176, 44, 40, 255});
        g.ellipseOutlined(0.12f, -0.52f, 0.1f, 0.1f, {250, 206, 80, 255});
        break;
    case PigType::TURBO:
        g.band(-0.6f, 0.13f, {86, 54, 30, 255});
        for (float gx : {-0.18f, 0.32f}) {
            g.ellipseOutlined(gx, -0.62f, 0.2f, 0.18f, {222, 178, 72, 255});
            g.ellipse(gx, -0.62f, 0.13f, 0.12f, {96, 220, 246, 255});
            g.ellipse(gx - 0.05f, -0.66f, 0.05f, 0.04f, {235, 255, 255, 255});
        }
        break;
    case PigType::SPLIT:
        g.band(-0.5f, 0.16f, {214, 52, 48, 255});
        g.polyOutlined({{-0.06f, -0.62f}, {0.3f, -0.62f}, {0.3f, -0.38f}, {-0.06f, -0.38f}}, {214, 224, 236, 255});
        g.ellipse(0.12f, -0.5f, 0.06f, 0.06f, {50, 170, 110, 255});
        break;
    case PigType::BOMB:
        // Riveted steel brow plate and a lit fuse on top.
        g.band(-0.55f, 0.2f, {58, 62, 76, 255});
        for (float rx : {-0.5f, -0.17f, 0.17f, 0.5f}) g.ellipse(rx, -0.55f, 0.045f, 0.045f, {232, 168, 74, 255});
        g.line({0.05f, -0.9f}, {-0.02f, -1.18f}, 0.08f, {150, 120, 84, 255});
        g.ellipse(-0.04f, -1.28f, 0.17f, 0.17f, {255, 140, 40, 200});
        g.ellipse(-0.04f, -1.28f, 0.1f, 0.1f, {255, 236, 150, 255});
        break;
    case PigType::GLIDER:
        g.band(-0.6f, 0.13f, {70, 56, 46, 255});
        for (float gx : {-0.16f, 0.34f}) {
            g.ellipseOutlined(gx, -0.62f, 0.19f, 0.17f, {226, 184, 76, 255});
            g.ellipse(gx, -0.62f, 0.12f, 0.11f, {150, 236, 255, 255});
            g.ellipse(gx - 0.05f, -0.66f, 0.045f, 0.04f, {255, 255, 255, 255});
        }
        break;
    case PigType::GRAVITON:
        for (float ax : {-0.32f, 0.34f}) {
            g.line({ax, -0.82f}, {ax * 1.25f, -1.32f}, 0.07f + g.ow / r * 2.0f, INK);
            g.line({ax, -0.82f}, {ax * 1.25f, -1.32f}, 0.07f, {222, 146, 66, 255});
            g.ellipse(ax * 1.25f, -1.36f, 0.2f, 0.2f, {110, 230, 255, 90});
            g.ellipseOutlined(ax * 1.25f, -1.36f, 0.11f, 0.11f, {140, 240, 255, 255});
            g.ellipse(ax * 1.25f - 0.03f, -1.39f, 0.04f, 0.04f, {255, 255, 255, 255});
        }
        g.polyOutlined({{0.05f, -0.7f}, {0.16f, -0.56f}, {0.05f, -0.42f}, {-0.06f, -0.56f}}, {240, 110, 255, 255});
        break;
    case PigType::DRILL:
        // Miner's hard hat with a headlamp.
        g.polyOutlined({{-0.9f, -0.5f}, {0.95f, -0.5f}, {0.85f, -0.36f}, {-0.82f, -0.36f}}, {176, 104, 34, 255});
        {
            std::vector<Vec2> dome;
            for (int i = 0; i <= 12; ++i) {
                float a = PI + PI * i / 12.0f;
                dome.push_back({0.02f + std::cos(a) * 0.72f, -0.5f + std::sin(a) * 0.6f});
            }
            g.stroke(dome, g.ow, INK);
            g.shaded(dome, {250, 186, 90, 255}, {214, 136, 44, 255}, {160, 94, 24, 255}, {0.02f, -0.75f});
        }
        g.ellipseOutlined(0.28f, -0.72f, 0.15f, 0.15f, {236, 190, 70, 255});
        g.ellipse(0.28f, -0.72f, 0.1f, 0.1f, {255, 252, 200, 255});
        break;
    }

    // --- Face.
    Vec2 look(1.0f, 0.1f);
    if (pose.look.x != 0.0f || pose.look.y != 0.0f) look = g.toLocalDir(pose.look.normalized());
    SDL_Color browCol = shade(dark, 0.7f);
    if (pose.mood == Mood::OUCH) {
        g.squint(-0.12f, -0.24f, 0.14f, 1.0f);
        g.squint(0.36f, -0.26f, 0.13f, -1.0f);
        g.line({-0.3f, -0.5f}, {0.04f, -0.56f}, 0.085f, browCol);
        g.line({0.22f, -0.58f}, {0.52f, -0.5f}, 0.08f, browCol);
    } else if (pose.mood == Mood::YELL) {
        // Wide-eyed battle cry.
        g.eye(-0.12f, -0.26f, 0.22f, 0.27f, look, iris, 0.75f);
        g.eye(0.36f, -0.28f, 0.2f, 0.25f, look, iris, 0.75f);
        g.line({-0.32f, -0.62f}, {0.04f, -0.6f}, 0.085f, browCol);
        g.line({0.2f, -0.62f}, {0.54f, -0.66f}, 0.08f, browCol);
    } else {
        g.eye(-0.12f, -0.24f, 0.2f, 0.24f, look, iris);
        g.eye(0.36f, -0.26f, 0.18f, 0.22f, look, iris);
        // Determined brows.
        g.line({-0.3f, -0.56f}, {0.04f, -0.48f}, 0.085f, browCol);
        g.line({0.22f, -0.5f}, {0.52f, -0.58f}, 0.08f, browCol);
    }
    // Rosy cheek.
    g.ellipse(-0.42f, 0.18f, 0.16f, 0.1f, {255, 120, 130, 90});

    // Snout.
    g.ellipseShaded(0.6f, 0.16f, 0.34f, 0.25f, tint(snout, 0.3f), snout, snoutDark);
    g.ellipse(0.48f, 0.17f, 0.055f, 0.1f, shade(snoutDark, 0.55f));
    g.ellipse(0.72f, 0.17f, 0.055f, 0.1f, shade(snoutDark, 0.55f));
    if (pose.mood == Mood::YELL || pose.mood == Mood::OUCH) {
        // Open mouth with a tongue.
        float h = pose.mood == Mood::YELL ? 0.17f : 0.1f;
        g.ellipseOutlined(0.5f, 0.5f, 0.17f, h, {96, 24, 34, 255});
        g.ellipse(0.5f, 0.5f + h * 0.45f, 0.11f, h * 0.45f, {240, 110, 120, 255});
    } else {
        // Little smile under the snout.
        g.arc({0.36f, 0.34f}, 0.14f, PI * 0.15f, PI * 0.75f, 0.05f, shade(dark, 0.6f), 6);
    }

    if (type == PigType::TROTTER || type == PigType::BOMB) {
        SDL_Color tusk = (type == PigType::BOMB) ? SDL_Color{226, 232, 240, 255} : SDL_Color{255, 250, 236, 255};
        g.polyOutlined({{0.34f, 0.4f}, {0.46f, 0.4f}, {0.38f, 0.62f}}, tusk);
        g.polyOutlined({{0.76f, 0.38f}, {0.88f, 0.36f}, {0.86f, 0.58f}}, tusk);
    }
    if (type == PigType::DRILL) {
        // Spiral drill bit on the snout.
        std::vector<Vec2> bit = {{0.78f, 0.0f}, {1.5f, 0.16f}, {0.78f, 0.34f}};
        g.polyOutlined(bit, {200, 206, 218, 255});
        g.fill({{0.78f, 0.17f}, {1.5f, 0.16f}, {0.78f, 0.34f}}, {156, 162, 176, 255});
        for (float t : {0.25f, 0.5f, 0.72f}) {
            float x = 0.78f + 0.72f * t, hh = 0.17f * (1.0f - t);
            g.line({x - 0.05f, 0.17f - hh}, {x + 0.05f, 0.17f + hh}, 0.04f, {236, 190, 70, 255});
        }
    }
}

// ============================================================================
// BIRDS
// ============================================================================
void renderTotemBirdGeometry(SDL_Renderer* ren, BirdClass kind, Vec2 pos, float r, float angle, float hpRatio,
                             bool hasHelmet, float helmetHpRatio, bool hasShield, float shieldHpRatio,
                             bool inZeppelin, float balloonHpRatio, const CharPose& pose) {
    Painter g(ren, pos, r, angle, pose.squash, pose.squashDir);

    SDL_Color base, light, dark, wing, belly, beak, beakDark, crest, iris;
    switch (kind) {
    case BirdClass::TOUCAN:
        base = {46, 156, 168, 255}; light = {110, 210, 214, 255}; dark = {22, 96, 112, 255};
        wing = {30, 112, 130, 255}; belly = {255, 214, 96, 255}; beak = {255, 150, 50, 255};
        beakDark = {226, 70, 52, 255}; crest = {255, 196, 64, 255}; iris = {40, 40, 60, 255};
        break;
    case BirdClass::OWL:
        base = {98, 104, 164, 255}; light = {158, 164, 214, 255}; dark = {56, 60, 108, 255};
        wing = {70, 74, 126, 255}; belly = {220, 224, 242, 255}; beak = {244, 196, 76, 255};
        beakDark = {196, 146, 44, 255}; crest = {190, 200, 236, 255}; iris = {236, 170, 40, 255};
        break;
    case BirdClass::FALCON:
        base = {72, 94, 118, 255}; light = {130, 156, 182, 255}; dark = {36, 52, 70, 255};
        wing = {48, 66, 86, 255}; belly = {236, 238, 246, 255}; beak = {250, 196, 50, 255};
        beakDark = {200, 140, 28, 255}; crest = {222, 168, 48, 255}; iris = {200, 120, 30, 255};
        break;
    case BirdClass::CONDOR:
        base = {52, 52, 62, 255}; light = {104, 104, 118, 255}; dark = {24, 24, 30, 255};
        wing = {124, 34, 42, 255}; belly = {238, 232, 222, 255}; beak = {240, 182, 56, 255};
        beakDark = {190, 128, 32, 255}; crest = {248, 244, 238, 255}; iris = {180, 40, 40, 255};
        break;
    case BirdClass::HERON:
    default:
        base = {246, 240, 226, 255}; light = {255, 255, 250, 255}; dark = {196, 184, 162, 255};
        wing = {184, 42, 52, 255}; belly = {250, 214, 96, 255}; beak = {240, 116, 66, 255};
        beakDark = {190, 70, 44, 255}; crest = {255, 212, 58, 255}; iris = {60, 40, 90, 255};
        break;
    }

    // Zeppelin first (it hangs above the bird).
    if (inZeppelin) {
        float by = -2.0f;
        for (float x : {-0.6f, -0.2f, 0.2f, 0.6f}) g.line({x * 1.3f, by + 0.5f}, {x * 0.7f, -0.8f}, 0.035f, {70, 60, 52, 255});
        g.ellipseShaded(0.0f, by, 1.4f, 0.62f, {236, 200, 150, 255}, {198, 156, 104, 255}, {140, 100, 62, 255});
        for (float x : {-0.7f, 0.0f, 0.7f}) {
            float hh = 0.62f * std::sqrt(std::max(0.0f, 1.0f - (x / 1.4f) * (x / 1.4f)));
            g.line({x, by - hh + 0.06f}, {x, by + hh - 0.06f}, 0.05f, {140, 96, 56, 255});
        }
        g.polyOutlined({{1.25f, by - 0.1f}, {1.7f, by - 0.5f}, {1.7f, by + 0.5f}, {1.25f, by + 0.1f}}, {170, 60, 56, 255});
        g.ellipseOutlined(-1.35f, by, 0.1f, 0.1f, {236, 186, 60, 255});
        if (balloonHpRatio < 0.55f) {
            g.line({-0.4f, by - 0.3f}, {0.1f, by + 0.2f}, 0.06f, {80, 50, 30, 255});
            g.ellipse(0.5f, by - 0.1f, 0.12f, 0.09f, {80, 50, 30, 255});
        }
    }

    // Tail feathers and feet behind the body.
    g.polyOutlined({{0.55f, 0.55f}, {1.2f, 0.78f}, {1.05f, 0.95f}, {0.5f, 0.85f}}, shade(wing, 0.9f));
    g.polyOutlined({{0.5f, 0.68f}, {1.05f, 1.02f}, {0.82f, 1.1f}, {0.42f, 0.88f}}, wing);
    SDL_Color foot{246, 170, 52, 255};
    g.ellipseOutlined(-0.32f, 0.97f, 0.2f, 0.09f, foot);
    g.ellipseOutlined(0.28f, 0.97f, 0.2f, 0.09f, foot);

    // Crests behind the head.
    if (kind == BirdClass::TOUCAN) {
        g.polyOutlined({{-0.18f, -0.82f}, {-0.05f, -1.36f}, {0.12f, -0.86f}}, {242, 118, 48, 255});
        g.polyOutlined({{0.0f, -0.86f}, {0.32f, -1.4f}, {0.34f, -0.82f}}, crest);
        g.polyOutlined({{0.2f, -0.82f}, {0.62f, -1.2f}, {0.5f, -0.72f}}, {92, 218, 168, 255});
    } else if (kind == BirdClass::OWL && !hasHelmet) {
        g.polyOutlined({{-0.72f, -0.62f}, {-0.98f, -1.32f}, {-0.3f, -0.86f}}, base);
        g.polyOutlined({{0.3f, -0.86f}, {0.98f, -1.32f}, {0.72f, -0.62f}}, base);
    } else if (kind == BirdClass::FALCON) {
        g.polyOutlined({{0.3f, -0.8f}, {0.9f, -1.05f}, {0.62f, -0.62f}}, crest);
    }

    // Body.
    g.ellipseShaded(0.0f, 0.0f, 0.97f, 1.0f, light, base, dark);
    // Belly patch.
    g.ellipse(-0.05f, 0.38f, 0.6f, 0.55f, ui::alpha(belly, 0.95f));
    g.ellipse(-0.18f, 0.22f, 0.3f, 0.22f, ui::alpha(tint(belly, 0.4f), 0.6f));
    if (kind == BirdClass::OWL) {
        for (float y : {0.3f, 0.52f}) {
            for (float x : {-0.28f, 0.0f, 0.24f}) {
                g.arc({x - 0.05f, y}, 0.09f, PI * 0.15f, PI * 0.85f, 0.045f, shade(belly, 0.7f), 5);
            }
        }
    } else if (kind == BirdClass::HERON) {
        // Royal sash with a ruby medallion.
        g.fill({{-0.78f, 0.05f}, {-0.62f, -0.12f}, {0.72f, 0.62f}, {0.56f, 0.78f}}, {200, 44, 58, 255});
        g.ellipseOutlined(0.0f, 0.42f, 0.12f, 0.12f, {246, 196, 60, 255});
        g.ellipse(0.0f, 0.42f, 0.065f, 0.065f, {214, 36, 52, 255});
    } else if (kind == BirdClass::CONDOR) {
        // Fluffy white neck ruff.
        g.ellipseOutlined(0.0f, 0.12f, 0.8f, 0.2f, crest);
        for (int i = 0; i < 6; ++i) g.ellipse(-0.6f + i * 0.24f, 0.26f, 0.14f, 0.1f, crest);
    }

    // Wings at the sides.
    g.ellipseShaded(0.86f, 0.22f, 0.24f, 0.46f, tint(wing, 0.25f), wing, shade(wing, 0.7f), true, -0.25f);
    g.ellipseShaded(-0.86f, 0.24f, 0.22f, 0.42f, tint(wing, 0.25f), wing, shade(wing, 0.7f), true, 0.25f);

    // Eyes: glaring at the pigs; worried and wide when hurt or scared.
    bool scared = pose.mood == Mood::SCARED;
    bool hurt = hpRatio < 0.38f || scared;
    float pupil = (hpRatio < 0.3f || scared) ? 0.65f : 1.0f;
    float t = SDL_GetTicks() * 0.001f;
    float phase = std::fmod(t + pos.x * 0.013f, 4.2f);
    bool blink = phase < 0.12f && !scared;
    Vec2 look(-1.0f, 0.05f);
    if (pose.look.x != 0.0f || pose.look.y != 0.0f) look = g.toLocalDir(pose.look.normalized());
    SDL_Color white = (kind == BirdClass::OWL) ? SDL_Color{255, 236, 140, 255} : SDL_Color{255, 255, 250, 255};
    float ey = -0.36f;
    if (kind == BirdClass::OWL) {
        g.ellipse(-0.32f, ey, 0.32f, 0.32f, shade(light, 1.05f));
        g.ellipse(0.24f, ey, 0.3f, 0.3f, shade(light, 1.05f));
    }
    if (pose.mood == Mood::OUCH) {
        g.squint(-0.31f, ey, 0.15f, 1.0f);
        g.squint(0.23f, ey, 0.14f, -1.0f);
    } else if (blink) {
        g.line({-0.5f, ey}, {-0.12f, ey}, 0.07f, INK);
        g.line({0.06f, ey}, {0.4f, ey}, 0.07f, INK);
    } else if (scared) {
        g.eye(-0.31f, ey, 0.24f, 0.28f, look, iris, pupil, white);
        g.eye(0.23f, ey, 0.22f, 0.26f, look, iris, pupil, white);
    } else {
        g.eye(-0.31f, ey, 0.21f, 0.24f, look, iris, pupil, white);
        g.eye(0.23f, ey, 0.19f, 0.22f, look, iris, pupil, white);
    }
    // Brows: angry V when healthy, raised when hurt.
    float inner = hurt ? -0.66f : -0.52f, outer = hurt ? -0.58f : -0.7f;
    g.line({-0.56f, outer}, {-0.12f, inner}, 0.1f, INK);
    g.line({0.04f, inner}, {0.44f, outer}, 0.1f, INK);
    if (hpRatio < 0.38f) {
        // Bandage.
        g.polyOutlined({{0.38f, -0.84f}, {0.66f, -0.72f}, {0.6f, -0.6f}, {0.32f, -0.72f}}, {246, 226, 180, 255});
        g.line({0.4f, -0.68f}, {0.58f, -0.82f}, 0.035f, {210, 180, 130, 255});
    }

    // Beaks (pointing at the catapult).
    switch (kind) {
    case BirdClass::TOUCAN:
        g.polyOutlined({{-0.42f, -0.24f}, {-1.15f, -0.2f}, {-1.55f, -0.02f}, {-1.1f, 0.12f}, {-0.42f, 0.14f}}, beak);
        g.fill({{-0.42f, -0.04f}, {-1.48f, 0.0f}, {-1.1f, 0.11f}, {-0.42f, 0.12f}}, beakDark);
        g.fill({{-1.25f, -0.15f}, {-1.53f, -0.02f}, {-1.2f, 0.08f}}, {36, 30, 40, 255});
        g.line({-0.5f, -0.14f}, {-1.05f, -0.13f}, 0.035f, {255, 222, 140, 200});
        break;
    case BirdClass::HERON:
        g.polyOutlined({{-0.4f, -0.2f}, {-1.4f, 0.02f}, {-0.4f, 0.16f}}, beak);
        g.fill({{-0.4f, 0.0f}, {-1.38f, 0.03f}, {-0.4f, 0.15f}}, beakDark);
        break;
    case BirdClass::OWL:
        g.polyOutlined({{-0.18f, -0.18f}, {0.06f, -0.18f}, {-0.06f, 0.12f}}, beak);
        break;
    case BirdClass::FALCON:
    case BirdClass::CONDOR:
    default:
        g.polyOutlined({{-0.22f, -0.2f}, {-0.62f, -0.08f}, {-0.6f, 0.12f}, {-0.46f, 0.2f}, {-0.08f, 0.04f}}, beak);
        g.fill({{-0.6f, 0.04f}, {-0.6f, 0.12f}, {-0.46f, 0.2f}, {-0.3f, 0.06f}}, beakDark);
        break;
    }

    // Crown for the Archduke.
    if (kind == BirdClass::HERON && !hasHelmet) {
        std::vector<Vec2> crown = {{-0.5f, -0.78f}, {-0.56f, -1.26f}, {-0.3f, -1.0f}, {-0.02f, -1.4f},
                                   {0.26f, -1.0f}, {0.5f, -1.26f}, {0.46f, -0.78f}};
        g.stroke(crown, g.ow, INK);
        // Fill as a fan of triangles from the base centre (the outline is concave).
        for (size_t i = 0; i + 1 < crown.size(); ++i) {
            g.fill({{-0.02f, -0.8f}, crown[i], crown[i + 1]}, crest);
        }
        g.band(-0.84f, 0.1f, {222, 160, 30, 255}, 0.55f);
        g.ellipse(-0.02f, -1.05f, 0.08f, 0.08f, {224, 40, 56, 255});
        g.ellipse(-0.32f, -0.94f, 0.05f, 0.05f, {60, 196, 236, 255});
        g.ellipse(0.28f, -0.94f, 0.05f, 0.05f, {60, 196, 236, 255});
    }

    // Iron cauldron helmet.
    if (hasHelmet) {
        std::vector<Vec2> dome;
        for (int i = 0; i <= 14; ++i) {
            float a = PI + PI * i / 14.0f;
            dome.push_back({std::cos(a) * 0.82f, -0.6f + std::sin(a) * 0.72f});
        }
        g.stroke(dome, g.ow, INK);
        g.shaded(dome, {150, 158, 176, 255}, {96, 104, 122, 255}, {56, 60, 74, 255}, {0.0f, -0.9f});
        g.polyOutlined({{-0.94f, -0.66f}, {0.94f, -0.66f}, {0.9f, -0.5f}, {-0.9f, -0.5f}}, {70, 76, 92, 255});
        for (int rv = -3; rv <= 3; ++rv) g.ellipse(rv * 0.24f, -0.58f, 0.04f, 0.04f, {232, 186, 76, 255});
        g.arc({0.0f, -1.38f}, 0.14f, PI, PI * 2.0f, 0.06f, {60, 64, 78, 255}, 6);
        g.ellipse(-0.3f, -1.04f, 0.16f, 0.09f, {255, 255, 255, 60}, -0.4f);
        if (helmetHpRatio < 0.55f) {
            g.line({-0.3f, -1.22f}, {0.05f, -0.92f}, 0.05f, {28, 30, 38, 255});
            g.line({0.05f, -0.92f}, {0.28f, -0.7f}, 0.045f, {28, 30, 38, 255});
        }
    }

    // Commander's tower shield, held towards the catapult.
    if (hasShield) {
        std::vector<Vec2> sh = {{-1.42f, -0.92f}, {-0.86f, -0.98f}, {-0.84f, 0.7f}, {-1.12f, 1.02f}, {-1.42f, 0.7f}};
        g.stroke(sh, g.ow, INK);
        g.shaded(sh, {140, 154, 180, 255}, {88, 100, 124, 255}, {52, 60, 78, 255}, {-1.13f, 0.0f});
        g.line({-1.13f, -0.78f}, {-1.13f, 0.78f}, 0.08f, {236, 190, 70, 255});
        g.line({-1.34f, -0.15f}, {-0.92f, -0.15f}, 0.08f, {236, 190, 70, 255});
        for (Vec2 rv : {Vec2(-1.33f, -0.82f), Vec2(-0.94f, -0.86f), Vec2(-1.33f, 0.62f), Vec2(-0.94f, 0.62f)}) {
            g.ellipse(rv.x, rv.y, 0.045f, 0.045f, {250, 214, 100, 255});
        }
        if (shieldHpRatio < 0.55f) {
            g.line({-1.32f, -0.4f}, {-1.05f, 0.1f}, 0.05f, {24, 26, 34, 255});
            g.line({-1.05f, 0.1f}, {-1.25f, 0.5f}, 0.045f, {24, 26, 34, 255});
        }
    }
}

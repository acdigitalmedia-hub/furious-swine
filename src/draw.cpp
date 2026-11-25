#include "draw.h"

#include "text.h"

// ============================================================================
// BITMAP FONT & GEOMETRIC DRAWING PRIMITIVES
// ============================================================================
static const uint8_t FONT_5X7[96][7] = {
    {0,0,0,0,0,0,0}, // ' '
    {4,4,4,4,0,0,4}, // '!'
    {10,10,10,0,0,0,0}, // '"'
    {10,31,10,10,31,10,0}, // '#'
    {4,15,20,14,5,30,4}, // '$'
    {24,25,2,4,8,19,3}, // '%'
    {12,18,20,8,21,18,13}, // '&'
    {4,4,8,0,0,0,0}, // '\''
    {2,4,8,8,8,4,2}, // '('
    {8,4,2,2,2,4,8}, // ')'
    {0,4,21,14,21,4,0}, // '*'
    {0,4,4,31,4,4,0}, // '+'
    {0,0,0,0,4,4,8}, // ','
    {0,0,0,31,0,0,0}, // '-'
    {0,0,0,0,0,12,12}, // '.'
    {1,2,2,4,8,8,16}, // '/'
    {14,17,19,21,25,17,14}, // '0'
    {4,12,4,4,4,4,14}, // '1'
    {14,17,1,6,8,16,31}, // '2'
    {31,2,4,6,1,17,14}, // '3'
    {2,6,10,18,31,2,2}, // '4'
    {31,16,30,1,1,17,14}, // '5'
    {6,8,16,30,17,17,14}, // '6'
    {31,1,2,4,8,8,8}, // '7'
    {14,17,17,14,17,17,14}, // '8'
    {14,17,17,15,1,2,12}, // '9'
    {0,12,12,0,12,12,0}, // ':'
    {0,12,12,0,12,4,8}, // ';'
    {2,4,8,16,8,4,2}, // '<'
    {0,0,31,0,31,0,0}, // '='
    {8,4,2,1,2,4,8}, // '>'
    {14,17,1,2,4,0,4}, // '?'
    {14,17,23,21,23,16,14}, // '@'
    {14,17,17,31,17,17,17}, // 'A'
    {30,17,17,30,17,17,30}, // 'B'
    {14,17,16,16,16,17,14}, // 'C'
    {30,17,17,17,17,17,30}, // 'D'
    {31,16,16,30,16,16,31}, // 'E'
    {31,16,16,30,16,16,16}, // 'F'
    {14,17,16,23,17,17,15}, // 'G'
    {17,17,17,31,17,17,17}, // 'H'
    {14,4,4,4,4,4,14}, // 'I'
    {7,2,2,2,2,18,12}, // 'J'
    {17,18,20,24,20,18,17}, // 'K'
    {16,16,16,16,16,16,31}, // 'L'
    {17,27,21,21,17,17,17}, // 'M'
    {17,25,21,19,17,17,17}, // 'N'
    {14,17,17,17,17,17,14}, // 'O'
    {30,17,17,30,16,16,16}, // 'P'
    {14,17,17,17,21,18,13}, // 'Q'
    {30,17,17,30,20,18,17}, // 'R'
    {15,16,16,14,1,1,30}, // 'S'
    {31,4,4,4,4,4,4}, // 'T'
    {17,17,17,17,17,17,14}, // 'U'
    {17,17,17,17,17,10,4}, // 'V'
    {17,17,17,21,21,27,17}, // 'W'
    {17,17,10,4,10,17,17}, // 'X'
    {17,17,10,4,4,4,4}, // 'Y'
    {31,1,2,4,8,16,31}, // 'Z'
    {14,8,8,8,8,8,14}, // '['
    {16,8,8,4,2,2,1}, // '\\'
    {14,2,2,2,2,2,14}, // ']'
    {4,10,17,0,0,0,0}, // '^'
    {0,0,0,0,0,0,31}, // '_'
    {8,4,2,0,0,0,0}, // '`'
    {0,0,14,1,15,17,15}, // 'a'
    {16,16,30,17,17,17,30}, // 'b'
    {0,0,14,16,16,17,14}, // 'c'
    {1,1,15,17,17,17,15}, // 'd'
    {0,0,14,17,31,16,14}, // 'e'
    {6,8,8,28,8,8,8}, // 'f'
    {0,0,15,17,15,1,14}, // 'g'
    {16,16,30,17,17,17,17}, // 'h'
    {4,0,12,4,4,4,14}, // 'i'
    {2,0,6,2,2,18,12}, // 'j'
    {16,16,18,20,24,20,18}, // 'k'
    {12,4,4,4,4,4,14}, // 'l'
    {0,0,26,21,21,17,17}, // 'm'
    {0,0,30,17,17,17,17}, // 'n'
    {0,0,14,17,17,17,14}, // 'o'
    {0,0,30,17,30,16,16}, // 'p'
    {0,0,15,17,15,1,1}, // 'q'
    {0,0,22,25,16,16,16}, // 'r'
    {0,0,15,16,14,1,30}, // 's'
    {8,8,28,8,8,9,6}, // 't'
    {0,0,17,17,17,19,13}, // 'u'
    {0,0,17,17,17,10,4}, // 'v'
    {0,0,17,17,21,21,10}, // 'w'
    {0,0,17,10,4,10,17}, // 'x'
    {0,0,17,17,15,1,14}, // 'y'
    {0,0,31,2,4,8,31}, // 'z'
    {2,4,4,8,4,4,2}, // '{'
    {4,4,4,4,4,4,4}, // '|'
    {8,4,4,2,4,4,8}, // '}'
    {0,0,8,21,2,0,0}, // '~'
    {0,0,0,0,0,0,0}
};

std::string formatNumber(int n) {
    std::string s = std::to_string(n);
    int insertPos = static_cast<int>(s.length()) - 3;
    while (insertPos > 0) {
        s.insert(insertPos, ",");
        insertPos -= 3;
    }
    return s;
}

void drawText(SDL_Renderer* ren, const std::string& text, float x, float y,
              float scale, SDL_Color col, bool center, bool shadow) {
    if (textFontAvailable()) {
        // Legacy call sites size text by the old 5x7 grid: capitals were
        // 7*scale tall with their top at y. Match that with the real font.
        TextStyle st;
        st.face = FontFace::BOLD;
        float capRatio = textCapHeight(FontFace::BOLD, 100.0f) / 100.0f;
        st.size = 7.0f * scale / std::max(0.3f, capRatio);
        st.color = col;
        st.align = center ? TextAlign::CENTER : TextAlign::LEFT;
        if (shadow && col.a > 35) {
            st.shadow = {8, 10, 16, static_cast<uint8_t>(col.a * 0.55f)};
            st.shadowDy = std::max(1.0f, scale * 0.8f);
        }
        drawTextStyled(ren, text, x, y - textCapTop(st.face, st.size), st);
        return;
    }
    float charW = 6.0f * scale;
    float startX = center ? (x - (text.size() * charW) * 0.5f) : x;

    auto renderGlyphs = [&](float ox, float oy, SDL_Color c) {
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char ch = static_cast<unsigned char>(text[i]);
            if (ch < 32 || ch >= 128) ch = '?';
            const uint8_t* glyph = FONT_5X7[ch - 32];
            float gx = startX + i * charW + ox;
            for (int row = 0; row < 7; ++row) {
                uint8_t bits = glyph[row];
                for (int colBit = 0; colBit < 5; ++colBit) {
                    if (bits & (1 << (4 - colBit))) {
                        SDL_FRect r{gx + colBit * scale, y + row * scale + oy, scale, scale};
                        SDL_RenderFillRectF(ren, &r);
                    }
                }
            }
        }
    };

    if (shadow && col.a > 35) {
        SDL_Color shadowCol{8, 10, 16, static_cast<uint8_t>(col.a * 0.85f)};
        renderGlyphs(std::max(1.0f, scale * 0.6f), std::max(1.0f, scale * 0.6f), shadowCol);
    }
    renderGlyphs(0.0f, 0.0f, col);
}


void drawTriangle(SDL_Renderer* ren, Vec2 a, Vec2 b, Vec2 c, SDL_Color col) {
    SDL_Vertex v[3] = {
        {{a.x, a.y}, col, {0, 0}},
        {{b.x, b.y}, col, {0, 0}},
        {{c.x, c.y}, col, {0, 0}}
    };
    SDL_RenderGeometry(ren, nullptr, v, 3, nullptr, 0);
}

void drawConvexPoly(SDL_Renderer* ren, const std::vector<Vec2>& pts, SDL_Color col) {
    if (pts.size() < 3) return;
    std::vector<SDL_Vertex> verts(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        verts[i] = {{pts[i].x, pts[i].y}, col, {0.0f, 0.0f}};
    }
    std::vector<int> indices;
    indices.reserve((pts.size() - 2) * 3);
    for (size_t i = 1; i + 1 < pts.size(); ++i) {
        indices.push_back(0);
        indices.push_back(static_cast<int>(i));
        indices.push_back(static_cast<int>(i + 1));
    }
    SDL_RenderGeometry(ren, nullptr, verts.data(), static_cast<int>(verts.size()),
                       indices.data(), static_cast<int>(indices.size()));
}

void drawThickLine(SDL_Renderer* ren, Vec2 a, Vec2 b, float thickness, SDL_Color col) {
    Vec2 dir = (b - a).normalized();
    Vec2 n(-dir.y * thickness * 0.5f, dir.x * thickness * 0.5f);
    std::vector<Vec2> quad = {a + n, b + n, b - n, a - n};
    drawConvexPoly(ren, quad, col);
}

void drawNGon(SDL_Renderer* ren, Vec2 center, float radius, int sides,
              float rotRad, SDL_Color col) {
    std::vector<Vec2> pts(sides);
    for (int i = 0; i < sides; ++i) {
        float a = rotRad + (2.0f * PI * i) / sides;
        pts[i] = {center.x + std::cos(a) * radius, center.y + std::sin(a) * radius};
    }
    drawConvexPoly(ren, pts, col);
}

void drawStarIcon(SDL_Renderer* ren, Vec2 center, float outerR, float innerR,
                  SDL_Color fillCol, SDL_Color outlineCol) {
    for (int i = 0; i < 5; ++i) {
        float a0 = -PI * 0.5f + i * (2.0f * PI / 5.0f);
        float a1 = a0 + (PI / 5.0f);
        float a2 = a0 - (PI / 5.0f);
        Vec2 pOuter(center.x + std::cos(a0) * outerR, center.y + std::sin(a0) * outerR);
        Vec2 pRight(center.x + std::cos(a1) * innerR, center.y + std::sin(a1) * innerR);
        Vec2 pLeft(center.x + std::cos(a2) * innerR, center.y + std::sin(a2) * innerR);
        drawTriangle(ren, center, pLeft, pOuter, fillCol);
        drawTriangle(ren, center, pOuter, pRight, outlineCol);
    }
}

// ============================================================================
// HIGH-RESOLUTION MULTI-FACETED ORIGAMI CHARACTER MODELS (PIGS & TOTEM BIRDS)
// ============================================================================
void renderOrigamiPigGeometry(SDL_Renderer* ren, PigType type, Vec2 pos,
                              float r, float angle) {
    float c = std::cos(angle), s = std::sin(angle);
    auto rot = [&](float lx, float ly) -> Vec2 {
        return {pos.x + (lx * c - ly * s) * r, pos.y + (lx * s + ly * c) * r};
    };
    auto tri = [&](float x0, float y0, float x1, float y1, float x2, float y2, SDL_Color col) {
        drawTriangle(ren, rot(x0, y0), rot(x1, y1), rot(x2, y2), col);
    };
    auto quad = [&](float x0, float y0, float x1, float y1,
                    float x2, float y2, float x3, float y3, SDL_Color col) {
        drawConvexPoly(ren, {rot(x0, y0), rot(x1, y1), rot(x2, y2), rot(x3, y3)}, col);
    };
    auto crease = [&](float x0, float y0, float x1, float y1, SDL_Color col, float w = 1.4f) {
        drawThickLine(ren, rot(x0, y0), rot(x1, y1), w, col);
    };

    // Palette per Pig Class
    SDL_Color cCrown, cLight, cMid, cDark, cDeep, cRim, cSnoutL, cSnoutD, cEarIn;
    if (type == PigType::TROTTER) {
        // Trotter: Crimson Terracotta Vanguard Boar
        cCrown  = {244, 138, 112, 255};
        cLight  = {226, 114, 88, 255};
        cMid    = {198, 86, 62, 255};
        cDark   = {162, 62, 44, 255};
        cDeep   = {124, 42, 30, 255};
        cRim    = {32, 20, 18, 255};
        cSnoutL = {252, 176, 152, 255};
        cSnoutD = {218, 128, 106, 255};
        cEarIn  = {248, 162, 146, 255};
    } else if (type == PigType::TURBO) {
        // Bramble: Golden Amber Supersonic Aviator Boar
        cCrown  = {255, 214, 96, 255};
        cLight  = {242, 182, 58, 255};
        cMid    = {214, 146, 34, 255};
        cDark   = {176, 112, 20, 255};
        cDeep   = {136, 80, 12, 255};
        cRim    = {34, 24, 14, 255};
        cSnoutL = {255, 226, 138, 255};
        cSnoutD = {224, 174, 82, 255};
        cEarIn  = {245, 132, 68, 255};
    } else if (type == PigType::SPLIT) {
        // Clover: Jade-Emerald Shinobi / Ranger Piglet
        cCrown  = {122, 224, 186, 255};
        cLight  = {88, 192, 154, 255};
        cMid    = {62, 158, 124, 255};
        cDark   = {42, 122, 94, 255};
        cDeep   = {28, 88, 68, 255};
        cRim    = {18, 32, 28, 255};
        cSnoutL = {166, 238, 208, 255};
        cSnoutD = {114, 194, 162, 255};
        cEarIn  = {182, 245, 218, 255};
    } else if (type == PigType::GLIDER) {
        // Gale: Sky-Blue / Cerulean Origami Aeronaut Swine
        cCrown  = {140, 215, 255, 255};
        cLight  = {100, 190, 248, 255};
        cMid    = {64, 158, 224, 255};
        cDark   = {36, 120, 185, 255};
        cDeep   = {22, 80, 138, 255};
        cRim    = {18, 30, 48, 255};
        cSnoutL = {185, 235, 255, 255};
        cSnoutD = {130, 205, 245, 255};
        cEarIn  = {215, 245, 255, 255};
    } else if (type == PigType::GRAVITON) {
        // Atticus: Cosmic Violet / Deep Indigo Tesla Boar
        cCrown  = {175, 120, 245, 255};
        cLight  = {140, 85, 215, 255};
        cMid    = {105, 50, 180, 255};
        cDark   = {75, 25, 140, 255};
        cDeep   = {45, 12, 95, 255};
        cRim    = {22, 10, 42, 255};
        cSnoutL = {210, 170, 255, 255};
        cSnoutD = {160, 110, 230, 255};
        cEarIn  = {240, 190, 255, 255};
    } else if (type == PigType::DRILL) {
        // Rusty: Industrial Russet Copper / Bronze Drill Hog
        cCrown  = {235, 165, 80, 255};
        cLight  = {205, 130, 45, 255};
        cMid    = {165, 95, 25, 255};
        cDark   = {125, 65, 15, 255};
        cDeep   = {85, 40, 10, 255};
        cRim    = {35, 20, 10, 255};
        cSnoutL = {245, 205, 150, 255};
        cSnoutD = {200, 155, 95, 255};
        cEarIn  = {255, 180, 120, 255};
    } else {
        // Vulcan: Obsidian Dreadnought Demolition Hog
        cCrown  = {112, 120, 138, 255};
        cLight  = {86, 94, 110, 255};
        cMid    = {64, 70, 84, 255};
        cDark   = {44, 48, 58, 255};
        cDeep   = {28, 32, 40, 255};
        cRim    = {18, 18, 22, 255};
        cSnoutL = {148, 156, 172, 255};
        cSnoutD = {104, 112, 126, 255};
        cEarIn  = {238, 106, 48, 255};
    }

    // 1. Rear Curled Origami Tail & Trailing Accessories
    if (type == PigType::TURBO) {
        // Crimson Aviator Scarf fluttering behind Bramble
        tri(-0.85f, 0.15f, -1.48f, -0.05f, -1.25f, 0.32f, {214, 48, 42, 255});
        tri(-0.85f, 0.22f, -1.38f, 0.38f,  -1.10f, 0.55f, {168, 32, 28, 255});
    } else if (type == PigType::SPLIT) {
        // Ninja headband tails behind Clover
        tri(-0.78f, -0.35f, -1.42f, -0.58f, -1.22f, -0.25f, {224, 58, 52, 255});
        tri(-0.78f, -0.28f, -1.35f, -0.15f, -1.12f, 0.05f,  {178, 38, 34, 255});
    } else if (type == PigType::GLIDER) {
        // Origami Aero-Glider Tail Rudder
        tri(-0.85f, -0.10f, -1.55f, -0.42f, -1.25f, 0.15f, {185, 225, 255, 255});
        tri(-0.85f, 0.05f, -1.45f, 0.38f, -1.15f, 0.50f, {120, 180, 230, 255});
    } else if (type == PigType::GRAVITON) {
        // Dual Tesla Coils emitting crackling energy
        tri(-0.45f, -0.85f, -0.45f, -1.35f, -0.25f, -0.85f, {175, 120, 255, 255});
        drawNGon(ren, rot(-0.35f, -1.35f), r * 0.16f, 6, angle, {80, 240, 255, 255});
        tri(0.25f, -0.85f, 0.45f, -1.35f, 0.45f, -0.85f, {175, 120, 255, 255});
        drawNGon(ren, rot(0.45f, -1.35f), r * 0.16f, 6, angle, {80, 240, 255, 255});
    } else if (type == PigType::DRILL) {
        // Heavy cog gear & mechanical drive socket
        drawNGon(ren, rot(-0.85f, 0.05f), r * 0.24f, 6, angle * 3.0f, {160, 165, 175, 255});
        drawNGon(ren, rot(-0.85f, 0.05f), r * 0.12f, 6, angle * 3.0f, {45, 45, 55, 255});
    } else {
        // Geometric curled origami pig tail
        tri(-0.90f, 0.05f, -1.22f, -0.18f, -1.08f, 0.18f, cDark);
        tri(-1.22f, -0.18f, -1.34f, -0.02f, -1.08f, 0.18f, cMid);
    }

    // 2. Outer Dark Woodblock Silhouette Rim (12-sided sculpted hull)
    if (type == PigType::TURBO) {
        std::vector<Vec2> rimHull = {
            rot(1.24f, 0.02f), rot(0.72f, -0.68f), rot(-0.08f, -0.98f),
            rot(-0.84f, -0.78f), rot(-1.12f, 0.0f), rot(-0.84f, 0.78f),
            rot(-0.08f, 0.94f), rot(0.75f, 0.68f)
        };
        drawConvexPoly(ren, rimHull, cRim);
    } else {
        drawNGon(ren, pos, r * 1.05f, 12, angle + PI / 12.0f, cRim);
    }

    // 3. Multi-Faceted Origami Cranium & Cheek Planes (10 interlocking paper facets)
    // Top crown facet
    quad(-0.48f, -0.88f, 0.48f, -0.88f, 0.68f, -0.42f, -0.58f, -0.42f, cCrown);
    // Upper-left sunlit temple facet
    tri(-0.58f, -0.42f, -0.48f, -0.88f, -0.92f, -0.32f, cLight);
    // Upper-right forward brow facet
    tri(0.48f, -0.88f, 0.92f, -0.32f, 0.68f, -0.42f, cLight);
    // Central bridge facet
    quad(-0.58f, -0.42f, 0.68f, -0.42f, 0.78f, 0.18f, -0.62f, 0.18f, cMid);
    // Left rear cheek facet
    quad(-0.92f, -0.32f, -0.58f, -0.42f, -0.62f, 0.18f, -0.94f, 0.25f, cMid);
    // Right forward cheek facet
    quad(0.68f, -0.42f, 0.92f, -0.32f, 0.96f, 0.25f, 0.78f, 0.18f, cLight);
    // Lower jaw & chin shadow facets
    quad(-0.94f, 0.25f, -0.62f, 0.18f, -0.42f, 0.86f, -0.76f, 0.68f, cDark);
    quad(-0.62f, 0.18f, 0.78f, 0.18f, 0.52f, 0.88f, -0.42f, 0.86f, cDark);
    quad(0.78f, 0.18f, 0.96f, 0.25f, 0.76f, 0.70f, 0.52f, 0.88f, cDeep);

    // Subtle origami paper-fold crease lines across the cranium
    crease(-0.58f, -0.42f, 0.68f, -0.42f, cCrown, 1.2f);
    crease(-0.58f, -0.42f, -0.62f, 0.18f, cDark, 1.2f);
    crease(-0.62f, 0.18f, 0.78f, 0.18f, cDeep, 1.2f);

    // 4. Sculpted 3-Facet Origami Ears (Outer shell, Inner folded recess, Shadow tip)
    // Left ear
    tri(-0.68f, -0.72f, -0.28f, -1.28f, -0.10f, -0.78f, cRim);
    tri(-0.62f, -0.74f, -0.28f, -1.20f, -0.15f, -0.78f, cMid);
    tri(-0.48f, -0.78f, -0.28f, -1.08f, -0.22f, -0.78f, cEarIn);
    // Right ear
    tri(0.12f, -0.78f, 0.48f, -1.28f, 0.74f, -0.70f, cRim);
    tri(0.17f, -0.78f, 0.46f, -1.20f, 0.68f, -0.72f, cDark);
    tri(0.24f, -0.78f, 0.45f, -1.08f, 0.56f, -0.75f, cEarIn);

    // 5. Class-Specific Headgear & Signature Details
    if (type == PigType::TROTTER) {
        // Vanguard Boar Crimson-Gold Battle Crest Band across brow + origami ear-notch
        quad(-0.62f, -0.58f, 0.72f, -0.58f, 0.78f, -0.40f, -0.66f, -0.40f, {138, 34, 28, 255});
        tri(-0.05f, -0.64f, 0.15f, -0.64f, 0.05f, -0.36f, {245, 202, 82, 255});
        // Gleaming 2-facet curved origami boar tusks jutting from lower jaw
        tri(-0.08f, 0.42f, 0.06f, 0.82f, 0.18f, 0.44f, {252, 248, 235, 255});
        tri(0.06f, 0.82f, 0.18f, 0.44f, 0.24f, 0.72f, {210, 202, 185, 255});
        tri(0.55f, 0.42f, 0.74f, 0.80f, 0.82f, 0.40f, {252, 248, 235, 255});
    } else if (type == PigType::TURBO) {
        // Bramble's Twin Brass-and-Cyan Supersonic Aviator Goggles on Forehead
        quad(-0.74f, -0.64f, 0.80f, -0.64f, 0.84f, -0.48f, -0.78f, -0.48f, {72, 46, 24, 255});
        // Left goggle rim & lens
        drawNGon(ren, rot(-0.22f, -0.58f), r * 0.24f, 8, angle, {218, 172, 68, 255});
        drawNGon(ren, rot(-0.22f, -0.58f), r * 0.17f, 8, angle, {72, 215, 245, 255});
        tri(-0.30f, -0.66f, -0.14f, -0.66f, -0.26f, -0.52f, {220, 252, 255, 240});
        // Right goggle rim & lens
        drawNGon(ren, rot(0.32f, -0.58f), r * 0.24f, 8, angle, {218, 172, 68, 255});
        drawNGon(ren, rot(0.32f, -0.58f), r * 0.17f, 8, angle, {72, 215, 245, 255});
        tri(0.24f, -0.66f, 0.40f, -0.66f, 0.28f, -0.52f, {220, 252, 255, 240});
    } else if (type == PigType::SPLIT) {
        // Clover's Crimson Shinobi Headband with Silver Metal Forehead Plate & Clover Emblem
        quad(-0.68f, -0.62f, 0.74f, -0.62f, 0.78f, -0.42f, -0.72f, -0.42f, {204, 46, 42, 255});
        quad(-0.18f, -0.64f, 0.34f, -0.64f, 0.34f, -0.40f, -0.18f, -0.40f, {215, 226, 238, 255});
        drawNGon(ren, rot(0.08f, -0.52f), r * 0.08f, 4, angle, {42, 168, 118, 255});
        // Rosy origami cheek blushes
        drawNGon(ren, rot(-0.38f, 0.24f), r * 0.13f, 6, angle, {245, 148, 156, 200});
    } else if (type == PigType::GLIDER) {
        // Gale's Folded Origami Glider Wings (swept aerofoils) & Brass Aviator Goggles
        tri(-0.35f, -0.45f, -0.95f, -1.35f, 0.15f, -0.85f, {210, 240, 255, 255});
        tri(-0.35f, -0.45f, -1.15f, -1.15f, -0.95f, -1.35f, {150, 200, 245, 255});
        crease(-0.35f, -0.45f, -0.95f, -1.35f, {255, 255, 255, 220}, 1.5f);
        quad(-0.70f, -0.62f, 0.76f, -0.62f, 0.80f, -0.46f, -0.74f, -0.46f, {62, 50, 42, 255});
        drawNGon(ren, rot(-0.20f, -0.56f), r * 0.22f, 8, angle, {228, 185, 75, 255});
        drawNGon(ren, rot(-0.20f, -0.56f), r * 0.16f, 8, angle, {140, 235, 255, 255});
        tri(-0.28f, -0.63f, -0.12f, -0.63f, -0.24f, -0.50f, {255, 255, 255, 240});
        drawNGon(ren, rot(0.34f, -0.56f), r * 0.22f, 8, angle, {228, 185, 75, 255});
        drawNGon(ren, rot(0.34f, -0.56f), r * 0.16f, 8, angle, {140, 235, 255, 255});
        tri(0.26f, -0.63f, 0.42f, -0.63f, 0.30f, -0.50f, {255, 255, 255, 240});
    } else if (type == PigType::GRAVITON) {
        // Atticus's Twin Electric Tesla Coils & Cosmic Monocle / Diadem
        quad(-0.65f, -0.62f, 0.70f, -0.62f, 0.75f, -0.46f, -0.68f, -0.46f, {45, 30, 75, 255});
        // Left & right copper tesla coils
        crease(-0.35f, -0.62f, -0.40f, -1.25f, {220, 140, 60, 255}, 2.5f);
        drawNGon(ren, rot(-0.40f, -1.25f), r * 0.12f, 6, angle, {120, 230, 255, 255});
        drawNGon(ren, rot(-0.40f, -1.25f), r * 0.06f, 6, angle, {255, 255, 255, 255});
        crease(0.35f, -0.62f, 0.40f, -1.25f, {220, 140, 60, 255}, 2.5f);
        drawNGon(ren, rot(0.40f, -1.25f), r * 0.12f, 6, angle, {120, 230, 255, 255});
        drawNGon(ren, rot(0.40f, -1.25f), r * 0.06f, 6, angle, {255, 255, 255, 255});
        // Cosmic glowing forehead crest gem
        drawNGon(ren, rot(0.0f, -0.54f), r * 0.14f, 4, angle, {240, 100, 255, 255});
        drawNGon(ren, rot(0.0f, -0.54f), r * 0.07f, 4, angle, {255, 255, 255, 255});
    } else if (type == PigType::DRILL) {
        // Rusty's Heavy Industrial Miner Helmet with Brass Headlamp
        quad(-0.72f, -0.68f, 0.76f, -0.68f, 0.80f, -0.44f, -0.76f, -0.44f, {65, 45, 30, 255});
        // Miner hard-hat dome
        quad(-0.62f, -1.15f, 0.62f, -1.15f, 0.74f, -0.68f, -0.74f, -0.68f, {195, 120, 40, 255});
        tri(-0.62f, -1.15f, 0.0f, -1.35f, 0.62f, -1.15f, {225, 145, 55, 255});
        // Center brass carbide miner headlamp
        drawNGon(ren, rot(0.0f, -0.78f), r * 0.18f, 8, angle, {235, 185, 60, 255});
        drawNGon(ren, rot(0.0f, -0.78f), r * 0.12f, 8, angle, {255, 250, 180, 255});
    } else {
        // Vulcan's Riveted Steel Brow Plate & Multi-Layer Burning Bomb Fuse
        quad(-0.58f, -0.82f, 0.58f, -0.82f, 0.68f, -0.46f, -0.64f, -0.46f, {42, 46, 56, 255});
        drawNGon(ren, rot(-0.38f, -0.64f), r * 0.06f, 6, angle, {235, 162, 68, 255});
        drawNGon(ren, rot(0.0f,   -0.64f), r * 0.06f, 6, angle, {235, 162, 68, 255});
        drawNGon(ren, rot(0.38f,  -0.64f), r * 0.06f, 6, angle, {235, 162, 68, 255});
        // Fuse cord & 3-layer sparkler flame
        crease(0.0f, -0.85f, -0.08f, -1.22f, {168, 138, 96, 255}, 3.2f);
        tri(-0.34f, -1.16f, -0.08f, -1.62f, 0.20f, -1.16f, {255, 88, 28, 255});
        tri(-0.24f, -1.18f, -0.08f, -1.48f, 0.10f, -1.18f, {255, 182, 42, 255});
        tri(-0.14f, -1.20f, -0.08f, -1.36f, 0.02f, -1.20f, {255, 250, 195, 255});
        // Heavy iron war-tusks
        tri(-0.10f, 0.44f, 0.04f, 0.86f, 0.18f, 0.44f, {225, 230, 238, 255});
        tri(0.56f, 0.42f, 0.76f, 0.84f, 0.84f, 0.40f, {225, 230, 238, 255});
    }

    // 6. High-Resolution Expressive Eyes (Outline + Sclera + Iris + Pupil + Specular Catchlight + Brow)
    auto drawDetailedEye = [&](float ex, float ey, float er, float browTilt) {
        drawNGon(ren, rot(ex, ey), r * (er + 0.04f), 8, angle, cRim);
        drawNGon(ren, rot(ex, ey), r * er, 8, angle, {255, 252, 242, 255});
        SDL_Color irisCol = (type == PigType::BOMB)     ? SDL_Color{235, 92, 34, 255} :
                            (type == PigType::SPLIT)    ? SDL_Color{42, 138, 96, 255} :
                            (type == PigType::GLIDER)   ? SDL_Color{28, 140, 230, 255} :
                            (type == PigType::GRAVITON) ? SDL_Color{210, 80, 255, 255} :
                            (type == PigType::DRILL)    ? SDL_Color{240, 160, 40, 255} :
                                                          SDL_Color{92, 54, 32, 255};
        drawNGon(ren, rot(ex + 0.03f, ey), r * (er * 0.62f), 8, angle, irisCol);
        drawNGon(ren, rot(ex + 0.04f, ey), r * (er * 0.38f), 8, angle, {18, 18, 24, 255});
        // Bright white specular catchlight dot
        drawNGon(ren, rot(ex - 0.03f, ey - 0.05f), r * (er * 0.24f), 6, angle, {255, 255, 255, 255});
        // Origami furrowed eyebrow plate
        quad(ex - er * 1.15f, ey - er * 1.05f - browTilt,
             ex + er * 1.15f, ey - er * 1.05f + browTilt,
             ex + er * 1.10f, ey - er * 0.62f + browTilt,
             ex - er * 1.10f, ey - er * 0.62f - browTilt, cDeep);
    };
    drawDetailedEye(-0.24f, -0.18f, 0.20f, 0.05f);
    drawDetailedEye(0.46f,  -0.22f, 0.18f, -0.05f);

    // 7. Dimensional 3D Origami Snout (Outer bezel, top lit facet, lower shadow facet, 2 sculpted nostrils)
    std::vector<Vec2> snoutRim = {
        rot(0.02f, -0.22f), rot(0.84f, -0.22f), rot(0.98f, 0.12f),
        rot(0.84f, 0.48f),  rot(0.02f, 0.48f),  rot(-0.12f, 0.12f)
    };
    drawConvexPoly(ren, snoutRim, cRim);
    // Upper sunlit snout plane
    quad(0.06f, -0.17f, 0.80f, -0.17f, 0.91f, 0.12f, -0.05f, 0.12f, cSnoutL);
    // Lower shaded snout plane
    quad(-0.05f, 0.12f, 0.91f, 0.12f, 0.80f, 0.42f, 0.06f, 0.42f, cSnoutD);
    // Center snout ridge crease
    crease(-0.04f, 0.12f, 0.90f, 0.12f, {255, 240, 225, 140}, 1.3f);
    // Sculpted diamond nostrils with inner depth
    drawNGon(ren, rot(0.28f, 0.11f), r * 0.13f, 6, angle, cDeep);
    drawNGon(ren, rot(0.30f, 0.12f), r * 0.08f, 6, angle, {24, 16, 16, 255});
    drawNGon(ren, rot(0.64f, 0.11f), r * 0.13f, 6, angle, cDeep);
    drawNGon(ren, rot(0.66f, 0.12f), r * 0.08f, 6, angle, {24, 16, 16, 255});

    if (type == PigType::DRILL) {
        // Forward spiral titanium drill bore extending past snout
        tri(0.40f, -0.08f, 1.45f, 0.12f, 0.40f, 0.32f, {210, 215, 225, 255});
        tri(0.40f, 0.32f, 1.45f, 0.12f, 0.60f, 0.45f, {170, 175, 185, 255});
        crease(0.45f, 0.05f, 1.30f, 0.12f, {245, 195, 60, 255}, 2.4f);
        crease(0.45f, 0.22f, 1.30f, 0.12f, {245, 195, 60, 255}, 2.4f);
        drawNGon(ren, rot(1.45f, 0.12f), r * 0.14f, 4, angle, {255, 220, 80, 255});
    }
}

void renderTotemBirdGeometry(SDL_Renderer* ren, BirdClass kind, Vec2 pos,
                             float r, float angle, float hpRatio,
                             bool hasHelmet, float helmetHpRatio,
                             bool hasShield, float shieldHpRatio,
                             bool inZeppelin, float balloonHpRatio) {
    float c = std::cos(angle), s = std::sin(angle);
    auto rot = [&](float lx, float ly) -> Vec2 {
        return {pos.x + (lx * c - ly * s) * r, pos.y + (lx * s + ly * c) * r};
    };
    auto tri = [&](float x0, float y0, float x1, float y1, float x2, float y2, SDL_Color col) {
        drawTriangle(ren, rot(x0, y0), rot(x1, y1), rot(x2, y2), col);
    };
    auto quad = [&](float x0, float y0, float x1, float y1,
                    float x2, float y2, float x3, float y3, SDL_Color col) {
        drawConvexPoly(ren, {rot(x0, y0), rot(x1, y1), rot(x2, y2), rot(x3, y3)}, col);
    };
    auto crease = [&](float x0, float y0, float x1, float y1, SDL_Color col, float w = 1.4f) {
        drawThickLine(ren, rot(x0, y0), rot(x1, y1), w, col);
    };

    SDL_Color cRim{22, 20, 26, 255};
    SDL_Color cTop, cMid, cDark, cWing1, cWing2, cBreast, cCrest, cBeak1, cBeak2;

    if (kind == BirdClass::TOUCAN) {
        // Sentry Toucan: Tropical Emerald-Teal & Rainbow Prism Beak
        cTop    = {58, 168, 178, 255};
        cMid    = {38, 132, 146, 255};
        cDark   = {24, 92, 106, 255};
        cWing1  = {28, 104, 122, 255};
        cWing2  = {18, 72, 88, 255};
        cBreast = {252, 206, 78, 255};
        cCrest  = {255, 196, 64, 255};
        cBeak1  = {255, 148, 48, 255};
        cBeak2  = {224, 68, 52, 255};
    } else if (kind == BirdClass::OWL) {
        // Warden Owl: Royal Indigo, Slate Armor & Silver Feather Chevrons
        cTop    = {112, 118, 168, 255};
        cMid    = {82, 88, 136, 255};
        cDark   = {56, 60, 102, 255};
        cWing1  = {64, 68, 114, 255};
        cWing2  = {44, 48, 84, 255};
        cBreast = {212, 218, 238, 255};
        cCrest  = {192, 202, 235, 255};
        cBeak1  = {238, 192, 78, 255};
        cBeak2  = {188, 142, 44, 255};
    } else if (kind == BirdClass::FALCON) {
        // Commander Falcon: Steel Peregrine Slate & Heavy Directional Tower Shield
        cTop    = {64, 84, 104, 255};
        cMid    = {44, 62, 80, 255};
        cDark   = {28, 42, 56, 255};
        cWing1  = {36, 52, 68, 255};
        cWing2  = {22, 34, 46, 255};
        cBreast = {230, 235, 245, 255};
        cCrest  = {210, 160, 45, 255};
        cBeak1  = {245, 190, 40, 255};
        cBeak2  = {195, 140, 25, 255};
    } else if (kind == BirdClass::CONDOR) {
        // Baron Von Condor: Jet-Black Mantle, Velvet Maroon Cowl & Steampunk Zeppelin
        cTop    = {45, 45, 52, 255};
        cMid    = {30, 30, 36, 255};
        cDark   = {18, 18, 22, 255};
        cWing1  = {115, 30, 36, 255};
        cWing2  = {80, 20, 24, 255};
        cBreast = {235, 230, 220, 255};
        cCrest  = {245, 240, 235, 255}; // Grand snow-white neck ruff
        cBeak1  = {235, 175, 50, 255};
        cBeak2  = {185, 125, 30, 255};
    } else {
        // Archduke Heron: Imperial Ivory, Royal Crimson Mantle & Golden Crown
        cTop    = {248, 244, 232, 255};
        cMid    = {224, 216, 198, 255};
        cDark   = {188, 178, 158, 255};
        cWing1  = {178, 38, 48, 255};  // Royal crimson cape/wings
        cWing2  = {132, 24, 32, 255};
        cBreast = {245, 204, 72, 255};
        cCrest  = {255, 212, 58, 255};
        cBeak1  = {238, 108, 64, 255};
        cBeak2  = {186, 64, 42, 255};
    }

    // 1. Splayed Origami Tail Feathers & Golden Talons at Base
    tri(-0.55f, 0.82f, -0.78f, 1.16f, -0.25f, 1.04f, cWing2);
    tri(0.25f,  1.04f, 0.78f,  1.16f, 0.55f,  0.82f, cWing2);
    // Left & right 2-toe origami talons gripping the perch
    tri(-0.48f, 0.94f, -0.58f, 1.12f, -0.34f, 1.06f, {235, 178, 54, 255});
    tri(-0.36f, 0.94f, -0.34f, 1.12f, -0.20f, 1.04f, {212, 152, 38, 255});
    tri(0.20f,  1.04f, 0.34f,  1.12f, 0.36f,  0.94f, {212, 152, 38, 255});
    tri(0.34f,  1.06f, 0.58f,  1.12f, 0.48f,  0.94f, {235, 178, 54, 255});

    // 2. Sculpted 10-Point Totem Silhouette Outline
    std::vector<Vec2> outline = {
        rot(-0.68f, -1.08f), rot(0.68f, -1.08f), rot(0.98f, -0.35f),
        rot(0.92f, 0.52f),   rot(0.68f, 1.04f),  rot(-0.68f, 1.04f),
        rot(-0.92f, 0.52f),  rot(-0.98f, -0.35f)
    };
    drawConvexPoly(ren, outline, cRim);

    // 3. Multi-Faceted Origami Torso & Head Planes
    quad(-0.60f, -0.98f, 0.60f, -0.98f, 0.78f, -0.32f, -0.78f, -0.32f, cTop);
    quad(-0.78f, -0.32f, 0.78f, -0.32f, 0.74f, 0.42f,  -0.74f, 0.42f,  cMid);
    quad(-0.74f, 0.42f,  0.74f, 0.42f,  0.58f, 0.95f,  -0.58f, 0.95f,  cDark);

    // 4. Layered 3-Tier Origami Folded Wings (Shoulder, Primary, Secondary Feathers)
    // Left Wing
    tri(-0.76f, -0.42f, -0.96f, 0.18f, -0.52f, 0.22f, cWing1);
    tri(-0.92f, 0.18f,  -0.86f, 0.82f, -0.44f, 0.62f, cWing2);
    tri(-0.76f, -0.10f, -0.88f, 0.52f, -0.48f, 0.42f, cWing1);
    // Right Wing
    tri(0.76f, -0.42f, 0.52f, 0.22f, 0.96f, 0.18f, cWing1);
    tri(0.92f, 0.18f,  0.44f, 0.62f, 0.86f, 0.82f, cWing2);
    tri(0.76f, -0.10f, 0.48f, 0.42f, 0.88f, 0.52f, cWing1);

    // 5. Geometric Chest Plastron & Feather Chevrons
    tri(-0.52f, 0.04f, 0.52f, 0.04f, 0.0f, 0.84f, cBreast);
    if (kind == BirdClass::OWL) {
        // Silver parliament owl chest chevrons
        tri(-0.34f, 0.24f, 0.34f, 0.24f, 0.0f, 0.54f, cDark);
        tri(-0.22f, 0.46f, 0.22f, 0.46f, 0.0f, 0.72f, cDark);
    } else if (kind == BirdClass::HERON) {
        // Archduke's Golden Royal Gorget & Crimson Ruby Medallion
        quad(-0.48f, 0.02f, 0.48f, 0.02f, 0.34f, 0.26f, -0.34f, 0.26f, {245, 192, 52, 255});
        drawNGon(ren, rot(0.0f, 0.18f), r * 0.15f, 6, angle, {214, 32, 48, 255});
        drawNGon(ren, rot(-0.02f, 0.16f), r * 0.06f, 4, angle, {255, 195, 205, 255});
    } else {
        // Toucan tropical crimson chest bib border
        tri(-0.36f, 0.08f, 0.36f, 0.08f, 0.0f, 0.46f, {228, 76, 54, 255});
    }

    // 6. Class-Specific Crown / Horns / Crest
    if (kind == BirdClass::HERON) {
        // 5-Spire Imperial Gold Crown with Jewel Facets
        quad(-0.54f, -1.04f, 0.54f, -1.04f, 0.60f, -0.84f, -0.60f, -0.84f, {218, 156, 28, 255});
        tri(-0.58f, -0.98f, -0.44f, -1.56f, -0.20f, -0.98f, cCrest);
        tri(-0.28f, -0.98f, -0.16f, -1.42f,  0.0f,  -0.98f, {235, 178, 38, 255});
        tri(-0.22f, -0.98f,  0.0f,  -1.74f,  0.22f, -0.98f, cCrest);
        tri(0.0f,   -0.98f,  0.16f, -1.42f,  0.28f, -0.98f, {235, 178, 38, 255});
        tri(0.20f,  -0.98f,  0.44f, -1.56f,  0.58f, -0.98f, cCrest);
        // Crown jewels
        drawNGon(ren, rot(0.0f, -1.12f), r * 0.09f, 4, angle, {224, 38, 52, 255});
        drawNGon(ren, rot(-0.34f, -1.08f), r * 0.07f, 4, angle, {52, 195, 235, 255});
        drawNGon(ren, rot(0.34f,  -1.08f), r * 0.07f, 4, angle, {52, 195, 235, 255});
    } else if (kind == BirdClass::OWL) {
        // Swept-Up Great Horned Owl 3-Facet Ear Tufts + Facial Disc
        tri(-0.68f, -0.88f, -1.02f, -1.52f, -0.24f, -0.96f, cRim);
        tri(-0.62f, -0.90f, -0.94f, -1.44f, -0.28f, -0.96f, cCrest);
        tri(0.24f,  -0.96f,  1.02f, -1.52f,  0.68f, -0.88f, cRim);
        tri(0.28f,  -0.96f,  0.94f, -1.44f,  0.62f, -0.90f, cCrest);
        // Owl concentric facial disc rings
        drawNGon(ren, rot(-0.30f, -0.45f), r * 0.30f, 8, angle, {162, 170, 212, 255});
        drawNGon(ren, rot(0.30f,  -0.45f), r * 0.30f, 8, angle, {162, 170, 212, 255});
    } else {
        // Toucan 3-Feather Tropical Sunburst Crest
        tri(-0.32f, -0.96f, -0.12f, -1.46f, 0.12f, -0.96f, {242, 118, 48, 255});
        tri(-0.08f, -0.96f,  0.28f, -1.54f, 0.42f, -0.96f, cCrest);
        tri(0.18f,  -0.96f,  0.56f, -1.36f, 0.58f, -0.92f, {92, 218, 168, 255});
    }

    // 7. Expressive Avian Eyes with Specular Highlights & Angry/Panicked Totem Brows
    float pupilScale = (hpRatio < 0.30f) ? 0.16f : ((hpRatio < 0.60f) ? 0.12f : 0.085f);
    float browSlopeAdj = (hpRatio < 0.38f) ? -0.11f : 0.0f;
    auto drawBirdEye = [&](float ex, float ey, float browSlope) {
        drawNGon(ren, rot(ex, ey), r * 0.23f, 8, angle, cRim);
        drawNGon(ren, rot(ex, ey), r * 0.19f, 8, angle,
                 (kind == BirdClass::OWL) ? SDL_Color{255, 224, 98, 255} : SDL_Color{255, 250, 236, 255});
        drawNGon(ren, rot(ex - 0.02f, ey), r * pupilScale, 8, angle, {18, 18, 24, 255});
        drawNGon(ren, rot(ex + 0.04f, ey - 0.05f), r * 0.055f, 6, angle, {255, 255, 255, 255});
        // Dynamic brow ridge: fierce V-shape when healthy, worried/shocked raise when damaged
        quad(ex - 0.24f, ey - 0.20f - browSlope,
             ex + 0.24f, ey - 0.20f + browSlope,
             ex + 0.22f, ey - 0.10f + browSlope,
             ex - 0.22f, ey - 0.10f - browSlope, cRim);
    };
    drawBirdEye(-0.30f, -0.46f, -0.06f - browSlopeAdj);
    drawBirdEye(0.30f,  -0.46f,  0.06f + browSlopeAdj);

    // Cross-shaped adhesive bandage on temple when heavily damaged
    if (hpRatio < 0.38f) {
        quad(0.36f, -0.74f, 0.58f, -0.66f, 0.52f, -0.52f, 0.30f, -0.60f, {242, 222, 172, 240});
        quad(0.48f, -0.74f, 0.40f, -0.52f, 0.48f, -0.48f, 0.56f, -0.70f, {242, 222, 172, 240});
    }

    // 8. Sculpted Multi-Faceted Beaks
    if (kind == BirdClass::TOUCAN) {
        // Giant 4-Facet Tropical Prism Beak projecting left toward the catapult
        tri(-0.38f, -0.44f, -1.52f, -0.14f, -0.38f, -0.08f, cBeak1);
        tri(-0.38f, -0.08f, -1.52f, -0.14f, -0.42f, 0.22f,  cBeak2);
        tri(-1.12f, -0.24f, -1.52f, -0.14f, -1.12f, -0.02f, {32, 28, 36, 255}); // Black toucan beak tip
        crease(-0.38f, -0.08f, -1.48f, -0.14f, {255, 238, 140, 220}, 1.6f);
    } else {
        // Symmetrical Diamond-Cut Raptor/Heron Beak with central ridge
        tri(-0.34f, -0.22f, 0.0f, -0.34f, 0.0f, 0.42f, cBeak1);
        tri(0.0f,   -0.34f, 0.34f, -0.22f, 0.0f, 0.42f, cBeak2);
        crease(0.0f, -0.32f, 0.0f, 0.40f, {255, 244, 195, 200}, 1.5f);
    }

    // 9. Cauldron / Iron Kettle Helmet (Feature Pack B)
    if (hasHelmet) {
        // Iron Cauldron Helmet Body
        quad(-0.62f, -1.35f, 0.62f, -1.35f, 0.72f, -0.72f, -0.72f, -0.72f, {78, 84, 98, 255});
        // Top dome
        tri(-0.62f, -1.35f, 0.0f, -1.65f, 0.62f, -1.35f, {92, 100, 116, 255});
        // Flared rim with brass rivet studs
        quad(-0.76f, -0.72f, 0.76f, -0.72f, 0.74f, -0.58f, -0.74f, -0.58f, {52, 56, 68, 255});
        for (int rv = -3; rv <= 3; ++rv) {
            drawNGon(ren, rot(rv * 0.20f, -0.65f), r * 0.045f, 4, angle, {228, 185, 75, 255});
        }
        // Top kettle handle loop
        drawNGon(ren, rot(0.0f, -1.72f), r * 0.12f, 6, angle, {65, 70, 82, 255});
        drawNGon(ren, rot(0.0f, -1.72f), r * 0.06f, 6, angle, {20, 24, 32, 255});
        // Ear flaps
        tri(-0.74f, -0.72f, -0.84f, -0.32f, -0.62f, -0.44f, {72, 78, 92, 255});
        tri(0.74f, -0.72f, 0.62f, -0.44f, 0.84f, -0.32f, {72, 78, 92, 255});
        // Armor crack if damaged
        if (helmetHpRatio < 0.55f) {
            crease(-0.35f, -1.35f, 0.15f, -0.82f, {25, 28, 36, 255}, 2.5f);
            crease(0.15f, -0.82f, 0.40f, -0.64f, {25, 28, 36, 255}, 2.0f);
        }
    }

    // 10. Commander Falcon Heavy Directional Tower Shield
    if (hasShield) {
        float sX = -0.92f;
        quad(sX - 0.42f, -0.95f, sX + 0.18f, -0.95f, sX + 0.14f, 0.95f, sX - 0.46f, 0.95f, {32, 36, 44, 255});
        quad(sX - 0.38f, -0.90f, sX + 0.14f, -0.90f, sX + 0.10f, 0.90f, sX - 0.42f, 0.90f, {78, 88, 106, 255});
        drawThickLine(ren, rot(sX - 0.14f, -0.75f), rot(sX - 0.14f, 0.75f), 3.2f, {230, 185, 65, 255});
        drawThickLine(ren, rot(sX - 0.36f, -0.10f), rot(sX + 0.08f, -0.10f), 3.2f, {230, 185, 65, 255});
        drawNGon(ren, rot(sX - 0.32f, -0.82f), r * 0.045f, 4, angle, {245, 205, 80, 255});
        drawNGon(ren, rot(sX + 0.08f, -0.82f), r * 0.045f, 4, angle, {245, 205, 80, 255});
        drawNGon(ren, rot(sX - 0.35f, 0.82f),  r * 0.045f, 4, angle, {245, 205, 80, 255});
        drawNGon(ren, rot(sX + 0.05f, 0.82f),  r * 0.045f, 4, angle, {245, 205, 80, 255});
        if (shieldHpRatio < 0.55f) {
            crease(sX - 0.32f, -0.35f, sX - 0.05f, 0.15f, {22, 24, 30, 255}, 2.4f);
            crease(sX - 0.05f, 0.15f, sX - 0.28f, 0.55f, {22, 24, 30, 255}, 2.0f);
        }
    }

    // 11. Baron Von Condor Suspended Steampunk Zeppelin
    if (inZeppelin) {
        float bY = -1.95f;
        drawConvexPoly(ren, {
            rot(-1.35f, bY), rot(-0.95f, bY - 0.65f), rot(0.95f, bY - 0.65f),
            rot(1.35f, bY),  rot(0.95f, bY + 0.65f),  rot(-0.95f, bY + 0.65f)
        }, {38, 28, 20, 255});
        quad(-0.90f, bY - 0.58f, 0.90f, bY - 0.58f, 0.90f, bY + 0.58f, -0.90f, bY + 0.58f, {185, 145, 95, 255});
        tri(-0.90f, bY - 0.58f, -1.28f, bY, -0.90f, bY + 0.58f, {155, 115, 75, 255});
        tri(0.90f, bY - 0.58f, 1.28f, bY, 0.90f, bY + 0.58f, {215, 175, 115, 255});
        crease(-0.45f, bY - 0.58f, -0.45f, bY + 0.58f, {125, 85, 45, 255}, 2.2f);
        crease(0.0f,   bY - 0.62f, 0.0f,   bY + 0.62f, {125, 85, 45, 255}, 2.5f);
        crease(0.45f,  bY - 0.58f, 0.45f,  bY + 0.58f, {125, 85, 45, 255}, 2.2f);
        drawNGon(ren, rot(1.28f, bY), r * 0.12f, 6, angle, {235, 185, 55, 255});
        tri(-1.28f, bY, -1.55f, bY - 0.45f, -1.15f, bY - 0.45f, {135, 90, 50, 255});
        tri(-1.28f, bY, -1.55f, bY + 0.45f, -1.15f, bY + 0.45f, {135, 90, 50, 255});
        crease(-0.75f, bY + 0.58f, -0.45f, -0.75f, {55, 50, 45, 255}, 1.4f);
        crease(-0.25f, bY + 0.58f, -0.15f, -0.75f, {55, 50, 45, 255}, 1.4f);
        crease(0.25f,  bY + 0.58f, 0.15f,  -0.75f, {55, 50, 45, 255}, 1.4f);
        crease(0.75f,  bY + 0.58f, 0.45f,  -0.75f, {55, 50, 45, 255}, 1.4f);
        if (balloonHpRatio < 0.55f) {
            crease(-0.35f, bY - 0.35f, 0.15f, bY + 0.25f, {45, 30, 20, 255}, 2.8f);
            crease(0.20f, bY - 0.20f, 0.55f, bY + 0.15f, {45, 30, 20, 255}, 2.4f);
        }
    }
}

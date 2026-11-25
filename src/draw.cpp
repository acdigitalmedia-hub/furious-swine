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

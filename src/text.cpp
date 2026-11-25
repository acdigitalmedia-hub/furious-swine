#include "text.h"

#include <cstring>
#include <fstream>
#include <unordered_map>

namespace {

struct GlyphRect {
    int16_t x = 0, y = 0, w = 0, h = 0, ox = 0, oy = 0;
};

struct Glyph {
    float advance = 0.0f;
    GlyphRect fill, stroke;
};

struct BakedFace {
    uint8_t id = 0;
    float px = 0.0f, ascent = 0.0f, line = 0.0f, strokePx = 0.0f;
    float capTop = 0.0f, capHeight = 0.0f;  // at baked size, from 'H'
    std::unordered_map<uint32_t, Glyph> glyphs;
};

struct FontAtlas {
    bool tried = false;
    bool ok = false;
    int w = 0, h = 0;
    std::vector<uint8_t> alpha;
    std::vector<BakedFace> faces;
    std::unordered_map<SDL_Renderer*, SDL_Texture*> textures;
};

FontAtlas gFont;

template <typename T>
bool readPod(const std::vector<char>& buf, size_t& pos, T& out) {
    if (pos + sizeof(T) > buf.size()) return false;
    std::memcpy(&out, buf.data() + pos, sizeof(T));
    pos += sizeof(T);
    return true;
}

bool parseFont(const std::vector<char>& buf, FontAtlas& f) {
    size_t pos = 0;
    if (buf.size() < 8 || std::memcmp(buf.data(), "SSFONT2\0", 8) != 0) return false;
    pos = 8;
    uint16_t aw = 0, ah = 0;
    uint32_t rleBytes = 0;
    if (!readPod(buf, pos, aw) || !readPod(buf, pos, ah) || !readPod(buf, pos, rleBytes)) return false;
    if (aw == 0 || ah == 0 || pos + rleBytes > buf.size()) return false;
    f.w = aw;
    f.h = ah;
    f.alpha.assign(static_cast<size_t>(aw) * ah, 0);
    size_t out = 0;
    size_t end = pos + rleBytes;
    while (pos < end && out < f.alpha.size()) {
        uint8_t b = static_cast<uint8_t>(buf[pos++]);
        if (b == 0) {
            uint16_t run = 0;
            if (!readPod(buf, pos, run)) return false;
            out += run;
        } else {
            f.alpha[out++] = b;
        }
    }
    pos = end;
    uint16_t faceCount = 0;
    if (!readPod(buf, pos, faceCount)) return false;
    for (uint16_t i = 0; i < faceCount; ++i) {
        BakedFace face;
        uint16_t glyphCount = 0;
        if (!readPod(buf, pos, face.id) || !readPod(buf, pos, face.px) || !readPod(buf, pos, face.ascent) ||
            !readPod(buf, pos, face.line) || !readPod(buf, pos, face.strokePx) || !readPod(buf, pos, glyphCount)) {
            return false;
        }
        for (uint16_t g = 0; g < glyphCount; ++g) {
            uint32_t cp = 0;
            Glyph gl;
            if (!readPod(buf, pos, cp) || !readPod(buf, pos, gl.advance)) return false;
            for (GlyphRect* r : {&gl.fill, &gl.stroke}) {
                if (!readPod(buf, pos, r->x) || !readPod(buf, pos, r->y) || !readPod(buf, pos, r->w) ||
                    !readPod(buf, pos, r->h) || !readPod(buf, pos, r->ox) || !readPod(buf, pos, r->oy)) {
                    return false;
                }
            }
            face.glyphs[cp] = gl;
        }
        auto H = face.glyphs.find('H');
        if (H != face.glyphs.end()) {
            face.capTop = H->second.fill.oy;
            face.capHeight = H->second.fill.h;
        } else {
            face.capTop = face.ascent * 0.25f;
            face.capHeight = face.px * 0.7f;
        }
        f.faces.push_back(std::move(face));
    }
    return !f.faces.empty();
}

void ensureLoaded() {
    if (!gFont.tried) textLoadFont();
}

const BakedFace* pickFace(FontFace face, float size) {
    ensureLoaded();
    const BakedFace* best = nullptr;
    const BakedFace* largest = nullptr;
    for (const auto& f : gFont.faces) {
        if (f.id != static_cast<uint8_t>(face)) continue;
        if (!largest || f.px > largest->px) largest = &f;
        // Smallest bake that is at least ~the requested size (downscaling
        // a little looks crisp; upscaling looks soft).
        if (f.px >= size * 0.92f && (!best || f.px < best->px)) best = &f;
    }
    return best ? best : largest;
}

uint32_t nextCodepoint(const std::string& s, size_t& i) {
    unsigned char c = static_cast<unsigned char>(s[i++]);
    if (c < 0x80) return c;
    int extra = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
    uint32_t cp = c & (0x3F >> extra);
    for (int k = 0; k < extra && i < s.size(); ++k) {
        cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
    }
    return cp;
}

const Glyph* findGlyph(const BakedFace& f, uint32_t cp) {
    auto it = f.glyphs.find(cp);
    if (it != f.glyphs.end()) return &it->second;
    it = f.glyphs.find('?');
    return it != f.glyphs.end() ? &it->second : nullptr;
}

SDL_Texture* atlasTexture(SDL_Renderer* ren) {
    auto it = gFont.textures.find(ren);
    if (it != gFont.textures.end()) return it->second;
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, gFont.w, gFont.h, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Texture* tex = nullptr;
    if (surf) {
        for (int y = 0; y < gFont.h; ++y) {
            auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(surf->pixels) + y * surf->pitch);
            for (int x = 0; x < gFont.w; ++x) {
                row[x] = (static_cast<uint32_t>(gFont.alpha[y * gFont.w + x]) << 24) | 0x00FFFFFFu;
            }
        }
        tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (tex) {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(tex, 1);
        }
    }
    gFont.textures[ren] = tex;
    return tex;
}

// Emits one pass (fill or stroke masks) of a line into vertex/index buffers.
void emitPass(const BakedFace& f, const std::string& s, float x, float y, float scale, bool stroke,
              SDL_Color col, std::vector<SDL_Vertex>& v, std::vector<int>& idx) {
    float pen = x;
    float invW = 1.0f / gFont.w, invH = 1.0f / gFont.h;
    for (size_t i = 0; i < s.size();) {
        uint32_t cp = nextCodepoint(s, i);
        const Glyph* g = findGlyph(f, cp);
        if (!g) continue;
        const GlyphRect& r = stroke ? g->stroke : g->fill;
        if (r.w > 1 && r.h > 1) {
            float gx = pen + r.ox * scale;
            float gy = y + r.oy * scale;
            if (scale > 0.98f && scale < 1.02f) {  // keep 1:1 glyphs pixel-aligned
                gx = std::round(gx);
                gy = std::round(gy);
            }
            float gw = r.w * scale, gh = r.h * scale;
            float u0 = r.x * invW, v0 = r.y * invH, u1 = (r.x + r.w) * invW, v1 = (r.y + r.h) * invH;
            int base = static_cast<int>(v.size());
            v.push_back({{gx, gy}, col, {u0, v0}});
            v.push_back({{gx + gw, gy}, col, {u1, v0}});
            v.push_back({{gx + gw, gy + gh}, col, {u1, v1}});
            v.push_back({{gx, gy + gh}, col, {u0, v1}});
            for (int k : {0, 1, 2, 0, 2, 3}) idx.push_back(base + k);
        }
        pen += g->advance * scale;
    }
}

}  // namespace

bool textLoadFont(const std::string& path) {
    gFont.tried = true;
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return gFont.ok;  // keep whatever is already loaded
    std::vector<char> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    FontAtlas fresh;
    fresh.tried = true;
    if (!parseFont(buf, fresh)) {
        std::cerr << "[TEXT] Could not parse font atlas " << path << "; using bitmap font\n";
        return gFont.ok;
    }
    fresh.ok = true;
    gFont = std::move(fresh);
    return true;
}

bool textFontAvailable() {
    ensureLoaded();
    return gFont.ok;
}

float textWidth(const std::string& s, FontFace face, float size) {
    const BakedFace* f = pickFace(face, size);
    if (!f) return s.size() * size * 0.6f;
    float scale = size / f->px, w = 0.0f;
    for (size_t i = 0; i < s.size();) {
        const Glyph* g = findGlyph(*f, nextCodepoint(s, i));
        if (g) w += g->advance * scale;
    }
    return w;
}

float textCapTop(FontFace face, float size) {
    const BakedFace* f = pickFace(face, size);
    return f ? f->capTop * size / f->px : 0.0f;
}

float textCapHeight(FontFace face, float size) {
    const BakedFace* f = pickFace(face, size);
    return f ? f->capHeight * size / f->px : size * 0.7f;
}

float textLineHeight(FontFace face, float size) {
    const BakedFace* f = pickFace(face, size);
    return f ? f->line * size / f->px : size * 1.2f;
}

float drawTextStyled(SDL_Renderer* ren, const std::string& s, float x, float y, const TextStyle& st) {
    if (s.empty()) return 0.0f;
    const BakedFace* f = pickFace(st.face, st.size);
    if (!f) return 0.0f;
    SDL_Texture* tex = atlasTexture(ren);
    if (!tex) return 0.0f;

    float scale = st.size / f->px;
    float w = textWidth(s, st.face, st.size);
    if (st.align == TextAlign::CENTER) x -= w * 0.5f;
    else if (st.align == TextAlign::RIGHT) x -= w;

    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    v.reserve(s.size() * 12);
    idx.reserve(s.size() * 18);
    bool outlined = st.outline.a > 0;
    if (st.shadow.a > 0) {
        emitPass(*f, s, x, y + st.shadowDy, scale, outlined, st.shadow, v, idx);
    }
    if (outlined) emitPass(*f, s, x, y, scale, true, st.outline, v, idx);
    emitPass(*f, s, x, y, scale, false, st.color, v, idx);
    if (!idx.empty()) {
        SDL_RenderGeometry(ren, tex, v.data(), static_cast<int>(v.size()), idx.data(), static_cast<int>(idx.size()));
    }
    return w;
}

float drawTextWrapped(SDL_Renderer* ren, const std::string& s, float x, float y, float maxWidth,
                      const TextStyle& st, float lineGap) {
    float lh = textLineHeight(st.face, st.size) * lineGap;
    std::vector<std::string> lines;
    std::string line, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string trial = line.empty() ? word : line + " " + word;
        if (!line.empty() && textWidth(trial, st.face, st.size) > maxWidth) {
            lines.push_back(line);
            line = word;
        } else {
            line = trial;
        }
        word.clear();
    };
    for (char c : s) {
        if (c == ' ') flushWord();
        else if (c == '\n') { flushWord(); lines.push_back(line); line.clear(); }
        else word += c;
    }
    flushWord();
    if (!line.empty()) lines.push_back(line);
    for (size_t i = 0; i < lines.size(); ++i) drawTextStyled(ren, lines[i], x, y + i * lh, st);
    return lines.size() * lh;
}

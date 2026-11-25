#include "level_format.h"

#include "game.h"

#include <charconv>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {

// ---------------------------------------------------------------- name tables
struct NamedMaterial { const char* name; MaterialTier mat; };
const NamedMaterial kMaterials[] = {
    {"twig", MaterialTier::TWIG},   {"timber", MaterialTier::TIMBER},
    {"stone", MaterialTier::STONE}, {"ice", MaterialTier::ICE},
    {"tnt", MaterialTier::TNT},     {"trampoline", MaterialTier::TRAMPOLINE},
    {"magma", MaterialTier::MAGMA_VENT},
};
struct NamedBird { const char* name; BirdClass kind; };
const NamedBird kBirds[] = {
    {"toucan", BirdClass::TOUCAN}, {"owl", BirdClass::OWL}, {"heron", BirdClass::HERON},
    {"falcon", BirdClass::FALCON}, {"condor", BirdClass::CONDOR},
};
struct NamedPig { const char* name; PigType type; };
const NamedPig kPigs[] = {
    {"trotter", PigType::TROTTER}, {"turbo", PigType::TURBO}, {"split", PigType::SPLIT},
    {"bomb", PigType::BOMB},       {"glider", PigType::GLIDER}, {"graviton", PigType::GRAVITON},
    {"drill", PigType::DRILL},
};

// Per-entity stat fields that a level file may override with field=value.
template <typename T>
struct FloatField { const char* name; float T::*ptr; };
template <typename T>
struct BoolField { const char* name; bool T::*ptr; };

const FloatField<BirdTarget> kBirdFloats[] = {
    {"radius", &BirdTarget::radius}, {"angle", &BirdTarget::angle}, {"angVel", &BirdTarget::angVel},
    {"mass", &BirdTarget::mass}, {"invMass", &BirdTarget::invMass},
    {"hp", &BirdTarget::hp}, {"maxHp", &BirdTarget::maxHp},
    {"helmetHp", &BirdTarget::helmetHp}, {"maxHelmetHp", &BirdTarget::maxHelmetHp},
    {"shieldHp", &BirdTarget::shieldHp}, {"maxShieldHp", &BirdTarget::maxShieldHp},
    {"balloonHp", &BirdTarget::balloonHp}, {"maxBalloonHp", &BirdTarget::maxBalloonHp},
};
const BoolField<BirdTarget> kBirdBools[] = {
    {"hasHelmet", &BirdTarget::hasHelmet}, {"hasShield", &BirdTarget::hasShield},
    {"inZeppelin", &BirdTarget::inZeppelin},
};
const FloatField<BlockBody> kBlockFloats[] = {
    {"angle", &BlockBody::angle}, {"angVel", &BlockBody::angVel},
    {"mass", &BlockBody::mass}, {"invMass", &BlockBody::invMass},
    {"inertia", &BlockBody::inertia}, {"invInertia", &BlockBody::invInertia},
    {"restitution", &BlockBody::restitution}, {"friction", &BlockBody::friction},
    {"hp", &BlockBody::hp}, {"maxHp", &BlockBody::maxHp},
};
const FloatField<PendulumBody> kPendulumFloats[] = {
    {"angle", &PendulumBody::angle}, {"angVel", &PendulumBody::angVel},
    {"radius", &PendulumBody::radius}, {"mass", &PendulumBody::mass},
};

// ---------------------------------------------------------------- helpers
std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<std::string> splitWords(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string w;
    while (ss >> w) out.push_back(w);
    return out;
}

bool parseFloat(const std::string& s, float& out) {
    const char* first = s.data();
    const char* last = s.data() + s.size();
    if (first != last && *first == '+') ++first;
    float v = 0.0f;
    auto res = std::from_chars(first, last, v);
    if (res.ec != std::errc() || res.ptr != last || !std::isfinite(v)) return false;
    out = v;
    return true;
}

bool parseInt(const std::string& s, int& out) {
    auto res = std::from_chars(s.data(), s.data() + s.size(), out);
    return res.ec == std::errc() && res.ptr == s.data() + s.size();
}

// Shortest decimal text that parses back to exactly the same float.
std::string fmt(float v) {
    char buf[32];
    auto res = std::to_chars(buf, buf + sizeof(buf), v);
    return std::string(buf, res.ptr);
}

bool isSaneCoord(float v) { return v > -2000.0f && v < 4000.0f; }

template <typename T, size_t N>
bool applyOverride(T& obj, const std::string& token, const FloatField<T> (&floats)[N]) {
    size_t eq = token.find('=');
    if (eq == std::string::npos) return false;
    std::string key = token.substr(0, eq);
    float v;
    if (!parseFloat(token.substr(eq + 1), v)) return false;
    for (const auto& f : floats) {
        if (key == f.name) { obj.*(f.ptr) = v; return true; }
    }
    return false;
}

template <typename T, size_t N>
bool applyBoolOverride(T& obj, const std::string& token, const BoolField<T> (&bools)[N]) {
    size_t eq = token.find('=');
    if (eq == std::string::npos) return false;
    std::string key = token.substr(0, eq);
    std::string val = token.substr(eq + 1);
    if (val != "0" && val != "1") return false;
    for (const auto& f : bools) {
        if (key == f.name) { obj.*(f.ptr) = (val == "1"); return true; }
    }
    return false;
}

template <typename T, size_t N>
void emitFloatOverrides(std::ostream& out, const T& actual, const T& base, const FloatField<T> (&floats)[N]) {
    for (const auto& f : floats) {
        if (actual.*(f.ptr) != base.*(f.ptr)) out << ' ' << f.name << '=' << fmt(actual.*(f.ptr));
    }
}

template <typename T, size_t N>
void emitBoolOverrides(std::ostream& out, const T& actual, const T& base, const BoolField<T> (&bools)[N]) {
    for (const auto& f : bools) {
        if (actual.*(f.ptr) != base.*(f.ptr)) out << ' ' << f.name << '=' << (actual.*(f.ptr) ? 1 : 0);
    }
}

bool lookupMaterial(const std::string& w, MaterialTier& out) {
    int n;
    if (parseInt(w, n)) {
        if (n < 0 || n > static_cast<int>(MaterialTier::MAGMA_VENT)) return false;
        out = static_cast<MaterialTier>(n);
        return true;
    }
    for (const auto& m : kMaterials) if (w == m.name) { out = m.mat; return true; }
    return false;
}

bool lookupBird(const std::string& w, BirdClass& out) {
    int n;
    if (parseInt(w, n)) {
        if (n < 0 || n > static_cast<int>(BirdClass::CONDOR)) return false;
        out = static_cast<BirdClass>(n);
        return true;
    }
    for (const auto& b : kBirds) if (w == b.name) { out = b.kind; return true; }
    return false;
}

const char* materialName(MaterialTier m) {
    for (const auto& e : kMaterials) if (e.mat == m) return e.name;
    return "timber";
}
const char* birdName(BirdClass k) {
    for (const auto& e : kBirds) if (e.kind == k) return e.name;
    return "toucan";
}
const char* pigName(PigType t) {
    for (const auto& e : kPigs) if (e.type == t) return e.name;
    return "trotter";
}

bool parseBlock(const std::vector<std::string>& w, BlockBody& out) {
    MaterialTier mat;
    float x, y, bw, bh;
    if (w.size() < 5 || !lookupMaterial(w[0], mat) || !parseFloat(w[1], x) || !parseFloat(w[2], y) ||
        !parseFloat(w[3], bw) || !parseFloat(w[4], bh)) return false;
    if (!isSaneCoord(x) || !isSaneCoord(y) || bw < 4.0f || bh < 4.0f || bw > 1000.0f || bh > 1000.0f) return false;
    out = SnoutAndSiegeGame::makeBlock(mat, x, y, bw, bh);
    for (size_t i = 5; i < w.size(); ++i) {
        if (!applyOverride(out, w[i], kBlockFloats)) return false;
    }
    return true;
}

bool parseBird(const std::vector<std::string>& w, bool legacy, BirdTarget& out) {
    BirdClass kind;
    float x, y;
    if (w.size() < 3 || !lookupBird(w[0], kind) || !parseFloat(w[1], x) || !parseFloat(w[2], y)) return false;
    if (!isSaneCoord(x) || !isSaneCoord(y)) return false;
    bool helmet = false, shield = false, zeppelin = false;
    size_t i = 3;
    if (legacy && w.size() >= 4) {  // old editor format: "<kind> <x> <y> <0|1 helmet>"
        helmet = (w[3] != "0");
        i = 4;
    }
    std::vector<std::string> overrides;
    for (; i < w.size(); ++i) {
        if (w[i] == "helmet") helmet = true;
        else if (w[i] == "shield") shield = true;
        else if (w[i] == "zeppelin") zeppelin = true;
        else overrides.push_back(w[i]);
    }
    out = SnoutAndSiegeGame::makeBird(kind, x, y, helmet, shield, zeppelin);
    for (const auto& o : overrides) {
        if (!applyOverride(out, o, kBirdFloats) && !applyBoolOverride(out, o, kBirdBools)) return false;
    }
    return true;
}

bool parsePendulum(const std::vector<std::string>& w, bool legacy, PendulumBody& out) {
    float ax, ay, len;
    if (w.size() < 3 || !parseFloat(w[0], ax) || !parseFloat(w[1], ay) || !parseFloat(w[2], len)) return false;
    if (!isSaneCoord(ax) || !isSaneCoord(ay) || len <= 10.0f || len >= 1000.0f) return false;
    if (legacy) {  // old editor format: "<x> <y> <len> <0|1 spiked>"
        out = SnoutAndSiegeGame::makeEditorPendulum(ax, ay, w.size() >= 4 && w[3] != "0", len);
        return true;
    }
    out = PendulumBody{};
    out.anchor = {ax, ay};
    out.length = len;
    out.spiked = false;
    for (size_t i = 3; i < w.size(); ++i) {
        if (w[i] == "spiked") out.spiked = true;
        else if (!applyOverride(out, w[i], kPendulumFloats)) return false;
    }
    return out.radius > 0.0f && out.mass > 0.0f;
}

}  // namespace

bool parseLevelText(const std::string& text, ParsedLevel& out, std::string* warnings) {
    out = ParsedLevel{};
    out.spec.pigQueue.clear();
    // Files without a "format = 2" line are old editor saves.
    out.legacyFormat = (text.find("\nformat = 2") == std::string::npos && text.rfind("format = 2", 0) != 0);

    std::istringstream in(text);
    std::string raw;
    int lineNo = 0;
    bool anything = false;
    float truffleX = 0.0f, truffleY = 0.0f;
    bool haveTruffleX = false, haveTruffleY = false;
    bool truffleExists = false;
    auto warn = [&](const std::string& msg) {
        if (warnings) *warnings += "line " + std::to_string(lineNo) + ": " + msg + "\n";
    };

    while (std::getline(in, raw)) {
        ++lineNo;
        std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) { warn("missing '='"); continue; }
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        std::vector<std::string> words = splitWords(val);
        LevelSpec& L = out.spec;
        bool ok = true;

        if (key == "format") {
            // handled above
        } else if (key == "name") {
            L.name = val;
        } else if (key == "subtitle") {
            L.subtitle = val;
        } else if (key == "pigs") {
            out.hasPigs = true;
            for (const auto& w : words) {
                bool found = false;
                for (const auto& p : kPigs) {
                    if (w == p.name) { L.pigQueue.push_back(p.type); found = true; break; }
                }
                if (!found) warn("unknown pig '" + w + "'");
            }
        } else if (key == "wind") {
            float v;
            ok = parseFloat(val, v) && v >= -300.0f && v <= 300.0f;
            if (ok) L.wind = v;
        } else if (key == "storm") {
            L.hasStorm = (val != "0");
        } else if (key == "theme") {
            int t;
            ok = parseInt(val, t);
            if (ok) out.theme = std::clamp(t, 0, 5);
        } else if (key == "star2" || key == "star3") {
            int v;
            ok = parseInt(val, v) && v >= 0;
            if (ok) {
                (key == "star2" ? L.star2Threshold : L.star3Threshold) = v;
                out.hasStarThresholds = true;
            }
        } else if (key == "truffle") {
            ok = words.size() == 2 && parseFloat(words[0], truffleX) && parseFloat(words[1], truffleY) &&
                 isSaneCoord(truffleX) && isSaneCoord(truffleY);
            if (ok) { truffleExists = haveTruffleX = haveTruffleY = true; }
        } else if (key == "truffle.exists") {  // legacy editor keys
            truffleExists = (val != "0");
        } else if (key == "truffle.x") {
            haveTruffleX = parseFloat(val, truffleX) && isSaneCoord(truffleX);
        } else if (key == "truffle.y") {
            haveTruffleY = parseFloat(val, truffleY) && isSaneCoord(truffleY);
        } else if (key == "block") {
            BlockBody b;
            ok = parseBlock(words, b);
            if (ok) L.blocks.push_back(b);
        } else if (key == "bird") {
            BirdTarget t;
            ok = parseBird(words, out.legacyFormat, t);
            if (ok) L.birds.push_back(t);
        } else if (key == "pendulum") {
            PendulumBody p;
            ok = parsePendulum(words, out.legacyFormat, p);
            if (ok) L.pendulums.push_back(p);
        } else {
            warn("unknown key '" + key + "'");
            continue;
        }
        if (!ok) warn("invalid " + key + " '" + val + "'");
        else anything = true;
    }

    out.spec.hasTruffle = truffleExists && haveTruffleX && haveTruffleY;
    if (out.spec.hasTruffle) out.spec.trufflePos = {truffleX, truffleY};
    return anything;
}

std::string serializeLevel(const LevelSpec& L, int theme, bool starThresholds) {
    std::ostringstream out;
    out << "format = 2\n";
    if (!L.name.empty()) out << "name = " << L.name << "\n";
    if (!L.subtitle.empty()) out << "subtitle = " << L.subtitle << "\n";
    if (!L.pigQueue.empty()) {
        out << "pigs =";
        for (PigType p : L.pigQueue) out << ' ' << pigName(p);
        out << "\n";
    }
    if (theme >= 0) out << "theme = " << theme << "\n";
    if (L.wind != 0.0f) out << "wind = " << fmt(L.wind) << "\n";
    if (L.hasStorm) out << "storm = 1\n";
    if (L.hasTruffle) out << "truffle = " << fmt(L.trufflePos.x) << ' ' << fmt(L.trufflePos.y) << "\n";
    if (starThresholds) {
        out << "star2 = " << L.star2Threshold << "\n";
        out << "star3 = " << L.star3Threshold << "\n";
    }

    if (!L.blocks.empty()) out << "\n";
    for (const auto& b : L.blocks) {
        BlockBody base = SnoutAndSiegeGame::makeBlock(b.mat, b.pos.x, b.pos.y, b.w, b.h);
        out << "block = " << materialName(b.mat) << ' ' << fmt(b.pos.x) << ' ' << fmt(b.pos.y)
            << ' ' << fmt(b.w) << ' ' << fmt(b.h);
        emitFloatOverrides(out, b, base, kBlockFloats);
        out << "\n";
    }

    if (!L.birds.empty()) out << "\n";
    for (const auto& t : L.birds) {
        bool helmet = t.hasHelmet;
        bool shield = t.hasShield && t.kind != BirdClass::FALCON;
        bool zeppelin = t.inZeppelin && t.kind != BirdClass::CONDOR;
        BirdTarget base = SnoutAndSiegeGame::makeBird(t.kind, t.pos.x, t.pos.y, helmet, shield, zeppelin);
        out << "bird = " << birdName(t.kind) << ' ' << fmt(t.pos.x) << ' ' << fmt(t.pos.y);
        if (helmet) out << " helmet";
        if (shield) out << " shield";
        if (zeppelin) out << " zeppelin";
        emitBoolOverrides(out, t, base, kBirdBools);
        emitFloatOverrides(out, t, base, kBirdFloats);
        out << "\n";
    }

    if (!L.pendulums.empty()) out << "\n";
    for (const auto& p : L.pendulums) {
        PendulumBody base{};
        out << "pendulum = " << fmt(p.anchor.x) << ' ' << fmt(p.anchor.y) << ' ' << fmt(p.length);
        if (p.spiked) out << " spiked";
        emitFloatOverrides(out, p, base, kPendulumFloats);
        out << "\n";
    }
    return out.str();
}

bool readTextFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

bool writeTextFileAtomically(const std::string& path, const std::string& contents) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << contents;
        out.flush();
        if (!out) { std::remove(tmp.c_str()); return false; }
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::remove(tmp.c_str());
        return false;
    }
    return true;
}

// Shared constants, math helpers, enums and entity structs.
#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

#include "sdl_runtime.h"

// ============================================================================
// CONSTANTS & MATH UTILITIES
// ============================================================================
static constexpr int WINDOW_W = 1280;
static constexpr int WINDOW_H = 720;
static constexpr float GROUND_Y = 630.0f;
static constexpr float SLING_X = 215.0f;
static constexpr float SLING_Y = 505.0f;
static constexpr float MAX_PULL = 115.0f;
static constexpr float LAUNCH_POWER = 10.9f;
static constexpr float GRAVITY = 880.0f;
static constexpr float PI = 3.14159265358979323846f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator-() const { return {-x, -y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }
    Vec2 normalized() const {
        float l = length();
        return (l > 1e-5f) ? Vec2(x / l, y / l) : Vec2(0.0f, 0.0f);
    }
};

inline float cross(const Vec2& a, const Vec2& b) {
    return a.x * b.y - a.y * b.x;
}
inline Vec2 cross(float w, const Vec2& r) {
    return Vec2(-w * r.y, w * r.x);
}
inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

// ============================================================================
// GAME ENUMS & DATA STRUCTURES
// ============================================================================
enum class PigType {
    TROTTER  = 0, // Balanced clay-red origami pig -> Ability: Anvil Slam (vertical smash)
    TURBO    = 1, // Aerodynamic ochre boar        -> Ability: Supersonic Dash (2.6x speed)
    SPLIT    = 2, // Jade-green geometric piglet   -> Ability: Scatter Squadron (3-way split)
    BOMB     = 3, // Obsidian iron-plated hog      -> Ability: Seismic Detonation (shockwave)
    GLIDER   = 4, // Sky-blue origami aeronaut     -> Ability: Aeronaut Glide & Bomb Drop
    GRAVITON = 5, // Cosmic violet tesla boar      -> Ability: Graviton Vortex (implosion pull)
    DRILL    = 6  // Russet copper drill hog       -> Ability: Supersonic Bore & Foundation Burrowing
};

struct AerialBomb {
    Vec2 pos{};
    Vec2 vel{};
    float radius = 9.0f;
    float blastRadius = 95.0f;
    float damage = 120.0f;
    bool active = true;
    float lifeTimer = 0.0f;
};

enum class MaterialTier {
    TWIG       = 0,  // Tier 1: Wicker / Reed scaffolding (45 HP, light, shatters easily)
    TIMBER     = 1,  // Tier 2: Seasoned Oak / Walnut beams (125 HP, medium, cracks)
    STONE      = 2,  // Tier 3: Carved Slate / Basalt blocks (260 HP, heavy, armor-grade)
    ICE        = 3,  // Tier 4: Glacial Frost Ice (38 HP, ultra-slick friction, brittle crystalline shatter)
    TNT        = 4,  // Tier 5: Volatile Gunpowder Keg (35 HP, explosive chain-reaction detonation)
    TRAMPOLINE = 5,  // Tier 6: High-Tension Trampoline Deflector (high elastic ricochet rebound)
    MAGMA_VENT = 6   // Tier 7: Geothermal Magma Vent (indestructible/anchor basalt geyser)
};

enum class BirdClass {
    TOUCAN = 0,  // Sentry Toucan (78 HP, 5,000 pts) - Geometric prism beak
    OWL    = 1,  // Warden Owl (140 HP, 7,500 pts)   - Monolith armored owl
    HERON  = 2,  // Archduke Heron (210 HP, 10,000 pts) - Crowned royal totem bird
    FALCON = 3,  // Commander Falcon (150 HP + 120 Shield HP, 12,500 pts) - Directional Tower Shield
    CONDOR = 4   // Baron Von Condor (240 HP + 70 Balloon HP, 15,000 pts) - Suspended Imperial Zeppelin
};

struct PigBody {
    PigType type = PigType::TROTTER;
    Vec2 pos{};
    Vec2 vel{};
    float radius = 20.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float mass = 2.5f;
    float invMass = 0.4f;
    float restitution = 0.38f;
    bool active = false;
    bool launched = false;
    bool abilityUsed = false;
    bool exploded = false;
    float fuseTimer = -1.0f;
    float restTimer = 0.0f;
    float lifeTimer = 0.0f;
    std::vector<Vec2> trail;
    float vortexTimer = -1.0f;
    float drillTimer = -1.0f;
};

struct BlockBody {
    MaterialTier mat = MaterialTier::TWIG;
    Vec2 pos{};
    Vec2 vel{};
    float w = 40.0f;
    float h = 40.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float mass = 2.0f;
    float invMass = 0.5f;
    float inertia = 100.0f;
    float invInertia = 0.01f;
    float restitution = 0.22f;
    float friction = 0.55f;
    float hp = 45.0f;
    float maxHp = 45.0f;
    bool destroyed = false;
    bool hasGroundPath = false;
    bool stablySupported = false;
    float tntFuse = -1.0f;
    bool ignited = false;
    int drillStamp = 0;  // last drill bore that pierced this block (see drillBoreId)
};

struct BirdTarget {
    BirdClass kind = BirdClass::TOUCAN;
    Vec2 pos{};
    Vec2 vel{};
    float radius = 22.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float mass = 1.6f;
    float invMass = 0.625f;
    float hp = 60.0f;
    float maxHp = 60.0f;
    bool defeated = false;
    float blinkTimer = 0.0f;
    bool hasHelmet = false;
    float helmetHp = 0.0f;
    float maxHelmetHp = 0.0f;
    bool hasShield = false;
    float shieldHp = 0.0f;
    float maxShieldHp = 0.0f;
    bool inZeppelin = false;
    float balloonHp = 0.0f;
    float maxBalloonHp = 0.0f;
    bool balloonPopped = false;
};

struct PendulumBody {
    Vec2 anchor{};
    float length = 135.0f;
    float angle = 0.0f;      // current radians (0 = hanging straight down)
    float angVel = 0.0f;     // angular velocity rad/sec
    float radius = 24.0f;    // wrecking ball radius
    float mass = 4.5f;       // heavy granite / iron sphere
    bool spiked = true;

    Vec2 getBallPos() const {
        return anchor + Vec2(std::sin(angle) * length, std::cos(angle) * length);
    }
    Vec2 getBallVel() const {
        return Vec2(std::cos(angle) * length * angVel, -std::sin(angle) * length * angVel);
    }
};

enum class ParticleKind {
    GENERIC = 0,
    WOOD_SPLINTER,    // Elongated tumbling wicker / wood sliver
    TIMBER_CHUNK,     // Angular tumbling wood block chip
    STONE_SHARD,      // Heavy angular stone pebble
    FEATHER,          // Fluttering, oscillating feather with barb vanes
    SMOKE_PUFF,       // Expanding, soft alpha circular dust puff
    SPARK_GLINT,      // Star/cross glint
    RING,             // Shockwave ring
    ICE_CRYSTAL,      // Crystalline frost diamond shard
    HELMET_DEBRIS,    // Tumbling iron cauldron helmet piece
    EMBER,            // Fiery rising glowing volcanic ember
    GOLD_SPARKLE,     // Twinkling radiant golden truffle / achievement star particle
    SHIELD_DEBRIS,    // Tumbling reinforced tower shield fragment
    BALLOON_SCRAP,    // Floating shredded zeppelin canvas scrap
    COSMIC_MOTE       // Swirling violet/cyan gravitational vortex mote
};

struct Particle {
    Vec2 pos{};
    Vec2 vel{};
    float size = 5.0f;
    float angle = 0.0f;
    float angVel = 0.0f;
    float life = 1.0f;
    float maxLife = 1.0f;
    SDL_Color color{255, 255, 255, 255};
    ParticleKind kind = ParticleKind::GENERIC;
    bool isRing = false;
    float ringRadius = 10.0f;
    float ringMaxRadius = 180.0f;
    float wobbleFreq = 9.0f;
    float wobblePhase = 0.0f;
};

struct FloatingScore {
    Vec2 pos{};
    std::string text;
    SDL_Color color{255, 240, 150, 255};
    float life = 1.2f;
    float maxLife = 1.2f;
};

struct LightningBolt {
    std::vector<Vec2> points;
    float life = 0.0f;
    float maxLife = 0.24f;
    float width = 3.5f;
    SDL_Color color{235, 248, 255, 255};
};

struct StormCloud {
    Vec2 pos{};
    float speed = 14.0f;
    float width = 160.0f;
    float height = 55.0f;
};

// Feature Pack D: Secret Golden Truffle Collectible
struct GoldenTruffle {
    Vec2 pos{};
    float radius = 14.0f;
    bool exists = false;
    bool active = false;
    bool collected = false;
    float pulseTime = 0.0f;
};

// Feature Pack D: Royal Swine Achievement System
enum class AchievementId {
    FIRST_BLOOD = 0,       // "First Strike"
    MARKSMAN,              // "Long-Distance Swine"
    CAULDRON_CRACKER,      // "Cauldron Cracker"
    SEISMIC_ANNIHILATION,  // "Seismic Annihilation"
    ANVIL_EXECUTIONER,     // "Anvil Executioner"
    POWDER_MAESTRO,        // "Powder Keg Maestro"
    ACROBATIC_HOG,         // "Acrobatic Swine"
    TRUFFLE_SNIFFER,       // "Truffle Sniffer"
    TRUFFLE_CONNOISSEUR,   // "Truffle Connoisseur"
    VAULT_CONQUEROR,       // "Vault Conqueror"
    VORTEX_MAESTRO,        // "Event Horizon" (Atticus Graviton Vortex)
    DEEP_BORE,             // "Deep Bore" (Rusty Drill Hog)
    COUNT
};

struct AchievementDef {
    AchievementId id;
    std::string key;
    std::string title;
    std::string desc;
    int targetCount = 1;
};

struct AchievementToast {
    std::string title;
    std::string desc;
    float timer = 0.0f;
    float slideY = -80.0f;
    bool active = false;
};

struct LevelSpec {
    std::string name;
    std::string subtitle;
    std::vector<PigType> pigQueue;
    std::vector<BlockBody> blocks;
    std::vector<BirdTarget> birds;
    std::vector<PendulumBody> pendulums;
    int star2Threshold = 20000;
    int star3Threshold = 30000;
    float wind = 0.0f;
    bool hasStorm = false;
    bool hasTruffle = false;
    Vec2 trufflePos{0.0f, 0.0f};
};

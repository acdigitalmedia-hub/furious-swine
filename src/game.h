// The game: state, level flow, physics, rendering and UI.
// Member functions are implemented across several .cpp files by subsystem:
//   game.cpp          core state, achievements, level flow, saves, camera, UI clicks
//   levels.cpp        the 150 handcrafted campaign + vault levels
//   editor.cpp        Fortress Architect editor and custom level files
//   endless.cpp       Endless Siege Gauntlet wave generation
//   combat.cpp        pig abilities, explosions, damage and particle effects
//   physics.cpp       rigid-body simulation and collision resolution
//   render_world.cpp  backdrop, fortress, pigs, birds
//   render_ui.cpp     title, level select, trophies, editor, HUD screens
#pragma once

#include "assets.h"
#include "draw.h"

// ============================================================================
// MAIN GAME ENGINE & 2D RIGID-BODY PHYSICS SOLVER
// ============================================================================
constexpr int NUM_CAMPAIGN_LEVELS = 140;
constexpr int NUM_VAULT_LEVELS = 10;
constexpr int NUM_LEVELS = NUM_CAMPAIGN_LEVELS + NUM_VAULT_LEVELS; // 150

enum class ScreenState {
    TITLE = 0,
    LEVEL_SELECT = 1,
    PLAYING = 2,
    ACHIEVEMENTS = 3,
    EDITOR = 4,
    ENDLESS = 5
};

struct UIButton {
    std::string id;
    SDL_FRect rect{};
    std::string label;
    SDL_Color bgCol{38, 48, 68, 245};
    SDL_Color hoverCol{60, 80, 116, 255};
    SDL_Color borderCol{210, 172, 85, 255};
    SDL_Color textCol{248, 244, 230, 255};
    float textScale = 2.0f;

    bool contains(float mx, float my) const {
        return mx >= rect.x && mx <= (rect.x + rect.w) &&
               my >= rect.y && my <= (rect.y + rect.h);
    }
};

void drawButton(SDL_Renderer* ren, const UIButton& btn, float mx, float my);

// ============================================================================
// MAIN GAME ENGINE & 2D RIGID-BODY PHYSICS SOLVER
// ============================================================================
class SnoutAndSiegeGame {
public:
    AssetPipeline assets;
    ScreenState screen = ScreenState::TITLE;
    std::vector<LevelSpec> levels;
    int currentLevelIdx = 0;
    int levelSelectPage = 0;       // 0 for Levels 1-25, 1 for Levels 26-50

    std::array<int, NUM_LEVELS> bestStars{};
    std::array<int, NUM_LEVELS> bestScores{};

    // Active level state
    std::vector<PigType> remainingPigs;
    std::vector<PigBody> activePigs;
    std::vector<BlockBody> blocks;
    std::vector<BirdTarget> birds;
    std::vector<PendulumBody> pendulums;
    std::vector<Particle> particles;
    std::vector<FloatingScore> floatScores;

    // Feature Pack C: Atmospheric Hazards & Dynamic Weather
    float currentWind = 0.0f;
    float windGustTimer = 0.0f;
    float skyFlashAlpha = 0.0f;
    float stormStrikeTimer = 12.0f;
    float simTime = 0.0f;  // seconds of simulation since the level loaded
    std::vector<StormCloud> stormClouds;
    std::vector<LightningBolt> lightningBolts;

    // Feature Pack D: Secret Truffle Hunt, Bonus Vaults, and Achievement System
    std::array<bool, NUM_LEVELS> trufflesFound{};
    std::array<bool, 12> achievementsUnlocked{};
    std::array<int, 12> achievementProgress{};
    std::vector<AchievementDef> achievementDefs;
    GoldenTruffle levelTruffle;
    AchievementToast toast;

    // Turn / shot statistics for dynamic achievement tracking
    int shotBlocksDestroyed = 0;
    int shotTntDetonated = 0;
    bool shotHitTrampoline = false;
    bool shotUsedAnvil = false;
    int shotDrillPierces = 0;
    int drillBoreId = 0;  // increments per Drill activation so each block is counted once per bore
    Vec2 shotLaunchPos{0.0f, 0.0f};

    // UI & High Score Notification State
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    std::vector<UIButton> activeButtons;
    SDL_Window* appWindow = nullptr;
    bool isFullscreen = false;
    bool isNewRecordThisRun = false;
    int prevLevelBest = 0;
    float recordAnimTimer = 0.0f;

    SnoutAndSiegeGame();

    void initAchievements();

    int getAchievementsMask() const;

    int getAchievementsCount() const;

    int getTrufflesFoundCount() const;

    static int getVaultTruffleReq(int vaultIdx);

    void showAchievementToast(const std::string& title, const std::string& desc);

    void updateAchievementToast(float dt);

    void unlockAchievement(AchievementId id);

    void collectGoldenTruffle();

    void toggleFullscreen();

    bool draggingSling = false;
    Vec2 slingPullPos{SLING_X, SLING_Y};
    int score = 0;
    int earnedStars = 0;
    bool levelComplete = false;
    bool levelFailed = false;
    bool fortressAwake = false;
    float settleTimer = 0.0f;
    float physicsAccumulator = 0.0f;
    static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
    float simSpeed = 1.0f; // 1.0f = normal speed, 2.5f = fast forward

    Vec2 camPos{0.0f, 0.0f};
    Vec2 camTarget{0.0f, 0.0f};
    Vec2 camShakeOffset{0.0f, 0.0f};
    float shakeTrauma = 0.0f;
    float shakeTime = 0.0f;
    bool isManualPanning = false;
    float manualPanTimer = 0.0f;
    float lastPanMouseX = 0.0f;

    // Visual Polish & Juice
    std::vector<Vec2> ghostTrajectory;
    std::vector<Vec2> currentFlightTrail;
    float slingRecoilTimer = 0.0f;
    Vec2 lastLaunchDir{1.0f, 0.0f};

    // Feature Pack E: Gale the Aeronaut & Aerial Ordnance
    std::vector<AerialBomb> aerialBombs;

    // Feature Pack E: Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
    bool sloMoActive = false;
    float sloMoTimer = 0.0f;
    float sloMoFactor = 0.22f;
    float letterboxY = 0.0f;
    Vec2 sloMoFocusPos{0.0f, 0.0f};
    bool enableSloMoCam = true;

    // Feature Pack E: Interactive Fortress Architect & Level Sandbox
    int editorTool = 1; // 0=BLOCK, 1=BIRD, 2=PENDULUM, 3=TRUFFLE, 4=ERASER
    MaterialTier editorMat = MaterialTier::TIMBER;
    float editorBlockW = 60.0f, editorBlockH = 22.0f;
    BirdClass editorBird = BirdClass::TOUCAN;
    bool editorBirdHelmet = false;
    bool editorSpikedPendulum = true;
    bool editorSnapGrid = true;
    float editorWind = 0.0f;
    bool editorStorm = false;
    int editorTheme = 0; // 0=Meadow, 1=Timber, 2=Quarry, 3=Glacier, 4=Thunder, 5=Volcano
    LevelSpec customLevel;
    LevelSpec customBackup;
    bool isTestPlaying = false;
    int customSlot = 1;

    // Feature Pack E: Endless Siege Gauntlet Mode
    int endlessWave = 1;
    int endlessScore = 0;
    int endlessBestWave = 0;
    float endlessComboMultiplier = 1.0f;
    bool endlessGameOver = false;

    void addTrauma(float amount);

    static BlockBody makeBlock(MaterialTier mat, float x, float y, float w, float h);

    // Single source of truth for bird stats, shared by campaign levels, the
    // Fortress Architect editor, saved custom levels, and Endless mode.
    static BirdTarget makeBird(BirdClass kind, float x, float y, bool withHelmet = false, bool withShield = false, bool withZeppelin = false);

    // Loads the 150 campaign/vault levels from <dir>/level_NNN.cfg. Returns
    // false (after logging) if any file is missing or unreadable.
    bool initLevels(const std::string& dir = "assets/levels");
    static void calibrateStarThresholds(LevelSpec& L);

    void loadCustomLevelSpec(const LevelSpec& L);

    void loadLevel(int idx);

    void saveCustomLevel(int slot);


    // Pendulum as placed by the Fortress Architect; also used when reloading a
    // saved custom level so a reloaded pendulum matches the one that was placed.
    static PendulumBody makeEditorPendulum(float ax, float ay, bool spiked, float length = 85.0f);

    void loadCustomLevel(int slot);

    void initEditor();

    void startEditorTestPlay();

    void returnToEditor();
    void handleEditorCanvasClick(float mx, float my);

    void generateEndlessWave(int wave);

    void startEndlessMode();

    int getHighestUnlockedLevel() const;

    bool isLevelUnlocked(int idx) const;

    bool recordLevelScore(int lvlIdx, int newScore, int stars);

    int getTotalStars() const;

    int getLevelsCleared() const;

    PigBody createPigInstance(PigType type, Vec2 pos);

    void spawnNextPigInSlingshot();

    void launchCurrentPig(Vec2 pullVector);

    void activateMidAirAbility();

    void triggerGravitonImplosion(Vec2 center);

    void triggerBombDetonation(PigBody& bombPig);

    void detonateAerialBomb(AerialBomb& ab);

    void triggerTntDetonation(BlockBody& tntBlock);

    void spawnBurst(Vec2 pos, int count, SDL_Color col, float maxSpeed);

    void spawnSplinters(Vec2 pos, MaterialTier mat, int count, float maxSpeed);

    void spawnFeatherBurst(Vec2 pos, BirdClass kind, int count, float maxSpeed);

    void eruptMagmaVent(BlockBody& b);

    void triggerLightningStrike();

    void damageBlock(BlockBody& b, float dmg);

    void damageBird(BirdTarget& t, float dmg);

    // Helper: Get the 4 world-space vertices of an oriented BlockBody
    static std::array<Vec2, 4> getBlockVerts(const BlockBody& b);

    static float getTargetFlatAngle(const BlockBody& b);

    static float getTiltFromStable(const BlockBody& b);

    // Computes bottom-up grounded support propagation from GROUND_Y through the block graph
    void updateGroundedSupportGraph(float subDt);

    void stepPhysics(float dt);

    void resolveBlockGround(BlockBody& b, bool allowDamage, float subDt);

    void resolveCircleOBB(PigBody& p, BlockBody& b);

    void resolveBirdBlock(BirdTarget& t, BlockBody& b, bool allowDamage, float subDt);

    // True OBB vs OBB Separating Axis Theorem (SAT) with Multi-Point Manifolds, Restitution Threshold & Positional Correction
    void resolveBlockBlock(BlockBody& A, BlockBody& B, bool allowDamage, float /*subDt*/);

    void evaluateTurnAndWinState(float dt);

    void updateCamera(float dt);

    void updateGame(float frameTime);

    bool persistProgress = true;  // false in tools: never write save_progress.cfg
    void saveProgress();

    void loadProgress();

    // ========================================================================
    // RENDERING & INTERACTIVE UI SCREENS (TITLE, LEVEL SELECT, HUD)
    // ========================================================================
    int getActiveChapter() const;

    void renderBackgroundBackdrop(SDL_Renderer* ren, Vec2 cam = {0.0f, 0.0f});

    void renderSiegeCatapultBase(SDL_Renderer* ren, float sx, float sy, float groundY = GROUND_Y);

    void render(SDL_Renderer* ren);

    void renderTitleScreen(SDL_Renderer* ren);

    void renderLevelSelectScreen(SDL_Renderer* ren);

    void renderBlock(SDL_Renderer* ren, const BlockBody& b, Vec2 cam = {0.0f, 0.0f});

    void renderPendulum(SDL_Renderer* ren, const PendulumBody& pend, Vec2 cam = {0.0f, 0.0f});

    void renderPig(SDL_Renderer* ren, const PigBody& p, Vec2 cam = {0.0f, 0.0f});

    void renderBird(SDL_Renderer* ren, const BirdTarget& t, Vec2 cam = {0.0f, 0.0f});

    void renderGoldenTruffle(SDL_Renderer* ren, Vec2 cam = {0.0f, 0.0f});

    void renderAchievementToast(SDL_Renderer* ren);

    void renderAchievementsScreen(SDL_Renderer* ren);

    void renderEditorScreen(SDL_Renderer* ren);

    void renderHUD(SDL_Renderer* ren);

    bool handleButtonClick(const std::string& id, bool& running);
};

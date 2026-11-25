#include "game.h"
#include "level_format.h"

SnoutAndSiegeGame::SnoutAndSiegeGame() {
    initAchievements();
}

void SnoutAndSiegeGame::initAchievements() {
    achievementDefs = {
        {AchievementId::FIRST_BLOOD, "first_blood", "First Strike", "Defeat your first aristocratic Totem Bird sentry.", 1},
        {AchievementId::MARKSMAN, "marksman", "Long-Distance Swine", "Defeat a Totem Bird from over 750px slingshot range.", 1},
        {AchievementId::CAULDRON_CRACKER, "cauldron_cracker", "Cauldron Cracker", "Shatter 5 iron cauldron helmets off sentries.", 5},
        {AchievementId::SEISMIC_ANNIHILATION, "seismic_annihilation", "Seismic Annihilation", "Demolish 12 or more fortress blocks in a single shot.", 12},
        {AchievementId::ANVIL_EXECUTIONER, "anvil_executioner", "Anvil Executioner", "Crush an Archduke Heron with Trotter's Anvil Slam dive.", 1},
        {AchievementId::POWDER_MAESTRO, "powder_maestro", "Powder Keg Maestro", "Detonate 3 or more TNT gunpowder kegs in a single shot.", 3},
        {AchievementId::ACROBATIC_HOG, "acrobatic_hog", "Acrobatic Swine", "Defeat a Totem Bird on a ricochet after hitting a Trampoline.", 1},
        {AchievementId::TRUFFLE_SNIFFER, "truffle_sniffer", "Truffle Sniffer", "Discover your first Hidden Golden Truffle in the campaign.", 1},
        {AchievementId::TRUFFLE_CONNOISSEUR, "truffle_connoisseur", "Truffle Connoisseur", "Discover 20 Hidden Golden Truffles across the campaign.", 20},
        {AchievementId::VAULT_CONQUEROR, "vault_conqueror", "Vault Conqueror", "Conquer all 10 Secret Truffle Vault challenge levels.", 10},
        {AchievementId::VORTEX_MAESTRO, "vortex_maestro", "Event Horizon", "Suck 8 blocks or birds into Atticus's Graviton Vortex.", 8},
        {AchievementId::DEEP_BORE, "deep_bore", "Deep Bore", "Bore clean through 3 or more fortress blocks in a single drill shot.", 3}
    };
}

int SnoutAndSiegeGame::getAchievementsMask() const {
    int mask = 0;
    for (int i = 0; i < static_cast<int>(AchievementId::COUNT); ++i) {
        if (achievementsUnlocked[i]) mask |= (1 << i);
    }
    return mask;
}

int SnoutAndSiegeGame::getAchievementsCount() const {
    int count = 0;
    for (int i = 0; i < static_cast<int>(AchievementId::COUNT); ++i) {
        if (achievementsUnlocked[i]) count++;
    }
    return count;
}

int SnoutAndSiegeGame::getTrufflesFoundCount() const {
    int count = 0;
    for (bool b : trufflesFound) {
        if (b) count++;
    }
    return count;
}

int SnoutAndSiegeGame::getVaultTruffleReq(int vaultIdx) {
    static const int reqs[10] = {2, 5, 8, 11, 14, 18, 22, 26, 30, 35};
    if (vaultIdx >= 0 && vaultIdx < NUM_VAULT_LEVELS) return reqs[vaultIdx];
    return 99;
}

void SnoutAndSiegeGame::showAchievementToast(const std::string& title, const std::string& desc) {
    toast.title = title;
    toast.desc = desc;
    toast.timer = 4.5f;
    toast.slideY = -80.0f;
    toast.active = true;
}

void SnoutAndSiegeGame::updateAchievementToast(float dt) {
    if (toast.active) {
        toast.timer -= dt;
        if (toast.timer <= 0.0f) {
            toast.active = false;
        } else if (toast.timer > 4.0f) {
            float t = (4.5f - toast.timer) / 0.5f;
            toast.slideY = -80.0f + t * 96.0f;
        } else if (toast.timer < 0.5f) {
            float t = toast.timer / 0.5f;
            toast.slideY = -80.0f + t * 96.0f;
        } else {
            toast.slideY = 16.0f;
        }
    }
}

void SnoutAndSiegeGame::unlockAchievement(AchievementId id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(AchievementId::COUNT) || achievementsUnlocked[idx]) return;
    achievementsUnlocked[idx] = true;
    if (idx < static_cast<int>(achievementDefs.size())) {
        achievementProgress[idx] = achievementDefs[idx].targetCount;
        showAchievementToast(achievementDefs[idx].title, achievementDefs[idx].desc);
    }
    assets.playSound("sound.achievement");
    saveProgress();
}

void SnoutAndSiegeGame::collectGoldenTruffle() {
    if (!levelTruffle.exists || levelTruffle.collected) return;
    levelTruffle.collected = true;
    // Only campaign/vault truffles count toward progress. Endless waves and
    // editor test plays reuse whatever currentLevelIdx was last loaded, so
    // recording them would credit (and unlock vaults for) the wrong level.
    const bool countsForProgress = (screen == ScreenState::PLAYING) &&
                                   currentLevelIdx >= 0 && currentLevelIdx < NUM_LEVELS;
    if (countsForProgress) {
        trufflesFound[currentLevelIdx] = true;
    }

    // Spawn 24 golden sparkle particles radiating outward
    for (int i = 0; i < 24; ++i) {
        Particle p{};
        p.pos = levelTruffle.pos;
        float ang = (i * (360.0f / 24.0f) + (SDL_GetTicks() % 15)) * (PI / 180.0f);
        float spd = 70.0f + (i % 6) * 35.0f;
        p.vel = Vec2(std::cos(ang) * spd, std::sin(ang) * spd);
        p.size = 6.0f + (i % 4) * 2.5f;
        p.life = 1.2f + (i % 5) * 0.2f;
        p.maxLife = p.life;
        p.kind = ParticleKind::GOLD_SPARKLE;
        int cVar = i % 3;
        p.color = (cVar == 0) ? SDL_Color{255, 235, 90, 255}
                              : ((cVar == 1) ? SDL_Color{255, 205, 40, 255}
                                             : SDL_Color{255, 255, 230, 255});
        particles.push_back(p);
    }

    assets.playSound("sound.truffle");
    addTrauma(0.32f);
    score += 2500;
    floatScores.push_back({levelTruffle.pos - camPos, "+2500 GOLDEN TRUFFLE!", {255, 220, 75, 255}, 1.8f, 1.8f});
    if (!countsForProgress) return;

    unlockAchievement(AchievementId::TRUFFLE_SNIFFER);
    if (getTrufflesFoundCount() >= 20) {
        unlockAchievement(AchievementId::TRUFFLE_CONNOISSEUR);
    }

    int tCount = getTrufflesFoundCount();
    for (int v = 0; v < NUM_VAULT_LEVELS; ++v) {
        if (tCount == getVaultTruffleReq(v)) {
            std::string vNum = (v + 1 < 10 ? "0" : "") + std::to_string(v + 1);
            floatScores.push_back({{WINDOW_W * 0.5f, 180.0f},
                                  "SECRET VAULT " + vNum + " UNLOCKED!",
                                  {225, 150, 255, 255}, 2.4f, 2.4f});
            break;
        }
    }
    saveProgress();
}

void SnoutAndSiegeGame::toggleFullscreen() {
    isFullscreen = !isFullscreen;
    if (appWindow) {
        SDL_SetWindowFullscreen(appWindow, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }
}

void SnoutAndSiegeGame::addTrauma(float amount) {
    // Toned down camera shake as requested: scale down trauma by 0.45x
    shakeTrauma = clampf(shakeTrauma + amount * 0.45f, 0.0f, 1.0f);
}

BlockBody SnoutAndSiegeGame::makeBlock(MaterialTier mat, float x, float y, float w, float h) {
    BlockBody b{};
    b.mat = mat;
    b.pos = {x, y};
    b.w = w;
    b.h = h;
    float density = (mat == MaterialTier::TWIG)       ? 0.75f :
                    (mat == MaterialTier::TIMBER     ? 1.45f :
                    (mat == MaterialTier::STONE      ? 2.85f :
                    (mat == MaterialTier::ICE        ? 0.95f :
                    (mat == MaterialTier::TRAMPOLINE ? 3.00f :
                    (mat == MaterialTier::MAGMA_VENT ? 8.00f : 1.35f))))); // TNT = 1.35f
    b.mass = (mat == MaterialTier::TRAMPOLINE) ? 6.0f :
             (mat == MaterialTier::MAGMA_VENT) ? 8.0f :
             std::max(1.0f, (w * h * 0.0012f) * density);
    b.invMass = 1.0f / b.mass;
    b.inertia = b.mass * (w * w + h * h) / 36.0f;
    b.invInertia = 1.0f / b.inertia;
    b.maxHp = (mat == MaterialTier::TWIG)       ? 68.0f :
              (mat == MaterialTier::TIMBER     ? 170.0f :
              (mat == MaterialTier::STONE      ? 350.0f :
              (mat == MaterialTier::ICE        ? 38.0f :
              (mat == MaterialTier::TRAMPOLINE ? 9999.0f :
              (mat == MaterialTier::MAGMA_VENT ? 9999.0f : 35.0f))))); // TNT = 35 HP (volatile!)
    b.hp = b.maxHp;
    b.restitution = (mat == MaterialTier::TRAMPOLINE) ? 1.65f :
                    ((mat == MaterialTier::STONE)     ? 0.10f :
                    (mat == MaterialTier::ICE        ? 0.08f :
                    (mat == MaterialTier::MAGMA_VENT ? 0.15f : 0.16f)));
    b.friction = (mat == MaterialTier::TRAMPOLINE) ? 0.08f :
                 ((mat == MaterialTier::STONE)     ? 0.96f :
                 (mat == MaterialTier::ICE        ? 0.04f :
                 (mat == MaterialTier::MAGMA_VENT ? 0.95f : 0.88f)));
    b.tntFuse = -1.0f;
    b.ignited = false;
    return b;
}

BirdTarget SnoutAndSiegeGame::makeBird(BirdClass kind, float x, float y, bool withHelmet, bool withShield, bool withZeppelin) {
    BirdTarget t{};
    t.kind = kind;
    t.pos = {x, y};
    t.radius = (kind == BirdClass::CONDOR) ? 28.0f :
               (kind == BirdClass::HERON ? 26.0f :
               (kind == BirdClass::FALCON ? 23.0f :
               (kind == BirdClass::OWL ? 23.0f : 20.0f)));
    t.maxHp = (kind == BirdClass::CONDOR) ? 240.0f :
              (kind == BirdClass::HERON ? 210.0f :
              (kind == BirdClass::FALCON ? 150.0f :
              (kind == BirdClass::OWL ? 140.0f : 78.0f)));
    t.hp = t.maxHp;
    t.mass = (kind == BirdClass::CONDOR) ? 2.8f :
             (kind == BirdClass::HERON ? 2.4f :
             (kind == BirdClass::FALCON ? 2.0f :
             (kind == BirdClass::OWL ? 1.8f : 1.4f)));
    t.invMass = 1.0f / t.mass;
    t.hasHelmet = withHelmet;
    if (withHelmet) {
        t.helmetHp = (kind == BirdClass::CONDOR) ? 160.0f :
                     (kind == BirdClass::HERON ? 140.0f :
                     (kind == BirdClass::FALCON ? 110.0f :
                     (kind == BirdClass::OWL ? 100.0f : 70.0f)));
        t.maxHelmetHp = t.helmetHp;
    }
    t.hasShield = withShield || (kind == BirdClass::FALCON);
    if (t.hasShield) {
        t.shieldHp = 120.0f;
        t.maxShieldHp = 120.0f;
    }
    t.inZeppelin = withZeppelin || (kind == BirdClass::CONDOR);
    if (t.inZeppelin) {
        t.balloonHp = 70.0f;
        t.maxBalloonHp = 70.0f;
    }
    return t;
}

void SnoutAndSiegeGame::loadCustomLevelSpec(const LevelSpec& L) {
    remainingPigs = L.pigQueue;
    blocks = L.blocks;
    birds = L.birds;
    pendulums = L.pendulums;
    activePigs.clear();
    particles.clear();
    floatScores.clear();
    aerialBombs.clear();
    score = 0;
    earnedStars = 0;
    levelComplete = false;
    levelFailed = false;
    fortressAwake = false;
    simSpeed = 1.0f;
    isNewRecordThisRun = false;
    recordAnimTimer = 0.0f;
    settleTimer = 0.0f;
    physicsAccumulator = 0.0f;
    camPos = {0.0f, 0.0f};
    camTarget = {0.0f, 0.0f};
    camShakeOffset = {0.0f, 0.0f};
    shakeTrauma = 0.0f;
    isManualPanning = false;
    manualPanTimer = 0.0f;
    ghostTrajectory.clear();
    currentFlightTrail.clear();
    slingRecoilTimer = 0.0f;
    draggingSling = false;
    slingPullPos = {SLING_X, SLING_Y};

    levelTruffle.exists = L.hasTruffle;
    levelTruffle.active = L.hasTruffle;
    levelTruffle.pos = L.trufflePos;
    levelTruffle.collected = false;
    levelTruffle.pulseTime = 0.0f;

    shotBlocksDestroyed = 0;
    shotTntDetonated = 0;
    shotHitTrampoline = false;
    shotUsedAnvil = false;
    shotDrillPierces = 0;
    shotLaunchPos = {SLING_X, SLING_Y};

    currentWind = L.wind;
    windGustTimer = 0.0f;
    skyFlashAlpha = 0.0f;
    stormStrikeTimer = 12.0f;
    simTime = 0.0f;
    lightningBolts.clear();
    stormClouds.clear();
    if (L.hasStorm) {
        StormCloud c1;
        c1.pos = {280.0f, 75.0f};
        c1.speed = 12.0f;
        c1.width = 175.0f;
        c1.height = 55.0f;
        StormCloud c2;
        c2.pos = {760.0f, 60.0f};
        c2.speed = 16.0f;
        c2.width = 210.0f;
        c2.height = 62.0f;
        stormClouds.push_back(c1);
        stormClouds.push_back(c2);
    }
    if (std::abs(L.wind) > 20.0f) {
        assets.playSound("sound.wind", 0.70f);
    }

    spawnNextPigInSlingshot();
}

void SnoutAndSiegeGame::loadLevel(int idx) {
    if (idx < 0 || idx >= static_cast<int>(levels.size())) idx = 0;
    currentLevelIdx = idx;
    screen = ScreenState::PLAYING;
    const LevelSpec& L = levels[idx];
    loadCustomLevelSpec(L);
    levelTruffle.collected = (idx >= 0 && idx < NUM_LEVELS) ? trufflesFound[idx] : false;
    prevLevelBest = (idx >= 0 && idx < NUM_LEVELS) ? bestScores[idx] : 0;
}

int SnoutAndSiegeGame::getHighestUnlockedLevel() const {
    for (int i = 0; i < NUM_CAMPAIGN_LEVELS; ++i) {
        if (bestStars[i] == 0) return i;
    }
    return NUM_CAMPAIGN_LEVELS - 1;
}

bool SnoutAndSiegeGame::isLevelUnlocked(int idx) const {
    if (idx == 0) return true;
    if (idx < 0 || idx >= NUM_LEVELS) return false;
    if (idx >= NUM_CAMPAIGN_LEVELS) {
        int vaultIdx = idx - NUM_CAMPAIGN_LEVELS;
        return getTrufflesFoundCount() >= getVaultTruffleReq(vaultIdx);
    }
    return bestStars[idx - 1] > 0;
}

bool SnoutAndSiegeGame::recordLevelScore(int lvlIdx, int newScore, int stars) {
    if (lvlIdx < 0 || lvlIdx >= NUM_LEVELS) return false;
    bool isNewRecord = (newScore > bestScores[lvlIdx]);
    bestScores[lvlIdx] = std::max(bestScores[lvlIdx], newScore);
    bestStars[lvlIdx] = std::max(bestStars[lvlIdx], stars);
    saveProgress();
    return isNewRecord;
}

int SnoutAndSiegeGame::getTotalStars() const {
    int total = 0;
    for (int s : bestStars) total += s;
    return total;
}

int SnoutAndSiegeGame::getLevelsCleared() const {
    int cleared = 0;
    for (int s : bestStars) if (s > 0) cleared++;
    return cleared;
}

PigBody SnoutAndSiegeGame::createPigInstance(PigType type, Vec2 pos) {
    PigBody p{};
    p.type = type;
    p.pos = pos;
    if (type == PigType::TROTTER) {
        p.radius = 21.0f;
        p.mass = 2.8f;
        p.restitution = 0.36f;
    } else if (type == PigType::TURBO) {
        p.radius = 20.0f;
        p.mass = 2.4f;
        p.restitution = 0.32f;
    } else if (type == PigType::SPLIT) {
        p.radius = 17.0f;
        p.mass = 1.7f;
        p.restitution = 0.44f;
    } else if (type == PigType::GLIDER) {
        p.radius = 18.0f;
        p.mass = 2.0f;
        p.restitution = 0.38f;
    } else if (type == PigType::GRAVITON) {
        p.radius = 23.0f;
        p.mass = 3.4f;
        p.restitution = 0.28f;
    } else if (type == PigType::DRILL) {
        p.radius = 19.0f;
        p.mass = 3.6f;
        p.restitution = 0.15f;
    } else {
        p.radius = 24.0f;
        p.mass = 3.8f;
        p.restitution = 0.25f;
    }
    p.invMass = 1.0f / p.mass;
    p.active = true;
    p.launched = false;
    return p;
}

void SnoutAndSiegeGame::spawnNextPigInSlingshot() {
    simSpeed = 1.0f;
    if (!currentFlightTrail.empty()) {
        ghostTrajectory = currentFlightTrail;
        currentFlightTrail.clear();
    }
    if (remainingPigs.empty()) return;
    PigType nextType = remainingPigs.front();
    remainingPigs.erase(remainingPigs.begin());
    activePigs.clear();
    activePigs.push_back(createPigInstance(nextType, {SLING_X, SLING_Y}));
    slingPullPos = {SLING_X, SLING_Y};
}

void SnoutAndSiegeGame::launchCurrentPig(Vec2 pullVector) {
    if (activePigs.empty() || activePigs[0].launched) return;
    float pullLen = pullVector.length();
    if (pullLen < 12.0f) {
        slingPullPos = {SLING_X, SLING_Y};
        return;
    }
    if (pullLen > MAX_PULL) {
        pullVector = pullVector.normalized() * MAX_PULL;
    }
    PigBody& pig = activePigs[0];
    pig.pos = {SLING_X, SLING_Y};
    pig.vel = pullVector * (-LAUNCH_POWER);
    pig.launched = true;
    pig.restTimer = 0.0f;
    pig.lifeTimer = 0.0f;
    isManualPanning = false;
    manualPanTimer = 0.0f;
    slingRecoilTimer = 0.35f;
    lastLaunchDir = (pullVector.lengthSq() > 1.0f) ? (pullVector * -1.0f).normalized() : Vec2(1.0f, 0.0f);
    currentFlightTrail.clear();
    currentFlightTrail.push_back({SLING_X, SLING_Y});
    addTrauma(0.12f);
    assets.playSound("sound.launch");
    assets.playSound("sound.oink", 0.75f);

    shotBlocksDestroyed = 0;
    shotTntDetonated = 0;
    shotHitTrampoline = false;
    shotUsedAnvil = false;
    shotDrillPierces = 0;
    shotLaunchPos = {SLING_X, SLING_Y};
}

void SnoutAndSiegeGame::evaluateTurnAndWinState(float dt) {
    if (levelComplete || levelFailed) return;

    int aliveBirds = 0;
    for (const auto& t : birds) {
        if (!t.defeated) aliveBirds++;
    }

    bool anyPigWaiting = false;
    bool anyPigMoving = false;
    for (auto& p : activePigs) {
        if (!p.active) continue;
        if (!p.launched) {
            anyPigWaiting = true;
        } else {
            if (p.vel.lengthSq() < 1100.0f || p.lifeTimer > 4.2f) {
                p.restTimer += dt;
                if (p.restTimer > 1.15f) {
                    if (p.type == PigType::BOMB && !p.exploded) {
                        triggerBombDetonation(p);
                    }
                    p.active = false;
                } else {
                    anyPigMoving = true;
                }
            } else {
                p.restTimer = 0.0f;
                anyPigMoving = true;
            }
        }
    }

    aerialBombs.erase(std::remove_if(aerialBombs.begin(), aerialBombs.end(),
                                     [](const AerialBomb& b) { return !b.active; }),
                      aerialBombs.end());
    for (const auto& ab : aerialBombs) {
        if (ab.active) anyPigMoving = true;
    }

    bool worldMoving = anyPigMoving;
    for (const auto& b : blocks) {
        if (!b.destroyed && (b.vel.lengthSq() > 1600.0f || (b.mat == MaterialTier::TNT && b.ignited))) worldMoving = true;
    }
    for (const auto& t : birds) {
        if (!t.defeated && t.vel.lengthSq() > 1600.0f) worldMoving = true;
    }

    // Feature Pack E: Endless Mode Wave Progression
    if (screen == ScreenState::ENDLESS) {
        if (aliveBirds == 0 && !worldMoving) {
            int unusedCount = static_cast<int>(remainingPigs.size()) + (anyPigWaiting ? 1 : 0);
            int bonus = unusedCount * 6000 + endlessWave * 4000;
            endlessScore += bonus;
            endlessWave++;
            endlessBestWave = std::max(endlessBestWave, endlessWave);
            saveProgress();
            floatScores.push_back({{WINDOW_W * 0.5f, 180.0f},
                                   "WAVE CLEARED! +" + std::to_string(bonus),
                                   {255, 230, 90, 255}, 1.8f, 1.8f});
            assets.playSound("sound.victory");
            generateEndlessWave(endlessWave);
            return;
        }
        if (!anyPigWaiting && !anyPigMoving && !worldMoving) {
            settleTimer += dt;
            if (settleTimer > 0.45f) {
                settleTimer = 0.0f;
                if (!remainingPigs.empty()) {
                    spawnNextPigInSlingshot();
                } else if (aliveBirds > 0) {
                    endlessGameOver = true;
                }
            }
        } else {
            settleTimer = 0.0f;
        }
        return;
    }

    // Feature Pack E: Fortress Architect Test Play Completion
    if (screen == ScreenState::EDITOR && isTestPlaying) {
        if (aliveBirds == 0 && !worldMoving) {
            levelComplete = true;
            return;
        }
        if (!anyPigWaiting && !anyPigMoving && !worldMoving) {
            settleTimer += dt;
            if (settleTimer > 0.45f) {
                settleTimer = 0.0f;
                if (!remainingPigs.empty()) {
                    spawnNextPigInSlingshot();
                } else if (aliveBirds > 0) {
                    levelFailed = true;
                }
            }
        } else {
            settleTimer = 0.0f;
        }
        return;
    }

    if (aliveBirds == 0 && !worldMoving) {
        int unusedCount = static_cast<int>(remainingPigs.size()) + (anyPigWaiting ? 1 : 0);
        int bonus = unusedCount * 6000;
        score += bonus;
        if (bonus > 0) {
            floatScores.push_back({{SLING_X, SLING_Y - 50.0f},
                                   "PIG BONUS +" + std::to_string(bonus),
                                   {255, 225, 90, 255}, 2.2f, 2.2f});
        }
        const LevelSpec& L = levels[currentLevelIdx];
        earnedStars = 1;
        if (score >= L.star2Threshold) earnedStars = 2;
        if (score >= L.star3Threshold) earnedStars = 3;

        bestStars[currentLevelIdx] = std::max(bestStars[currentLevelIdx], earnedStars);
        isNewRecordThisRun = (score > bestScores[currentLevelIdx]);
        bestScores[currentLevelIdx] = std::max(bestScores[currentLevelIdx], score);
        saveProgress();

        levelComplete = true;
        recordAnimTimer = 0.0f;

        // Check Achievement: VAULT_CONQUEROR (Clear all 5 Secret Truffle Vault challenge levels)
        bool allVaultsCleared = true;
        for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) {
            if (bestStars[v] <= 0) {
                allVaultsCleared = false;
                break;
            }
        }
        if (allVaultsCleared) {
            unlockAchievement(AchievementId::VAULT_CONQUEROR);
        }
        if (isNewRecordThisRun) {
            assets.playSound("sound.highscore");
            for (int i = 0; i < 48; ++i) {
                float ang = (i / 48.0f) * 6.28318f;
                float spd = 100.0f + static_cast<float>((i * 47) % 200);
                Particle p{};
                p.pos = {WINDOW_W * 0.5f, WINDOW_H * 0.38f};
                p.vel = {std::cos(ang) * spd, std::sin(ang) * spd - 60.0f};
                p.maxLife = 2.4f;
                p.life = p.maxLife;
                p.color = ((i % 3) == 0) ? SDL_Color{255, 230, 80, 255} :
                          (((i % 3) == 1) ? SDL_Color{255, 175, 40, 255} : SDL_Color{120, 240, 255, 255});
                p.size = 5.0f;
                particles.push_back(p);
            }
        } else {
            assets.playSound("sound.victory");
        }
        return;
    }

    if (!anyPigWaiting && !anyPigMoving && !worldMoving) {
        settleTimer += dt;
        if (settleTimer > 0.45f) {
            settleTimer = 0.0f;
            if (!remainingPigs.empty()) {
                spawnNextPigInSlingshot();
            } else if (aliveBirds > 0) {
                levelFailed = true;
            }
        }
    } else {
        settleTimer = 0.0f;
    }
}

void SnoutAndSiegeGame::updateCamera(float dt) {
    // 1. Visceral Trauma-based Screen Shake (toned down to keep action clear)
    shakeTime += dt * 36.0f;
    if (shakeTrauma > 0.0f) {
        shakeTrauma = std::max(0.0f, shakeTrauma - 3.2f * dt);
        float intensity = shakeTrauma * shakeTrauma; // non-linear quadratic curve
        float maxShakeX = 4.5f;
        float maxShakeY = 3.0f;
        float ox = (std::sin(shakeTime * 1.55f) * 0.65f + std::sin(shakeTime * 3.45f) * 0.35f) * maxShakeX * intensity;
        float oy = (std::cos(shakeTime * 2.15f) * 0.65f + std::cos(shakeTime * 4.65f) * 0.35f) * maxShakeY * intensity;
        camShakeOffset = {ox, oy};
    } else {
        camShakeOffset = {0.0f, 0.0f};
    }

    if (screen != ScreenState::PLAYING && !(screen == ScreenState::EDITOR && isTestPlaying) && screen != ScreenState::ENDLESS) {
        camTarget = {0.0f, 0.0f};
        camPos = {0.0f, 0.0f};
        return;
    }

    // 2. Dynamic Camera Target Determination
    if (manualPanTimer > 0.0f) {
        manualPanTimer -= dt;
        if (manualPanTimer <= 0.0f) {
            isManualPanning = false;
        }
    }

    if (isManualPanning) {
        // Player is manually inspecting the level with right-click or keys
    } else if (sloMoActive) {
        float desiredX = sloMoFocusPos.x - WINDOW_W * 0.5f;
        camTarget.x = clampf(desiredX, 0.0f, 420.0f);
        camTarget.y = 0.0f;
    } else if (draggingSling) {
        // Actively aiming catapult: smoothly center on slingshot origin
        camTarget = {0.0f, 0.0f};
    } else if (!activePigs.empty() && activePigs[0].launched && activePigs[0].active) {
        // Pig in flight: frame the pig at 38% screen width so the fortress ahead is clearly visible
        const PigBody& pig = activePigs[0];
        float desiredX = pig.pos.x - WINDOW_W * 0.38f;
        float desiredY = (pig.pos.y < 320.0f) ? (pig.pos.y - 320.0f) : 0.0f;
        camTarget.x = clampf(desiredX, 0.0f, 420.0f);
        camTarget.y = clampf(desiredY, -240.0f, 0.0f);
    } else {
        // Action / Collapsing phase: focus on dynamically moving debris
        float moveSumX = 0.0f;
        int moveCount = 0;
        for (const auto& b : blocks) {
            if (!b.destroyed && b.vel.lengthSq() > 1800.0f) {
                moveSumX += b.pos.x;
                moveCount++;
            }
        }
        for (const auto& t : birds) {
            if (!t.defeated && t.vel.lengthSq() > 1800.0f) {
                moveSumX += t.pos.x;
                moveCount++;
            }
        }
        if (moveCount > 0) {
            float centerActionX = moveSumX / moveCount;
            float desiredX = centerActionX - WINDOW_W * 0.50f;
            camTarget.x = clampf(desiredX, 0.0f, 420.0f);
            camTarget.y = 0.0f;
        } else {
            // Everything is settled: return home to slingshot
            camTarget = {0.0f, 0.0f};
        }
    }

    // 3. Smooth Critically Damped Camera Interpolation
    float lerpSpeed = (camTarget.x == 0.0f) ? 3.6f : 5.8f;
    if (draggingSling) lerpSpeed = 9.0f;
    if (isManualPanning) lerpSpeed = 12.0f;
    float factor = clampf(lerpSpeed * dt, 0.0f, 1.0f);
    camPos.x += (camTarget.x - camPos.x) * factor;
    camPos.y += (camTarget.y - camPos.y) * factor;

    if (std::abs(camPos.x - camTarget.x) < 0.25f) camPos.x = camTarget.x;
    if (std::abs(camPos.y - camTarget.y) < 0.25f) camPos.y = camTarget.y;
}

void SnoutAndSiegeGame::updateGame(float frameTime) {
    updateAchievementToast(frameTime);
    if (screen == ScreenState::PLAYING || (screen == ScreenState::EDITOR && isTestPlaying) || screen == ScreenState::ENDLESS) {
        assets.playMusic("music.battle");
        levelTruffle.pulseTime += frameTime;
        if (levelComplete) {
            recordAnimTimer += frameTime;
        }

        // Feature Pack E: Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
        float effectiveSpeed = simSpeed;
        if (sloMoActive) {
            sloMoTimer -= frameTime;
            effectiveSpeed *= sloMoFactor;
            letterboxY = std::min(46.0f, letterboxY + frameTime * 140.0f);
            if (sloMoTimer <= 0.0f) {
                sloMoActive = false;
            }
        } else {
            letterboxY = std::max(0.0f, letterboxY - frameTime * 140.0f);
        }

        physicsAccumulator += frameTime * effectiveSpeed;
        int steps = 0;
        int maxSteps = (effectiveSpeed > 1.0f) ? 20 : 8;
        while (physicsAccumulator >= FIXED_TIMESTEP && steps < maxSteps) {
            stepPhysics(FIXED_TIMESTEP);
            physicsAccumulator -= FIXED_TIMESTEP;
            steps++;
        }
        if (steps >= maxSteps) {
            physicsAccumulator = 0.0f;
        }
        if (slingRecoilTimer > 0.0f) {
            slingRecoilTimer = std::max(0.0f, slingRecoilTimer - frameTime);
        }
        updateCamera(frameTime);
    } else {
        assets.playMusic("music.title");
        letterboxY = 0.0f;
        sloMoActive = false;
        physicsAccumulator = 0.0f;
        camPos = {0.0f, 0.0f};
        camTarget = {0.0f, 0.0f};
        camShakeOffset = {0.0f, 0.0f};
        shakeTrauma = 0.0f;
    }
}

void SnoutAndSiegeGame::saveProgress() {
    if (!persistProgress) return;
    std::ostringstream out;
    out << "audio.music_muted = " << (assets.musicMuted ? 1 : 0) << "\n";
    out << "audio.sfx_muted = " << (assets.sfxMuted ? 1 : 0) << "\n";
    out << "achievements.mask = " << getAchievementsMask() << "\n";
    out << "achievements.helmets = " << achievementProgress[static_cast<int>(AchievementId::CAULDRON_CRACKER)] << "\n";
    out << "endless.best_wave = " << endlessBestWave << "\n";
    for (int i = 0; i < NUM_LEVELS; ++i) {
        out << "level." << (i + 1) << ".stars = " << bestStars[i] << "\n";
        out << "level." << (i + 1) << ".score = " << bestScores[i] << "\n";
        out << "level." << (i + 1) << ".truffle = " << (trufflesFound[i] ? 1 : 0) << "\n";
    }
    if (!writeTextFileAtomically("save_progress.cfg", out.str())) {
        std::cerr << "[SAVE] Failed to write save_progress.cfg\n";
    }
}

void SnoutAndSiegeGame::loadProgress() {
    std::ifstream in("save_progress.cfg");
    if (!in.good()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("audio.music_muted = ", 0) == 0) {
            assets.musicMuted = (std::atoi(line.substr(20).c_str()) != 0);
            continue;
        }
        if (line.rfind("audio.sfx_muted = ", 0) == 0) {
            assets.sfxMuted = (std::atoi(line.substr(18).c_str()) != 0);
            continue;
        }
        if (line.rfind("achievements.mask = ", 0) == 0) {
            int mask = std::atoi(line.substr(20).c_str());
            for (int a = 0; a < static_cast<int>(AchievementId::COUNT); ++a) {
                if (mask & (1 << a)) {
                    achievementsUnlocked[a] = true;
                    if (a < static_cast<int>(achievementDefs.size())) {
                        achievementProgress[a] = achievementDefs[a].targetCount;
                    }
                }
            }
            continue;
        }
        if (line.rfind("achievements.helmets = ", 0) == 0) {
            achievementProgress[static_cast<int>(AchievementId::CAULDRON_CRACKER)] = std::atoi(line.substr(23).c_str());
            continue;
        }
        if (line.rfind("endless.best_wave = ", 0) == 0) {
            endlessBestWave = std::max(0, std::atoi(line.substr(20).c_str()));
            continue;
        }
        // level.<N>.<field> = <value>
        int lvl = 0;
        char field[16] = {0};
        int value = 0;
        if (std::sscanf(line.c_str(), "level.%d.%15[a-z] = %d", &lvl, field, &value) == 3 &&
            lvl >= 1 && lvl <= NUM_LEVELS) {
            int i = lvl - 1;
            std::string f(field);
            if (f == "stars") bestStars[i] = std::clamp(value, 0, 3);
            else if (f == "score") bestScores[i] = std::max(0, value);
            else if (f == "truffle") trufflesFound[i] = (value != 0);
        }
    }
}

bool SnoutAndSiegeGame::handleButtonClick(const std::string& id, bool& running) {
    assets.playSound("sound.ui_click", 0.55f);

    if (id == "START_CAMPAIGN") {
        int nextLvl = getHighestUnlockedLevel();
        loadLevel(nextLvl);
        screen = ScreenState::PLAYING;
        return true;
    }
    if (id == "TOGGLE_SPEED") {
        simSpeed = (simSpeed > 1.0f) ? 1.0f : 2.5f;
        return true;
    }
    if (id == "TOGGLE_MUSIC") {
        assets.musicMuted = !assets.musicMuted;
        saveProgress();
        return true;
    }
    if (id == "TOGGLE_SFX") {
        assets.sfxMuted = !assets.sfxMuted;
        saveProgress();
        return true;
    }
    if (id == "TOGGLE_FULLSCREEN") {
        toggleFullscreen();
        return true;
    }
    if (id == "OPEN_LEVEL_SELECT") {
        // Open on the chapter being played (or the furthest one reached).
        int focus = (screen == ScreenState::PLAYING) ? currentLevelIdx : getHighestUnlockedLevel();
        levelSelectPage = (focus >= NUM_CAMPAIGN_LEVELS) ? LEVEL_SELECT_VAULT_PAGE
                                                         : std::clamp(focus / LEVELS_PER_CHAPTER, 0, NUM_CHAPTERS - 1);
        screen = ScreenState::LEVEL_SELECT;
        return true;
    }
    if (id == "BACK_TITLE") {
        screen = ScreenState::TITLE;
        return true;
    }
    if (id == "QUIT_GAME") {
        running = false;
        return true;
    }
    if (id == "RETRY_LEVEL") {
        if (screen == ScreenState::EDITOR && isTestPlaying) {
            startEditorTestPlay();
        } else if (screen == ScreenState::ENDLESS) {
            startEndlessMode();
        } else {
            loadLevel(currentLevelIdx);
            screen = ScreenState::PLAYING;
        }
        return true;
    }
    if (id == "OPEN_EDITOR") {
        screen = ScreenState::EDITOR;
        initEditor();
        return true;
    }
    if (id == "START_ENDLESS" || id == "RETRY_ENDLESS") {
        startEndlessMode();
        return true;
    }
    if (id == "EDITOR_TEST_PLAY") {
        startEditorTestPlay();
        return true;
    }
    if (id == "RETURN_TO_EDITOR") {
        returnToEditor();
        return true;
    }
    if (id == "EDITOR_SLOT_1") {
        customSlot = 1;
        loadCustomLevel(1);
        return true;
    }
    if (id == "EDITOR_SLOT_2") {
        customSlot = 2;
        loadCustomLevel(2);
        return true;
    }
    if (id == "EDITOR_SLOT_3") {
        customSlot = 3;
        loadCustomLevel(3);
        return true;
    }
    if (id == "EDITOR_SAVE") {
        saveCustomLevel(customSlot);
        return true;
    }
    if (id == "EDITOR_LOAD") {
        loadCustomLevel(customSlot);
        return true;
    }
    if (id == "EDITOR_CLEAR") {
        customLevel.blocks.clear();
        customLevel.birds.clear();
        customLevel.pendulums.clear();
        customLevel.hasTruffle = false;
        floatScores.push_back({{WINDOW_W * 0.5f, 120.0f}, "FORTRESS CLEARED!", {255, 140, 140, 255}, 1.4f, 1.4f});
        return true;
    }
    if (id == "TOOL_BLOCK") { editorTool = 0; return true; }
    if (id == "TOOL_BIRD") { editorTool = 1; return true; }
    if (id == "TOOL_PENDULUM") { editorTool = 2; return true; }
    if (id == "TOOL_TRUFFLE") { editorTool = 3; return true; }
    if (id == "TOOL_ERASER") { editorTool = 4; return true; }

    if (id == "MAT_WOOD") { editorMat = MaterialTier::TIMBER; return true; }
    if (id == "MAT_STONE") { editorMat = MaterialTier::STONE; return true; }
    if (id == "MAT_ICE") { editorMat = MaterialTier::ICE; return true; }
    if (id == "MAT_TNT") { editorMat = MaterialTier::TNT; return true; }
    if (id == "MAT_BOUNCE") { editorMat = MaterialTier::TRAMPOLINE; return true; }

    if (id == "BLOCK_SIZE_SM") { editorBlockW = 16.0f; editorBlockH = 50.0f; return true; }
    if (id == "BLOCK_SIZE_MED") { editorBlockW = 20.0f; editorBlockH = 70.0f; return true; }
    if (id == "BLOCK_SIZE_LG") { editorBlockW = 24.0f; editorBlockH = 90.0f; return true; }
    if (id == "BLOCK_SIZE_HORIZ") { editorBlockW = 80.0f; editorBlockH = 18.0f; return true; }
    if (id == "BLOCK_SIZE_SQR") { editorBlockW = 36.0f; editorBlockH = 36.0f; return true; }

    if (id == "BIRD_TOUCAN") { editorBird = BirdClass::TOUCAN; return true; }
    if (id == "BIRD_OWL") { editorBird = BirdClass::OWL; return true; }
    if (id == "BIRD_HERON") { editorBird = BirdClass::HERON; return true; }
    if (id == "BIRD_FALCON") { editorBird = BirdClass::FALCON; return true; }
    if (id == "BIRD_CONDOR") { editorBird = BirdClass::CONDOR; return true; }

    if (id == "TOGGLE_HELMET") { editorBirdHelmet = !editorBirdHelmet; return true; }
    if (id == "TOGGLE_SPIKE") { editorSpikedPendulum = !editorSpikedPendulum; return true; }
    if (id == "TOGGLE_GRID") { editorSnapGrid = !editorSnapGrid; return true; }
    if (id == "CYCLE_THEME") { editorTheme = (editorTheme + 1) % (NUM_CHAPTERS + 1); return true; }
    if (id == "WIND_DEC") { editorWind = std::max(-120.0f, editorWind - 30.0f); return true; }
    if (id == "WIND_INC") { editorWind = std::min(120.0f, editorWind + 30.0f); return true; }
    if (id == "TOGGLE_STORM") { editorStorm = !editorStorm; return true; }

    if (id == "NEXT_LEVEL_BTN") {
        if (currentLevelIdx + 1 < NUM_LEVELS && isLevelUnlocked(currentLevelIdx + 1)) {
            loadLevel(currentLevelIdx + 1);
            screen = ScreenState::PLAYING;
        }
        return true;
    }
    if (id == "OPEN_ACHIEVEMENTS") {
        screen = ScreenState::ACHIEVEMENTS;
        return true;
    }
    if (id == "PAGE_PREV") {
        if (levelSelectPage > 0) levelSelectPage--;
        return true;
    }
    if (id == "PAGE_NEXT") {
        if (levelSelectPage < LEVEL_SELECT_VAULT_PAGE) levelSelectPage++;
        return true;
    }
    if (id.rfind("GOTO_PAGE_", 0) == 0) {
        levelSelectPage = std::clamp(std::atoi(id.c_str() + 10), 0, LEVEL_SELECT_VAULT_PAGE);
        return true;
    }
    if (id.rfind("PLAY_LEVEL_", 0) == 0) {
        int idx = std::atoi(id.substr(11).c_str());
        if (idx >= 0 && idx < NUM_LEVELS && isLevelUnlocked(idx)) {
            loadLevel(idx);
            screen = ScreenState::PLAYING;
        }
        return true;
    }
    return false;
}

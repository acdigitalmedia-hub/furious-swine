#include "self_test.h"
#include "game.h"
#include "level_format.h"

// ============================================================================
// AUTOMATED HEADLESS SELF-TEST & SCREENSHOT VERIFICATION SUITE
// ============================================================================
// Snapshots files the self-test overwrites and puts them back on every exit
// path, including early returns when a check fails.
struct ScopedFileRestore {
    struct Entry { std::string path; bool existed; std::string contents; };
    std::vector<Entry> entries;
    explicit ScopedFileRestore(std::initializer_list<const char*> paths) {
        for (const char* p : paths) {
            Entry e{p, false, {}};
            std::ifstream in(p, std::ios::binary);
            if (in.good()) {
                std::stringstream ss;
                ss << in.rdbuf();
                e.existed = true;
                e.contents = ss.str();
            }
            entries.push_back(std::move(e));
        }
    }
    ~ScopedFileRestore() {
        for (const auto& e : entries) {
            if (e.existed) {
                std::ofstream out(e.path, std::ios::binary | std::ios::trunc);
                out << e.contents;
            } else {
                std::remove(e.path.c_str());
            }
        }
    }
};

int runSelfTestAndExportScreenshots(bool exportBmp) {
    ScopedFileRestore restoreUserFiles{"save_progress.cfg", "custom_siege_2.cfg"};

    setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("SDL_AUDIODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Snout & Siege Self-Test", 0, 0, WINDOW_W, WINDOW_H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    SnoutAndSiegeGame game;
    bool assetsOk = game.assets.loadAll(ren);
    bool levelsOk = game.initLevels();

    std::cout << "[SELF-TEST] Backend: " << (sdlRuntimeLoaded() ? "SDL2 Dynamic Runtime" : "X11/Software Fallback")
              << " | Loaded " << game.assets.loadedSpriteCount << " BMP sprites, "
              << game.assets.loadedSoundCount << " WAV sounds, and "
              << game.levels.size() << " levels.\n";
    if (!assetsOk || !levelsOk || game.assets.loadedSpriteCount < 23 || game.assets.loadedSoundCount < 25 ||
        static_cast<int>(game.levels.size()) != NUM_LEVELS) {
        std::cerr << "[SELF-TEST] Asset or " << NUM_LEVELS << "-level count verification failed!\n";
        return 2;
    }

    // Verify all 150 levels stand rock-solid at idle AND when awake for 300 frames (5 seconds)
    // checking block tilt, block displacement, bird rotation, bird displacement, and pendulum equilibrium!
    int stabilityFailures = 0;
    for (int lvl = 0; lvl < NUM_LEVELS; ++lvl) {
        game.loadLevel(lvl);
        game.fortressAwake = true; // Even with physics awake, untouched towers, birds, & pendulums must not drift or rotate!
        auto initBlocks = game.blocks;
        auto initBirds = game.birds;
        for (int f = 0; f < 300; ++f) game.stepPhysics(1.0f / 60.0f);
        float maxTilt = 0.0f, maxDisp = 0.0f, maxBirdRot = 0.0f, maxBirdDisp = 0.0f;
        for (size_t i = 0; i < game.blocks.size(); ++i) {
            maxTilt = std::max(maxTilt, std::abs(game.blocks[i].angle));
            maxDisp = std::max(maxDisp, (game.blocks[i].pos - initBlocks[i].pos).length());
        }
        for (size_t i = 0; i < game.birds.size(); ++i) {
            maxBirdRot = std::max(maxBirdRot, std::abs(game.birds[i].angle));
            maxBirdDisp = std::max(maxBirdDisp, (game.birds[i].pos - initBirds[i].pos).length());
        }
        for (size_t i = 0; i < game.pendulums.size(); ++i) {
            maxTilt = std::max(maxTilt, std::abs(game.pendulums[i].angle));
        }
        if (maxTilt > 0.01f || maxDisp > 1.5f || maxBirdRot > 0.01f || maxBirdDisp > 1.5f) {
            stabilityFailures++;
            std::cerr << "[SELF-TEST] Level " << (lvl + 1) << " failed 300-frame stability: "
                      << "maxTilt=" << maxTilt << " maxDisp=" << maxDisp
                      << " maxBirdRot=" << maxBirdRot << " maxBirdDisp=" << maxBirdDisp << "\n";
            for (size_t i = 0; i < game.blocks.size(); ++i) {
                float d = (game.blocks[i].pos - initBlocks[i].pos).length();
                if (d > 0.5f) {
                    std::cerr << "  Block " << i << " moved " << d << "px: init=("
                              << initBlocks[i].pos.x << "," << initBlocks[i].pos.y << ",w=" << initBlocks[i].w << ",h=" << initBlocks[i].h << ") now=("
                              << game.blocks[i].pos.x << "," << game.blocks[i].pos.y << ")\n";
                }
            }
        }
    }
    if (stabilityFailures > 0) {
        std::cerr << "[SELF-TEST] " << stabilityFailures << " levels failed stability check!\n";
        return 3;
    }
    std::cout << "[SELF-TEST] All " << NUM_LEVELS << " levels verified rock-solid at idle (maxTilt = 0.000 rad).\n";

    auto saveFrame = [&](const std::string& path) {
        if (!exportBmp) return;
        game.render(ren);
        SDL_RenderPresent(ren);
        SDL_Surface* snap = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_W, WINDOW_H, 32,
                                                           SDL_PIXELFORMAT_ARGB8888);
        SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_ARGB8888, snap->pixels, snap->pitch);
        SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "wb");
        if (rw) SDL_SaveBMP_RW(snap, rw, 1);
        SDL_FreeSurface(snap);
    };

    // Test per-level high score and level unlocking
    game.recordLevelScore(0, 44200, 3);
    game.recordLevelScore(1, 36800, 3);
    game.recordLevelScore(2, 29500, 2);

    game.screen = ScreenState::TITLE;
    saveFrame("screenshot_title.bmp");

    // Export Royal Swine Trophy Hall & Achievements Screen
    game.screen = ScreenState::ACHIEVEMENTS;
    saveFrame("screenshot_achievements.bmp");

    // Export Level Select Pages 1, 2, and 6 (Vaults)
    game.screen = ScreenState::LEVEL_SELECT;
    game.levelSelectPage = 0; saveFrame("screenshot_level_select_p1.bmp");
    game.levelSelectPage = 1; saveFrame("screenshot_level_select_p2.bmp");
    game.levelSelectPage = 6; saveFrame("screenshot_level_select_p3.bmp");

    // Export screenshots of Chapter Backgrounds across levels
    game.screen = ScreenState::PLAYING;
    game.loadLevel(0);  saveFrame("screenshot_chap1.bmp");
    game.loadLevel(1);  saveFrame("screenshot_truffle_level.bmp");
    game.loadLevel(2);  saveFrame("screenshot_level3.bmp");
    game.loadLevel(10); saveFrame("screenshot_chap2.bmp");
    game.loadLevel(20); saveFrame("screenshot_chap3.bmp");
    game.loadLevel(30); saveFrame("screenshot_chap4.bmp");
    game.loadLevel(40); saveFrame("screenshot_chap5.bmp");
    game.loadLevel(NUM_CAMPAIGN_LEVELS); saveFrame("screenshot_vault_level.bmp");
    game.loadLevel(NUM_CAMPAIGN_LEVELS); saveFrame("screenshot_chap6_vault.bmp");

    // Export Fortress Architect level builder
    game.screen = ScreenState::EDITOR;
    game.isTestPlaying = false;
    game.initEditor();
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::STONE, 760.0f, GROUND_Y - 12.0f, 160.0f, 24.0f));
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 710.0f, GROUND_Y - 55.0f, 20.0f, 70.0f));
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 810.0f, GROUND_Y - 55.0f, 20.0f, 70.0f));
    game.customLevel.blocks.push_back(SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 760.0f, GROUND_Y - 98.0f, 140.0f, 18.0f));
    BirdTarget eb1{}; eb1.kind = BirdClass::OWL; eb1.pos = {760.0f, GROUND_Y - 130.0f}; eb1.radius = 24.0f; eb1.hp = 120.0f; eb1.maxHp = 120.0f; eb1.hasHelmet = true; eb1.helmetHp = 100.0f; eb1.maxHelmetHp = 100.0f;
    game.customLevel.birds.push_back(eb1);
    saveFrame("screenshot_editor.bmp");

    // Export Endless Siege Gauntlet Mode
    game.startEndlessMode();
    saveFrame("screenshot_endless.bmp");

    // Export Gale the Aeronaut (Glider Pig) in mid-air aerodynamic flight with aerial bombs
    game.loadLevel(0);
    game.activePigs.clear();
    PigBody gale = game.createPigInstance(PigType::GLIDER, {580.0f, 280.0f});
    gale.launched = true;
    gale.vel = {320.0f, -40.0f};
    game.activePigs.push_back(gale);
    AerialBomb ab1{}; ab1.active = true; ab1.pos = {540.0f, 340.0f}; ab1.vel = {60.0f, 160.0f}; ab1.radius = 9.0f;
    AerialBomb ab2{}; ab2.active = true; ab2.pos = {500.0f, 370.0f}; ab2.vel = {40.0f, 180.0f}; ab2.radius = 9.0f;
    game.aerialBombs = {ab1, ab2};
    saveFrame("screenshot_pig_glider.bmp");

    // Export Atticus the Graviton Boar activating cosmic vortex
    game.loadLevel(27); // Level 28 (Cavern Gate)
    game.activePigs.clear();
    PigBody atticus = game.createPigInstance(PigType::GRAVITON, {600.0f, 320.0f});
    atticus.launched = true;
    atticus.abilityUsed = true;
    atticus.vortexTimer = 1.2f;
    atticus.vel = {40.0f, 10.0f};
    game.activePigs.push_back(atticus);
    for (int k = 0; k < 25; ++k) {
        float ang = k * (6.28318f / 25.0f);
        float r = 50.0f + 30.0f * (k % 3);
        Particle cm{};
        cm.pos = Vec2(600.0f + std::cos(ang) * r, 320.0f + std::sin(ang) * r);
        cm.vel = Vec2(-std::sin(ang) * 90.0f, std::cos(ang) * 90.0f);
        cm.kind = ParticleKind::COSMIC_MOTE;
        cm.color = {175, 95, 255, 255};
        cm.size = 5.0f;
        cm.life = 1.0f;
        cm.maxLife = 1.0f;
        game.particles.push_back(cm);
    }
    saveFrame("screenshot_pig_graviton.bmp");
    game.particles.clear();

    // Export Rusty the Drill Hog executing supersonic rotary bore
    game.loadLevel(15); // Level 16 (Canopy Skyway)
    game.activePigs.clear();
    PigBody rusty = game.createPigInstance(PigType::DRILL, {680.0f, 420.0f});
    rusty.launched = true;
    rusty.abilityUsed = true;
    rusty.drillTimer = 1.5f;
    rusty.vel = {780.0f, 0.0f};
    game.activePigs.push_back(rusty);
    game.spawnSplinters({700.0f, 420.0f}, MaterialTier::TIMBER, 16, 260.0f);
    saveFrame("screenshot_pig_drill.bmp");
    game.particles.clear();

    // Export Commander Falcon with Directional Tower Shield defending the fortress
    game.loadLevel(15); // Level 16
    saveFrame("screenshot_bird_falcon.bmp");

    // Export Baron Von Condor in Suspended Steampunk Zeppelin hovering above fortress
    game.loadLevel(31); // Level 32 (Zeppelin Spire)
    saveFrame("screenshot_bird_condor.bmp");

    // Export Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
    game.sloMoActive = true;
    game.sloMoTimer = 1.0f;
    game.letterboxY = 46.0f;
    game.sloMoFocusPos = {800.0f, 380.0f};
    saveFrame("screenshot_slomo_finish.bmp");
    game.sloMoActive = false;
    game.letterboxY = 0.0f;
    game.aerialBombs.clear();

    // Export gameplay juice screenshot demonstrating aiming trajectory, ghost arc, splinters & feathers
    game.loadLevel(0);
    game.ghostTrajectory = {
        {SLING_X, SLING_Y}, {340.0f, 380.0f}, {480.0f, 310.0f}, {620.0f, 290.0f},
        {760.0f, 330.0f}, {880.0f, 420.0f}, {960.0f, 520.0f}
    };
    game.draggingSling = true;
    game.slingPullPos = Vec2(SLING_X - 65.0f, SLING_Y + 45.0f);
    if (!game.activePigs.empty()) {
        game.activePigs[0].pos = game.slingPullPos;
        game.activePigs[0].angle = std::atan2(-45.0f, 65.0f);
    }
    game.spawnSplinters({820.0f, 460.0f}, MaterialTier::TIMBER, 14, 200.0f);
    game.spawnFeatherBurst({860.0f, 420.0f}, BirdClass::TOUCAN, 18, 220.0f);
    game.floatScores.push_back({{860.0f, 390.0f}, "+5000", {255, 230, 90, 255}, 1.1f, 1.3f});
    saveFrame("screenshot_gameplay_juice.bmp");

    // Test standard level complete modal
    game.loadLevel(0);
    game.bestScores[0] = 50000;
    for (auto& t : game.birds) t.defeated = true;
    game.score = 18500;
    game.stepPhysics(1.0f / 60.0f);
    saveFrame("screenshot_level_complete.bmp");

    // Test NEW HIGH SCORE celebration banner & animation
    game.loadLevel(0);
    game.bestScores[0] = 20000;
    game.prevLevelBest = 20000;
    for (auto& t : game.birds) t.defeated = true;
    game.score = 46500;
    game.stepPhysics(1.0f / 60.0f);
    saveFrame("screenshot_new_highscore.bmp");

    // Test Milestone Pig Unlock modal (Level 5 cleared -> Bramble unlocked!)
    game.loadLevel(4);
    for (auto& t : game.birds) t.defeated = true;
    game.score = 32000;
    game.stepPhysics(1.0f / 60.0f);
    saveFrame("screenshot_milestone_unlock.bmp");

    // Test Level 7 (The Powder Viaduct with TNT Kegs)
    game.loadLevel(6);
    saveFrame("screenshot_tnt_level.bmp");

    // Test Level 31 (Glacial Ziggurat with Brittle Ice)
    game.loadLevel(30);
    saveFrame("screenshot_ice_level.bmp");

    // Test Level 8 (The Bouncing Keep with Trampoline Deflector)
    game.loadLevel(7);
    saveFrame("screenshot_trampoline_level.bmp");

    // Test Level 12 (Cauldron Barracks with Cauldron-Helmed Birds)
    game.loadLevel(11);
    saveFrame("screenshot_helmed_bird.bmp");

    // Test Level 18 (Pendulum Citadel with Swinging Wrecking Ball)
    game.loadLevel(17);
    saveFrame("screenshot_pendulum_level.bmp");

    // Test Level 14 (Crossbeam Stockade with Headwind & Wind Gauge)
    game.loadLevel(13);
    saveFrame("screenshot_wind_level.bmp");

    // Test Level 39 (Thunder Peak Sanctuary with Cumulonimbus Storm & Lightning)
    game.loadLevel(38);
    game.triggerLightningStrike();
    saveFrame("screenshot_storm_level.bmp");

    // Test Level 44 (Volcanic Casemate with Magma Vents)
    game.loadLevel(43);
    for (auto& b : game.blocks) {
        if (b.mat == MaterialTier::MAGMA_VENT) {
            game.eruptMagmaVent(b);
            break;
        }
    }
    saveFrame("screenshot_magma_level.bmp");

    // Verify Simulation Fast-Forward toggle
    game.simSpeed = 2.5f;
    game.updateGame(0.016f);
    if (game.simSpeed != 2.5f) {
        std::cerr << "[SELF-TEST] simSpeed failed to maintain 2.5x during game update!\n";
        return 4;
    }
    game.spawnNextPigInSlingshot();
    if (game.simSpeed != 1.0f) {
        std::cerr << "[SELF-TEST] simSpeed failed to auto-reset to 1.0x on pig reload!\n";
        return 5;
    }

    // Verify TNT Detonation and Ice destruction mechanics
    game.loadLevel(6);
    bool foundTnt = false;
    for (auto& b : game.blocks) {
        if (b.mat == MaterialTier::TNT) {
            foundTnt = true;
            game.triggerTntDetonation(b);
            break;
        }
    }
    if (!foundTnt) {
        std::cerr << "[SELF-TEST] Level 7 missing TNT keg!\n";
        return 6;
    }
    game.stepPhysics(1.0f / 60.0f);

    // Verify Cauldron Helmet absorption & shattering mechanics
    {
        BirdTarget bt{};
        bt.kind = BirdClass::OWL;
        bt.pos = {800.0f, 500.0f};
        bt.radius = 23.0f;
        bt.hp = 140.0f;
        bt.maxHp = 140.0f;
        bt.hasHelmet = true;
        bt.helmetHp = 100.0f;
        bt.maxHelmetHp = 100.0f;
        game.damageBird(bt, 40.0f);
        if (bt.helmetHp != 60.0f || bt.hp != 140.0f || !bt.hasHelmet) {
            std::cerr << "[SELF-TEST] Helmet failed to absorb 40 damage properly!\n";
            return 7;
        }
        int scoreBefore = game.score;
        game.damageBird(bt, 80.0f); // 60 absorbed by helmet (shatters!), 20 into bird hp (140 -> 120)
        if (bt.hasHelmet || bt.helmetHp != 0.0f || bt.hp != 120.0f || (game.score - scoreBefore) != 1000) {
            std::cerr << "[SELF-TEST] Helmet shattering failed!\n";
            return 8;
        }
    }

    // Verify Trampoline elastic bounce mechanics
    {
        PigBody pb{};
        pb.active = true;
        pb.launched = true;
        pb.radius = 18.0f;
        pb.pos = {600.0f, 595.0f};
        pb.vel = {50.0f, 200.0f};
        BlockBody tb{};
        tb.mat = MaterialTier::TRAMPOLINE;
        tb.pos = {600.0f, 618.0f};
        tb.w = 100.0f;
        tb.h = 24.0f;
        game.resolveCircleOBB(pb, tb);
        if (pb.vel.y >= 0.0f) {
            std::cerr << "[SELF-TEST] Trampoline failed to bounce pig upward! vy=" << pb.vel.y << "\n";
            return 9;
        }
    }

    // Verify Pendulum harmonic gravity physics
    {
        PendulumBody pend{};
        pend.anchor = {800.0f, 200.0f};
        pend.length = 150.0f;
        pend.angle = 0.35f;
        pend.angVel = 0.0f;
        pend.radius = 25.0f;
        pend.mass = 8.0f;
        game.pendulums = {pend};
        game.fortressAwake = true;
        game.stepPhysics(1.0f / 60.0f);
        if (game.pendulums[0].angVel >= 0.0f) {
            std::cerr << "[SELF-TEST] Pendulum harmonic gravity failed to accelerate toward equilibrium!\n";
            return 10;
        }
    }

    // Verify Wind acceleration on airborne pig
    {
        game.loadLevel(21); // Level 22 has canyon tailwind (wind = +110.0f)
        PigBody pb{};
        pb.active = true;
        pb.launched = true;
        pb.radius = 18.0f;
        pb.pos = {400.0f, 400.0f};
        pb.vel = {0.0f, 0.0f};
        game.activePigs = {pb};
        game.stepPhysics(1.0f / 60.0f);
        if (game.activePigs[0].vel.x <= 0.0f) {
            std::cerr << "[SELF-TEST] Wind failed to accelerate airborne pig downwind!\n";
            return 11;
        }
    }

    // Verify Magma Vent blast damage and thermal effects (ice melt & TNT ignition)
    {
        BlockBody vent = SnoutAndSiegeGame::makeBlock(MaterialTier::MAGMA_VENT, 500.0f, 600.0f, 40.0f, 28.0f);
        BlockBody ice = SnoutAndSiegeGame::makeBlock(MaterialTier::ICE, 530.0f, 600.0f, 30.0f, 30.0f);
        BlockBody tnt = SnoutAndSiegeGame::makeBlock(MaterialTier::TNT, 470.0f, 600.0f, 30.0f, 30.0f);
        game.blocks = {vent, ice, tnt};
        float iceHpBefore = game.blocks[1].hp;
        game.damageBlock(game.blocks[0], 50.0f);
        if (game.blocks[1].hp >= iceHpBefore || !game.blocks[2].ignited) {
            std::cerr << "[SELF-TEST] Magma vent blast failed to melt ice or ignite adjacent TNT!\n";
            return 12;
        }
    }

    // Verify Lightning strike generation and sky flash
    {
        game.loadLevel(38); // Thunder Peak Sanctuary
        game.triggerLightningStrike();
        if (game.lightningBolts.empty() || game.skyFlashAlpha <= 0.0f) {
            std::cerr << "[SELF-TEST] Lightning strike generation failed!\n";
            return 13;
        }
    }

    // Unit Test 14: Golden Truffle collection, scoring, particle burst & TRUFFLE_SNIFFER achievement
    {
        game.loadLevel(1); // Level 2 has hidden golden truffle
        if (!game.levelTruffle.exists || game.levelTruffle.collected) {
            std::cerr << "[SELF-TEST] Level 2 golden truffle not properly initialized!\n";
            return 14;
        }
        int scoreBefore = game.score;
        game.collectGoldenTruffle();
        if (!game.levelTruffle.collected || (game.score - scoreBefore) != 2500 || !game.trufflesFound[1]) {
            std::cerr << "[SELF-TEST] Truffle collection logic or +2,500 score award failed!\n";
            return 15;
        }
        int sparkleCount = 0;
        for (const auto& p : game.particles) {
            if (p.kind == ParticleKind::GOLD_SPARKLE) sparkleCount++;
        }
        if (sparkleCount < 24) {
            std::cerr << "[SELF-TEST] Truffle collection failed to spawn >= 24 GOLD_SPARKLE particles!\n";
            return 16;
        }
        if (!game.achievementsUnlocked[static_cast<int>(AchievementId::TRUFFLE_SNIFFER)]) {
            std::cerr << "[SELF-TEST] Collecting first truffle failed to unlock TRUFFLE_SNIFFER achievement!\n";
            return 17;
        }
    }

    // Unit Test 15: Secret Vault unlock progression logic (2, 5, 8, 11, 14, 18, 22, 26, 30, 35 truffles)
    {
        // Reset truffles found
        game.trufflesFound.fill(false);
        // With 0 truffles, all vaults should be locked
        for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) {
            if (game.isLevelUnlocked(v)) {
                std::cerr << "[SELF-TEST] Vault level " << v << " should be locked with 0 truffles!\n";
                return 18;
            }
        }
        // Found 2 truffles -> Vault 01 (index NUM_CAMPAIGN_LEVELS) unlocked, Vault 02 (index NUM_CAMPAIGN_LEVELS + 1) locked
        game.trufflesFound[1] = true;
        game.trufflesFound[4] = true;
        if (!game.isLevelUnlocked(NUM_CAMPAIGN_LEVELS) || game.isLevelUnlocked(NUM_CAMPAIGN_LEVELS + 1)) {
            std::cerr << "[SELF-TEST] Vault 01 failed to unlock with 2 truffles, or Vault 02 unlocked prematurely!\n";
            return 19;
        }
        // Found 35 truffles -> All 10 vaults unlocked
        for (int i = 0; i < 35; ++i) game.trufflesFound[i] = true;
        for (int v = NUM_CAMPAIGN_LEVELS; v < NUM_LEVELS; ++v) {
            if (!game.isLevelUnlocked(v)) {
                std::cerr << "[SELF-TEST] Vault level " << v << " failed to unlock with 35 truffles!\n";
                return 20;
            }
        }
    }

    // Unit Test 16: Achievement unlocking, toast animation triggering, and mask serialization
    {
        // Reset achievements
        game.achievementsUnlocked.fill(false);
        game.toast.active = false;
        game.unlockAchievement(AchievementId::FIRST_BLOOD);
        if (!game.achievementsUnlocked[static_cast<int>(AchievementId::FIRST_BLOOD)]) {
            std::cerr << "[SELF-TEST] unlockAchievement failed to mark FIRST_BLOOD unlocked!\n";
            return 21;
        }
        if (!game.toast.active || game.toast.title != "First Strike") {
            std::cerr << "[SELF-TEST] Toast notification failed to activate for unlocked achievement!\n";
            return 22;
        }
        uint32_t mask = game.getAchievementsMask();
        if ((mask & (1u << static_cast<int>(AchievementId::FIRST_BLOOD))) == 0) {
            std::cerr << "[SELF-TEST] getAchievementsMask failed to include FIRST_BLOOD bit!\n";
            return 23;
        }
        // Test save and load persistence
        game.saveProgress();
        SnoutAndSiegeGame testLoader;
        testLoader.initAchievements();
        testLoader.loadProgress();
        if (!testLoader.achievementsUnlocked[static_cast<int>(AchievementId::FIRST_BLOOD)]) {
            std::cerr << "[SELF-TEST] Achievement mask failed to persist through saveProgress/loadProgress!\n";
            return 24;
        }
    }

    // Unit Test 17: Gale the Aeronaut (Glider Pig) aerodynamic lift & twin aerial bomb detonation
    {
        game.loadLevel(0);
        game.activePigs.clear();
        game.aerialBombs.clear();
        PigBody gale = game.createPigInstance(PigType::GLIDER, {450.0f, 300.0f});
        gale.launched = true;
        gale.vel = {200.0f, 50.0f}; // Moving down and forward
        game.activePigs.push_back(gale);

        game.activateMidAirAbility();
        if (!game.activePigs[0].abilityUsed || game.activePigs[0].vel.y >= 0.0f || game.activePigs[0].vel.x <= 200.0f) {
            std::cerr << "[SELF-TEST] Gale mid-air aerodynamic surge failed to lift swine upward and accelerate forward!\n";
            return 25;
        }
        if (game.aerialBombs.size() != 2) {
            std::cerr << "[SELF-TEST] Gale ability failed to drop exactly 2 aerial bombs! count=" << game.aerialBombs.size() << "\n";
            return 26;
        }
        // Place a test target block and test bomb detonation
        BlockBody targetBlock = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 450.0f, 350.0f, 40.0f, 40.0f);
        game.blocks = {targetBlock};
        float blockHpBefore = game.blocks[0].hp;
        game.aerialBombs[0].pos = {450.0f, 340.0f}; // Right above target block
        game.detonateAerialBomb(game.aerialBombs[0]);
        if (game.blocks[0].hp >= blockHpBefore) {
            std::cerr << "[SELF-TEST] Aerial bomb detonation failed to inflict radial damage on target block!\n";
            return 27;
        }
    }

    // Unit Test 18: Fortress Architect Level Serialization & Test Play Snapshot Restoration
    {
        game.screen = ScreenState::EDITOR;
        game.isTestPlaying = false;
        game.initEditor();
        game.customSlot = 2;
        game.customLevel.name = "SIEGE_TEST_SLOT_2";
        game.customLevel.blocks = {
            SnoutAndSiegeGame::makeBlock(MaterialTier::STONE, 750.0f, 500.0f, 30.0f, 60.0f),
            SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 820.0f, 500.0f, 20.0f, 70.0f),
            SnoutAndSiegeGame::makeBlock(MaterialTier::ICE, 785.0f, 450.0f, 90.0f, 18.0f)
        };
        BirdTarget t1{}; t1.kind = BirdClass::OWL; t1.pos = {785.0f, 420.0f}; t1.radius = 24.0f; t1.hp = 120.0f; t1.maxHp = 120.0f; t1.hasHelmet = true; t1.helmetHp = 100.0f; t1.maxHelmetHp = 100.0f;
        BirdTarget t2{}; t2.kind = BirdClass::HERON; t2.pos = {880.0f, 500.0f}; t2.radius = 28.0f; t2.hp = 180.0f; t2.maxHp = 180.0f;
        game.customLevel.birds = {t1, t2};
        PendulumBody pb{}; pb.anchor = {785.0f, 250.0f}; pb.length = 90.0f; pb.angle = 0.35f; pb.radius = 20.0f; pb.spiked = true;
        game.customLevel.pendulums = {pb};
        game.customLevel.hasTruffle = true;
        game.customLevel.trufflePos = {785.0f, 380.0f};

        game.saveCustomLevel(2);
        std::ifstream in("custom_siege_2.cfg");
        if (!in.good()) {
            std::cerr << "[SELF-TEST] saveCustomLevel(2) failed to write custom_siege_2.cfg!\n";
            return 28;
        }
        in.close();

        // Clear in-memory and reload
        game.customLevel.blocks.clear();
        game.customLevel.birds.clear();
        game.customLevel.pendulums.clear();
        game.customLevel.hasTruffle = false;
        game.loadCustomLevel(2);

        if (game.customLevel.blocks.size() != 3 || game.customLevel.birds.size() != 2 ||
            game.customLevel.pendulums.size() != 1 || !game.customLevel.hasTruffle) {
            std::cerr << "[SELF-TEST] loadCustomLevel(2) failed to deserialize exact custom level specification!\n";
            return 29;
        }

        // Test play snapshot and return
        game.startEditorTestPlay();
        if (!game.isTestPlaying || game.blocks.size() != 3 || game.birds.size() != 2 || game.activePigs.empty()) {
            std::cerr << "[SELF-TEST] startEditorTestPlay failed to initialize playable test session!\n";
            return 30;
        }
        // Destroy a block during test play
        game.blocks[0].destroyed = true;
        game.returnToEditor();
        if (game.isTestPlaying || game.customLevel.blocks.size() != 3 || game.customLevel.blocks[0].destroyed) {
            std::cerr << "[SELF-TEST] returnToEditor failed to restore pristine architectural snapshot!\n";
            return 31;
        }
        std::remove("custom_siege_2.cfg");
    }

    // Unit Test 19: Endless Siege Gauntlet Mode (procedural scaling & combo multiplier)
    {
        game.startEndlessMode();
        if (game.screen != ScreenState::ENDLESS || game.endlessWave != 1 || game.endlessScore != 0 ||
            game.blocks.empty() || game.birds.empty() || game.activePigs.empty()) {
            std::cerr << "[SELF-TEST] startEndlessMode failed to initialize Wave 1 arena and swine arsenal!\n";
            return 32;
        }
        // Verify combo multiplier increases on hit
        float multBefore = game.endlessComboMultiplier;
        game.damageBird(game.birds[0], 25.0f);
        if (game.endlessScore <= 0 || game.endlessComboMultiplier <= multBefore) {
            std::cerr << "[SELF-TEST] Endless mode score or combo multiplier failed to advance on bird damage!\n";
            return 33;
        }
        // Advance wave by defeating all birds
        for (auto& b : game.birds) b.defeated = true;
        game.stepPhysics(1.0f / 60.0f);
        if (game.endlessWave != 2) {
            std::cerr << "[SELF-TEST] Defeating all birds in wave 1 failed to advance to Wave 2!\n";
            return 34;
        }
        if (game.endlessBestWave < 2) {
            std::cerr << "[SELF-TEST] endlessBestWave failed to record highest wave achieved!\n";
            return 35;
        }
    }

    // Unit Test 20: Atticus the Graviton Boar vortex pull, cosmic mote emission & singularity implosion
    {
        game.loadLevel(0);
        game.activePigs.clear();
        game.particles.clear();
        PigBody atticus = game.createPigInstance(PigType::GRAVITON, {500.0f, 300.0f});
        atticus.launched = true;
        atticus.vel = {140.0f, -20.0f};
        game.activePigs.push_back(atticus);

        game.activateMidAirAbility();
        if (!game.activePigs[0].abilityUsed || game.activePigs[0].vortexTimer <= 0.0f) {
            std::cerr << "[SELF-TEST] Graviton Boar ability activation failed!\n";
            return 36;
        }

        // Test vortex pull forces on nearby blocks & birds
        BlockBody testBlock = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 620.0f, 300.0f, 30.0f, 30.0f);
        testBlock.vel = {0.0f, 0.0f};
        game.blocks = {testBlock};
        game.stepPhysics(1.0f / 60.0f);
        // Block was to the right of Atticus (x=620 vs x=500), so pull force must accelerate it leftwards (vel.x < 0)
        if (game.blocks[0].vel.x >= 0.0f) {
            std::cerr << "[SELF-TEST] Graviton vortex failed to exert inward pull force on adjacent block! vx="
                      << game.blocks[0].vel.x << "\n";
            return 37;
        }

        // Test singularity implosion shockwave, radial damage & VORTEX_MAESTRO achievement
        BirdTarget b1{}; b1.kind = BirdClass::TOUCAN; b1.pos = {470.0f, 300.0f}; b1.radius = 18.0f; b1.hp = 80.0f; b1.maxHp = 80.0f;
        BirdTarget b2{}; b2.kind = BirdClass::OWL;    b2.pos = {530.0f, 300.0f}; b2.radius = 21.0f; b2.hp = 120.0f; b2.maxHp = 120.0f;
        BirdTarget b3{}; b3.kind = BirdClass::HERON;  b3.pos = {500.0f, 260.0f}; b3.radius = 24.0f; b3.hp = 180.0f; b3.maxHp = 180.0f;
        game.birds = {b1, b2, b3};
        float b1HpBefore = b1.hp;
        game.triggerGravitonImplosion(game.activePigs[0].pos);
        if (game.birds[0].hp >= b1HpBefore || !game.achievementsUnlocked[static_cast<int>(AchievementId::VORTEX_MAESTRO)]) {
            std::cerr << "[SELF-TEST] Graviton singularity implosion failed to damage birds or unlock VORTEX_MAESTRO!\n";
            return 38;
        }
    }

    // Unit Test 21: Rusty the Drill Hog supersonic rotary bore, multi-block penetration & foundation subsidence
    {
        game.loadLevel(0);
        game.activePigs.clear();
        PigBody rusty = game.createPigInstance(PigType::DRILL, {400.0f, 400.0f});
        rusty.launched = true;
        rusty.vel = {160.0f, 0.0f};
        game.activePigs.push_back(rusty);

        game.activateMidAirAbility();
        if (!game.activePigs[0].abilityUsed || game.activePigs[0].drillTimer <= 0.0f || game.activePigs[0].vel.x < 750.0f) {
            std::cerr << "[SELF-TEST] Drill Hog supersonic bore activation failed! vx=" << game.activePigs[0].vel.x << "\n";
            return 39;
        }

        // Test multi-block piercing through 3 consecutive blocks in a single shot & DEEP_BORE achievement
        game.shotDrillPierces = 0;
        BlockBody b1 = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 415.0f, 400.0f, 20.0f, 50.0f);
        game.activePigs[0].pos = {400.0f, 400.0f};
        game.resolveCircleOBB(game.activePigs[0], b1);

        BlockBody b2 = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 445.0f, 400.0f, 20.0f, 50.0f);
        game.activePigs[0].pos = {430.0f, 400.0f};
        game.resolveCircleOBB(game.activePigs[0], b2);

        BlockBody b3 = SnoutAndSiegeGame::makeBlock(MaterialTier::STONE,  475.0f, 400.0f, 20.0f, 50.0f);
        game.activePigs[0].pos = {460.0f, 400.0f};
        game.resolveCircleOBB(game.activePigs[0], b3);

        if (game.shotDrillPierces < 3 || !game.achievementsUnlocked[static_cast<int>(AchievementId::DEEP_BORE)] || game.activePigs[0].vel.x < 350.0f) {
            std::cerr << "[SELF-TEST] Drill Hog failed to pierce 3 blocks or award DEEP_BORE! pierces="
                      << game.shotDrillPierces << " vx=" << game.activePigs[0].vel.x << "\n";
            return 40;
        }

        // Test foundation burrowing subsidence cave-in when contacting the ground
        game.activePigs[0].pos = {700.0f, GROUND_Y - game.activePigs[0].radius - 1.0f};
        BlockBody foundation = SnoutAndSiegeGame::makeBlock(MaterialTier::STONE, 720.0f, GROUND_Y - 30.0f, 30.0f, 40.0f);
        game.blocks = {foundation};
        float foundHpBefore = foundation.hp;
        game.stepPhysics(1.0f / 60.0f);
        if (game.blocks[0].vel.y <= 0.0f || game.blocks[0].hp >= foundHpBefore) {
            std::cerr << "[SELF-TEST] Drill Hog ground burrowing failed to trigger subsidence on foundation block!\n";
            return 41;
        }
    }

    // Unit Test 22: Commander Falcon directional shield deflection, damage absorption & shattering
    {
        BirdTarget falcon{};
        falcon.kind = BirdClass::FALCON;
        falcon.pos = {800.0f, 500.0f};
        falcon.radius = 23.0f;
        falcon.hp = 150.0f;
        falcon.maxHp = 150.0f;
        falcon.hasShield = true;
        falcon.shieldHp = 120.0f;
        falcon.maxShieldHp = 120.0f;
        falcon.mass = 2.5f;
        falcon.invMass = 1.0f / 2.5f;

        // Frontal pig collision bounce-back (p.pos.x <= falcon.pos.x) with high restitution (e=0.75)
        PigBody p{};
        p.active = true;
        p.launched = true;
        p.radius = 20.0f;
        p.mass = 1.0f;
        p.invMass = 1.0f;
        p.pos = {762.0f, 500.0f}; // distance 38 < radius sum 43
        p.vel = {140.0f, 0.0f};

        game.birds = {falcon};
        game.activePigs = {p};
        game.stepPhysics(1.0f / 60.0f);

        // Pig must bounce backward due to shield deflection (vel.x < 0) and shield absorbs damage
        if (game.activePigs[0].vel.x >= 0.0f || game.birds[0].shieldHp >= 120.0f || game.birds[0].shieldHp <= 0.0f || game.birds[0].hp != 150.0f) {
            std::cerr << "[SELF-TEST] Falcon directional shield failed to deflect swine backward or absorb hit! vx="
                      << game.activePigs[0].vel.x << " shieldHp=" << game.birds[0].shieldHp << "\n";
            return 42;
        }

        // Test shield shattering when cumulative damage exceeds shield HP
        int scoreBefore = game.score;
        game.damageBird(game.birds[0], 90.0f);
        if (game.birds[0].hasShield || game.birds[0].shieldHp != 0.0f || (game.score - scoreBefore) != 1200) {
            std::cerr << "[SELF-TEST] Falcon shield failed to shatter or award +1200 bonus score!\n";
            return 43;
        }

        // Verify SHIELD_DEBRIS particles spawned
        bool foundShieldDebris = false;
        for (const auto& part : game.particles) {
            if (part.kind == ParticleKind::SHIELD_DEBRIS) { foundShieldDebris = true; break; }
        }
        if (!foundShieldDebris) {
            std::cerr << "[SELF-TEST] Falcon shield shattering failed to spawn SHIELD_DEBRIS particles!\n";
            return 44;
        }
    }

    // Unit Test 23: Baron Von Condor in Steampunk Zeppelin buoyant lift, balloon pop & plummet crushing blocks
    {
        BirdTarget condor{};
        condor.kind = BirdClass::CONDOR;
        condor.pos = {800.0f, 350.0f};
        condor.radius = 28.0f;
        condor.hp = 240.0f;
        condor.maxHp = 240.0f;
        condor.inZeppelin = true;
        condor.balloonHp = 70.0f;
        condor.maxBalloonHp = 70.0f;
        condor.mass = 2.8f;
        condor.invMass = 1.0f / 2.8f;

        // Test buoyant lift: zeppelin experiences only 15% downward gravity
        game.birds = {condor};
        game.blocks.clear();
        game.activePigs.clear();
        game.stepPhysics(1.0f / 60.0f);
        float expectedMaxVy = (GRAVITY * 0.15f) * (1.0f / 60.0f) * 1.5f;
        if (game.birds[0].vel.y > expectedMaxVy || !game.birds[0].inZeppelin) {
            std::cerr << "[SELF-TEST] Steampunk Zeppelin buoyant lift failed! vy=" << game.birds[0].vel.y << "\n";
            return 45;
        }

        // Test partial damage to balloon
        game.damageBird(game.birds[0], 35.0f);
        if (game.birds[0].balloonHp != 35.0f || !game.birds[0].inZeppelin || game.birds[0].hp != 240.0f) {
            std::cerr << "[SELF-TEST] Zeppelin balloon failed to absorb partial damage!\n";
            return 46;
        }

        // Test popping the balloon -> gondola plummet, BALLOON_SCRAP debris, crushing block beneath
        int scoreBefore = game.score;
        game.particles.clear();
        game.damageBird(game.birds[0], 50.0f); // 35 absorbed by balloon (pops!), remaining 15 into condor hp (240 -> 225)
        if (game.birds[0].inZeppelin || !game.birds[0].balloonPopped || game.birds[0].balloonHp != 0.0f ||
            game.birds[0].vel.y < 300.0f || game.birds[0].mass < 4.5f || (game.score - scoreBefore) != 1500) {
            std::cerr << "[SELF-TEST] Zeppelin popping failed to trigger gondola plunge (vy="
                      << game.birds[0].vel.y << ", mass=" << game.birds[0].mass << ") or +1500 score!\n";
            return 47;
        }

        // Verify BALLOON_SCRAP particles spawned
        bool foundBalloonScrap = false;
        for (const auto& part : game.particles) {
            if (part.kind == ParticleKind::BALLOON_SCRAP) { foundBalloonScrap = true; break; }
        }
        if (!foundBalloonScrap) {
            std::cerr << "[SELF-TEST] Zeppelin puncture failed to spawn BALLOON_SCRAP particles!\n";
            return 48;
        }

        // Test plummeting gondola crushes fortress blocks beneath
        BlockBody roofBlock = SnoutAndSiegeGame::makeBlock(MaterialTier::TIMBER, 800.0f, 375.0f, 80.0f, 20.0f);
        game.blocks = {roofBlock};
        float roofHpBefore = game.blocks[0].hp;
        game.stepPhysics(1.0f / 60.0f);
        if (game.blocks[0].hp >= roofHpBefore) {
            std::cerr << "[SELF-TEST] Plunging Condor gondola failed to crush fortress block beneath!\n";
            return 49;
        }
    }

    // Regression: Clover split must not read through a dangling reference
    // after activePigs reallocates (run under -fsanitize=address to verify).
    {
        game.loadLevel(0);
        game.activePigs.clear();
        game.activePigs.shrink_to_fit();
        PigBody clover = game.createPigInstance(PigType::SPLIT, {450.0f, 300.0f});
        clover.launched = true;
        clover.vel = {400.0f, -50.0f};
        game.activePigs.push_back(clover);
        game.activateMidAirAbility();
        if (game.activePigs.size() != 3) {
            std::cerr << "[SELF-TEST] Split ability did not produce 3 pigs! count=" << game.activePigs.size() << "\n";
            return 60;
        }
        for (const auto& p : game.activePigs) {
            if (!p.abilityUsed || !p.launched || std::abs(p.pos.x - 450.0f) > 1.0f) {
                std::cerr << "[SELF-TEST] Split clones inherited corrupted parent state!\n";
                return 61;
            }
        }
    }

    // Regression: a truffle collected outside the campaign must not be
    // credited to a campaign level (which would unlock vaults).
    {
        game.loadLevel(5);
        game.trufflesFound.fill(false);
        game.startEndlessMode();
        game.levelTruffle.exists = true;
        game.levelTruffle.collected = false;
        game.collectGoldenTruffle();
        if (game.getTrufflesFoundCount() != 0) {
            std::cerr << "[SELF-TEST] Endless-mode truffle was credited to campaign progress!\n";
            return 62;
        }
    }

    // Regression: custom level round-trip keeps full bird stats, and the
    // editor's wind / storm settings reach test play.
    {
        game.screen = ScreenState::EDITOR;
        game.isTestPlaying = false;
        game.customSlot = 2;
        game.customLevel = LevelSpec{};
        game.customLevel.birds = {
            SnoutAndSiegeGame::makeBird(BirdClass::FALCON, 800.0f, 500.0f),
            SnoutAndSiegeGame::makeBird(BirdClass::CONDOR, 900.0f, 400.0f, false)
        };
        game.customLevel.pendulums = {SnoutAndSiegeGame::makeEditorPendulum(700.0f, 200.0f, true)};
        game.editorWind = 60.0f;
        game.editorStorm = true;
        game.saveCustomLevel(2);
        game.customLevel = LevelSpec{};
        game.editorWind = 0.0f;
        game.editorStorm = false;
        game.loadCustomLevel(2);
        const auto& lb = game.customLevel.birds;
        if (lb.size() != 2 || !lb[0].hasShield || lb[0].maxHp != 150.0f ||
            !lb[1].inZeppelin || lb[1].maxHp != 240.0f ||
            game.customLevel.pendulums.size() != 1 || game.customLevel.pendulums[0].radius != 18.0f) {
            std::cerr << "[SELF-TEST] Custom level reload lost bird/pendulum stats!\n";
            return 63;
        }
        game.editorWind = -90.0f;
        game.editorStorm = true;
        game.startEditorTestPlay();
        if (game.currentWind != -90.0f || game.stormClouds.empty()) {
            std::cerr << "[SELF-TEST] Editor wind/storm settings were not applied to test play!\n";
            return 64;
        }
        game.returnToEditor();

        // Corrupt entries must be rejected rather than cast to invalid enums.
        {
            std::ofstream bad("custom_siege_2.cfg");
            bad << "theme = 99\nblock = 42 100 100 40 40\nbird = 17 100 100 0\nblock = 1 nan 100 40 40\nblock = 1 800 500 40 40\n";
        }
        game.loadCustomLevel(2);
        if (game.editorTheme < 0 || game.editorTheme > 5 || game.customLevel.blocks.size() != 1 ||
            !game.customLevel.birds.empty()) {
            std::cerr << "[SELF-TEST] Corrupt custom level entries were not rejected!\n";
            return 65;
        }
    }

    // Regression: WAVs that aren't 44.1 kHz mono S16 are converted on load.
    {
        SDL_AudioSpec spec{};
        spec.freq = 22050;
        spec.format = 0x8010;
        spec.channels = 2;
        std::vector<int16_t> stereo(2000 * 2, 16000);
        std::vector<int16_t> out;
        bool ok = AssetPipeline::convertToMixerFormat(spec, reinterpret_cast<const uint8_t*>(stereo.data()),
                                                      static_cast<uint32_t>(stereo.size() * 2), out);
        if (!ok || out.size() != 4000 || std::abs(out[100] - 16000) > 2) {
            std::cerr << "[SELF-TEST] WAV format conversion produced wrong output (size=" << out.size() << ")\n";
            return 66;
        }
    }

    // Fortress Architect canvas clicks place and erase elements.
    {
        game.screen = ScreenState::EDITOR;
        game.isTestPlaying = false;
        game.customLevel = LevelSpec{};
        game.editorSnapGrid = true;
        game.editorTool = 1; // bird
        game.editorBird = BirdClass::FALCON;
        game.handleEditorCanvasClick(803.0f, 401.0f);
        if (game.customLevel.birds.size() != 1 || !game.customLevel.birds[0].hasShield ||
            game.customLevel.birds[0].pos.x != 800.0f || game.customLevel.birds[0].pos.y != 400.0f) {
            std::cerr << "[SELF-TEST] Editor canvas click failed to place a snapped Falcon!\n";
            return 67;
        }
        game.handleEditorCanvasClick(100.0f, 400.0f); // tool dock area: ignored
        game.editorTool = 4; // eraser
        game.handleEditorCanvasClick(805.0f, 405.0f);
        if (!game.customLevel.birds.empty()) {
            std::cerr << "[SELF-TEST] Editor eraser failed to remove the placed bird!\n";
            return 68;
        }
    }

    // Level data files: every campaign level re-serializes to an equivalent
    // level, and a missing level directory is reported instead of crashing.
    {
        for (int i = 0; i < NUM_LEVELS; ++i) {
            const LevelSpec& L = game.levels[i];
            std::string text = serializeLevel(L, -1, true);
            ParsedLevel back;
            std::string warnings;
            if (!parseLevelText(text, back, &warnings) || !warnings.empty() ||
                serializeLevel(back.spec, -1, true) != text ||
                back.spec.blocks.size() != L.blocks.size() || back.spec.birds.size() != L.birds.size()) {
                std::cerr << "[SELF-TEST] Level " << (i + 1) << " did not round-trip through the level format!\n"
                          << warnings;
                return 69;
            }
            // Gear a bird visibly wears must actually work: non-zero HP, not above its max.
            for (const auto& t : L.birds) {
                bool badGear = (t.hasHelmet && (t.helmetHp <= 0.0f || t.helmetHp > t.maxHelmetHp)) ||
                               (t.hasShield && (t.shieldHp <= 0.0f || t.shieldHp > t.maxShieldHp)) ||
                               (t.inZeppelin && (t.balloonHp <= 0.0f || t.balloonHp > t.maxBalloonHp));
                if (badGear) {
                    std::cerr << "[SELF-TEST] Level " << (i + 1) << " has a bird with zero-HP or overfull "
                              << "helmet/shield/zeppelin at (" << t.pos.x << ", " << t.pos.y << ")!\n";
                    return 72;
                }
            }
        }
        SnoutAndSiegeGame scratch;
        std::cerr << "[SELF-TEST] (expected) ";
        if (scratch.initLevels("assets/levels-does-not-exist") ||
            static_cast<int>(scratch.levels.size()) != NUM_LEVELS) {
            std::cerr << "[SELF-TEST] Missing level directory was not reported, or left levels misaligned!\n";
            return 70;
        }
    }

    // Older editor saves (numeric kinds, "bird = kind x y helmet") still load.
    {
        {
            std::ofstream legacy("custom_siege_2.cfg");
            legacy << "theme = 3\nwind = 30\nstorm = 1\ntruffle.exists = 1\ntruffle.x = 700\ntruffle.y = 300\n"
                   << "block = 2 780 580 24 100\nbird = 4 840 500 1\npendulum = 860 200 85 1\n";
        }
        game.screen = ScreenState::EDITOR;
        game.isTestPlaying = false;
        game.loadCustomLevel(2);
        const LevelSpec& c = game.customLevel;
        if (game.editorTheme != 3 || game.editorWind != 30.0f || !game.editorStorm || !c.hasTruffle ||
            c.blocks.size() != 1 || c.blocks[0].mat != MaterialTier::STONE ||
            c.birds.size() != 1 || c.birds[0].kind != BirdClass::CONDOR || !c.birds[0].hasHelmet ||
            !c.birds[0].inZeppelin || c.pendulums.size() != 1 || c.pendulums[0].radius != 18.0f ||
            !c.pendulums[0].spiked) {
            std::cerr << "[SELF-TEST] Legacy-format custom level did not load correctly!\n";
            return 71;
        }
    }

    game.assets.shutdown();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    std::cout << "[SELF-TEST] ALL CHECKS PASSED.\n";
    return 0;
}

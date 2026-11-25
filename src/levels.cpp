#include "game.h"

void SnoutAndSiegeGame::initLevels() {
    levels.clear();
    levels.reserve(NUM_LEVELS);


    auto birdRad = [](BirdClass k) -> float {
        return (k == BirdClass::CONDOR) ? 28.0f :
               (k == BirdClass::HERON ? 26.0f :
               (k == BirdClass::FALCON ? 23.0f :
               (k == BirdClass::OWL ? 23.0f : 20.0f)));
    };

    // Helper to add a rock-solid modular 1-, 2-, or 3-story architectural bay centered at cx
    // Floor 1 top = 508.0, Floor 2 top = 404.0, Floor 3 top = 314.0
    auto addModularBay = [&](LevelSpec& L, float cx, int stories,
                             MaterialTier m1Pillar, MaterialTier m1Deck,
                             MaterialTier m2Pillar, MaterialTier m2Deck,
                             MaterialTier m3Pillar, MaterialTier m3Deck,
                             int birdFloor1, BirdClass b1Kind,
                             int birdFloor2, BirdClass b2Kind,
                             int birdFloor3, BirdClass b3Kind) {
        if (stories >= 1) {
            L.blocks.push_back(makeBlock(m1Pillar, cx - 42.0f, 580.0f, 24.0f, 100.0f));
            L.blocks.push_back(makeBlock(m1Pillar, cx + 42.0f, 580.0f, 24.0f, 100.0f));
            L.blocks.push_back(makeBlock(m1Deck,   cx,         519.0f, 112.0f, 22.0f));
            if (birdFloor1 > 0) {
                L.birds.push_back(makeBird(b1Kind, cx, GROUND_Y - birdRad(b1Kind)));
            }
        }
        if (stories >= 2) {
            L.blocks.push_back(makeBlock(m2Pillar, cx - 36.0f, 466.0f, 22.0f, 84.0f));
            L.blocks.push_back(makeBlock(m2Pillar, cx + 36.0f, 466.0f, 22.0f, 84.0f));
            L.blocks.push_back(makeBlock(m2Deck,   cx,         414.0f, 102.0f, 20.0f));
            if (birdFloor2 > 0) {
                L.birds.push_back(makeBird(b2Kind, cx, 508.0f - birdRad(b2Kind)));
            }
        } else if (birdFloor2 > 0) {
            L.birds.push_back(makeBird(b2Kind, cx, 508.0f - birdRad(b2Kind)));
        }
        if (stories >= 3) {
            L.blocks.push_back(makeBlock(m3Pillar, cx - 31.0f, 368.0f, 20.0f, 72.0f));
            L.blocks.push_back(makeBlock(m3Pillar, cx + 31.0f, 368.0f, 20.0f, 72.0f));
            L.blocks.push_back(makeBlock(m3Deck,   cx,         323.0f, 92.0f, 18.0f));
            if (birdFloor3 > 0) {
                L.birds.push_back(makeBird(b3Kind, cx, 404.0f - birdRad(b3Kind)));
            }
        } else if (stories == 2 && birdFloor3 > 0) {
            L.birds.push_back(makeBird(b3Kind, cx, 404.0f - birdRad(b3Kind)));
        }
    };

    auto addFrontShield = [&](LevelSpec& L, float x, MaterialTier mat, bool tall) {
        if (tall) {
            L.blocks.push_back(makeBlock(mat, x, 570.0f, 28.0f, 120.0f));
        } else {
            L.blocks.push_back(makeBlock(mat, x, 585.0f, 26.0f, 90.0f));
        }
    };

    // Helper to add a massive Megalithic Bay with wide stone foundation plinths and grand beam
    auto addMonolithBay = [&](LevelSpec& L, float cx, int stories,
                              MaterialTier mPlinth, MaterialTier mPillar, MaterialTier mBeam,
                              int birdFloor1, BirdClass b1Kind,
                              int birdFloor2, BirdClass b2Kind,
                              int birdFloor3, BirdClass b3Kind) {
        // Foundation plinths: w = 44.0, h = 40.0. bottom = 630.0, top = 590.0, pos.y = 610.0
        L.blocks.push_back(makeBlock(mPlinth, cx - 52.0f, 610.0f, 44.0f, 40.0f));
        L.blocks.push_back(makeBlock(mPlinth, cx + 52.0f, 610.0f, 44.0f, 40.0f));
        // Monolith pillars: w = 32.0, h = 90.0. bottom = 590.0, top = 500.0, pos.y = 545.0
        L.blocks.push_back(makeBlock(mPillar, cx - 50.0f, 545.0f, 32.0f, 90.0f));
        L.blocks.push_back(makeBlock(mPillar, cx + 50.0f, 545.0f, 32.0f, 90.0f));
        // Grand monolithic beam: w = 160.0, h = 28.0. bottom = 500.0, top = 472.0, pos.y = 486.0
        L.blocks.push_back(makeBlock(mBeam, cx, 486.0f, 160.0f, 28.0f));
        if (birdFloor1 > 0) {
            L.birds.push_back(makeBird(b1Kind, cx, GROUND_Y - birdRad(b1Kind)));
        }
        if (stories >= 2) {
            // Tier 2 stanchions: w = 26.0, h = 60.0. bottom = 472.0, top = 412.0, pos.y = 442.0
            L.blocks.push_back(makeBlock(mPillar, cx - 42.0f, 442.0f, 26.0f, 60.0f));
            L.blocks.push_back(makeBlock(mPillar, cx + 42.0f, 442.0f, 26.0f, 60.0f));
            // Upper plinth slab: w = 120.0, h = 24.0. bottom = 412.0, top = 388.0, pos.y = 400.0
            L.blocks.push_back(makeBlock(mBeam, cx, 400.0f, 120.0f, 24.0f));
            if (birdFloor2 > 0) {
                L.birds.push_back(makeBird(b2Kind, cx, 472.0f - birdRad(b2Kind)));
            }
            if (birdFloor3 > 0) {
                L.birds.push_back(makeBird(b3Kind, cx, 388.0f - birdRad(b3Kind)));
            }
        } else {
            if (birdFloor2 > 0) {
                L.birds.push_back(makeBird(b2Kind, cx, 472.0f - birdRad(b2Kind)));
            }
        }
    };

    // Helper to add an Eastern Tiered Pagoda with flared eaves and bamboo posts
    auto addPagodaBay = [&](LevelSpec& L, float cx, int stories,
                            MaterialTier mPillar, MaterialTier mEaves,
                            int birdFloor1, BirdClass b1Kind,
                            int birdFloor2, BirdClass b2Kind,
                            int birdFloor3, BirdClass b3Kind) {
        // Tier 1 pillars: w = 20.0, h = 90.0. bottom = 630.0, top = 540.0, pos.y = 585.0
        L.blocks.push_back(makeBlock(mPillar, cx - 46.0f, 585.0f, 20.0f, 90.0f));
        L.blocks.push_back(makeBlock(mPillar, cx + 46.0f, 585.0f, 20.0f, 90.0f));
        // First flared eaves: w = 144.0, h = 18.0. bottom = 540.0, top = 522.0, pos.y = 531.0
        L.blocks.push_back(makeBlock(mEaves, cx, 531.0f, 144.0f, 18.0f));
        if (birdFloor1 > 0) {
            L.birds.push_back(makeBird(b1Kind, cx, GROUND_Y - birdRad(b1Kind)));
        }
        if (stories >= 2) {
            // Tier 2 pillars: w = 18.0, h = 76.0. bottom = 522.0, top = 446.0, pos.y = 484.0
            L.blocks.push_back(makeBlock(mPillar, cx - 40.0f, 484.0f, 18.0f, 76.0f));
            L.blocks.push_back(makeBlock(mPillar, cx + 40.0f, 484.0f, 18.0f, 76.0f));
            // Second flared eaves: w = 120.0, h = 16.0. bottom = 446.0, top = 430.0, pos.y = 438.0
            L.blocks.push_back(makeBlock(mEaves, cx, 438.0f, 120.0f, 16.0f));
            if (birdFloor2 > 0) {
                L.birds.push_back(makeBird(b2Kind, cx, 522.0f - birdRad(b2Kind)));
            }
        } else if (birdFloor2 > 0) {
            L.birds.push_back(makeBird(b2Kind, cx, 522.0f - birdRad(b2Kind)));
        }
        if (stories >= 3) {
            // Tier 3 pillars: w = 16.0, h = 64.0. bottom = 430.0, top = 366.0, pos.y = 398.0
            L.blocks.push_back(makeBlock(mPillar, cx - 34.0f, 398.0f, 16.0f, 64.0f));
            L.blocks.push_back(makeBlock(mPillar, cx + 34.0f, 398.0f, 16.0f, 64.0f));
            // Top roof cap: w = 100.0, h = 16.0. bottom = 366.0, top = 350.0, pos.y = 358.0
            L.blocks.push_back(makeBlock(mEaves, cx, 358.0f, 100.0f, 16.0f));
            if (birdFloor3 > 0) {
                L.birds.push_back(makeBird(b3Kind, cx, 430.0f - birdRad(b3Kind)));
            }
        } else if (stories == 2 && birdFloor3 > 0) {
            L.birds.push_back(makeBird(b3Kind, cx, 430.0f - birdRad(b3Kind)));
        }
    };

    // Helper to add a Stepped Pyramid / Ziggurat structure
    auto addPyramidBay = [&](LevelSpec& L, float cx, int tiers,
                             MaterialTier mBase, MaterialTier mMid, MaterialTier mTop,
                             BirdClass bTop) {
        // Tier 1: w = 140.0, h = 28.0. bottom = 630.0, top = 602.0, pos.y = 616.0
        L.blocks.push_back(makeBlock(mBase, cx, 616.0f, 140.0f, 28.0f));
        if (tiers >= 2) {
            // Tier 2: w = 100.0, h = 24.0. bottom = 602.0, top = 578.0, pos.y = 590.0
            L.blocks.push_back(makeBlock(mMid, cx, 590.0f, 100.0f, 24.0f));
        }
        if (tiers >= 3) {
            // Tier 3: w = 60.0, h = 22.0. bottom = 578.0, top = 556.0, pos.y = 567.0
            L.blocks.push_back(makeBlock(mTop, cx, 567.0f, 60.0f, 22.0f));
            L.birds.push_back(makeBird(bTop, cx, 556.0f - birdRad(bTop)));
        } else if (tiers == 2) {
            L.birds.push_back(makeBird(bTop, cx, 578.0f - birdRad(bTop)));
        } else {
            L.birds.push_back(makeBird(bTop, cx, 602.0f - birdRad(bTop)));
        }
    };

    // Helper to add a Heavy Bomb-Proof Casemate bunker with thick walls and parapets
    auto addFortifiedCasemate = [&](LevelSpec& L, float cx,
                                    MaterialTier mWall, MaterialTier mRoof, MaterialTier mCrate,
                                    int bInside, BirdClass bIKind,
                                    int bRoof, BirdClass bRKind) {
        // Walls: w = 36.0, h = 100.0. bottom = 630.0, top = 530.0, pos.y = 580.0
        L.blocks.push_back(makeBlock(mWall, cx - 60.0f, 580.0f, 36.0f, 100.0f));
        L.blocks.push_back(makeBlock(mWall, cx + 60.0f, 580.0f, 36.0f, 100.0f));
        // Center crate: w = 42.0, h = 32.0. bottom = 630.0, top = 598.0, pos.y = 614.0
        L.blocks.push_back(makeBlock(mCrate, cx, 614.0f, 42.0f, 32.0f));
        // Heavy roof slab: w = 170.0, h = 26.0. bottom = 530.0, top = 504.0, pos.y = 517.0
        L.blocks.push_back(makeBlock(mRoof, cx, 517.0f, 170.0f, 26.0f));
        // Parapets: w = 20.0, h = 28.0. bottom = 504.0, top = 476.0, pos.y = 490.0
        L.blocks.push_back(makeBlock(mWall, cx - 72.0f, 490.0f, 20.0f, 28.0f));
        L.blocks.push_back(makeBlock(mWall, cx + 72.0f, 490.0f, 20.0f, 28.0f));
        if (bInside > 0) {
            L.birds.push_back(makeBird(bIKind, cx, 598.0f - birdRad(bIKind)));
        }
        if (bRoof > 0) {
            L.birds.push_back(makeBird(bRKind, cx, 504.0f - birdRad(bRKind)));
        }
    };

    // Helper to add an Industrial Gantry structure with crane girders and optional pendulum
    auto addClockworkGantry = [&](LevelSpec& L, float cx,
                                  MaterialTier mPillar, MaterialTier mBeam,
                                  bool withPendulum, BirdClass bRoof) {
        // Stanchions: w = 28.0, h = 110.0. bottom = 630.0, top = 520.0, pos.y = 575.0
        L.blocks.push_back(makeBlock(mPillar, cx - 56.0f, 575.0f, 28.0f, 110.0f));
        L.blocks.push_back(makeBlock(mPillar, cx + 56.0f, 575.0f, 28.0f, 110.0f));
        // Girder deck: w = 170.0, h = 24.0. bottom = 520.0, top = 496.0, pos.y = 508.0
        L.blocks.push_back(makeBlock(mBeam, cx, 508.0f, 170.0f, 24.0f));
        // Upper stanchions: w = 22.0, h = 80.0. bottom = 496.0, top = 416.0, pos.y = 456.0
        L.blocks.push_back(makeBlock(mPillar, cx - 42.0f, 456.0f, 22.0f, 80.0f));
        L.blocks.push_back(makeBlock(mPillar, cx + 42.0f, 456.0f, 22.0f, 80.0f));
        // Top crane beam: w = 130.0, h = 20.0. bottom = 416.0, top = 396.0, pos.y = 406.0
        L.blocks.push_back(makeBlock(mBeam, cx, 406.0f, 130.0f, 20.0f));
        L.birds.push_back(makeBird(bRoof, cx, 396.0f - birdRad(bRoof)));
        if (withPendulum) {
            PendulumBody pb{};
            pb.anchor = {cx, 406.0f};
            pb.length = 150.0f;
            pb.radius = 26.0f;
            pb.mass = 10.0f;
            pb.spiked = true;
            L.pendulums.push_back(pb);
        }
    };

    // ====================================================================
    // CHAPTER I (LEVELS 1-10): DAWN MEADOW & THE WICKER OUTPOSTS
    // Levels 1-5: STANDARD PIG (TROTTER) ONLY
    // Levels 6-10: BRAMBLE (TURBO BOAR) UNLOCKED AT LEVEL 6
    // ====================================================================
    // LEVEL 1: Wicker Watchtower
    {
        LevelSpec L;
        L.name = "LEVEL 1: WICKER WATCHTOWER";
        L.subtitle = "Pull the catapult & launch the Trotter! Press SPACE mid-air for Anvil Slam!";
        L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
        addFrontShield(L, 710.0f, MaterialTier::TWIG, false);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 780, 580, 24, 100));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 900, 580, 24, 100));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 840, 519, 160, 22));
        L.blocks.push_back(makeBlock(MaterialTier::TWIG,   800, 466, 22, 84));
        L.blocks.push_back(makeBlock(MaterialTier::TWIG,   880, 466, 22, 84));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 840, 414, 130, 20));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 840, 610));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 840, 488));
        levels.push_back(L);
    }

    // LEVEL 2: Timber Bastion
    {
        LevelSpec L;
        L.name = "LEVEL 2: TIMBER BASTION";
        L.subtitle = "Standard pigs ready! Aim for the central timber pillar to trigger a collapse!";
        L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
        addFrontShield(L, 665.0f, MaterialTier::TIMBER, false);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 730, 575, 26, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850, 575, 26, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 970, 575, 26, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850, 509, 240, 22));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 790, 457, 22, 82));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 910, 457, 22, 82));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850, 406, 160, 20));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 790, 610));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 910, 610));
        L.birds.push_back(makeBird(BirdClass::OWL,    850, 475));
        levels.push_back(L);
    }

    // LEVEL 3: Twin Timber Watchtowers (REBALANCED FOR STANDARD PIGS)
    {
        LevelSpec L;
        L.name = "LEVEL 3: TWIN TIMBER WATCHTOWERS";
        L.subtitle = "Strike the outer timber supports to topple both towers into the bridge!";
        L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 715, 575, 24, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 795, 575, 24, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 755, 509, 116, 22));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 935, 575, 24, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 1015, 575, 24, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 975, 509, 116, 22));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 865, 487, 140, 22));
        L.blocks.push_back(makeBlock(MaterialTier::TWIG,   835, 434, 20, 84));
        L.blocks.push_back(makeBlock(MaterialTier::TWIG,   895, 434, 20, 84));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 865, 382, 100, 20));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 755, 610));
        L.birds.push_back(makeBird(BirdClass::OWL,    975, 607));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 865, 610));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 865, 456));
        levels.push_back(L);
    }

    // LEVEL 4: Skylight Bunker
    {
        LevelSpec L;
        L.name = "LEVEL 4: SKYLIGHT BUNKER";
        L.subtitle = "High timber bunker! Lob a high shot & use Trotter's Anvil Slam to crush the roof!";
        L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 690, 565, 36, 130));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 690, 435, 36, 130));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 800, 575, 26, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 950, 575, 26, 110));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 1040, 565, 34, 130));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 875, 509, 180, 22));
        L.blocks.push_back(makeBlock(MaterialTier::TWIG,   820, 456, 22, 84));
        L.blocks.push_back(makeBlock(MaterialTier::TWIG,   930, 456, 22, 84));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 875, 404, 150, 20));
        L.birds.push_back(makeBird(BirdClass::OWL,    748, 607));
        L.birds.push_back(makeBird(BirdClass::HERON,  875, 604));
        L.birds.push_back(makeBird(BirdClass::OWL,    875, 475));
        levels.push_back(L);
    }

    // LEVEL 5: Bamboo Colonnade
    {
        LevelSpec L;
        L.name = "LEVEL 5: BAMBOO COLONNADE";
        L.subtitle = "Chapter 1 Milestone! Demolish the colonnade to unlock Bramble the Turbo Boar!";
        L.pigQueue = {PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
        addModularBay(L, 715, 2, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 2, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 995, 2, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 6: Stepped Meadow Gazebo (BRAMBLE UNLOCKED!)
    {
        LevelSpec L;
        L.name = "LEVEL 6: STEPPED MEADOW GAZEBO";
        L.subtitle = "[UNLOCKED] BRAMBLE! Press SPACE mid-air for supersonic dash through wood!";
        L.pigQueue = {PigType::TURBO, PigType::TURBO, PigType::TROTTER, PigType::TROTTER};
        addModularBay(L, 735, 1, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 875, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1015, 1, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 7: The Powder Viaduct
    {
        LevelSpec L;
        L.name = "LEVEL 7: THE POWDER VIADUCT";
        L.subtitle = "Target the central volatile TNT keg between the towers to trigger a massive collapse!";
        L.pigQueue = {PigType::TURBO, PigType::TROTTER, PigType::TURBO, PigType::TROTTER};
        addFrontShield(L, 660.0f, MaterialTier::TWIG, false);
        addModularBay(L, 750, 2, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 930, 2, MaterialTier::TWIG, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 840.0f, 616.0f, 28.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 8: The Bouncing Keep
    {
        LevelSpec L;
        L.name = "LEVEL 8: THE BOUNCING KEEP";
        L.subtitle = "Bounce off the woven ground trampoline to catapult pigs over the forward barricade!";
        L.pigQueue = {PigType::TURBO, PigType::TROTTER, PigType::TURBO, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 590.0f, 618.0f, 100.0f, 24.0f));
        addFrontShield(L, 680.0f, MaterialTier::TIMBER, true);
        addModularBay(L, 770, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 950, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 9: Meadow Redoubt
    {
        LevelSpec L;
        L.name = "LEVEL 9: MEADOW REDOUBT";
        L.subtitle = "Charge through the timber barricades to reach the aviary!";
        L.pigQueue = {PigType::TURBO, PigType::TURBO, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
        addFrontShield(L, 620.0f, MaterialTier::TIMBER, true);
        addModularBay(L, 715, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 995, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 10: Grand Aviary Citadel (Chapter 1 Boss)
    {
        LevelSpec L;
        L.name = "LEVEL 10: GRAND AVIARY CITADEL";
        L.subtitle = "Chapter 1 Finale! Defeat Archduke Heron to unlock Clover the Tri-Piglet!";
        L.pigQueue = {PigType::TURBO, PigType::TURBO, PigType::TROTTER, PigType::TROTTER, PigType::TROTTER};
        addFrontShield(L, 625.0f, MaterialTier::TIMBER, true);
        addModularBay(L, 730, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 865, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL,    1, BirdClass::TOUCAN);
        addModularBay(L, 1000, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER II (LEVELS 11-20): AUTUMN WOODLAND PALISADES
    // Levels 11-20: CLOVER (TRI-PIGLET) UNLOCKED AT LEVEL 11
    // Available: TROTTER, TURBO, SPLIT (NO BOMB)
    // ====================================================================
    // LEVEL 11: Reed Colonnade (CLOVER UNLOCKED!)
    {
        LevelSpec L;
        L.name = "LEVEL 11: REED COLONNADE";
        L.subtitle = "[UNLOCKED] CLOVER! Press SPACE mid-air to split into a 3-way squadron!";
        L.pigQueue = {PigType::SPLIT, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
        addFrontShield(L, 635.0f, MaterialTier::TIMBER, false);
        addModularBay(L, 720, 2, MaterialTier::TWIG,   MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 990, 2, MaterialTier::TWIG,   MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 12: Cauldron Barracks
    {
        LevelSpec L;
        L.name = "LEVEL 12: CAULDRON BARRACKS";
        L.subtitle = "Beware iron-kettle helmets! Avian sentries withstand heavy hits before their armor shatters!";
        L.pigQueue = {PigType::TROTTER, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addModularBay(L, 715, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::OWL, 1, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::OWL, 855, 508.0f - birdRad(BirdClass::OWL), true));
        addModularBay(L, 995, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 995, GROUND_Y - birdRad(BirdClass::TOUCAN), true));
        levels.push_back(L);
    }

    // LEVEL 13: Walnut Stockade
    {
        LevelSpec L;
        L.name = "LEVEL 13: WALNUT STOCKADE";
        L.subtitle = "Dense walnut beams resist blunt impacts. Use Bramble's supersonic dash!";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
        addFrontShield(L, 645.0f, MaterialTier::TIMBER, true);
        addModularBay(L, 745, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 915, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 14: Crossbeam Stockade
    {
        LevelSpec L;
        L.name = "LEVEL 14: CROSSBEAM STOCKADE";
        L.subtitle = "Gusting crosswinds resist your siege! Adjust trajectory to compensate for headwind.";
        L.pigQueue = {PigType::SPLIT, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        L.wind = -65.0f;
        addFrontShield(L, 620.0f, MaterialTier::TIMBER, true);
        addModularBay(L, 710, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 845, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 985, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 15: Double Gable Powder Keep
    {
        LevelSpec L;
        L.name = "LEVEL 15: DOUBLE GABLE POWDER KEEP";
        L.subtitle = "Detonate the central TNT cache between the keeps to trigger a dual collapse!";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
        addFrontShield(L, 675.0f, MaterialTier::TIMBER, false);
        addModularBay(L, 765, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 935, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 850.0f, 616.0f, 28.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 16: Canopy Skyway
    {
        LevelSpec L;
        L.name = "LEVEL 16: CANOPY SKYWAY";
        L.subtitle = "A long suspended timber platform guarded by Commander Falcon's directional tower shield!";
        L.pigQueue = {PigType::DRILL, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
        addModularBay(L, 700, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 840, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 980, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 17: Foxfire Barricade
    {
        LevelSpec L;
        L.name = "LEVEL 17: FOXFIRE BARRICADE";
        L.subtitle = "Front timber baffles shelter nested owls. Strike the base with Anvil Slam!";
        L.pigQueue = {PigType::TROTTER, PigType::SPLIT, PigType::TURBO, PigType::TROTTER};
        addFrontShield(L, 640.0f, MaterialTier::TIMBER, true);
        addFrontShield(L, 675.0f, MaterialTier::TIMBER, false);
        addModularBay(L, 775, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 935, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 18: Pendulum Citadel
    {
        LevelSpec L;
        L.name = "LEVEL 18: PENDULUM CITADEL";
        L.subtitle = "Strike the massive iron wrecking ball to swing it through the central timber gallery!";
        L.pigQueue = {PigType::SPLIT, PigType::TURBO, PigType::TROTTER, PigType::SPLIT, PigType::TROTTER};
        addModularBay(L, 720, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1000, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 860, 580, 26, 100));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 860, 519, 100, 22));
        L.birds.push_back(makeBird(BirdClass::HERON, 860, 508.0f - birdRad(BirdClass::HERON)));
        PendulumBody pb{};
        pb.anchor = {860.0f, 260.0f};
        pb.length = 160.0f;
        pb.angle = 0.0f;
        pb.angVel = 0.0f;
        pb.radius = 26.0f;
        pb.mass = 8.0f;
        pb.spiked = false;
        L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 19: Warden's Grove
    {
        LevelSpec L;
        L.name = "LEVEL 19: WARDEN'S GROVE";
        L.subtitle = "Three reinforced keeps spanning the grove under heavy guard.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
        addFrontShield(L, 625.0f, MaterialTier::TIMBER, true);
        addModularBay(L, 715, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 995, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 20: Palisade Stronghold (Chapter 2 Boss)
    {
        LevelSpec L;
        L.name = "LEVEL 20: PALISADE STRONGHOLD";
        L.subtitle = "Chapter 2 Finale! Defeat Archduke Heron to unlock Vulcan the Seismic Bomb Hog!";
        L.pigQueue = {PigType::SPLIT, PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
        addFrontShield(L, 630.0f, MaterialTier::TIMBER, true);
        addModularBay(L, 720, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 860, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL,    1, BirdClass::TOUCAN);
        addModularBay(L, 1000, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER III (LEVELS 21-30): BASALT CANYON & RED MESA VIADUCTS
    // Levels 21-50: VULCAN (SEISMIC BOMB) UNLOCKED AT LEVEL 21
    // Available: ALL 4 PIG CLASSES (TROTTER, TURBO, SPLIT, BOMB)
    // ====================================================================
    // LEVEL 21: Canyon Viaduct (VULCAN UNLOCKED!)
    {
        LevelSpec L;
        L.name = "LEVEL 21: CANYON VIADUCT";
        L.subtitle = "[UNLOCKED] VULCAN! Press SPACE or CLICK to detonate seismic blast through basalt!";
        L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO};
        addFrontShield(L, 670.0f, MaterialTier::STONE, true);
        addModularBay(L, 765, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 935, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 22: Tri-Pillar Sanctum
    {
        LevelSpec L;
        L.name = "LEVEL 22: TRI-PILLAR SANCTUM";
        L.subtitle = "Fierce canyon tailwinds propel airborne swine with tremendous velocity!";
        L.pigQueue = {PigType::BOMB, PigType::SPLIT, PigType::TROTTER};
        L.wind = 110.0f;
        addModularBay(L, 705, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 850, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 995, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 23: Ironwood Abbey
    {
        LevelSpec L;
        L.name = "LEVEL 23: IRONWOOD ABBEY";
        L.subtitle = "Twin 3-story bell towers flank the central stone sanctuary!";
        L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TROTTER, PigType::TURBO};
        addFrontShield(L, 625.0f, MaterialTier::STONE, false);
        addModularBay(L, 715, 3, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 860, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1005, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 1, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 24: Basalt Powder Bunkers
    {
        LevelSpec L;
        L.name = "LEVEL 24: BASALT POWDER BUNKERS";
        L.subtitle = "Target the volatile TNT cache at the base of the central basalt wall!";
        L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::BOMB, PigType::TURBO};
        addFrontShield(L, 635.0f, MaterialTier::STONE, true);
        addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addFrontShield(L, 855.0f, MaterialTier::STONE, true);
        addModularBay(L, 985, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 616.0f, 28.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 25: Slate Casemate
    {
        LevelSpec L;
        L.name = "LEVEL 25: SLATE CASEMATE";
        L.subtitle = "Blast through the outer stone casemates or drop an Anvil Slam from above!";
        L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
        addFrontShield(L, 630.0f, MaterialTier::STONE, true);
        addModularBay(L, 720, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 860, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1000, 2, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 26: The Trampoline Redoubt
    {
        LevelSpec L;
        L.name = "LEVEL 26: THE TRAMPOLINE REDOUBT";
        L.subtitle = "Twin elastic trampolines! Bank supersonic ricochets into the inner basalt towers!";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 480.0f, 618.0f, 80.0f, 24.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 575.0f, 618.0f, 80.0f, 24.0f));
        addModularBay(L, 680, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 840, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 980, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 27: Pillar of Basalt
    {
        LevelSpec L;
        L.name = "LEVEL 27: PILLAR OF BASALT";
        L.subtitle = "A soaring 3-story basalt column flanked by twin support keeps.";
        L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TROTTER};
        addModularBay(L, 710, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 1000, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 28: Cavern Gate
    {
        LevelSpec L;
        L.name = "LEVEL 28: CAVERN GATE";
        L.subtitle = "Heavy front blast walls defend the cavern. Unleash Atticus the Graviton Boar to collapse their defenses!";
        L.pigQueue = {PigType::GRAVITON, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addFrontShield(L, 620.0f, MaterialTier::STONE, true);
        addFrontShield(L, 655.0f, MaterialTier::STONE, false);
        addModularBay(L, 750, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 910, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 29: Gorge Bulwark
    {
        LevelSpec L;
        L.name = "LEVEL 29: GORGE BULWARK";
        L.subtitle = "Three interlocked stone keeps defending the canyon pass.";
        L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::BOMB};
        addFrontShield(L, 615.0f, MaterialTier::STONE, true);
        addModularBay(L, 705, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 845, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 985, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 30: The High Canyon Perch (Chapter 3 Boss)
    {
        LevelSpec L;
        L.name = "LEVEL 30: THE HIGH CANYON PERCH";
        L.subtitle = "A stepped pyramid fortress rising to Archduke Heron's high throne!";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::SPLIT};
        addFrontShield(L, 600.0f, MaterialTier::STONE, false);
        addModularBay(L, 685, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 815, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::OWL,    0, BirdClass::TOUCAN);
        addModularBay(L, 945, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL,    1, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER IV (LEVELS 31-40): FROSTPEAK AURORA HIGHLANDS
    // ====================================================================
    // LEVEL 31: Glacial Ziggurat
    {
        LevelSpec L;
        L.name = "LEVEL 31: GLACIAL ZIGGURAT";
        L.subtitle = "Chapter 4! Brittle Ice columns shatter instantly into crystalline shards!";
        L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addModularBay(L, 660, 2, MaterialTier::ICE,    MaterialTier::ICE,    MaterialTier::TWIG,   MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 790, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 920, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1050, 2, MaterialTier::ICE,   MaterialTier::ICE,    MaterialTier::TWIG,   MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 32: Twin Glacial Bunkers
    {
        LevelSpec L;
        L.name = "LEVEL 32: TWIN GLACIAL BUNKERS";
        L.subtitle = "Two heavily armored stone bunkers! Deploy Rusty the Drill Hog to bore straight through the granite!";
        L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TROTTER, PigType::TURBO};
        addFrontShield(L, 635.0f, MaterialTier::STONE, true);
        addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addFrontShield(L, 855.0f, MaterialTier::STONE, true);
        addModularBay(L, 985, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 33: The Monolith Gate
    {
        LevelSpec L;
        L.name = "LEVEL 33: THE MONOLITH GATE";
        L.subtitle = "A towering 3-story front stone gatehouse shields the inner court!";
        L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
        addModularBay(L, 720, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 865, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1005, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,  MaterialTier::TWIG,   MaterialTier::TWIG, 1, BirdClass::OWL,   0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 34: Crystal Conservatory
    {
        LevelSpec L;
        L.name = "LEVEL 34: CRYSTAL CONSERVATORY";
        L.subtitle = "Shatter the brittle ice conservatory spires to topple the royal roost!";
        L.pigQueue = {PigType::SPLIT, PigType::TROTTER, PigType::BOMB, PigType::TURBO};
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 665, 565, 36, 130));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 665, 435, 36, 130));
        addModularBay(L, 780, 3, MaterialTier::ICE,    MaterialTier::ICE,    MaterialTier::ICE,    MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 915, 3, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1045, 2, MaterialTier::ICE,   MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 35: Alpine Stockade
    {
        LevelSpec L;
        L.name = "LEVEL 35: BLIZZARD STOCKADE";
        L.subtitle = "Howling alpine blizzard winds drag artillery shots backward!";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::BOMB};
        L.wind = -140.0f;
        addFrontShield(L, 625.0f, MaterialTier::STONE, true);
        addModularBay(L, 715, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 860, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1005, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 36: Aurora Pagoda
    {
        LevelSpec L;
        L.name = "LEVEL 36: AURORA PAGODA";
        L.subtitle = "Soaring 3-story pagoda beneath shimmering northern auroras.";
        L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addModularBay(L, 710, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 850, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 990, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 37: Glacier Redoubt
    {
        LevelSpec L;
        L.name = "LEVEL 37: GLACIER REDOUBT";
        L.subtitle = "Four reinforced bays alternating heavy stone and seasoned timber.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
        addFrontShield(L, 600.0f, MaterialTier::STONE, true);
        addModularBay(L, 680, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 810, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 940, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1070, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 38: Frostfire Spire
    {
        LevelSpec L;
        L.name = "LEVEL 38: FROSTFIRE SPIRE";
        L.subtitle = "Target the combustible TNT depot beside the central glacial spire!";
        L.pigQueue = {PigType::TROTTER, PigType::BOMB, PigType::TURBO, PigType::SPLIT};
        addFrontShield(L, 630.0f, MaterialTier::STONE, false);
        addModularBay(L, 730, 2, MaterialTier::ICE,   MaterialTier::ICE,    MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 870, 3, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 1010, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 800.0f, 618.0f, 24.0f, 24.0f));
        levels.push_back(L);
    }

    // LEVEL 39: Thunder Peak Sanctuary
    {
        LevelSpec L;
        L.name = "LEVEL 39: THUNDER PEAK SANCTUARY";
        L.subtitle = "Electrified cumulonimbus storm clouds unleash lightning bolts upon metal & stone!";
        L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.hasStorm = true;
        L.wind = -50.0f;
        addFrontShield(L, 635.0f, MaterialTier::STONE, true);
        addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addFrontShield(L, 855.0f, MaterialTier::STONE, true);
        addModularBay(L, 985, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 40: Bastion of the Winds (Chapter 4 Boss)
    {
        LevelSpec L;
        L.name = "LEVEL 40: BASTION OF THE WINDS";
        L.subtitle = "Four reinforced bays crowned by twin aerial watchtowers!";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::SPLIT};
        addModularBay(L, 660, 2, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 790, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 920, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1050, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER V (LEVELS 41-50): THE OBSIDIAN IMPERIAL CITADEL
    // ====================================================================
    // LEVEL 41: Obsidian Watch
    {
        LevelSpec L;
        L.name = "LEVEL 41: OBSIDIAN WATCH";
        L.subtitle = "Three carved slate vaults supporting an upper timber command deck!";
        L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::TURBO, PigType::SPLIT};
        addFrontShield(L, 625.0f, MaterialTier::STONE, true);
        addModularBay(L, 715, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,   1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 995, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 42: Pendulum of the Obsidian Emperor
    {
        LevelSpec L;
        L.name = "LEVEL 42: PENDULUM OF THE OBSIDIAN EMPEROR";
        L.subtitle = "A massive spiked iron wrecking ball guards cauldron-helmed Archduke Herons!";
        L.pigQueue = {PigType::BOMB, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
        addModularBay(L, 690, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1020, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        // Grand central imperial throne room with swinging spiked wrecking ball
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 855, 580, 32, 100));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 855, 519, 110, 22));
        L.birds.push_back(makeBird(BirdClass::HERON, 855, 508.0f - birdRad(BirdClass::HERON), true));
        L.birds.push_back(makeBird(BirdClass::HERON, 780, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{};
        pb.anchor = {855.0f, 180.0f};
        pb.length = 150.0f;
        pb.angle = 0.0f;
        pb.angVel = 0.0f;
        pb.radius = 28.0f;
        pb.mass = 12.0f;
        pb.spiked = true;
        L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 43: The Heron's Vault
    {
        LevelSpec L;
        L.name = "LEVEL 43: THE HERON'S VAULT";
        L.subtitle = "Three Archduke Herons fortified behind a double-height slate monolith!";
        L.pigQueue = {PigType::BOMB, PigType::TROTTER, PigType::BOMB, PigType::SPLIT};
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 660, 565, 36, 130));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 660, 435, 36, 130));
        addModularBay(L, 775, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 910, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1045, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 44: Volcanic Casemate
    {
        LevelSpec L;
        L.name = "LEVEL 44: VOLCANIC CASEMATE";
        L.subtitle = "Geothermal magma vents erupt fiery embers! Detonate adjacent TNT and melt ice defenses.";
        L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addFrontShield(L, 630.0f, MaterialTier::STONE, true);
        addModularBay(L, 725, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 885, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 805.0f, 616.0f, 40.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 45: Magma Bridge
    {
        LevelSpec L;
        L.name = "LEVEL 45: MAGMA BRIDGE";
        L.subtitle = "Elevated stone viaduct guarded by Baron Von Condor's airship and Commander Falcon's shield!";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
        addModularBay(L, 700, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 840, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        addModularBay(L, 980, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 46: Skybridge Spire
    {
        LevelSpec L;
        L.name = "LEVEL 46: SKYBRIDGE SPIRE";
        L.subtitle = "Four 3-story towers defending the approaches to the Imperial Eyrie!";
        L.pigQueue = {PigType::BOMB, PigType::SPLIT, PigType::TROTTER, PigType::TURBO, PigType::BOMB};
        addModularBay(L, 660, 3, MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 790, 2, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 920, 3, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1050, 2, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG,   MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL,    0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 47: Council of Archdukes
    {
        LevelSpec L;
        L.name = "LEVEL 47: COUNCIL OF ARCHDUKES";
        L.subtitle = "Three heavily fortified Heron thrones in reinforced slate vaults.";
        L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::SPLIT};
        addFrontShield(L, 615.0f, MaterialTier::STONE, true);
        addModularBay(L, 710, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 855, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        addModularBay(L, 1000, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 0, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 48: Obsidian Bastion Prime
    {
        LevelSpec L;
        L.name = "LEVEL 48: OBSIDIAN BASTION PRIME";
        L.subtitle = "Twin TNT kegs flank the central basalt gate! Detonate for a dual shockwave!";
        L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::BOMB};
        addFrontShield(L, 620.0f, MaterialTier::STONE, true);
        addModularBay(L, 725, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        addFrontShield(L, 855.0f, MaterialTier::STONE, true);
        addModularBay(L, 985, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 616.0f, 28.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 900.0f, 616.0f, 28.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 49: The Imperial Arena
    {
        LevelSpec L;
        L.name = "LEVEL 49: THE IMPERIAL ARENA";
        L.subtitle = "Four soaring stone and timber colosseum bays beneath the eclipse!";
        L.pigQueue = {PigType::SPLIT, PigType::BOMB, PigType::TURBO, PigType::TROTTER, PigType::BOMB};
        addModularBay(L, 650, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 780, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 910, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1040, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 50: The Emperor's Final High Eyrie (Ultimate Campaign Finale)
    {
        LevelSpec L;
        L.name = "LEVEL 50: EMPEROR'S FINAL HIGH EYRIE";
        L.subtitle = "The final siege! Storm clouds, lightning, crosswinds, magma vents, and the elite Avian Court guard Emperor Heron!";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -75.0f;
        L.hasStorm = true;
        addFrontShield(L, 580.0f, MaterialTier::STONE, true);
        addModularBay(L, 665, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 795, 3, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  1, BirdClass::HERON,  1, BirdClass::TOUCAN);
        addModularBay(L, 925, 3, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::STONE,  MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::HERON,  0, BirdClass::TOUCAN);
        addModularBay(L, 1055, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON,  0, BirdClass::OWL,    0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 515.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

// ====================================================================
    // CHAPTER VI (LEVELS 51-60): THE BAMBOO STEPPES & GILDED PAGODAS
    // ====================================================================
    // LEVEL 51: PAGODA OF THE WHISPERING REED
    {
        LevelSpec L;
        L.name = "LEVEL 51: PAGODA OF THE WHISPERING REED";
        L.subtitle = "Tiered bamboo pagoda with flared timber eaves swaying in the steppe breeze.";
        L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        L.wind = 35.0f;
        addPagodaBay(L, 850.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 690.0f, 580.0f, 22.0f, 100.0f));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 690.0f, 530.0f - birdRad(BirdClass::TOUCAN)));
        levels.push_back(L);
    }

    // LEVEL 52: BAMBOO GATEHOUSE
    {
        LevelSpec L;
        L.name = "LEVEL 52: BAMBOO GATEHOUSE";
        L.subtitle = "Twin bamboo gate towers flank an elevated timber watchway.";
        L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
        addPagodaBay(L, 740.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addPagodaBay(L, 960.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850.0f, 512.0f, 120.0f, 20.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 502.0f - birdRad(BirdClass::HERON)));
        levels.push_back(L);
    }

    // LEVEL 53: JADE TERRACE
    {
        LevelSpec L;
        L.name = "LEVEL 53: JADE TERRACE";
        L.subtitle = "A heavy stone plinth supports a bamboo shrine guarding the golden terrace.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::GLIDER, PigType::TROTTER};
        L.wind = -25.0f;
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 605.0f, 90.0f, 50.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 1020.0f, 605.0f, 90.0f, 50.0f));
        addPagodaBay(L, 860.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        L.birds.back().hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 54: STEPPE WINDBREAK
    {
        LevelSpec L;
        L.name = "LEVEL 54: STEPPE WINDBREAK";
        L.subtitle = "Violent steppe tailwinds propel siege hogs toward fortified bamboo bastions.";
        L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::DRILL, PigType::BOMB};
        L.wind = 65.0f;
        addFrontShield(L, 620.0f, MaterialTier::TWIG, true);
        addPagodaBay(L, 750.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addPagodaBay(L, 950.0f, 2, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 850.0f, 617.0f, 26.0f, 26.0f));
        levels.push_back(L);
    }

    // LEVEL 55: TWIN PAVILIONS OF THE LOTUS
    {
        LevelSpec L;
        L.name = "LEVEL 55: TWIN PAVILIONS OF THE LOTUS";
        L.subtitle = "Symmetrical pavilions built over reflection pools with trampoline banks.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 70.0f, 24.0f));
        addPagodaBay(L, 720.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addPagodaBay(L, 980.0f, 2, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850.0f, 610.0f, 40.0f, 40.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 590.0f - birdRad(BirdClass::HERON), true));
        levels.push_back(L);
    }

    // LEVEL 56: CRANE'S ROOST
    {
        LevelSpec L;
        L.name = "LEVEL 56: CRANE'S ROOST";
        L.subtitle = "Slender high-altitude bamboo stalks hoist shielded Falcon archers into the sky.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
        L.wind = -45.0f;
        addPagodaBay(L, 860.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::FALCON, 1, BirdClass::FALCON);
        L.birds[1].hasShield = true;
        L.birds[2].hasShield = true;
        addFrontShield(L, 660.0f, MaterialTier::STONE, false);
        levels.push_back(L);
    }

    // LEVEL 57: FLOATING LANTERN SHRINE
    {
        LevelSpec L;
        L.name = "LEVEL 57: FLOATING LANTERN SHRINE";
        L.subtitle = "A suspended shrine flanked by volatile lantern kegs above trampoline waters.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::DRILL, PigType::SPLIT};
        L.wind = 20.0f;
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 620.0f, 618.0f, 74.0f, 24.0f));
        addPagodaBay(L, 820.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 930.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 1020.0f, 580.0f, 24.0f, 100.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 1020.0f, 530.0f - birdRad(BirdClass::HERON)));
        levels.push_back(L);
    }

    // LEVEL 58: MONASTERY OF THE NORTH PEAK
    {
        LevelSpec L;
        L.name = "LEVEL 58: MONASTERY OF THE NORTH PEAK";
        L.subtitle = "Double stone casemates anchor high bamboo spires overlooking the misty pass.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
        L.wind = -50.0f;
        addFortifiedCasemate(L, 720.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addPagodaBay(L, 940.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.birds.back().hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 59: GILDED PAGODA CITADEL
    {
        LevelSpec L;
        L.name = "LEVEL 59: GILDED PAGODA CITADEL";
        L.subtitle = "Three-tiered grand pagoda complex with wide roof overhangs and explosive munitions.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -30.0f;
        addPagodaBay(L, 700.0f, 3, MaterialTier::TWIG, MaterialTier::TIMBER, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addPagodaBay(L, 880.0f, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::FALCON);
        addPagodaBay(L, 1060.0f, 2, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 790.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 970.0f, 617.0f, 26.0f, 26.0f));
        levels.push_back(L);
    }

    // LEVEL 60: PALACE OF THE GOLDEN HERON
    {
        LevelSpec L;
        L.name = "LEVEL 60: PALACE OF THE GOLDEN HERON";
        L.subtitle = "Chapter Boss! The Grand Pagoda Palace defended by Archduke Heron and elite avian guards.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -60.0f;
        L.hasStorm = true;
        addFrontShield(L, 570.0f, MaterialTier::STONE, true);
        addPagodaBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addPagodaBay(L, 860.0f, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::OWL);
        addPagodaBay(L, 1040.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::CONDOR, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER VII (LEVELS 61-70): SUNKEN GROTTO & CORAL AQUEDUCTS
    // ====================================================================
    // LEVEL 61: SUNKEN AQUEDUCT VIADUCT
    {
        LevelSpec L;
        L.name = "LEVEL 61: SUNKEN AQUEDUCT VIADUCT";
        L.subtitle = "Multi-span stone arch viaduct with water-channel decking over deep grotto cisterns.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::TROTTER};
        addMonolithBay(L, 750.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 970.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::ICE, 860.0f, 615.0f, 30.0f, 30.0f));
        levels.push_back(L);
    }

    // LEVEL 62: LIMESTONE STALAGMITE TOWER
    {
        LevelSpec L;
        L.name = "LEVEL 62: LIMESTONE STALAGMITE TOWER";
        L.subtitle = "Slender limestone columns crowned by frost-glazed bridge spans.";
        L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 720.0f, 570.0f, 20.0f, 120.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 570.0f, 20.0f, 120.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 1000.0f, 570.0f, 20.0f, 120.0f));
        L.blocks.push_back(makeBlock(MaterialTier::ICE, 790.0f, 500.0f, 140.0f, 20.0f));
        L.blocks.push_back(makeBlock(MaterialTier::ICE, 930.0f, 500.0f, 140.0f, 20.0f));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 790.0f, 490.0f - birdRad(BirdClass::TOUCAN)));
        L.birds.push_back(makeBird(BirdClass::OWL, 930.0f, 490.0f - birdRad(BirdClass::OWL)));
        levels.push_back(L);
    }

    // LEVEL 63: THE CORAL CISTERN
    {
        LevelSpec L;
        L.name = "LEVEL 63: THE CORAL CISTERN";
        L.subtitle = "Massive megaliths flanked by subterranean trampoline deflectors.";
        L.pigQueue = {PigType::TURBO, PigType::GLIDER, PigType::SPLIT, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 560.0f, 618.0f, 74.0f, 24.0f));
        addMonolithBay(L, 780.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 990.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 64: SUBTERRANEAN POWDER VAULT
    {
        LevelSpec L;
        L.name = "LEVEL 64: SUBTERRANEAN POWDER VAULT";
        L.subtitle = "Underground powder magazine where explosive kegs support stone architraves.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 700.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 1020.0f, 617.0f, 26.0f, 26.0f));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 700.0f, 604.0f - birdRad(BirdClass::TOUCAN)));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 1020.0f, 604.0f - birdRad(BirdClass::TOUCAN)));
        levels.push_back(L);
    }

    // LEVEL 65: CAVERN OF THE ECHOING DROP
    {
        LevelSpec L;
        L.name = "LEVEL 65: CAVERN OF THE ECHOING DROP";
        L.subtitle = "A heavy spiked iron pendulum hangs suspended in the cavern gloom.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addMonolithBay(L, 680.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {850.0f, 160.0f}; pb.length = 180.0f; pb.radius = 26.0f; pb.mass = 10.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 66: TWIN CORAL PILLARS
    {
        LevelSpec L;
        L.name = "LEVEL 66: TWIN CORAL PILLARS";
        L.subtitle = "Symmetrical megalith piers with ice bridges defended by Falcon sentries.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
        addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        L.birds[1].hasShield = true;
        L.birds[3].hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 67: DROWNED AMPHITHEATER
    {
        LevelSpec L;
        L.name = "LEVEL 67: DROWNED AMPHITHEATER";
        L.subtitle = "Stepped stone tiers descend toward sunken trampoline nets.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
        addPyramidBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 620.0f, 618.0f, 74.0f, 24.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 1100.0f, 618.0f, 74.0f, 24.0f));
        levels.push_back(L);
    }

    // LEVEL 68: MAGMA GROTTO FISSURE
    {
        LevelSpec L;
        L.name = "LEVEL 68: MAGMA GROTTO FISSURE";
        L.subtitle = "Underground magma vents spew molten rocks beneath stone viaduct pillars.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 600.0f, 616.0f, 36.0f, 28.0f));
        addModularBay(L, 760.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 69: GROTTO SOVEREIGN SANCTUM
    {
        LevelSpec L;
        L.name = "LEVEL 69: GROTTO SOVEREIGN SANCTUM";
        L.subtitle = "Reinforced stone casemate flanked by crystalline ice towers.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::OWL);
        addModularBay(L, 660.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1060.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 70: ABYSSAL GATE OF EMPEROR HERON
    {
        LevelSpec L;
        L.name = "LEVEL 70: ABYSSAL GATE OF EMPEROR HERON";
        L.subtitle = "Chapter Boss! Colossal double-aqueduct bastion defending the entrance to the deep abyss.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -35.0f;
        addMonolithBay(L, 680.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::CONDOR);
        addMonolithBay(L, 1080.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER VIII (LEVELS 71-80): IRON FOUNDRY & CLOCKWORK GANTRIES
    // ====================================================================
    // LEVEL 71: FOUNDRY GANTRY LINE
    {
        LevelSpec L;
        L.name = "LEVEL 71: FOUNDRY GANTRY LINE";
        L.subtitle = "Industrial crane girder spans across heavy timber stanchions.";
        L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
        addClockworkGantry(L, 850.0f, MaterialTier::TIMBER, MaterialTier::TIMBER, false, BirdClass::OWL);
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 850.0f, GROUND_Y - birdRad(BirdClass::TOUCAN)));
        levels.push_back(L);
    }

    // LEVEL 72: THE SLAG FURNACES
    {
        LevelSpec L;
        L.name = "LEVEL 72: THE SLAG FURNACES";
        L.subtitle = "Active magma forge vents flank reinforced timber crate stacks.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::GLIDER, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 620.0f, 616.0f, 36.0f, 28.0f));
        addModularBay(L, 780.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 960.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1080.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 73: CLOCKWORK ASSEMBLY BAY
    {
        LevelSpec L;
        L.name = "LEVEL 73: CLOCKWORK ASSEMBLY BAY";
        L.subtitle = "Twin heavy gantries house a suspended spiked wrecking hammer.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addClockworkGantry(L, 720.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::TOUCAN);
        addClockworkGantry(L, 1000.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {860.0f, 180.0f}; pb.length = 170.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 74: HIGH FORGE CATWALKS
    {
        LevelSpec L;
        L.name = "LEVEL 74: HIGH FORGE CATWALKS";
        L.subtitle = "Elevated timber walkways spanning high stone piers over explosive munitions.";
        L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::TURBO, PigType::BOMB};
        L.wind = 25.0f;
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 575.0f, 24.0f, 110.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 575.0f, 24.0f, 110.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 1020.0f, 575.0f, 24.0f, 110.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 780.0f, 508.0f, 160.0f, 22.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 940.0f, 508.0f, 160.0f, 22.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 780.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 940.0f, 617.0f, 26.0f, 26.0f));
        L.birds.push_back(makeBird(BirdClass::OWL, 780.0f, 497.0f - birdRad(BirdClass::OWL)));
        L.birds.push_back(makeBird(BirdClass::HERON, 940.0f, 497.0f - birdRad(BirdClass::HERON), true));
        levels.push_back(L);
    }

    // LEVEL 75: MUNITIONS STORAGE CASEMATE
    {
        LevelSpec L;
        L.name = "LEVEL 75: MUNITIONS STORAGE CASEMATE";
        L.subtitle = "Fortified industrial bunker packed with TNT gunpowder kegs.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        addFortifiedCasemate(L, 850.0f, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 710.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 990.0f, 617.0f, 26.0f, 26.0f));
        levels.push_back(L);
    }

    // LEVEL 76: THE POUNDING HAMMER
    {
        LevelSpec L;
        L.name = "LEVEL 76: THE POUNDING HAMMER";
        L.subtitle = "Heavy granite pendulum swinging between dual stone foundry towers.";
        L.pigQueue = {PigType::TURBO, PigType::BOMB, PigType::SPLIT, PigType::TROTTER};
        addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 190.0f; pb.radius = 30.0f; pb.mass = 14.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 77: SMELTING VIADUCT
    {
        LevelSpec L;
        L.name = "LEVEL 77: SMELTING VIADUCT";
        L.subtitle = "High stone viaduct above magma vents with cross-braced timber tiers.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO};
        L.wind = -30.0f;
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
        addClockworkGantry(L, 780.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::OWL);
        addClockworkGantry(L, 980.0f, MaterialTier::STONE, MaterialTier::TIMBER, false, BirdClass::HERON);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 78: THE IRON BASTION CRANE
    {
        LevelSpec L;
        L.name = "LEVEL 78: THE IRON BASTION CRANE";
        L.subtitle = "Asymmetrical gantry crane with counterweight megaliths.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
        addClockworkGantry(L, 800.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::FALCON);
        L.birds.back().hasShield = true;
        addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 79: OVERHEATED BOILER HOUSE
    {
        LevelSpec L;
        L.name = "LEVEL 79: OVERHEATED BOILER HOUSE";
        L.subtitle = "Triple magma vent defense with trampoline steam buffers.";
        L.pigQueue = {PigType::TURBO, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
        L.wind = 40.0f;
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 670.0f, 616.0f, 36.0f, 28.0f));
        addModularBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        addModularBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 80: GRAND FOUNDRY OF THE IRON CONDOR
    {
        LevelSpec L;
        L.name = "LEVEL 80: GRAND FOUNDRY OF THE IRON CONDOR";
        L.subtitle = "Chapter Boss! Colossal multi-tier clockwork gantry defended by an armored Zeppelin Condor.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -50.0f;
        L.hasStorm = true;
        addFrontShield(L, 570.0f, MaterialTier::STONE, true);
        addClockworkGantry(L, 700.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::FALCON);
        addClockworkGantry(L, 900.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::CONDOR);
        addClockworkGantry(L, 1100.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::HERON);
        L.birds[1].inZeppelin = true; L.birds[1].balloonHp = 120.0f;
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::CONDOR || bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER IX (LEVELS 81-90): ZEPHYR CRAGS & FLOATING SPIRE CASTLES
    // ====================================================================
    // LEVEL 81: GALE-FORCE OUTPOST
    {
        LevelSpec L;
        L.name = "LEVEL 81: GALE-FORCE OUTPOST";
        L.subtitle = "High wind drafts batter a swaying timber watchtower.";
        L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
        L.wind = -85.0f;
        addModularBay(L, 860.0f, 3, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addFrontShield(L, 680.0f, MaterialTier::TIMBER, false);
        levels.push_back(L);
    }

    // LEVEL 82: ZEPHYR SUSPENSION BRIDGE
    {
        LevelSpec L;
        L.name = "LEVEL 82: ZEPHYR SUSPENSION BRIDGE";
        L.subtitle = "Two high stone spires connected by an elevated suspended bridge deck.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::TROTTER};
        L.wind = 55.0f;
        addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1000.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 850.0f, 498.0f, 200.0f, 20.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 488.0f - birdRad(BirdClass::HERON)));
        levels.push_back(L);
    }

    // LEVEL 83: LIGHTNING ROOST
    {
        LevelSpec L;
        L.name = "LEVEL 83: LIGHTNING ROOST";
        L.subtitle = "Storm-charged clouds unleash static lightning strikes across high stone battlements.";
        L.pigQueue = {PigType::BOMB, PigType::GLIDER, PigType::DRILL, PigType::TROTTER};
        L.wind = -75.0f;
        L.hasStorm = true;
        addModularBay(L, 750.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 950.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds.back().hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 84: SKYSHIP MOORING PYLON
    {
        LevelSpec L;
        L.name = "LEVEL 84: SKYSHIP MOORING PYLON";
        L.subtitle = "A towering stone mast mooring an aerial Zeppelin Falcon sentry.";
        L.pigQueue = {PigType::GLIDER, PigType::TURBO, PigType::DRILL, PigType::BOMB};
        L.wind = -90.0f;
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 540.0f, 32.0f, 180.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 860.0f, 439.0f, 120.0f, 22.0f));
        L.birds.push_back(makeBird(BirdClass::FALCON, 860.0f, 428.0f - birdRad(BirdClass::FALCON), false, true, true));
        levels.push_back(L);
    }

    // LEVEL 85: CROSSWIND RIDGE
    {
        LevelSpec L;
        L.name = "LEVEL 85: CROSSWIND RIDGE";
        L.subtitle = "Violent fluctuating gales test precision trajectory calculation.";
        L.pigQueue = {PigType::SPLIT, PigType::GLIDER, PigType::TURBO, PigType::TROTTER};
        L.wind = 80.0f;
        addPyramidBay(L, 780.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, BirdClass::OWL);
        addPyramidBay(L, 980.0f, 3, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, BirdClass::HERON);
        levels.push_back(L);
    }

    // LEVEL 86: CLOUD CITADEL GATE
    {
        LevelSpec L;
        L.name = "LEVEL 86: CLOUD CITADEL GATE";
        L.subtitle = "Grand stone pylon gate with trampoline safety nets over the abyss.";
        L.pigQueue = {PigType::TURBO, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
        L.wind = -60.0f;
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 74.0f, 24.0f));
        addMonolithBay(L, 780.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 87: TEMPEST EYRIE
    {
        LevelSpec L;
        L.name = "LEVEL 87: TEMPEST EYRIE";
        L.subtitle = "Gale-force storm headwinds defend a triple-deck stone mountain eyrie.";
        L.pigQueue = {PigType::GLIDER, PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        L.wind = -110.0f;
        L.hasStorm = true;
        addFrontShield(L, 590.0f, MaterialTier::STONE, true);
        addModularBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 920.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 88: TWIN FLOATING SPIRES
    {
        LevelSpec L;
        L.name = "LEVEL 88: TWIN FLOATING SPIRES";
        L.subtitle = "Dual slender stone towers connected by a high aerial bridge.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
        L.wind = -40.0f;
        addModularBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 1020.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TIMBER, 870.0f, 498.0f, 200.0f, 20.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 870.0f, 488.0f - birdRad(BirdClass::HERON), true));
        levels.push_back(L);
    }

    // LEVEL 89: ZEPPELIN AIR-FLEET DOCK
    {
        LevelSpec L;
        L.name = "LEVEL 89: ZEPPELIN AIR-FLEET DOCK";
        L.subtitle = "Two Zeppelin-mounted Condor captains docked atop high fortress pylons.";
        L.pigQueue = {PigType::GLIDER, PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
        L.wind = -70.0f;
        L.hasStorm = true;
        addMonolithBay(L, 750.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 990.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
        L.birds[1].inZeppelin = true;
        L.birds[3].inZeppelin = true;
        levels.push_back(L);
    }

    // LEVEL 90: SKY FORTRESS OF THE STORM ARCHON
    {
        LevelSpec L;
        L.name = "LEVEL 90: SKY FORTRESS OF THE STORM ARCHON";
        L.subtitle = "Chapter Boss! Storm lightning, extreme winds, and an armored Zeppelin Archduke Heron.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -100.0f;
        L.hasStorm = true;
        addFrontShield(L, 560.0f, MaterialTier::STONE, true);
        addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.birds[2].inZeppelin = true;
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER X (LEVELS 91-100): CRYSTAL CAVERNS & PRISMATIC MONOLITHS
    // ====================================================================
    // LEVEL 91: PRISMATIC ICE SPIRE
    {
        LevelSpec L;
        L.name = "LEVEL 91: PRISMATIC ICE SPIRE";
        L.subtitle = "Tall crystalline ice tower with stone foundation plinth.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::TROTTER};
        addModularBay(L, 850.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 92: GLACIAL CREVASSE BRIDGE
    {
        LevelSpec L;
        L.name = "LEVEL 92: GLACIAL CREVASSE BRIDGE";
        L.subtitle = "Long timber and stone bridge spanning two slippery ice piers.";
        L.pigQueue = {PigType::DRILL, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
        addModularBay(L, 720.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 980.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 850.0f, 498.0f, 160.0f, 20.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 488.0f - birdRad(BirdClass::HERON)));
        levels.push_back(L);
    }

    // LEVEL 93: THE FROST VAULT
    {
        LevelSpec L;
        L.name = "LEVEL 93: THE FROST VAULT";
        L.subtitle = "Thick ice walls enclosing an explosive cache of gunpowder kegs.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        addFortifiedCasemate(L, 860.0f, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 720.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 1000.0f, 617.0f, 26.0f, 26.0f));
        levels.push_back(L);
    }

    // LEVEL 94: DIAMOND PILLAR HALL
    {
        LevelSpec L;
        L.name = "LEVEL 94: DIAMOND PILLAR HALL";
        L.subtitle = "Multi-column peristyle of alternating stone and crystal ice steles.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 575.0f, 22.0f, 110.0f));
        L.blocks.push_back(makeBlock(MaterialTier::ICE, 800.0f, 575.0f, 22.0f, 110.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 900.0f, 575.0f, 22.0f, 110.0f));
        L.blocks.push_back(makeBlock(MaterialTier::ICE, 1000.0f, 575.0f, 22.0f, 110.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 850.0f, 509.0f, 330.0f, 22.0f));
        L.birds.push_back(makeBird(BirdClass::OWL, 750.0f, 498.0f - birdRad(BirdClass::OWL)));
        L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 498.0f - birdRad(BirdClass::HERON)));
        L.birds.push_back(makeBird(BirdClass::OWL, 950.0f, 498.0f - birdRad(BirdClass::OWL)));
        levels.push_back(L);
    }

    // LEVEL 95: TRAMPOLINE REFLECTION GROTTO
    {
        LevelSpec L;
        L.name = "LEVEL 95: TRAMPOLINE REFLECTION GROTTO";
        L.subtitle = "Zig-zag trampoline deflectors allow high-velocity ricochets into frozen towers.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 700.0f, 618.0f, 70.0f, 24.0f));
        addModularBay(L, 860.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1020.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 96: THE GLACIAL PALISADE
    {
        LevelSpec L;
        L.name = "LEVEL 96: THE GLACIAL PALISADE";
        L.subtitle = "Layered ice slabs protect armored Falcon guards from direct frontal bombardment.";
        L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addFrontShield(L, 620.0f, MaterialTier::ICE, true);
        addFrontShield(L, 660.0f, MaterialTier::ICE, true);
        addModularBay(L, 780.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 960.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds[0].hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 97: CHASM OF THE FROZEN PENDULUM
    {
        LevelSpec L;
        L.name = "LEVEL 97: CHASM OF THE FROZEN PENDULUM";
        L.subtitle = "Heavy ice-sheathed wrecking pendulum poised above the frost altar.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addModularBay(L, 710.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1010.0f, 2, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 98: SUB-ZERO CASEMATE
    {
        LevelSpec L;
        L.name = "LEVEL 98: SUB-ZERO CASEMATE";
        L.subtitle = "Reinforced ice bunker with basalt lintels and thermal magma vents.";
        L.pigQueue = {PigType::DRILL, PigType::GRAVITON, PigType::BOMB, PigType::TURBO};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
        addFortifiedCasemate(L, 760.0f, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, 1, BirdClass::OWL, 1, BirdClass::FALCON);
        addModularBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::ICE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds[1].hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 99: PRISMATIC GRAND COLONNADE
    {
        LevelSpec L;
        L.name = "LEVEL 99: PRISMATIC GRAND COLONNADE";
        L.subtitle = "Long span of alternating crystalline ice and polished stone columns.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addMonolithBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 900.0f, 2, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 1100.0f, 2, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 100: ICE CITADEL OF THE GLACIAL KING
    {
        LevelSpec L;
        L.name = "LEVEL 100: ICE CITADEL OF THE GLACIAL KING";
        L.subtitle = "Chapter Boss! Massive multi-tiered ice and granite castle defended by Emperor Heron's frosty court.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -45.0f;
        L.hasStorm = true;
        addFrontShield(L, 560.0f, MaterialTier::STONE, true);
        addModularBay(L, 680.0f, 3, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        addModularBay(L, 1080.0f, 3, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::STONE, MaterialTier::ICE, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER XI (LEVELS 101-110): VOLCANIC CALDERA & ASHFLOW RAMPARTS
    // ====================================================================
    // LEVEL 101: CALDERA WATCHPOST
    {
        LevelSpec L;
        L.name = "LEVEL 101: CALDERA WATCHPOST";
        L.subtitle = "Heavy basalt tower constructed over bubbling magma vents.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 660.0f, 616.0f, 36.0f, 28.0f));
        addModularBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 980.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 102: STEPPED BASALT PYRAMID
    {
        LevelSpec L;
        L.name = "LEVEL 102: STEPPED BASALT PYRAMID";
        L.subtitle = "Three-tiered ziggurat with wide stone foundation slabs.";
        L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addPyramidBay(L, 850.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
        L.birds.back().hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 103: THE OBSIDIAN VIADUCT
    {
        LevelSpec L;
        L.name = "LEVEL 103: THE OBSIDIAN VIADUCT";
        L.subtitle = "High stone viaduct straddling active volcanic vents.";
        L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::BOMB, PigType::TURBO};
        L.wind = 25.0f;
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 620.0f, 616.0f, 36.0f, 28.0f));
        addMonolithBay(L, 780.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 104: ASHFLOW BUNKER
    {
        LevelSpec L;
        L.name = "LEVEL 104: ASHFLOW BUNKER";
        L.subtitle = "Low, ultra-heavy stone casemate with reinforced roof against falling volcanic bombs.";
        L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addFortifiedCasemate(L, 840.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::OWL);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 700.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 980.0f, 617.0f, 26.0f, 26.0f));
        levels.push_back(L);
    }

    // LEVEL 105: MOLTEN CHAMBER
    {
        LevelSpec L;
        L.name = "LEVEL 105: MOLTEN CHAMBER";
        L.subtitle = "Twin magma vents with trampoline safety deflectors.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 650.0f, 618.0f, 70.0f, 24.0f));
        addModularBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 106: THE BRIMSTONE BASTION
    {
        LevelSpec L;
        L.name = "LEVEL 106: THE BRIMSTONE BASTION";
        L.subtitle = "Volcanic rock fortress with integrated TNT demolition charges.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        L.wind = -20.0f;
        addMonolithBay(L, 760.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 870.0f, 617.0f, 26.0f, 26.0f));
        L.birds.push_back(makeBird(BirdClass::FALCON, 870.0f, 604.0f - birdRad(BirdClass::FALCON), false, true));
        levels.push_back(L);
    }

    // LEVEL 107: MAGMA CASCADE BRIDGE
    {
        LevelSpec L;
        L.name = "LEVEL 107: MAGMA CASCADE BRIDGE";
        L.subtitle = "Suspended stone bridge spanning across two volcanic spires.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
        L.wind = -40.0f;
        addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1000.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 850.0f, 498.0f, 200.0f, 20.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 850.0f, 488.0f - birdRad(BirdClass::HERON), true));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 850.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 108: THE FIRE DRAKE'S EYRIE
    {
        LevelSpec L;
        L.name = "LEVEL 108: THE FIRE DRAKE'S EYRIE";
        L.subtitle = "High basalt column topped by an armored Condor general.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
        L.wind = -55.0f;
        L.hasStorm = true;
        addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 1, BirdClass::CONDOR);
        L.birds.back().hasHelmet = true;
        addFrontShield(L, 680.0f, MaterialTier::STONE, true);
        levels.push_back(L);
    }

    // LEVEL 109: CALDERA GRAND REDOUBT
    {
        LevelSpec L;
        L.name = "LEVEL 109: CALDERA GRAND REDOUBT";
        L.subtitle = "Multi-bay fortified fortress with stepped plinths and magma vents.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -30.0f;
        L.hasStorm = true;
        addPyramidBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::OWL);
        addPyramidBay(L, 980.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 850.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 110: VOLCANIC CITADEL OF THE ASH EMPEROR
    {
        LevelSpec L;
        L.name = "LEVEL 110: VOLCANIC CITADEL OF THE ASH EMPEROR";
        L.subtitle = "Chapter Boss! Fortress surrounded by magma geysers and elite armored avian court.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -75.0f;
        L.hasStorm = true;
        addFrontShield(L, 560.0f, MaterialTier::STONE, true);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 500.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1160.0f, 616.0f, 36.0f, 28.0f));
        addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER XII (LEVELS 111-120): THE CELESTIAL OBSERVATORY & ASTRAL SPIRES
    // ====================================================================
    // LEVEL 111: ASTRAL SCRIPTORIUM
    {
        LevelSpec L;
        L.name = "LEVEL 111: ASTRAL SCRIPTORIUM";
        L.subtitle = "Tall slender stone steles supporting an elevated observation terrace.";
        L.pigQueue = {PigType::GRAVITON, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addModularBay(L, 850.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 112: THE MERIDIAN COLONNADE
    {
        LevelSpec L;
        L.name = "LEVEL 112: THE MERIDIAN COLONNADE";
        L.subtitle = "Stone colonnade with precision-balanced lintels under starry skies.";
        L.pigQueue = {PigType::DRILL, PigType::GRAVITON, PigType::TURBO, PigType::TROTTER};
        addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 113: ORRERY PENDULUM
    {
        LevelSpec L;
        L.name = "LEVEL 113: ORRERY PENDULUM";
        L.subtitle = "A suspended planetary wrecking ball between twin astronomical towers.";
        L.pigQueue = {PigType::BOMB, PigType::GRAVITON, PigType::TURBO, PigType::TROTTER};
        addModularBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 114: THE HORIZON PYLON
    {
        LevelSpec L;
        L.name = "LEVEL 114: THE HORIZON PYLON";
        L.subtitle = "Cantilevered observation platform extending into high altitude air currents.";
        L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        L.wind = 50.0f;
        addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 1, BirdClass::OWL);
        L.birds[1].hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 115: ZODIAC GATEHOUSE
    {
        LevelSpec L;
        L.name = "LEVEL 115: ZODIAC GATEHOUSE";
        L.subtitle = "Dual stone pylons with high timber observatory dome.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
        addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::OWL);
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 700.0f, 580.0f, 24.0f, 100.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 1020.0f, 580.0f, 24.0f, 100.0f));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 700.0f, 530.0f - birdRad(BirdClass::TOUCAN)));
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 1020.0f, 530.0f - birdRad(BirdClass::TOUCAN)));
        levels.push_back(L);
    }

    // LEVEL 116: VORTEX PROVING GROUND
    {
        LevelSpec L;
        L.name = "LEVEL 116: VORTEX PROVING GROUND";
        L.subtitle = "Dense clusters of timber crates and stone steles designed for Atticus's vortex.";
        L.pigQueue = {PigType::GRAVITON, PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
        addModularBay(L, 720.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        addModularBay(L, 1000.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 117: THE ECLIPSE TOWER
    {
        LevelSpec L;
        L.name = "LEVEL 117: THE ECLIPSE TOWER";
        L.subtitle = "Stepped stone tower with basalt lintels and helmed avian scholars.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TROTTER};
        L.wind = -35.0f;
        addPyramidBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
        L.birds.back().hasHelmet = true;
        addFrontShield(L, 680.0f, MaterialTier::STONE, false);
        levels.push_back(L);
    }

    // LEVEL 118: ASTRAL VIADUCT
    {
        LevelSpec L;
        L.name = "LEVEL 118: ASTRAL VIADUCT";
        L.subtitle = "Sky-high stone bridge spanning two astronomical pavilions.";
        L.pigQueue = {PigType::GLIDER, PigType::DRILL, PigType::TURBO, PigType::BOMB};
        L.wind = 60.0f;
        addModularBay(L, 720.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1000.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 498.0f, 180.0f, 20.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, 488.0f - birdRad(BirdClass::HERON), true));
        levels.push_back(L);
    }

    // LEVEL 119: THE SOLSTICE SHRINE
    {
        LevelSpec L;
        L.name = "LEVEL 119: THE SOLSTICE SHRINE";
        L.subtitle = "Triple-bay megalithic temple with pendulum center and trampoline wings.";
        L.pigQueue = {PigType::TURBO, PigType::GRAVITON, PigType::DRILL, PigType::BOMB};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
        addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {860.0f, 160.0f}; pb.length = 175.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 120: GRAND OBSERVATORY OF THE STAR LORD
    {
        LevelSpec L;
        L.name = "LEVEL 120: GRAND OBSERVATORY OF THE STAR LORD";
        L.subtitle = "Chapter Boss! Colossal multi-wing observatory defended by Archduke Heron and the Royal Astrologers.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -70.0f;
        L.hasStorm = true;
        addFrontShield(L, 560.0f, MaterialTier::STONE, true);
        addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON) { bd.hasHelmet = true; bd.helmetHp = 140.0f; bd.maxHelmetHp = 140.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER XIII (LEVELS 121-130): COLOSSEUM OF THE GREAT AVIAN KINGS
    // ====================================================================
    // LEVEL 121: COLOSSEUM OUTER GATE
    {
        LevelSpec L;
        L.name = "LEVEL 121: COLOSSEUM OUTER GATE";
        L.subtitle = "Heavy stone gatehouse with wide lintels and guarded parapets.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addMonolithBay(L, 850.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        L.birds.back().hasShield = true;
        addFrontShield(L, 680.0f, MaterialTier::STONE, true);
        levels.push_back(L);
    }

    // LEVEL 122: GLADIATORIAL ARENA TIER
    {
        LevelSpec L;
        L.name = "LEVEL 122: GLADIATORIAL ARENA TIER";
        L.subtitle = "Arena floor flanked by trampoline deflectors and timber holding cages.";
        L.pigQueue = {PigType::TURBO, PigType::GLIDER, PigType::DRILL, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 74.0f, 24.0f));
        addModularBay(L, 780.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 980.0f, 2, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 123: THE IMPERIAL LOGE
    {
        LevelSpec L;
        L.name = "LEVEL 123: THE IMPERIAL LOGE";
        L.subtitle = "Elevated royal viewing platform supported by megalith columns.";
        L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 1, BirdClass::HERON);
        L.birds.back().hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 124: HYPOGEUM POWDER VAULT
    {
        LevelSpec L;
        L.name = "LEVEL 124: HYPOGEUM POWDER VAULT";
        L.subtitle = "Under-arena powder magazine packed with TNT munitions.";
        L.pigQueue = {PigType::BOMB, PigType::DRILL, PigType::TURBO, PigType::TROTTER};
        addFortifiedCasemate(L, 850.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TNT, 1, BirdClass::HERON, 1, BirdClass::OWL);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 710.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 990.0f, 617.0f, 26.0f, 26.0f));
        levels.push_back(L);
    }

    // LEVEL 125: STEPPED GRANDSTAND
    {
        LevelSpec L;
        L.name = "LEVEL 125: STEPPED GRANDSTAND";
        L.subtitle = "Ascending tiers of stone seating filled with avian spectators.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::GLIDER, PigType::TROTTER};
        addPyramidBay(L, 760.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::OWL);
        addPyramidBay(L, 980.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
        levels.push_back(L);
    }

    // LEVEL 126: THE CHAMPION'S GAUNTLET
    {
        LevelSpec L;
        L.name = "LEVEL 126: THE CHAMPION'S GAUNTLET";
        L.subtitle = "Arena challenge course with swinging wrecking pendulum and trampoline nets.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 570.0f, 618.0f, 70.0f, 24.0f));
        addModularBay(L, 710.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 1010.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {860.0f, 150.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 127: TWIN ARENA PYLONS
    {
        LevelSpec L;
        L.name = "LEVEL 127: TWIN ARENA PYLONS";
        L.subtitle = "Towering arena gateway columns with high stone cross-beam.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO};
        addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 980.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        L.birds[1].hasShield = true;
        L.birds[3].hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 128: THE EMPEROR'S PODIUM
    {
        LevelSpec L;
        L.name = "LEVEL 128: THE EMPEROR'S PODIUM";
        L.subtitle = "Central elevated throne platform with stone battlements.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TROTTER};
        addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON);
        L.birds.back().hasHelmet = true;
        addFrontShield(L, 680.0f, MaterialTier::STONE, true);
        levels.push_back(L);
    }

    // LEVEL 129: GRAND AMPHITHEATER ARENA
    {
        LevelSpec L;
        L.name = "LEVEL 129: GRAND AMPHITHEATER ARENA";
        L.subtitle = "Multi-tiered stone colonnade spanning across the entire battlefield.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addMonolithBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addMonolithBay(L, 1060.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 130: THE AVIAN GRAND CHAMPIONSHIP
    {
        LevelSpec L;
        L.name = "LEVEL 130: THE AVIAN GRAND CHAMPIONSHIP";
        L.subtitle = "Chapter Boss! The ultimate gladiatorial tournament fortress defended by High Marshall Condor and Emperor Heron.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -60.0f;
        L.hasStorm = true;
        addFrontShield(L, 560.0f, MaterialTier::STONE, true);
        addModularBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) { bd.hasHelmet = true; bd.helmetHp = 150.0f; bd.maxHelmetHp = 150.0f; } }
        levels.push_back(L);
    }

    // ====================================================================
    // CHAPTER XIV (LEVELS 131-140): SOVEREIGN HIGH SANCTUM & EMPEROR'S ZENITH
    // ====================================================================
    // LEVEL 131: SANCTUM OUTER RAMPARTS
    {
        LevelSpec L;
        L.name = "LEVEL 131: SANCTUM OUTER RAMPARTS";
        L.subtitle = "Fortress perimeter wall with reinforced battlements and magma geysers.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::DRILL, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 620.0f, 616.0f, 36.0f, 28.0f));
        addMonolithBay(L, 820.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        levels.push_back(L);
    }

    // LEVEL 132: THE SOVEREIGN CAUSEWAY
    {
        LevelSpec L;
        L.name = "LEVEL 132: THE SOVEREIGN CAUSEWAY";
        L.subtitle = "Grand stone viaduct defended by Falcon shield-bearers.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
        L.wind = -30.0f;
        addMonolithBay(L, 740.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.birds[0].hasShield = true;
        L.birds[2].hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 133: THE MOATED KEEP
    {
        LevelSpec L;
        L.name = "LEVEL 133: THE MOATED KEEP";
        L.subtitle = "Stone citadel flanked by magma moats and trampoline buffers.";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::TROTTER};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 560.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 640.0f, 618.0f, 70.0f, 24.0f));
        addModularBay(L, 800.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        addModularBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1080.0f, 616.0f, 36.0f, 28.0f));
        levels.push_back(L);
    }

    // LEVEL 134: THE THUNDER BASTION
    {
        LevelSpec L;
        L.name = "LEVEL 134: THE THUNDER BASTION";
        L.subtitle = "Storm sky with heavy lightning and gale headwinds (-90).";
        L.pigQueue = {PigType::GLIDER, PigType::BOMB, PigType::DRILL, PigType::TURBO};
        L.wind = -90.0f;
        L.hasStorm = true;
        addFrontShield(L, 590.0f, MaterialTier::STONE, true);
        addModularBay(L, 720.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 920.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds.back().hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 135: SANCTUM CLOCKWORK GANTRY
    {
        LevelSpec L;
        L.name = "LEVEL 135: SANCTUM CLOCKWORK GANTRY";
        L.subtitle = "Industrial defense gantry with swinging wrecking hammer.";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addClockworkGantry(L, 720.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
        addClockworkGantry(L, 1000.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{}; pb.anchor = {860.0f, 160.0f}; pb.length = 180.0f; pb.radius = 28.0f; pb.mass = 12.0f; pb.spiked = true; L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 136: THE IMPERIAL STEPPED ZIGGURAT
    {
        LevelSpec L;
        L.name = "LEVEL 136: THE IMPERIAL STEPPED ZIGGURAT";
        L.subtitle = "Four-tiered grand stone pyramid with armored avian court.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addPyramidBay(L, 750.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
        addPyramidBay(L, 970.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, BirdClass::HERON);
        L.birds[0].hasHelmet = true;
        L.birds[1].hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 137: SKYWAY TO THE ZENITH
    {
        LevelSpec L;
        L.name = "LEVEL 137: SKYWAY TO THE ZENITH";
        L.subtitle = "Twin floating spires connected by suspended stone bridge.";
        L.pigQueue = {PigType::DRILL, PigType::GLIDER, PigType::TURBO, PigType::BOMB};
        L.wind = -50.0f;
        L.hasStorm = true;
        addModularBay(L, 700.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 1020.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 860.0f, 497.0f, 220.0f, 22.0f));
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, 486.0f - birdRad(BirdClass::HERON), true));
        levels.push_back(L);
    }

    // LEVEL 138: THE HIGH GRAND CASEMATE
    {
        LevelSpec L;
        L.name = "LEVEL 138: THE HIGH GRAND CASEMATE";
        L.subtitle = "Ultra-heavy bomb-resistant bunker with double roof slabs.";
        L.pigQueue = {PigType::DRILL, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        addFortifiedCasemate(L, 860.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 720.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 1000.0f, 617.0f, 26.0f, 26.0f));
        L.birds.back().hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 139: THE EMPEROR'S FINAL GUARD
    {
        LevelSpec L;
        L.name = "LEVEL 139: THE EMPEROR'S FINAL GUARD";
        L.subtitle = "Three-bay monolithic fortress defended by elite Zeppelin Condor captains.";
        L.pigQueue = {PigType::GLIDER, PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
        L.wind = -70.0f;
        L.hasStorm = true;
        addMonolithBay(L, 720.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 960.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
        L.birds[1].inZeppelin = true;
        L.birds[3].inZeppelin = true;
        levels.push_back(L);
    }

    // LEVEL 140: THE EMPEROR'S ZENITH - ABSOLUTE CLIMAX
    {
        LevelSpec L;
        L.name = "LEVEL 140: THE EMPEROR'S ZENITH - ABSOLUTE CLIMAX";
        L.subtitle = "The definitive campaign finale! A colossal, multi-tier imperial citadel combining every defense mechanism.";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -95.0f;
        L.hasStorm = true;
        addFrontShield(L, 550.0f, MaterialTier::STONE, true);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 490.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1170.0f, 616.0f, 36.0f, 28.0f));
        addMonolithBay(L, 660.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 860.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        addMonolithBay(L, 1060.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::CONDOR, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) { if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) { bd.hasHelmet = true; bd.helmetHp = 160.0f; bd.maxHelmetHp = 160.0f; } }
        levels.push_back(L);
    }

// ====================================================================
    // CHAPTER XV (LEVELS 141-150): THE 10 SECRET TRUFFLE VAULTS
    // High-concept secret puzzle vaults unlocked via Golden Truffles!
    // ====================================================================

    // LEVEL 141: Vault 01 - Powder Crypt of the Swine (Req: 2 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 141: VAULT 01 - POWDER CRYPT";
        L.subtitle = "Twin slate pillars house volatile TNT kegs ready for a chain reaction!";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::BOMB, PigType::TROTTER};
        // Pillars resting on ground
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 730, 575, 26, 110));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 970, 575, 26, 110));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 850, 509, 266, 22));
        // Pillars resting on deck 1
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 780, 452, 24, 92));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 920, 452, 24, 92));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 850, 396, 180, 20));
        // TNT kegs
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 790.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 910.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 485.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 890.0f, 485.0f, 26.0f, 26.0f));
        // Birds
        L.birds.push_back(makeBird(BirdClass::TOUCAN, 850, GROUND_Y - birdRad(BirdClass::TOUCAN)));
        L.birds.push_back(makeBird(BirdClass::OWL, 850, 498.0f - birdRad(BirdClass::OWL)));
        L.birds.push_back(makeBird(BirdClass::HERON, 850, 386.0f - birdRad(BirdClass::HERON), true));
        levels.push_back(L);
    }

    // LEVEL 142: Vault 02 - Trampoline Gauntlet (Req: 5 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 142: VAULT 02 - TRAMPOLINE GAUNTLET";
        L.subtitle = "Elastic trampoline deflectors line the cavern floor before the stone citadel!";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::TURBO};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 530.0f, 618.0f, 70.0f, 24.0f));
        L.blocks.push_back(makeBlock(MaterialTier::STONE, 615.0f, 550.0f, 28.0f, 160.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 700.0f, 618.0f, 70.0f, 24.0f));
        addModularBay(L, 810, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 950, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        L.birds.back().hasHelmet = true;
        L.birds.back().helmetHp = 140.0f;
        L.birds.back().maxHelmetHp = 140.0f;
        levels.push_back(L);
    }

    // LEVEL 143: Vault 03 - Pendulum Cathedral (Req: 8 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 143: VAULT 03 - PENDULUM CATHEDRAL";
        L.subtitle = "An immense spiked iron wrecking pendulum hangs poised over the altar!";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        addModularBay(L, 710, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        addModularBay(L, 1010, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb{};
        pb.anchor = {860.0f, 150.0f};
        pb.length = 180.0f;
        pb.angle = 0.0f;
        pb.angVel = 0.0f;
        pb.radius = 28.0f;
        pb.mass = 12.0f;
        pb.spiked = true;
        L.pendulums.push_back(pb);
        levels.push_back(L);
    }

    // LEVEL 144: Vault 04 - Thunder Ridge Bastion (Req: 11 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 144: VAULT 04 - THUNDER RIDGE BASTION";
        L.subtitle = "Violent gale headwinds and lightning-charged skies defend the high ridge!";
        L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER};
        L.wind = -110.0f;
        L.hasStorm = true;
        addFrontShield(L, 600.0f, MaterialTier::STONE, true);
        addModularBay(L, 690, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::TOUCAN, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 830, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 970, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 0, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.birds[1].hasHelmet = true; L.birds[1].helmetHp = 100.0f; L.birds[1].maxHelmetHp = 100.0f;
        L.birds[2].hasHelmet = true; L.birds[2].helmetHp = 140.0f; L.birds[2].maxHelmetHp = 140.0f;
        levels.push_back(L);
    }

    // LEVEL 145: Vault 05 - Obsidian Truffle Treasury (Req: 14 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 145: VAULT 05 - OBSIDIAN TRUFFLE TREASURY";
        L.subtitle = "The Grand Imperial Treasury! Magma geysers, trampolines, and the Emperor's Royal Guard!";
        L.pigQueue = {PigType::BOMB, PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::TROTTER, PigType::BOMB};
        L.wind = -45.0f;
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 500.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1140.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 580.0f, 618.0f, 74.0f, 24.0f));
        addFrontShield(L, 650.0f, MaterialTier::STONE, true);
        addModularBay(L, 740, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 880, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE,  MaterialTier::TIMBER, 1, BirdClass::HERON, 1, BirdClass::OWL, 1, BirdClass::TOUCAN);
        addModularBay(L, 1020, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::TOUCAN, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 616.0f, 28.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 950.0f, 616.0f, 28.0f, 28.0f));
        for (auto& bd : L.birds) {
            if (bd.kind == BirdClass::HERON) {
                bd.hasHelmet = true;
                bd.helmetHp = 140.0f;
                bd.maxHelmetHp = 140.0f;
            }
        }
        levels.push_back(L);
    }

    // LEVEL 146: Vault 06 - The Mirror Perils (Req: 18 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 146: VAULT 06 - THE MIRROR PERILS";
        L.subtitle = "Twin symmetrical towers with alternating trampoline bounce channels!";
        L.pigQueue = {PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::GLIDER};
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 540.0f, 618.0f, 70.0f, 24.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 860.0f, 618.0f, 70.0f, 24.0f));
        addMonolithBay(L, 700.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 1020.0f, 2, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::TOUCAN, 1, BirdClass::FALCON, 0, BirdClass::TOUCAN);
        L.birds[1].hasShield = true;
        L.birds[3].hasShield = true;
        levels.push_back(L);
    }

    // LEVEL 147: Vault 07 - Graviton Chasm (Req: 22 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 147: VAULT 07 - GRAVITON CHASM";
        L.subtitle = "Deep magma abyss flanked by floating aeries and volatile powder kegs!";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::BOMB, PigType::TURBO};
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 580.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1100.0f, 616.0f, 36.0f, 28.0f));
        addModularBay(L, 740.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::OWL, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addModularBay(L, 940.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::TIMBER, MaterialTier::TIMBER, MaterialTier::TWIG, MaterialTier::TWIG, 1, BirdClass::HERON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 840.0f, 617.0f, 26.0f, 26.0f));
        L.birds.push_back(makeBird(BirdClass::CONDOR, 840.0f, 604.0f - birdRad(BirdClass::CONDOR), true));
        levels.push_back(L);
    }

    // LEVEL 148: Vault 08 - Clockwork Catastrophe (Req: 26 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 148: VAULT 08 - CLOCKWORK CATASTROPHE";
        L.subtitle = "Dual swinging wrecking pendulums over stacked TNT munitions magazine!";
        L.pigQueue = {PigType::BOMB, PigType::TURBO, PigType::SPLIT, PigType::DRILL};
        addClockworkGantry(L, 700.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
        addClockworkGantry(L, 1020.0f, MaterialTier::STONE, MaterialTier::STONE, false, BirdClass::TOUCAN);
        L.birds.push_back(makeBird(BirdClass::HERON, 860.0f, GROUND_Y - birdRad(BirdClass::HERON), true));
        PendulumBody pb1{}; pb1.anchor = {780.0f, 160.0f}; pb1.length = 170.0f; pb1.radius = 26.0f; pb1.mass = 11.0f; pb1.spiked = true; L.pendulums.push_back(pb1);
        PendulumBody pb2{}; pb2.anchor = {940.0f, 160.0f}; pb2.length = 170.0f; pb2.radius = 26.0f; pb2.mass = 11.0f; pb2.spiked = true; L.pendulums.push_back(pb2);
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 810.0f, 617.0f, 26.0f, 26.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TNT, 910.0f, 617.0f, 26.0f, 26.0f));
        levels.push_back(L);
    }

    // LEVEL 149: Vault 09 - Bore Master's Labyrinth (Req: 30 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 149: VAULT 09 - BORE MASTER'S LABYRINTH";
        L.subtitle = "Massive solid stone megaliths with narrow target channels designed for Drill Hog!";
        L.pigQueue = {PigType::DRILL, PigType::DRILL, PigType::GRAVITON, PigType::BOMB, PigType::TURBO};
        L.wind = -20.0f;
        addFrontShield(L, 600.0f, MaterialTier::STONE, true);
        addFortifiedCasemate(L, 750.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::FALCON);
        addFortifiedCasemate(L, 970.0f, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::CONDOR);
        L.birds[1].hasShield = true;
        L.birds[3].hasHelmet = true;
        levels.push_back(L);
    }

    // LEVEL 150: Vault 10 - The Gilded Sovereign Sanctum (Req: 35 Truffles)
    {
        LevelSpec L;
        L.name = "LEVEL 150: VAULT 10 - GILDED SOVEREIGN SANCTUM";
        L.subtitle = "The ultimate secret truffle temple! 3-tier grand monoliths, golden ornamentation, and Emperor's personal retinue!";
        L.pigQueue = {PigType::GRAVITON, PigType::DRILL, PigType::GLIDER, PigType::BOMB, PigType::TURBO, PigType::TROTTER};
        L.wind = -80.0f;
        L.hasStorm = true;
        addFrontShield(L, 510.0f, MaterialTier::STONE, true);
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 450.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::MAGMA_VENT, 1180.0f, 616.0f, 36.0f, 28.0f));
        L.blocks.push_back(makeBlock(MaterialTier::TRAMPOLINE, 565.0f, 618.0f, 60.0f, 24.0f));
        addMonolithBay(L, 680.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::FALCON, 1, BirdClass::OWL, 0, BirdClass::TOUCAN);
        addMonolithBay(L, 880.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::HERON, 1, BirdClass::HERON, 1, BirdClass::TOUCAN);
        addMonolithBay(L, 1080.0f, 3, MaterialTier::STONE, MaterialTier::STONE, MaterialTier::STONE, 1, BirdClass::OWL, 1, BirdClass::CONDOR, 0, BirdClass::TOUCAN);
        for (auto& bd : L.birds) {
            if (bd.kind == BirdClass::HERON || bd.kind == BirdClass::CONDOR) {
                bd.hasHelmet = true;
                bd.helmetHp = 180.0f;
                bd.maxHelmetHp = 180.0f;
            }
        }
        levels.push_back(L);
    }

    // 35 Hidden Golden Origami Truffles across Campaign (2-3 per chapter)
    struct TrufflePlacement { int lvlIdx; Vec2 pos; };
    const TrufflePlacement truffleDefs[35] = {
        // Chapter 1 (Levels 1-10)
        { 1, {850.0f, 380.0f}}, // Level 2
        { 4, {855.0f, 385.0f}}, // Level 5
        { 8, {855.0f, 290.0f}}, // Level 9
        // Chapter 2 (Levels 11-20)
        {12, {915.0f, 380.0f}}, // Level 13
        {16, {775.0f, 380.0f}}, // Level 17
        {19, {860.0f, 290.0f}}, // Level 20
        // Chapter 3 (Levels 21-30)
        {22, {715.0f, 290.0f}}, // Level 23
        {26, {855.0f, 290.0f}}, // Level 27
        {29, {945.0f, 290.0f}}, // Level 30
        // Chapter 4 (Levels 31-40)
        {32, {720.0f, 290.0f}}, // Level 33
        {35, {850.0f, 290.0f}}, // Level 36
        {39, {790.0f, 290.0f}}, // Level 40
        // Chapter 5 (Levels 41-50)
        {42, {910.0f, 290.0f}}, // Level 43
        {46, {855.0f, 290.0f}}, // Level 47
        {49, {795.0f, 290.0f}}, // Level 50
        // Chapter 6 (Levels 51-60)
        {52, {860.0f, 330.0f}}, // Level 53
        {56, {820.0f, 330.0f}}, // Level 57
        // Chapter 7 (Levels 61-70)
        {62, {860.0f, 440.0f}}, // Level 63
        {66, {880.0f, 530.0f}}, // Level 67
        {69, {880.0f, 440.0f}}, // Level 70
        // Chapter 8 (Levels 71-80)
        {72, {860.0f, 380.0f}}, // Level 73
        {77, {800.0f, 380.0f}}, // Level 78
        // Chapter 9 (Levels 81-90)
        {82, {850.0f, 420.0f}}, // Level 83
        {86, {820.0f, 290.0f}}, // Level 87
        // Chapter 10 (Levels 91-100)
        {92, {860.0f, 490.0f}}, // Level 93
        {97, {760.0f, 490.0f}}, // Level 98
        // Chapter 11 (Levels 101-110)
        {102, {880.0f, 440.0f}}, // Level 103
        {106, {850.0f, 470.0f}}, // Level 107
        {109, {880.0f, 290.0f}}, // Level 110
        // Chapter 12 (Levels 111-120)
        {112, {860.0f, 380.0f}}, // Level 113
        {117, {860.0f, 470.0f}}, // Level 118
        // Chapter 13 (Levels 121-130)
        {122, {860.0f, 440.0f}}, // Level 123
        {126, {860.0f, 470.0f}}, // Level 127
        // Chapter 14 (Levels 131-140)
        {132, {860.0f, 470.0f}}, // Level 133
        {136, {860.0f, 470.0f}}  // Level 137
    };
    for (const auto& tp : truffleDefs) {
        if (tp.lvlIdx >= 0 && tp.lvlIdx < static_cast<int>(levels.size())) {
            levels[tp.lvlIdx].hasTruffle = true;
            levels[tp.lvlIdx].trufflePos = tp.pos;
        }
    }

    // Calibrate 2-Star and 3-Star thresholds across all 150 levels based on actual level composition
    for (auto& L : levels) {
        int birdPts = 0;
        for (const auto& t : L.birds) {
            birdPts += (t.kind == BirdClass::HERON) ? 10000 :
                       (t.kind == BirdClass::OWL ? 7500 : 5000);
            if (t.hasHelmet) birdPts += 1000;
        }
        int blockPts = 0;
        for (const auto& b : L.blocks) {
            blockPts += (b.mat == MaterialTier::TWIG)       ? 500 :
                        (b.mat == MaterialTier::TIMBER     ? 900 :
                        (b.mat == MaterialTier::STONE      ? 1500 :
                        (b.mat == MaterialTier::ICE        ? 600 :
                        (b.mat == MaterialTier::TRAMPOLINE ? 0 : 2500))));
        }
        L.star2Threshold = ((birdPts + static_cast<int>(blockPts * 0.55f) + 1500) / 1000) * 1000;
        L.star3Threshold = ((birdPts + static_cast<int>(blockPts * 0.65f) + 6000) / 1000) * 1000;
    }
}

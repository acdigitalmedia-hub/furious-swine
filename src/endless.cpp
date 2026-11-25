#include "game.h"

void SnoutAndSiegeGame::generateEndlessWave(int wave) {
    endlessWave = wave;
    LevelSpec spec;
    spec.name = "ENDLESS SIEGE GAUNTLET";
    spec.subtitle = "WAVE " + std::to_string(wave) + "  |  DEFEND THE SNOUT!";
    spec.wind = (wave >= 3) ? (((wave % 5) - 2) * 20.0f) : 0.0f;
    spec.hasStorm = (wave >= 5 && (wave % 4 == 0));
    spec.hasTruffle = (wave % 5 == 0);
    spec.trufflePos = {800.0f, 320.0f};

    spec.pigQueue.clear();
    spec.pigQueue.push_back(PigType::TROTTER);
    if (wave >= 2) spec.pigQueue.push_back(PigType::TURBO);
    if (wave >= 3) spec.pigQueue.push_back(PigType::SPLIT);
    if (wave >= 4) spec.pigQueue.push_back(PigType::BOMB);
    if (wave >= 5) spec.pigQueue.push_back(PigType::GLIDER);
    if (wave >= 6) spec.pigQueue.push_back(PigType::DRILL);
    if (wave >= 7) spec.pigQueue.push_back(PigType::GRAVITON);
    if (wave >= 8) spec.pigQueue.push_back((wave % 2 == 0) ? PigType::DRILL : PigType::GRAVITON);
    if (wave >= 10) spec.pigQueue.push_back(PigType::BOMB);

    int numBays = std::min(3, 1 + (wave / 5));
    float startX = 680.0f - (numBays - 1) * 75.0f;
    float baySpacing = 160.0f;

    MaterialTier wallMat = (wave <= 2) ? MaterialTier::TWIG :
                           (wave <= 5) ? MaterialTier::TIMBER :
                           (wave <= 9) ? MaterialTier::STONE :
                           ((wave % 2 == 0) ? MaterialTier::ICE : MaterialTier::STONE);

    for (int b = 0; b < numBays; ++b) {
        float bayX = startX + b * baySpacing;
        int floors = std::min(3, 1 + ((wave + b) / 4));

        spec.blocks.push_back(makeBlock(MaterialTier::STONE, bayX, GROUND_Y - 11.0f, 130.0f, 22.0f));

        float curY = GROUND_Y - 22.0f;
        for (int f = 0; f < floors; ++f) {
            float pillarH = 75.0f;
            spec.blocks.push_back(makeBlock(wallMat, bayX - 45.0f, curY - pillarH * 0.5f, 20.0f, pillarH));
            spec.blocks.push_back(makeBlock(wallMat, bayX + 45.0f, curY - pillarH * 0.5f, 20.0f, pillarH));

            if (f == 0 && wave >= 4 && (b % 2 == 1)) {
                spec.blocks.push_back(makeBlock(MaterialTier::TNT, bayX, curY - 18.0f, 36.0f, 36.0f));
            } else {
                BirdClass kind = (wave >= 9 && f >= 2) ? BirdClass::CONDOR :
                                 ((wave >= 6 && f >= 1) ? BirdClass::FALCON :
                                 ((f >= 2 || wave >= 8) ? BirdClass::HERON :
                                 ((f >= 1 || wave >= 4) ? BirdClass::OWL : BirdClass::TOUCAN)));
                bool helmet = wave >= 6 && (f == floors - 1 || wave >= 10) && kind != BirdClass::CONDOR;
                BirdTarget bt = makeBird(kind, bayX, 0.0f, helmet);
                bt.pos.y = curY - bt.radius - 2.0f;
                spec.birds.push_back(bt);
            }

            curY -= pillarH;
            MaterialTier roofMat = (wave >= 8 && f == floors - 1) ? MaterialTier::STONE : MaterialTier::TIMBER;
            spec.blocks.push_back(makeBlock(roofMat, bayX, curY - 11.0f, 125.0f, 22.0f));
            curY -= 22.0f;
        }
    }

    if (wave >= 8) {
        PendulumBody pb;
        pb.anchor = {startX + (numBays * 0.5f) * baySpacing, 120.0f};
        pb.length = 135.0f;
        pb.spiked = true;
        pb.radius = 24.0f;
        pb.mass = 4.5f;
        spec.pendulums.push_back(pb);
    }

    loadCustomLevelSpec(spec);
}

void SnoutAndSiegeGame::startEndlessMode() {
    screen = ScreenState::ENDLESS;
    endlessWave = 1;
    endlessScore = 0;
    endlessComboMultiplier = 1.0f;
    endlessGameOver = false;
    generateEndlessWave(1);
}

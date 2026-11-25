// Campaign levels are data: assets/levels/level_001.cfg .. level_150.cfg.
// See level_format.h for the file format.
#include "game.h"
#include "level_format.h"

#include <cstdio>

void SnoutAndSiegeGame::calibrateStarThresholds(LevelSpec& L) {
    // 2-star and 3-star thresholds derived from the level's composition.
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

bool SnoutAndSiegeGame::initLevels(const std::string& dir) {
    levels.clear();
    levels.reserve(NUM_LEVELS);
    bool ok = true;
    int missing = 0;
    std::string firstMissing;
    for (int i = 1; i <= NUM_LEVELS; ++i) {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/level_%03d.cfg", dir.c_str(), i);
        std::string text, warnings;
        ParsedLevel parsed;
        if (!readTextFile(path, text)) {
            if (missing++ == 0) firstMissing = path;
            ok = false;
        } else if (!parseLevelText(text, parsed, &warnings)) {
            std::cerr << "[LEVELS] Could not read any level data from " << path << "\n";
            ok = false;
        }
        if (!warnings.empty()) std::cerr << "[LEVELS] " << path << ":\n" << warnings;
        if (!parsed.hasStarThresholds) calibrateStarThresholds(parsed.spec);
        if (parsed.spec.pigQueue.empty()) parsed.spec.pigQueue = {PigType::TROTTER};
        if (parsed.spec.name.empty()) parsed.spec.name = "LEVEL " + std::to_string(i);
        // Always push something so indices stay aligned with NUM_LEVELS.
        levels.push_back(std::move(parsed.spec));
    }
    if (missing > 0) {
        std::cerr << "[LEVELS] " << missing << " level file(s) missing, starting with " << firstMissing << "\n";
    }
    return ok;
}

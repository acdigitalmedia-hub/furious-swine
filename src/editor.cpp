#include "game.h"
#include "level_format.h"

void SnoutAndSiegeGame::saveCustomLevel(int slot) {
    if (slot < 1 || slot > 3) slot = 1;
    std::string filename = "custom_siege_" + std::to_string(slot) + ".cfg";
    // Same format as the campaign levels in assets/levels (see level_format.h).
    LevelSpec L = customLevel;
    L.name.clear();
    L.subtitle.clear();
    L.pigQueue.clear();  // test play always uses the full arsenal
    L.wind = editorWind;
    L.hasStorm = editorStorm;
    if (!writeTextFileAtomically(filename, serializeLevel(L, editorTheme))) {
        floatScores.push_back({{WINDOW_W * 0.5f, 160.0f}, "SAVE FAILED!", {255, 120, 120, 255}, 1.6f, 1.6f});
        return;
    }
    floatScores.push_back({{WINDOW_W * 0.5f, 160.0f}, "SIEGE SLOT " + std::to_string(slot) + " SAVED!", {140, 240, 180, 255}, 1.4f, 1.4f});
    assets.playSound("sound.ui_click");
}

PendulumBody SnoutAndSiegeGame::makeEditorPendulum(float ax, float ay, bool spiked, float length) {
    PendulumBody pb{};
    pb.anchor = {ax, ay};
    pb.length = length;
    pb.angle = 0.40f;
    pb.radius = 18.0f;
    pb.mass = 8.0f;
    pb.spiked = spiked;
    return pb;
}

void SnoutAndSiegeGame::loadCustomLevel(int slot) {
    if (slot < 1 || slot > 3) slot = 1;
    customSlot = slot;
    std::string filename = "custom_siege_" + std::to_string(slot) + ".cfg";

    std::string text;
    ParsedLevel parsed;
    std::string warnings;
    bool loaded = readTextFile(filename, text) && parseLevelText(text, parsed, &warnings);
    if (!warnings.empty()) std::cerr << "[EDITOR] " << filename << ":\n" << warnings;

    if (loaded) {
        customLevel = parsed.spec;
        editorTheme = (parsed.theme >= 0) ? parsed.theme : (slot - 1) % 6;
        editorWind = clampf(customLevel.wind, -120.0f, 120.0f);
        editorStorm = customLevel.hasStorm;
    } else {
        customLevel = LevelSpec{};
        editorTheme = (slot - 1) % 6;
        editorWind = 0.0f;
        editorStorm = false;
        customLevel.blocks.push_back(makeBlock(MaterialTier::STONE, 780.0f, GROUND_Y - 11.0f, 160.0f, 22.0f));
        customLevel.blocks.push_back(makeBlock(MaterialTier::TIMBER, 725.0f, GROUND_Y - 55.0f, 20.0f, 80.0f));
        customLevel.blocks.push_back(makeBlock(MaterialTier::TIMBER, 835.0f, GROUND_Y - 55.0f, 20.0f, 80.0f));
        customLevel.blocks.push_back(makeBlock(MaterialTier::TIMBER, 780.0f, GROUND_Y - 105.0f, 150.0f, 22.0f));

        customLevel.birds.push_back(makeBird(BirdClass::TOUCAN, 780.0f, GROUND_Y - 22.0f - 20.0f));
        customLevel.birds.push_back(makeBird(BirdClass::OWL, 780.0f, GROUND_Y - 116.0f - 23.0f));

        customLevel.hasTruffle = true;
        customLevel.trufflePos = {780.0f, GROUND_Y - 175.0f};
    }
    customLevel.wind = editorWind;
    customLevel.hasStorm = editorStorm;
    customLevel.name = "CUSTOM SIEGE " + std::to_string(slot);
    customLevel.subtitle = "FORTRESS ARCHITECT SANDBOX";
    customLevel.pigQueue = {PigType::TROTTER, PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::GLIDER};
    customBackup = customLevel;
}

void SnoutAndSiegeGame::initEditor() {
    isTestPlaying = false;
    editorTool = 1;
    editorMat = MaterialTier::TIMBER;
    editorBlockW = 60.0f;
    editorBlockH = 22.0f;
    editorBird = BirdClass::TOUCAN;
    editorBirdHelmet = false;
    editorSpikedPendulum = true;
    editorSnapGrid = true;
    loadCustomLevel(customSlot);
}

void SnoutAndSiegeGame::startEditorTestPlay() {
    // The wind and storm buttons only edit editorWind / editorStorm, so
    // apply them to the level being test-played.
    customLevel.wind = editorWind;
    customLevel.hasStorm = editorStorm;
    customBackup = customLevel;
    customLevel.pigQueue = {PigType::TROTTER, PigType::TURBO, PigType::SPLIT, PigType::BOMB, PigType::GLIDER, PigType::GRAVITON, PigType::DRILL};
    customLevel.name = "CUSTOM SIEGE PLAYTEST";
    customLevel.subtitle = "FORTRESS SANDBOX PROVING GROUND";
    isTestPlaying = true;
    loadCustomLevelSpec(customLevel);
}

void SnoutAndSiegeGame::returnToEditor() {
    isTestPlaying = false;
    customLevel = customBackup;
    activePigs.clear();
    particles.clear();
    floatScores.clear();
    aerialBombs.clear();
    simSpeed = 1.0f;
    levelComplete = false;
    levelFailed = false;
}

// Places, or erases, an element at a click on the Fortress Architect canvas.
void SnoutAndSiegeGame::handleEditorCanvasClick(float mx, float my) {
    // Canvas build interaction (clicks inside arena zone)
    if (mx > 218.0f && my > 72.0f && my < GROUND_Y + 15.0f) {
        float gx = editorSnapGrid ? (std::round(mx / 20.0f) * 20.0f) : mx;
        float gy = editorSnapGrid ? (std::round(my / 20.0f) * 20.0f) : my;

        if (editorTool == 0) { // BLOCK
            customLevel.blocks.push_back(makeBlock(editorMat, gx, gy, editorBlockW, editorBlockH));
            assets.playSound("sound.hit_timber", 0.55f);
        } else if (editorTool == 1) { // BIRD
            customLevel.birds.push_back(
                makeBird(editorBird, gx, gy, editorBirdHelmet));
            assets.playSound("sound.ui_click", 0.6f);
        } else if (editorTool == 2) { // PENDULUM
            customLevel.pendulums.push_back(
                makeEditorPendulum(gx, gy, editorSpikedPendulum));
            assets.playSound("sound.hit_metal", 0.6f);
        } else if (editorTool == 3) { // TRUFFLE
            customLevel.hasTruffle = true;
            customLevel.trufflePos = {gx, gy};
            assets.playSound("sound.truffle", 0.6f);
        } else if (editorTool == 4) { // ERASER
            bool erased = false;
            if (customLevel.hasTruffle && (customLevel.trufflePos - Vec2(mx, my)).length() < 32.0f) {
                customLevel.hasTruffle = false;
                erased = true;
            }
            if (!erased) {
                for (auto it = customLevel.birds.begin(); it != customLevel.birds.end(); ++it) {
                    if ((it->pos - Vec2(mx, my)).length() < it->radius + 14.0f) {
                        customLevel.birds.erase(it);
                        erased = true;
                        break;
                    }
                }
            }
            if (!erased) {
                for (auto it = customLevel.pendulums.begin(); it != customLevel.pendulums.end(); ++it) {
                    Vec2 bobPos = it->anchor + Vec2(std::sin(it->angle), std::cos(it->angle)) * it->length;
                    if ((it->anchor - Vec2(mx, my)).length() < 18.0f || (bobPos - Vec2(mx, my)).length() < it->radius + 10.0f) {
                        customLevel.pendulums.erase(it);
                        erased = true;
                        break;
                    }
                }
            }
            if (!erased) {
                for (auto it = customLevel.blocks.begin(); it != customLevel.blocks.end(); ++it) {
                    float hx = it->w * 0.5f + 8.0f, hy = it->h * 0.5f + 8.0f;
                    if (std::abs(mx - it->pos.x) <= hx && std::abs(my - it->pos.y) <= hy) {
                        customLevel.blocks.erase(it);
                        erased = true;
                        break;
                    }
                }
            }
            if (erased) {
                assets.playSound("sound.hit_twig", 0.5f);
            }
        }
    }
}

// First-time tips: a short card the first time a level introduces a pig or
// a mechanic. Each tip is shown once ever (remembered in the save file),
// and at most one appears per level start.
#include "game.h"

namespace {

struct HintDef {
    HintId id;
    const char* title;
    const char* body;
};

const HintDef kHints[] = {
    {HintId::AIM, "Pull, aim, fire!",
     "Drag the pig back in the catapult and let go to launch it. The dotted line shows where it will fly."},
    {HintId::ABILITY, "Special abilities",
     "Every pig has a trick. While it's flying, press Space or click to use it. Trotter slams down like an anvil."},
    {HintId::PIG_TURBO, "Meet Bramble",
     "Press Space mid-air for a Supersonic Dash: a burst of speed in a straight line. Great against timber."},
    {HintId::PIG_SPLIT, "Meet Clover",
     "Press Space mid-air to split into three pigs. Spread the damage across wide or icy forts."},
    {HintId::PIG_BOMB, "Meet Vulcan",
     "Vulcan explodes on Space, or shortly after hitting something. The blast hurts birds through walls."},
    {HintId::PIG_GLIDER, "Meet Gale",
     "Press Space to level out and drop two bombs as Gale glides over the fortress."},
    {HintId::PIG_GRAVITON, "Meet Atticus",
     "Press Space to open a vortex that pulls blocks and birds together, then collapses with a bang."},
    {HintId::PIG_DRILL, "Meet Rusty",
     "Press Space to drill straight through blocks. Wood and ice are easy; stone wears the drill out fast."},
    {HintId::WIND, "Mind the wind",
     "Wind pushes your pig sideways mid-flight. Check the arrow at the top right and aim to compensate."},
    {HintId::HELMET, "Iron helmets",
     "Helmeted birds shrug off the first hits. Knock the helmet off, then finish the job."},
    {HintId::SHIELD, "Shielded falcons",
     "Falcons hold a shield towards the catapult. Hit them from above or behind, or blast around the shield."},
    {HintId::ZEPPELIN, "Zeppelin condors",
     "Pop the balloon to bring a condor crashing down."},
    {HintId::TNT, "Powder kegs",
     "TNT explodes when hit hard, and sets off any kegs nearby. One good shot can bring the whole fort down."},
    {HintId::ICE, "Brittle ice",
     "Ice shatters easily, so knock out ice supports to drop whatever sits on top."},
    {HintId::TRAMPOLINE, "Trampolines",
     "Trampolines can't be broken. Pigs bounce off them faster than they arrived: bank shots off them!"},
    {HintId::MAGMA, "Magma vents",
     "Hit a vent to make it erupt. The lava melts nearby ice and lights powder kegs."},
    {HintId::STORM, "Storm clouds",
     "Lightning strikes the fortress now and then. Sometimes the storm does your work for you."},
    {HintId::PENDULUM, "Wrecking balls",
     "Knock a pendulum to swing it into the fortress."},
    {HintId::TRUFFLE, "Golden truffle",
     "A golden truffle is hidden in this level. Collect truffles to unlock the secret vaults."},
};

bool levelHas(const LevelSpec& L, HintId id) {
    auto hasPig = [&](PigType t) { return std::find(L.pigQueue.begin(), L.pigQueue.end(), t) != L.pigQueue.end(); };
    auto hasMat = [&](MaterialTier m) {
        return std::any_of(L.blocks.begin(), L.blocks.end(), [&](const BlockBody& b) { return b.mat == m; });
    };
    switch (id) {
    case HintId::AIM:
    case HintId::ABILITY: return true;
    case HintId::PIG_TURBO: return hasPig(PigType::TURBO);
    case HintId::PIG_SPLIT: return hasPig(PigType::SPLIT);
    case HintId::PIG_BOMB: return hasPig(PigType::BOMB);
    case HintId::PIG_GLIDER: return hasPig(PigType::GLIDER);
    case HintId::PIG_GRAVITON: return hasPig(PigType::GRAVITON);
    case HintId::PIG_DRILL: return hasPig(PigType::DRILL);
    case HintId::WIND: return std::abs(L.wind) >= 10.0f;
    case HintId::HELMET:
        return std::any_of(L.birds.begin(), L.birds.end(), [](const BirdTarget& b) { return b.hasHelmet; });
    case HintId::SHIELD:
        return std::any_of(L.birds.begin(), L.birds.end(), [](const BirdTarget& b) { return b.hasShield; });
    case HintId::ZEPPELIN:
        return std::any_of(L.birds.begin(), L.birds.end(), [](const BirdTarget& b) { return b.inZeppelin; });
    case HintId::TNT: return hasMat(MaterialTier::TNT);
    case HintId::ICE: return hasMat(MaterialTier::ICE);
    case HintId::TRAMPOLINE: return hasMat(MaterialTier::TRAMPOLINE);
    case HintId::MAGMA: return hasMat(MaterialTier::MAGMA_VENT);
    case HintId::STORM: return L.hasStorm;
    case HintId::PENDULUM: return !L.pendulums.empty();
    case HintId::TRUFFLE: return L.hasTruffle;
    case HintId::COUNT: break;
    }
    return false;
}

const HintDef* findDef(int id) {
    for (const auto& h : kHints) {
        if (static_cast<int>(h.id) == id) return &h;
    }
    return nullptr;
}

}  // namespace

void SnoutAndSiegeGame::chooseHintForLevel(const LevelSpec& L) {
    activeHint = -1;
    if (!tutorialHints) return;
    for (const auto& h : kHints) {
        int bit = static_cast<int>(h.id);
        if (!(hintsSeen & (1u << bit)) && levelHas(L, h.id)) {
            activeHint = bit;
            return;
        }
    }
}

void SnoutAndSiegeGame::markHintsSeenForLevel(const LevelSpec& L) {
    for (const auto& h : kHints) {
        if (levelHas(L, h.id)) hintsSeen |= 1u << static_cast<int>(h.id);
    }
}

void SnoutAndSiegeGame::dismissHint() {
    if (activeHint < 0) return;
    hintsSeen |= 1u << activeHint;
    activeHint = -1;
    saveProgress();
}

void SnoutAndSiegeGame::renderHintCard(SDL_Renderer* ren) {
    using namespace ui;
    const HintDef* def = findDef(activeHint);
    if (!def) return;
    activeButtons.clear();  // only the tip card is clickable
    SDL_SetRenderDrawColor(ren, 8, 6, 20, 110);
    SDL_FRect dim{0, 0, WINDOW_W, WINDOW_H};
    SDL_RenderFillRectF(ren, &dim);

    SDL_FRect card{WINDOW_W * 0.5f - 330.0f, 190.0f, 660.0f, 250.0f};
    panel(ren, card, 24.0f, {40, 44, 78, 245});

    // Illustration: the real game object in a spotlight.
    Vec2 ic(card.x + 120.0f, card.y + card.h * 0.5f);
    fillCircleGrad(ren, ic, 86.0f, {88, 110, 170, 255}, {52, 60, 104, 255}, 40);
    HintId id = static_cast<HintId>(activeHint);
    auto pig = [&](PigType t) {
        CharPose pose;
        renderOrigamiPigGeometry(ren, t, ic, 44.0f, 0.0f, pose);
    };
    auto block = [&](MaterialTier m, float w, float h) {
        BlockBody b = makeBlock(m, ic.x, ic.y + 10.0f, w, h);
        renderBlock(ren, b, {0, 0});
    };
    switch (id) {
    case HintId::AIM:
        for (int i = 1; i <= 6; ++i) {
            float tt = i / 6.0f;
            fillCircle(ren, {ic.x - 40 + 90 * tt, ic.y + 10 - 80 * tt + 60 * tt * tt}, 4.5f - i * 0.4f, pal::CREAM, 12);
        }
        renderOrigamiPigGeometry(ren, PigType::TROTTER, ic + Vec2(-38, 22), 30.0f, -0.3f);
        break;
    case HintId::ABILITY: {
        CharPose pose;
        pose.mood = Mood::YELL;
        renderOrigamiPigGeometry(ren, PigType::TROTTER, ic, 44.0f, 0.4f, pose);
        break;
    }
    case HintId::PIG_TURBO: pig(PigType::TURBO); break;
    case HintId::PIG_SPLIT: pig(PigType::SPLIT); break;
    case HintId::PIG_BOMB: pig(PigType::BOMB); break;
    case HintId::PIG_GLIDER: pig(PigType::GLIDER); break;
    case HintId::PIG_GRAVITON: pig(PigType::GRAVITON); break;
    case HintId::PIG_DRILL: pig(PigType::DRILL); break;
    case HintId::WIND:
        for (int k = -1; k <= 1; ++k) {
            drawThickLine(ren, {ic.x - 48, ic.y + k * 24.0f}, {ic.x + 30, ic.y + k * 24.0f}, 7.0f, pal::CREAM);
            drawTriangle(ren, {ic.x + 50, ic.y + k * 24.0f}, {ic.x + 26, ic.y + k * 24.0f - 13},
                         {ic.x + 26, ic.y + k * 24.0f + 13}, pal::CREAM);
        }
        break;
    case HintId::HELMET:
        renderTotemBirdGeometry(ren, BirdClass::OWL, ic + Vec2(0, 10), 42.0f, 0.0f, 1.0f, true);
        break;
    case HintId::SHIELD:
        renderTotemBirdGeometry(ren, BirdClass::FALCON, ic + Vec2(10, 6), 40.0f, 0.0f, 1.0f, false, 1.0f, true);
        break;
    case HintId::ZEPPELIN:
        renderTotemBirdGeometry(ren, BirdClass::CONDOR, ic + Vec2(0, 34), 30.0f, 0.0f, 1.0f, false, 1.0f, false, 1.0f, true);
        break;
    case HintId::TNT: block(MaterialTier::TNT, 60, 60); break;
    case HintId::ICE: block(MaterialTier::ICE, 40, 100); break;
    case HintId::TRAMPOLINE: block(MaterialTier::TRAMPOLINE, 120, 26); break;
    case HintId::MAGMA: block(MaterialTier::MAGMA_VENT, 70, 50); break;
    case HintId::STORM:
        for (Vec2 o : {Vec2(-28, -14), Vec2(4, -26), Vec2(30, -10), Vec2(0, -4)}) fillCircle(ren, ic + o, 26.0f, {92, 98, 126, 255}, 24);
        drawConvexPoly(ren, {ic + Vec2(-4, 8), ic + Vec2(14, 8), ic + Vec2(2, 34), ic + Vec2(-12, 34)}, pal::GOLD);
        drawConvexPoly(ren, {ic + Vec2(-12, 30), ic + Vec2(10, 30), ic + Vec2(-10, 64)}, pal::GOLD);
        break;
    case HintId::PENDULUM:
        drawThickLine(ren, ic + Vec2(0, -70), ic + Vec2(22, 20), 3.0f, {200, 200, 220, 255});
        fillCircle(ren, ic + Vec2(0, -70), 6.0f, {200, 200, 220, 255}, 12);
        fillCircleGrad(ren, ic + Vec2(24, 26), 30.0f, {150, 156, 176, 255}, {70, 74, 92, 255}, 28);
        break;
    case HintId::TRUFFLE:
        fillCircleGrad(ren, ic, 34.0f, tint(pal::GOLD, 0.4f), pal::GOLD_DARK, 32);
        icon(ren, UiIcon::TRUFFLE, ic, 44.0f, {150, 92, 30, 255});
        break;
    case HintId::COUNT:
        break;
    }

    float tx = card.x + 238.0f, tw = card.w - 268.0f;
    TextStyle kick = textStyle(FontFace::BOLD, 15.0f, pal::GOLD);
    drawTextStyled(ren, "New!", tx, card.y + 28.0f, kick);
    title(ren, def->title, tx, card.y + 46.0f, 34.0f, pal::CREAM, TextAlign::LEFT);
    TextStyle body = textStyle(FontFace::BODY, 17.0f, {214, 218, 240, 255});
    drawTextWrapped(ren, def->body, tx, card.y + 96.0f, tw, body, 1.15f);
    uiButton(ren, "DISMISS_HINT", {card.x + card.w - 178.0f, card.y + card.h - 78.0f, 150.0f, 54.0f}, "Got it!",
             pal::GREEN, UiIcon::CHECK);
}

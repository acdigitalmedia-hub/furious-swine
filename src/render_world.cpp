#include "game.h"

#include "scenery.h"

int SnoutAndSiegeGame::getActiveChapter() const {
    if (screen == ScreenState::EDITOR) return std::clamp(editorTheme, 0, NUM_CHAPTERS);
    // Menus show the chapter you're browsing (level select) or have reached (title).
    if (screen == ScreenState::LEVEL_SELECT) return std::clamp(levelSelectPage, 0, NUM_CHAPTERS);
    if (screen == ScreenState::TITLE || screen == ScreenState::ACHIEVEMENTS) {
        return std::clamp(getHighestUnlockedLevel() / LEVELS_PER_CHAPTER, 0, NUM_CHAPTERS - 1);
    }
    if (screen == ScreenState::ENDLESS) return (endlessWave - 1) % (NUM_CHAPTERS + 1);
    if (currentLevelIdx >= NUM_CAMPAIGN_LEVELS) return 14;
    return clampf(currentLevelIdx / 10, 0, 13);
}

void SnoutAndSiegeGame::renderBackgroundBackdrop(SDL_Renderer* ren, Vec2 cam) {
    renderScenery(ren, getActiveChapter(), cam, SDL_GetTicks() * 0.001f);
}

namespace {

// A rounded wooden limb with a dark outline and a lit edge.
void woodLimb(SDL_Renderer* ren, Vec2 a, Vec2 b, float w) {
    const SDL_Color outline{52, 30, 20, 255}, wood{150, 96, 56, 255}, light{196, 136, 84, 255};
    drawThickLine(ren, a, b, w + 5.0f, outline);
    ui::fillCircle(ren, a, (w + 5.0f) * 0.5f, outline, 14);
    ui::fillCircle(ren, b, (w + 5.0f) * 0.5f, outline, 14);
    drawThickLine(ren, a, b, w, wood);
    ui::fillCircle(ren, a, w * 0.5f, wood, 14);
    ui::fillCircle(ren, b, w * 0.5f, wood, 14);
    Vec2 d = (b - a).normalized();
    Vec2 n(-d.y, d.x);
    drawThickLine(ren, a + n * (w * 0.22f), b + n * (w * 0.22f), w * 0.22f, light);
}

void leatherWrap(SDL_Renderer* ren, Vec2 a, Vec2 b, float t, float w) {
    Vec2 p = a + (b - a) * t;
    Vec2 d = (b - a).normalized();
    Vec2 n(-d.y, d.x);
    drawThickLine(ren, p - n * (w * 0.6f) - d * 4.0f, p + n * (w * 0.6f) - d * 4.0f, 3.0f, {92, 50, 30, 255});
    drawThickLine(ren, p - n * (w * 0.6f) + d * 2.0f, p + n * (w * 0.6f) + d * 2.0f, 3.0f, {92, 50, 30, 255});
}

}  // namespace

void drawCatapultFrontArm(SDL_Renderer* ren, float sx, float sy) {
    Vec2 fork(sx, sy + 30.0f), tip(sx - 18.0f, sy - 12.0f);
    woodLimb(ren, fork, tip, 11.0f);
    leatherWrap(ren, fork, tip, 0.78f, 11.0f);
}

void SnoutAndSiegeGame::renderSiegeCatapultBase(SDL_Renderer* ren, float sx, float sy, float groundY) {
    // Stone footing.
    ui::fillCircle(ren, {sx - 20.0f, groundY - 4.0f}, 14.0f, {88, 90, 104, 255}, 18);
    ui::fillCircle(ren, {sx + 18.0f, groundY - 3.0f}, 12.0f, {104, 106, 120, 255}, 18);
    ui::fillCircle(ren, {sx - 2.0f, groundY - 8.0f}, 16.0f, {120, 122, 138, 255}, 20);
    ui::fillCircle(ren, {sx - 7.0f, groundY - 13.0f}, 5.0f, {160, 162, 176, 255}, 12);
    // Trunk and the back fork arm (the front arm is drawn over the pig).
    Vec2 fork(sx, sy + 30.0f);
    woodLimb(ren, {sx, groundY - 14.0f}, fork, 16.0f);
    Vec2 backTip(sx + 18.0f, sy - 12.0f);
    woodLimb(ren, fork, backTip, 11.0f);
    leatherWrap(ren, fork, backTip, 0.78f, 11.0f);
}

void SnoutAndSiegeGame::render(SDL_Renderer* ren) {
    activeButtons.clear();
    Vec2 cam = (screen == ScreenState::PLAYING || (screen == ScreenState::EDITOR && isTestPlaying) || screen == ScreenState::ENDLESS) ? (camPos + camShakeOffset) : Vec2(0.0f, 0.0f);
    renderBackgroundBackdrop(ren, cam);

    if (screen == ScreenState::TITLE) {
        renderTitleScreen(ren);
        renderAchievementToast(ren);
        return;
    }
    if (screen == ScreenState::LEVEL_SELECT) {
        renderLevelSelectScreen(ren);
        renderAchievementToast(ren);
        return;
    }
    if (screen == ScreenState::ACHIEVEMENTS) {
        renderAchievementsScreen(ren);
        renderAchievementToast(ren);
        return;
    }
    if (screen == ScreenState::EDITOR && !isTestPlaying) {
        renderEditorScreen(ren);
        renderAchievementToast(ren);
        return;
    }

    // ScreenState::PLAYING
    // Drifting cumulonimbus storm clouds
    for (const auto& sc : stormClouds) {
        Vec2 c = sc.pos - cam * 0.35f;
        float hw = sc.width * 0.5f, hh = sc.height * 0.5f;
        const SDL_Color body{58, 64, 86, 235}, under{42, 46, 64, 240}, rim{96, 104, 132, 235};
        ui::fillRoundRect(ren, {c.x - hw * 0.8f, c.y - hh * 0.1f, hw * 1.6f, hh * 0.9f}, hh * 0.45f, under);
        ui::fillCircle(ren, {c.x - hw * 0.42f, c.y + hh * 0.05f}, hh * 0.72f, under, 20);
        ui::fillCircle(ren, {c.x + hw * 0.45f, c.y + hh * 0.1f}, hh * 0.66f, under, 20);
        ui::fillCircle(ren, {c.x - hw * 0.12f, c.y - hh * 0.42f}, hh * 0.86f, rim, 24);
        ui::fillCircle(ren, {c.x + hw * 0.25f, c.y - hh * 0.25f}, hh * 0.72f, rim, 22);
        ui::fillCircle(ren, {c.x - hw * 0.12f, c.y - hh * 0.34f}, hh * 0.8f, body, 24);
        ui::fillCircle(ren, {c.x + hw * 0.25f, c.y - hh * 0.18f}, hh * 0.66f, body, 22);
        ui::fillCircle(ren, {c.x - hw * 0.45f, c.y - hh * 0.02f}, hh * 0.6f, body, 20);
        ui::fillRoundRect(ren, {c.x - hw * 0.72f, c.y - hh * 0.05f, hw * 1.44f, hh * 0.6f}, hh * 0.3f, body);
    }

    // Active electric lightning bolts
    for (const auto& bolt : lightningBolts) {
        float bAlpha = clampf(bolt.life / bolt.maxLife, 0.0f, 1.0f);
        if (bolt.points.size() < 2) continue;
        SDL_Color coreCol = bolt.color;
        coreCol.a = static_cast<uint8_t>(255.0f * bAlpha);
        SDL_Color glowCol{180, 230, 255, static_cast<uint8_t>(140.0f * bAlpha)};
        for (size_t i = 0; i + 1 < bolt.points.size(); ++i) {
            Vec2 p0 = bolt.points[i] - cam;
            Vec2 p1 = bolt.points[i + 1] - cam;
            drawThickLine(ren, p0, p1, bolt.width * 2.2f, glowCol);
            drawThickLine(ren, p0, p1, bolt.width, coreCol);
        }
    }

    float screenGroundY = GROUND_Y - cam.y;
    for (size_t i = 0; i < remainingPigs.size(); ++i) {
        Vec2 qPos(125.0f - i * 42.0f - cam.x, screenGroundY - 18.0f);
        drawConvexPoly(ren, {{qPos.x - 15, screenGroundY + 2}, {qPos.x + 15, screenGroundY + 2},
                             {qPos.x + 11, screenGroundY + 6}, {qPos.x - 11, screenGroundY + 6}}, {15, 18, 24, 110});
        renderOrigamiPigGeometry(ren, remainingPigs[i], qPos, 17.0f, 0.0f);
    }

    renderSiegeCatapultBase(ren, SLING_X - cam.x, SLING_Y - cam.y, screenGroundY);

    // Previous Shot Ghost Trail Dots (Trajectory Memory)
    if (!ghostTrajectory.empty()) {
        for (size_t i = 0; i < ghostTrajectory.size(); ++i) {
            Vec2 gPt = ghostTrajectory[i] - cam;
            if (gPt.y > screenGroundY) continue;
            // Delicate translucent parchment ghost marker
            drawNGon(ren, gPt, (i % 2 == 0) ? 3.5f : 2.2f, 4, (i * 0.25f), {255, 235, 160, 95});
            drawNGon(ren, gPt, 1.4f, 4, 0.0f, {32, 26, 20, 120});
        }
    }

    // Catapult back elastic band
    bool hasAimingPig = (!activePigs.empty() && !activePigs[0].launched);
    Vec2 pouchPos = slingPullPos;
    if (hasAimingPig) {
        pouchPos = activePigs[0].pos;
        drawThickLine(ren, {SLING_X + 18.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 8.0f, {60, 30, 24, 255});
        drawThickLine(ren, {SLING_X + 18.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 5.0f, {150, 70, 52, 255});
    } else {
        Vec2 recoilOffset{0.0f, 0.0f};
        if (slingRecoilTimer > 0.0f) {
            float decay = slingRecoilTimer / 0.35f;
            float vibe = std::sin(slingRecoilTimer * 55.0f) * decay * 14.0f;
            recoilOffset = lastLaunchDir * vibe;
        }
        pouchPos = Vec2(SLING_X, SLING_Y) + recoilOffset;
        drawThickLine(ren, {SLING_X + 18.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 7.0f, {60, 30, 24, 255});
        drawThickLine(ren, {SLING_X + 18.0f - cam.x, SLING_Y - 8.0f - cam.y}, pouchPos - cam, 4.0f, {150, 70, 52, 255});
    }

    if (draggingSling && hasAimingPig) {
        Vec2 pull = slingPullPos - Vec2(SLING_X, SLING_Y);
        if (pull.length() > 12.0f) {
            Vec2 v0 = pull * (-LAUNCH_POWER);
            for (int i = 1; i <= 26; ++i) {
                float t = i * 0.050f;
                Vec2 pt(SLING_X + v0.x * t + 0.5f * (currentWind * 0.85f) * t * t - cam.x,
                        SLING_Y + v0.y * t + 0.5f * GRAVITY * t * t - cam.y);
                if (pt.y > screenGroundY) break;
                uint8_t alpha = static_cast<uint8_t>(std::max(40, 245 - i * 8));
                float r = std::max(2.5f, 6.0f - i * 0.13f);
                ui::fillCircle(ren, pt, r + 1.8f, {24, 22, 40, static_cast<uint8_t>(alpha * 0.55f)}, 14);
                ui::fillCircle(ren, pt, r, {255, 255, 255, alpha}, 14);
            }
        }
    }

    for (const auto& b : blocks) {
        if (b.destroyed) continue;
        renderBlock(ren, b, cam);
    }

    for (const auto& pend : pendulums) {
        renderPendulum(ren, pend, cam);
    }

    if (levelTruffle.exists && !levelTruffle.collected) {
        renderGoldenTruffle(ren, cam);
    }

    for (const auto& t : birds) {
        if (t.defeated) continue;
        renderBird(ren, t, cam);
    }

    for (const auto& p : activePigs) {
        for (size_t i = 0; i < p.trail.size(); ++i) {
            float trailFrac = static_cast<float>(i) / std::max(1, (int)p.trail.size());
            uint8_t alpha = static_cast<uint8_t>(50 + 190 * trailFrac);
            drawNGon(ren, p.trail[i] - cam, (i % 2 == 0) ? 4.2f : 2.8f, 6, 0.0f, {255, 242, 210, alpha});
        }
        if (p.active) {
            renderPig(ren, p, cam);
        }
    }

    // Render Falling Aerial Bombs
    for (const auto& ab : aerialBombs) {
        if (!ab.active) continue;
        Vec2 bPos = ab.pos - cam;
        drawNGon(ren, bPos, ab.radius, 10, ab.lifeTimer * 8.0f, {34, 38, 48, 255});
        drawNGon(ren, bPos, ab.radius * 0.72f, 10, ab.lifeTimer * 8.0f, {58, 64, 78, 255});
        drawNGon(ren, bPos + Vec2(-ab.radius * 0.25f, -ab.radius * 0.25f), ab.radius * 0.28f, 6, 0.0f, {180, 195, 215, 255});
        drawNGon(ren, bPos + Vec2(0.0f, -ab.radius * 0.9f), ab.radius * 0.35f, 4, 0.0f, {218, 175, 75, 255});
    }

    // Catapult front elastic band & leather cup
    {
        Vec2 anchor(SLING_X - 16.0f - cam.x, SLING_Y - 8.0f - cam.y);
        Vec2 pouch = pouchPos - cam - (hasAimingPig ? Vec2(4.0f, 0.0f) : Vec2(0.0f, 0.0f));
        drawThickLine(ren, anchor, pouch, hasAimingPig ? 9.0f : 8.0f, {60, 30, 24, 255});
        drawThickLine(ren, anchor, pouch, hasAimingPig ? 6.0f : 5.0f, {186, 88, 64, 255});
        ui::fillCircle(ren, pouch, hasAimingPig ? 11.0f : 9.0f, {52, 30, 20, 255}, 16);
        ui::fillCircle(ren, pouch, hasAimingPig ? 8.5f : 6.5f, {116, 70, 42, 255}, 16);
    }
    drawCatapultFrontArm(ren, SLING_X - cam.x, SLING_Y - cam.y);

    for (const auto& pt : particles) {
        float alphaRatio = clampf(pt.life / pt.maxLife, 0.0f, 1.0f);
        SDL_Color col = pt.color;
        col.a = static_cast<uint8_t>(255.0f * alphaRatio);
        Vec2 sPos = pt.pos - cam;

        if (pt.isRing || pt.kind == ParticleKind::RING) {
            int segs = 28;
            for (int i = 0; i < segs; ++i) {
                float a0 = (2.0f * PI * i) / segs;
                float a1 = (2.0f * PI * (i + 1)) / segs;
                Vec2 p0(sPos.x + std::cos(a0) * pt.ringRadius, sPos.y + std::sin(a0) * pt.ringRadius);
                Vec2 p1(sPos.x + std::cos(a1) * pt.ringRadius, sPos.y + std::sin(a1) * pt.ringRadius);
                drawThickLine(ren, p0, p1, 6.0f * alphaRatio, col);
            }
        } else if (pt.kind == ParticleKind::FEATHER) {
            // Fluttering Feather with Quill Spine and Barb Vanes
            float c = std::cos(pt.angle), s = std::sin(pt.angle);
            float fLen = pt.size * 3.4f;
            float fWid = pt.size * 1.5f;
            Vec2 q0(sPos.x - c * fLen * 0.5f, sPos.y - s * fLen * 0.5f);
            Vec2 q1(sPos.x + c * fLen * 0.5f, sPos.y + s * fLen * 0.5f);
            Vec2 perp(-s * fWid, c * fWid);

            drawTriangle(ren, q0, q0 + perp * 0.85f, q1, col);
            SDL_Color darkVane = {
                static_cast<uint8_t>(std::max(0, col.r - 25)),
                static_cast<uint8_t>(std::max(0, col.g - 25)),
                static_cast<uint8_t>(std::max(0, col.b - 25)),
                col.a
            };
            drawTriangle(ren, q0, q0 - perp * 0.85f, q1, darkVane);

            SDL_Color quillCol = {
                static_cast<uint8_t>(std::min(255, col.r + 55)),
                static_cast<uint8_t>(std::min(255, col.g + 55)),
                static_cast<uint8_t>(std::min(255, col.b + 55)),
                col.a
            };
            drawThickLine(ren, q0, q1, 1.8f, quillCol);
        } else if (pt.kind == ParticleKind::WOOD_SPLINTER) {
            // Elongated Tumbling Wood / Wicker Sliver
            float c = std::cos(pt.angle), s = std::sin(pt.angle);
            float hLen = pt.size * 2.2f;
            Vec2 s0(sPos.x - c * hLen, sPos.y - s * hLen);
            Vec2 s1(sPos.x + c * hLen, sPos.y + s * hLen);
            drawThickLine(ren, s0, s1, 2.5f, col);
            drawThickLine(ren, {s0.x + 1.0f, s0.y + 1.0f}, {s1.x + 1.0f, s1.y + 1.0f}, 1.2f, {24, 18, 12, col.a});
        } else if (pt.kind == ParticleKind::TIMBER_CHUNK || pt.kind == ParticleKind::STONE_SHARD) {
            // Angular Tumbling Block Chunk / Pebble
            drawNGon(ren, sPos, pt.size * alphaRatio, 5, pt.angle, col);
            drawNGon(ren, sPos, pt.size * alphaRatio * 0.6f, 5, pt.angle, {255, 255, 255, static_cast<uint8_t>(col.a * 0.4f)});
        } else if (pt.kind == ParticleKind::SMOKE_PUFF) {
            // Soft expanding circular dust cloud
            float pRatio = 1.0f - (pt.life / pt.maxLife);
            float curSize = pt.size * (0.85f + pRatio * 1.5f);
            SDL_Color pCol = col;
            pCol.a = static_cast<uint8_t>(col.a * 0.45f * (1.0f - pRatio));
            drawNGon(ren, sPos, curSize, 8, pt.angle, pCol);
            drawNGon(ren, sPos, curSize * 0.65f, 8, -pt.angle, pCol);
        } else if (pt.kind == ParticleKind::SPARK_GLINT) {
            // 4-point golden star glint
            float gSize = pt.size * (0.6f + 0.4f * std::sin(alphaRatio * PI));
            drawThickLine(ren, {sPos.x - gSize, sPos.y}, {sPos.x + gSize, sPos.y}, 2.0f, col);
            drawThickLine(ren, {sPos.x, sPos.y - gSize}, {sPos.x, sPos.y + gSize}, 2.0f, col);
            drawNGon(ren, sPos, gSize * 0.35f, 4, 0.0f, {255, 255, 255, col.a});
        } else if (pt.kind == ParticleKind::ICE_CRYSTAL) {
            // Sharp Crystalline Diamond / Frost Shard with Specular Sparkle
            float c = std::cos(pt.angle), s = std::sin(pt.angle);
            float r = pt.size * alphaRatio * 1.35f;
            Vec2 pTop(sPos.x - s * r * 1.35f, sPos.y + c * r * 1.35f);
            Vec2 pBot(sPos.x + s * r * 1.35f, sPos.y - c * r * 1.35f);
            Vec2 pLeft(sPos.x - c * r * 0.7f, sPos.y - s * r * 0.7f);
            Vec2 pRight(sPos.x + c * r * 0.7f, sPos.y + s * r * 0.7f);
            drawTriangle(ren, pTop, pLeft, pRight, col);
            drawTriangle(ren, pBot, pLeft, pRight, {static_cast<uint8_t>(col.r * 0.78f),
                                                    static_cast<uint8_t>(col.g * 0.82f),
                                                    static_cast<uint8_t>(col.b), col.a});
            drawNGon(ren, sPos, r * 0.35f, 4, 0.0f, {255, 255, 255, col.a});
        } else if (pt.kind == ParticleKind::HELMET_DEBRIS) {
            // Tumbling Iron Cauldron Helmet Piece
            float c = std::cos(pt.angle), s = std::sin(pt.angle);
            auto rPt = [&](float lx, float ly) -> Vec2 {
                return {sPos.x + (lx * c - ly * s) * pt.size, sPos.y + (lx * s + ly * c) * pt.size};
            };
            drawConvexPoly(ren, {rPt(-0.6f, 0.4f), rPt(0.6f, 0.4f), rPt(0.4f, -0.7f), rPt(-0.4f, -0.7f)},
                           {75, 82, 96, col.a});
            drawThickLine(ren, rPt(-0.75f, 0.4f), rPt(0.75f, 0.4f), 3.0f, {135, 142, 158, col.a});
            drawNGon(ren, rPt(0.0f, -0.8f), pt.size * 0.18f, 4, pt.angle, {218, 175, 75, col.a});
        } else if (pt.kind == ParticleKind::EMBER) {
            // Fiery Rising Volcanic Ember with Molten Core & Outer Halo
            float r = pt.size * (0.6f + 0.4f * alphaRatio);
            SDL_Color halo = col;
            halo.a = static_cast<uint8_t>(col.a * 0.35f);
            drawNGon(ren, sPos, r * 2.0f, 6, pt.angle, halo);
            drawNGon(ren, sPos, r, 6, pt.angle, col);
        } else if (pt.kind == ParticleKind::GOLD_SPARKLE) {
            // Radiating celestial golden star sparkle
            float gSize = pt.size * (0.7f + 0.5f * std::sin(alphaRatio * PI));
            drawThickLine(ren, {sPos.x - gSize, sPos.y}, {sPos.x + gSize, sPos.y}, 2.5f, col);
            drawThickLine(ren, {sPos.x, sPos.y - gSize}, {sPos.x, sPos.y + gSize}, 2.5f, col);
            float dSize = gSize * 0.65f;
            drawThickLine(ren, {sPos.x - dSize, sPos.y - dSize}, {sPos.x + dSize, sPos.y + dSize}, 1.5f, {255, 235, 120, col.a});
            drawThickLine(ren, {sPos.x - dSize, sPos.y + dSize}, {sPos.x + dSize, sPos.y - dSize}, 1.5f, {255, 235, 120, col.a});
            drawNGon(ren, sPos, gSize * 0.4f, 4, pt.angle, {255, 255, 255, col.a});
        } else if (pt.kind == ParticleKind::SHIELD_DEBRIS) {
            // Tumbling reinforced tower shield fragment
            float c = std::cos(pt.angle), s = std::sin(pt.angle);
            auto rPt = [&](float lx, float ly) -> Vec2 {
                return {sPos.x + (lx * c - ly * s) * pt.size, sPos.y + (lx * s + ly * c) * pt.size};
            };
            drawConvexPoly(ren, {rPt(-0.5f, -0.6f), rPt(0.5f, -0.6f), rPt(0.3f, 0.6f), rPt(-0.3f, 0.6f)},
                           {78, 88, 106, col.a});
            drawThickLine(ren, rPt(-0.4f, 0.0f), rPt(0.4f, 0.0f), 2.2f, {230, 185, 65, col.a});
        } else if (pt.kind == ParticleKind::BALLOON_SCRAP) {
            // Floating shredded zeppelin canvas scrap
            float c = std::cos(pt.angle), s = std::sin(pt.angle);
            auto rPt = [&](float lx, float ly) -> Vec2 {
                return {sPos.x + (lx * c - ly * s) * pt.size, sPos.y + (lx * s + ly * c) * pt.size};
            };
            drawTriangle(ren, rPt(-0.6f, -0.4f), rPt(0.6f, -0.2f), rPt(0.1f, 0.5f), {185, 145, 95, col.a});
            drawThickLine(ren, rPt(-0.6f, -0.4f), rPt(0.1f, 0.5f), 1.5f, {125, 85, 45, col.a});
        } else if (pt.kind == ParticleKind::COSMIC_MOTE) {
            // Swirling violet/cyan gravitational vortex mote
            float r = pt.size * (0.8f + 0.4f * std::sin(alphaRatio * PI));
            drawNGon(ren, sPos, r * 1.8f, 6, pt.angle, {180, 80, 255, static_cast<uint8_t>(col.a * 0.45f)});
            drawNGon(ren, sPos, r * 0.9f, 6, -pt.angle, {110, 230, 255, col.a});
            drawNGon(ren, sPos, r * 0.4f, 4, 0.0f, {255, 255, 255, col.a});
        } else {
            drawNGon(ren, sPos, pt.size * alphaRatio, 4, pt.life * 6.0f, col);
        }
    }

    for (const auto& fs : floatScores) {
        float pRatio = clampf(1.0f - fs.life / fs.maxLife, 0.0f, 1.0f);
        float popScale = 2.0f;
        if (pRatio < 0.22f) {
            popScale = 2.0f + 0.95f * std::sin((pRatio / 0.22f) * (PI * 0.5f));
        } else {
            popScale = 2.0f + 0.95f * std::cos(((pRatio - 0.22f) / 0.78f) * (PI * 0.5f));
        }
        float sx = fs.pos.x - cam.x;
        float sy = fs.pos.y - cam.y;
        uint8_t alpha = static_cast<uint8_t>(clampf(fs.life / fs.maxLife, 0.0f, 1.0f) * 255.0f);
        SDL_Color txtCol = fs.color;
        txtCol.a = alpha;
        ui::title(ren, fs.text, sx, sy - popScale * 4.0f, popScale * 11.0f, txtCol, TextAlign::CENTER,
                  {24, 22, 40, alpha});
    }

    // Full-screen lightning flash illumination
    if (skyFlashAlpha > 0.01f) {
        uint8_t flashA = static_cast<uint8_t>(clampf(skyFlashAlpha * 175.0f, 0.0f, 255.0f));
        SDL_SetRenderDrawColor(ren, 230, 245, 255, flashA);
        SDL_FRect screenRect{0.0f, 0.0f, static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)};
        SDL_RenderFillRectF(ren, &screenRect);
    }

    renderHUD(ren);
    if (paused) renderPauseMenu(ren);
    else if (activeHint >= 0) renderHintCard(ren);
    renderAchievementToast(ren);

    // Feature Pack E: Cinematic Slo-Mo Letterboxing Bars & Banner
    if (letterboxY > 0.5f) {
        SDL_SetRenderDrawColor(ren, 12, 14, 20, 255);
        SDL_FRect topBarRect{0.0f, 0.0f, static_cast<float>(WINDOW_W), letterboxY};
        SDL_FRect btmBarRect{0.0f, static_cast<float>(WINDOW_H) - letterboxY, static_cast<float>(WINDOW_W), letterboxY};
        SDL_RenderFillRectF(ren, &topBarRect);
        SDL_RenderFillRectF(ren, &btmBarRect);
        if (sloMoActive) {
            ui::title(ren, "Avian Reckoning!", WINDOW_W * 0.5f, letterboxY * 0.5f - 22.0f, 30.0f, ui::pal::GOLD);
        }
    }
}

void SnoutAndSiegeGame::renderBlock(SDL_Renderer* ren, const BlockBody& b, Vec2 cam) {
    Vec2 screenPos = b.pos - cam;
    float c = std::cos(b.angle), s = std::sin(b.angle);
    auto rotPt = [&](float lx, float ly) -> Vec2 {
        return {screenPos.x + lx * c - ly * s, screenPos.y + lx * s + ly * c};
    };
    float hw = b.w * 0.5f, hh = b.h * 0.5f;
    Vec2 p0 = rotPt(-hw, -hh), p1 = rotPt(hw, -hh), p2 = rotPt(hw, hh), p3 = rotPt(-hw, hh);

    // Soft drop shadow behind block
    Vec2 off(4.0f, 5.0f);
    drawConvexPoly(ren, {p0 + off, p1 + off, p2 + off, p3 + off}, {12, 14, 22, 85});

    std::string texKey = (b.mat == MaterialTier::TWIG)   ? "sprite.mat_twig" :
                         (b.mat == MaterialTier::TIMBER ? "sprite.mat_timber" :
                         (b.mat == MaterialTier::STONE  ? "sprite.mat_stone" :
                         (b.mat == MaterialTier::ICE    ? "sprite.mat_ice" :
                         (b.mat == MaterialTier::TNT    ? "sprite.mat_tnt" :
                         (b.mat == MaterialTier::MAGMA_VENT ? "sprite.mat_magma" : "sprite.mat_trampoline")))));
    SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture(texKey) : nullptr;
    if (tex) {
        SDL_FRect dst{screenPos.x - b.w * 0.5f, screenPos.y - b.h * 0.5f, b.w, b.h};
        SDL_RenderCopyExF(ren, tex, nullptr, &dst, b.angle * (180.0 / PI), nullptr, SDL_FLIP_NONE);
    } else {
        SDL_Color fill = (b.mat == MaterialTier::TWIG)   ? SDL_Color{216, 180, 102, 255} :
                         (b.mat == MaterialTier::TIMBER ? SDL_Color{156, 90, 52, 255} :
                         (b.mat == MaterialTier::STONE  ? SDL_Color{94, 106, 122, 255} :
                         (b.mat == MaterialTier::ICE    ? SDL_Color{170, 230, 250, 230} :
                         (b.mat == MaterialTier::TNT    ? SDL_Color{192, 45, 36, 255} :
                         (b.mat == MaterialTier::MAGMA_VENT ? SDL_Color{42, 34, 38, 255} :
                                                           SDL_Color{36, 115, 82, 255})))));
        drawConvexPoly(ren, {p0, p1, p2, p3}, fill);
    }

    if (b.mat == MaterialTier::ICE) {
        Vec2 midL = rotPt(-hw, 0.0f);
        Vec2 midR = rotPt(hw, 0.0f);
        Vec2 midT = rotPt(0.0f, -hh);
        Vec2 midB = rotPt(0.0f, hh);
        drawThickLine(ren, midL, midT, 1.4f, {235, 250, 255, 170});
        drawThickLine(ren, midT, midR, 1.4f, {235, 250, 255, 170});
        drawThickLine(ren, midR, midB, 1.4f, {120, 195, 230, 190});
        drawThickLine(ren, midB, midL, 1.4f, {120, 195, 230, 190});
        drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 2.0f), rotPt(hw - 2.0f, -hh + 2.0f), 2.0f, {255, 255, 255, 220});
        drawThickLine(ren, p0, p1, 2.2f, {140, 215, 240, 245});
        drawThickLine(ren, p1, p2, 2.2f, {80, 168, 200, 245});
        drawThickLine(ren, p2, p3, 2.2f, {80, 168, 200, 245});
        drawThickLine(ren, p3, p0, 2.2f, {140, 215, 240, 245});
    } else if (b.mat == MaterialTier::TNT) {
        drawThickLine(ren, rotPt(-hw, -hh * 0.45f), rotPt(hw, -hh * 0.45f), 3.0f, {34, 38, 44, 255});
        drawThickLine(ren, rotPt(-hw,  hh * 0.45f), rotPt(hw,  hh * 0.45f), 3.0f, {34, 38, 44, 255});
        drawThickLine(ren, p0, p1, 2.3f, {48, 20, 18, 255});
        drawThickLine(ren, p1, p2, 2.3f, {48, 20, 18, 255});
        drawThickLine(ren, p2, p3, 2.3f, {48, 20, 18, 255});
        drawThickLine(ren, p3, p0, 2.3f, {48, 20, 18, 255});
        float tScale = std::min(b.w, b.h) / 22.0f;
        SDL_Color tntTextCol = (b.ignited && (static_cast<int>(b.tntFuse * 30.0f) % 2 == 0)) ?
                               SDL_Color{255, 255, 255, 255} : SDL_Color{255, 235, 130, 255};
        drawText(ren, "TNT", screenPos.x, screenPos.y - 3.5f * tScale, tScale, tntTextCol, true, true);
        if (b.ignited) {
            Vec2 fuseTip = rotPt(0.0f, -hh - 5.0f);
            drawNGon(ren, fuseTip, 5.0f, 6, b.tntFuse * 40.0f, {255, 220, 60, 255});
            drawNGon(ren, fuseTip, 2.5f, 4, 0.0f, {255, 255, 255, 255});
        }
    } else if (b.mat == MaterialTier::MAGMA_VENT) {
        // Dark basalt rock with fiery glowing cracks and molten aperture
        drawThickLine(ren, rotPt(-hw * 0.45f, -hh * 0.2f), rotPt(hw * 0.35f, hh * 0.35f), 2.8f, {255, 80, 20, 255});
        drawThickLine(ren, rotPt(hw * 0.25f, -hh * 0.5f), rotPt(-hw * 0.15f, hh * 0.4f), 2.2f, {255, 180, 40, 255});
        drawThickLine(ren, rotPt(-hw * 0.35f, -hh + 3.0f), rotPt(hw * 0.35f, -hh + 3.0f), 3.5f, {255, 220, 60, 255});
        drawThickLine(ren, p0, p1, 2.4f, {32, 22, 24, 255});
        drawThickLine(ren, p1, p2, 2.4f, {32, 22, 24, 255});
        drawThickLine(ren, p2, p3, 2.4f, {32, 22, 24, 255});
        drawThickLine(ren, p3, p0, 2.4f, {32, 22, 24, 255});
    } else if (b.mat == MaterialTier::TRAMPOLINE) {
        // High-tension trampoline deflector frame & woven emerald mat
        drawConvexPoly(ren, {p0, p1, p2, p3}, {32, 38, 48, 255});
        float inHw = hw - 8.0f;
        float inHh = hh - 5.0f;
        Vec2 m0 = rotPt(-inHw, -inHh), m1 = rotPt(inHw, -inHh),
             m2 = rotPt(inHw, inHh),   m3 = rotPt(-inHw, inHh);
        drawConvexPoly(ren, {m0, m1, m2, m3}, {38, 120, 84, 255});
        int numRibs = static_cast<int>(b.w / 14.0f);
        if (numRibs < 2) numRibs = 2;
        for (int r = 1; r < numRibs; ++r) {
            float rx = -inHw + (2.0f * inHw * r) / numRibs;
            drawThickLine(ren, rotPt(rx, -inHh), rotPt(rx, inHh), 1.8f, {58, 172, 122, 220});
        }
        for (int r = 0; r < numRibs - 1; ++r) {
            float rx = -inHw + (2.0f * inHw * (r + 0.5f)) / numRibs;
            drawThickLine(ren, rotPt(rx - 3.5f, 0.0f), rotPt(rx, -3.5f), 1.8f, {255, 230, 95, 240});
            drawThickLine(ren, rotPt(rx, -3.5f), rotPt(rx + 3.5f, 0.0f), 1.8f, {255, 230, 95, 240});
        }
        drawThickLine(ren, p0, p1, 2.5f, {78, 86, 102, 255});
        drawThickLine(ren, p1, p2, 2.5f, {78, 86, 102, 255});
        drawThickLine(ren, p2, p3, 2.5f, {78, 86, 102, 255});
        drawThickLine(ren, p3, p0, 2.5f, {78, 86, 102, 255});
        drawNGon(ren, rotPt(-hw + 5.0f, -hh + 5.0f), 2.8f, 4, 0.0f, {228, 185, 75, 255});
        drawNGon(ren, rotPt(hw - 5.0f, -hh + 5.0f),  2.8f, 4, 0.0f, {228, 185, 75, 255});
        drawNGon(ren, rotPt(hw - 5.0f, hh - 5.0f),   2.8f, 4, 0.0f, {228, 185, 75, 255});
        drawNGon(ren, rotPt(-hw + 5.0f, hh - 5.0f),  2.8f, 4, 0.0f, {228, 185, 75, 255});
    } else {
        // Distinct Architectural Detailing Based on Material & Aspect Ratio
        if (b.mat == MaterialTier::STONE) {
            if (b.w >= 60.0f && b.w / b.h >= 2.0f) {
                // Wide Stone Architrave / Lintel: Render masonry relief groove & vertical block seams
                drawThickLine(ren, rotPt(-hw + 6.0f, 0.0f), rotPt(hw - 6.0f, 0.0f), 1.6f, {65, 75, 88, 220});
                int seams = static_cast<int>(b.w / 40.0f);
                for (int sm = 1; sm < seams; ++sm) {
                    float sx = -hw + (b.w * sm) / seams;
                    drawThickLine(ren, rotPt(sx, -hh + 3.0f), rotPt(sx, hh - 3.0f), 1.6f, {65, 75, 88, 220});
                }
                drawThickLine(ren, rotPt(-hw + 8.0f, hh - 2.5f), rotPt(hw - 8.0f, hh - 2.5f), 1.8f, {48, 56, 68, 240});
            } else if (b.h >= 60.0f && b.h / b.w >= 2.0f) {
                // Tall Classical Column / Stele: Capital and base rims + vertical fluting grooves
                drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 7.0f), rotPt(hw - 2.0f, -hh + 7.0f), 2.0f, {135, 148, 168, 240});
                drawThickLine(ren, rotPt(-hw + 2.0f, hh - 7.0f), rotPt(hw - 2.0f, hh - 7.0f), 2.0f, {55, 62, 74, 240});
                drawThickLine(ren, rotPt(-hw * 0.35f, -hh + 9.0f), rotPt(-hw * 0.35f, hh - 9.0f), 1.4f, {68, 76, 88, 200});
                drawThickLine(ren, rotPt(hw * 0.35f, -hh + 9.0f), rotPt(hw * 0.35f, hh - 9.0f), 1.4f, {68, 76, 88, 200});
            } else if (b.w >= 40.0f && b.h >= 40.0f) {
                // Heavy Dressed Ashlar Megalith / Plinth: Inset masonry relief panel
                drawThickLine(ren, rotPt(-hw + 6.0f, -hh + 6.0f), rotPt(hw - 6.0f, -hh + 6.0f), 1.5f, {125, 138, 155, 200});
                drawThickLine(ren, rotPt(hw - 6.0f, -hh + 6.0f), rotPt(hw - 6.0f, hh - 6.0f), 1.5f, {55, 62, 72, 200});
                drawThickLine(ren, rotPt(hw - 6.0f, hh - 6.0f), rotPt(-hw + 6.0f, hh - 6.0f), 1.5f, {55, 62, 72, 200});
                drawThickLine(ren, rotPt(-hw + 6.0f, hh - 6.0f), rotPt(-hw + 6.0f, -hh + 6.0f), 1.5f, {125, 138, 155, 200});
            }
        } else if (b.mat == MaterialTier::TIMBER) {
            if (b.w >= 34.0f && b.h >= 34.0f && std::abs(b.w - b.h) < 16.0f) {
                // Reinforced Timber Crate: Cross-brace "X" with iron corner rivets
                drawThickLine(ren, rotPt(-hw + 5.0f, -hh + 5.0f), rotPt(hw - 5.0f, hh - 5.0f), 2.2f, {112, 60, 32, 240});
                drawThickLine(ren, rotPt(-hw + 5.0f, hh - 5.0f), rotPt(hw - 5.0f, -hh + 5.0f), 2.2f, {112, 60, 32, 240});
                drawNGon(ren, rotPt(-hw + 5.0f, -hh + 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                drawNGon(ren, rotPt(hw - 5.0f, -hh + 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                drawNGon(ren, rotPt(hw - 5.0f, hh - 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                drawNGon(ren, rotPt(-hw + 5.0f, hh - 5.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
            } else if (b.w >= 60.0f && b.w / b.h >= 2.0f) {
                // Wide Timber Girder: Longitudinal grain lines and iron bolt pegs
                drawThickLine(ren, rotPt(-hw + 14.0f, -hh * 0.3f), rotPt(hw - 14.0f, -hh * 0.3f), 1.4f, {115, 62, 34, 210});
                drawThickLine(ren, rotPt(-hw + 14.0f,  hh * 0.3f), rotPt(hw - 14.0f,  hh * 0.3f), 1.4f, {115, 62, 34, 210});
                drawNGon(ren, rotPt(-hw + 10.0f, 0.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
                drawNGon(ren, rotPt(hw - 10.0f, 0.0f), 2.6f, 4, 0.0f, {52, 48, 44, 255});
            } else if (b.h >= 55.0f && b.h / b.w >= 2.0f) {
                // Tall Timber Post: Vertical wood grain and iron collar bands
                drawThickLine(ren, rotPt(0.0f, -hh + 12.0f), rotPt(0.0f, hh - 12.0f), 1.4f, {115, 62, 34, 210});
                drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 14.0f), rotPt(hw - 2.0f, -hh + 14.0f), 2.4f, {52, 48, 44, 245});
                drawThickLine(ren, rotPt(-hw + 2.0f,  hh - 14.0f), rotPt(hw - 2.0f,  hh - 14.0f), 2.4f, {52, 48, 44, 245});
            }
        } else if (b.mat == MaterialTier::TWIG) {
            // Bamboo / Wicker nodes along length
            if (b.w >= 50.0f) {
                int nodes = static_cast<int>(b.w / 22.0f);
                for (int nd = 1; nd < nodes; ++nd) {
                    float nx = -hw + (b.w * nd) / nodes;
                    drawThickLine(ren, rotPt(nx, -hh + 2.0f), rotPt(nx, hh - 2.0f), 2.0f, {158, 126, 62, 230});
                    drawNGon(ren, rotPt(nx, 0.0f), 2.2f, 4, 0.0f, {245, 210, 130, 240});
                }
            } else if (b.h >= 50.0f) {
                int nodes = static_cast<int>(b.h / 22.0f);
                for (int nd = 1; nd < nodes; ++nd) {
                    float ny = -hh + (b.h * nd) / nodes;
                    drawThickLine(ren, rotPt(-hw + 2.0f, ny), rotPt(hw - 2.0f, ny), 2.0f, {158, 126, 62, 230});
                    drawNGon(ren, rotPt(0.0f, ny), 2.2f, 4, 0.0f, {245, 210, 130, 240});
                }
            }
        }
        // Inner highlight bevel along top edge & crisp woodblock outline
        drawThickLine(ren, rotPt(-hw + 2.0f, -hh + 2.0f), rotPt(hw - 2.0f, -hh + 2.0f),
                      1.8f, {255, 245, 220, 70});
        drawThickLine(ren, p0, p1, 2.3f, {28, 22, 18, 235});
        drawThickLine(ren, p1, p2, 2.3f, {28, 22, 18, 235});
        drawThickLine(ren, p2, p3, 2.3f, {28, 22, 18, 235});
        drawThickLine(ren, p3, p0, 2.3f, {28, 22, 18, 235});
    }

    float hpRatio = b.hp / b.maxHp;
    if (hpRatio < 0.66f) {
        SDL_Color crackCol = (b.mat == MaterialTier::ICE) ? SDL_Color{255, 255, 255, 230} : SDL_Color{22, 18, 16, 235};
        drawThickLine(ren, rotPt(-hw * 0.5f, -hh * 0.6f), rotPt(hw * 0.2f, hh * 0.4f),
                      2.2f, crackCol);
    }
    if (hpRatio < 0.35f) {
        SDL_Color crackCol = (b.mat == MaterialTier::ICE) ? SDL_Color{255, 255, 255, 245} : SDL_Color{16, 12, 10, 255};
        drawThickLine(ren, rotPt(hw * 0.6f, -hh * 0.5f), rotPt(-hw * 0.3f, hh * 0.6f),
                      2.6f, crackCol);
    }
}

void SnoutAndSiegeGame::renderPendulum(SDL_Renderer* ren, const PendulumBody& pend, Vec2 cam) {
    Vec2 screenAnchor = pend.anchor - cam;
    Vec2 bPos = pend.getBallPos();
    Vec2 screenBall = bPos - cam;

    // 1. Pivot bracket at ceiling
    drawNGon(ren, screenAnchor, 11.0f, 6, 0.0f, {85, 92, 108, 255});
    drawNGon(ren, screenAnchor, 6.0f, 6, 0.0f, {218, 175, 75, 255});

    // 2. Chain links from anchor to ball
    Vec2 seg = screenBall - screenAnchor;
    float segLen = seg.length();
    int numLinks = static_cast<int>(segLen / 16.0f);
    if (numLinks < 2) numLinks = 2;
    Vec2 linkDir = (segLen > 1.0f) ? (seg * (1.0f / segLen)) : Vec2(0.0f, 1.0f);
    Vec2 linkPerp(-linkDir.y, linkDir.x);

    for (int i = 1; i < numLinks; ++i) {
        float frac = static_cast<float>(i) / numLinks;
        Vec2 lp = screenAnchor + seg * frac;
        if (i % 2 == 0) {
            drawThickLine(ren, lp - linkDir * 6.0f - linkPerp * 3.5f,
                               lp + linkDir * 6.0f - linkPerp * 3.5f, 2.5f, {72, 78, 92, 255});
            drawThickLine(ren, lp - linkDir * 6.0f + linkPerp * 3.5f,
                               lp + linkDir * 6.0f + linkPerp * 3.5f, 2.5f, {72, 78, 92, 255});
            drawThickLine(ren, lp - linkDir * 6.0f - linkPerp * 3.5f,
                               lp - linkDir * 6.0f + linkPerp * 3.5f, 2.5f, {95, 102, 118, 255});
            drawThickLine(ren, lp + linkDir * 6.0f - linkPerp * 3.5f,
                               lp + linkDir * 6.0f + linkPerp * 3.5f, 2.5f, {50, 54, 65, 255});
        } else {
            drawThickLine(ren, lp - linkDir * 7.0f, lp + linkDir * 7.0f, 4.0f, {90, 96, 112, 255});
            drawThickLine(ren, lp - linkDir * 5.0f, lp + linkDir * 5.0f, 2.0f, {145, 152, 168, 255});
        }
    }

    // 3. Drop shadow under ball
    drawNGon(ren, screenBall + Vec2(5.0f, 6.0f), pend.radius, 12, pend.angle, {15, 18, 26, 90});

    // 4. Spiked Iron Wrecking Ball
    if (pend.spiked) {
        for (int s = 0; s < 8; ++s) {
            float sa = pend.angle + s * (2.0f * PI / 8.0f);
            Vec2 spTip  = screenBall + Vec2(std::cos(sa), std::sin(sa)) * (pend.radius + 12.0f);
            Vec2 pLeft  = screenBall + Vec2(std::cos(sa + 0.18f), std::sin(sa + 0.18f)) * (pend.radius - 2.0f);
            Vec2 pRight = screenBall + Vec2(std::cos(sa - 0.18f), std::sin(sa - 0.18f)) * (pend.radius - 2.0f);
            drawTriangle(ren, spTip, pLeft, pRight, {82, 88, 102, 255});
            drawTriangle(ren, spTip, pLeft, screenBall + Vec2(std::cos(sa), std::sin(sa)) * (pend.radius - 2.0f), {55, 60, 72, 255});
        }
    }

    drawNGon(ren, screenBall, pend.radius, 12, pend.angle, {58, 62, 75, 255});
    drawNGon(ren, screenBall, pend.radius * 0.82f, 10, pend.angle, {72, 78, 94, 255});
    drawThickLine(ren, screenBall - Vec2(pend.radius * 0.8f, 0.0f),
                       screenBall + Vec2(pend.radius * 0.8f, 0.0f), 5.0f, {42, 46, 56, 255});
    for (int r = -2; r <= 2; ++r) {
        drawNGon(ren, screenBall + Vec2(r * 8.0f, 0.0f), 2.2f, 4, 0.0f, {218, 175, 75, 255});
    }
    drawNGon(ren, screenBall + Vec2(-pend.radius * 0.35f, -pend.radius * 0.35f), pend.radius * 0.28f, 6, 0.0f, {140, 150, 170, 200});
}

void SnoutAndSiegeGame::renderPig(SDL_Renderer* ren, const PigBody& p, Vec2 cam) {
    Vec2 screenPos = p.pos - cam;
    float screenGroundY = GROUND_Y - cam.y;
    if (p.pos.y + p.radius >= GROUND_Y - 6.0f) {
        drawConvexPoly(ren, {{screenPos.x - p.radius * 0.85f, screenGroundY + 2},
                             {screenPos.x + p.radius * 0.85f, screenGroundY + 2},
                             {screenPos.x + p.radius * 0.65f, screenGroundY + 6},
                             {screenPos.x - p.radius * 0.65f, screenGroundY + 6}}, {12, 15, 22, 105});
    }
    CharPose pose;
    float sq = squashAmount(p.squashAmp, p.squashTime);
    float speed = p.vel.length();
    if (std::abs(sq) > 0.02f) {
        pose.squash = sq;
        pose.squashDir = p.squashDir;
    } else if (p.launched && speed > 300.0f) {
        // Stretch along the direction of travel.
        pose.squash = -std::min(0.14f, (speed - 300.0f) / 5000.0f);
        pose.squashDir = std::atan2(p.vel.y, p.vel.x);
    }
    if (p.launched) {
        if (p.squashTime < 0.45f && p.squashAmp > 0.1f) pose.mood = Mood::OUCH;
        else if (speed > 260.0f) pose.mood = Mood::YELL;
        if (speed > 60.0f) pose.look = p.vel;
    }
    renderOrigamiPigGeometry(ren, p.type, screenPos, p.radius, p.angle, pose);
}

void SnoutAndSiegeGame::renderBird(SDL_Renderer* ren, const BirdTarget& t, Vec2 cam) {
    Vec2 screenPos = t.pos - cam;
    float screenGroundY = GROUND_Y - cam.y;
    if (t.pos.y + t.radius >= GROUND_Y - 6.0f) {
        drawConvexPoly(ren, {{screenPos.x - t.radius * 0.85f, screenGroundY + 2},
                             {screenPos.x + t.radius * 0.85f, screenGroundY + 2},
                             {screenPos.x + t.radius * 0.65f, screenGroundY + 6},
                             {screenPos.x - t.radius * 0.65f, screenGroundY + 6}}, {12, 15, 22, 105});
    }
    float helmRatio = (t.maxHelmetHp > 0.0f) ? (t.helmetHp / t.maxHelmetHp) : 1.0f;
    float shieldRatio = (t.maxShieldHp > 0.0f) ? (t.shieldHp / t.maxShieldHp) : 1.0f;
    float balloonRatio = (t.maxBalloonHp > 0.0f) ? (t.balloonHp / t.maxBalloonHp) : 1.0f;
    CharPose pose;
    pose.squash = squashAmount(t.squashAmp, t.squashTime);
    pose.squashDir = t.squashDir;
    // Watch the nearest flying pig; panic when it gets close.
    float nearest = 1e9f;
    for (const auto& p : activePigs) {
        if (!p.active || !p.launched) continue;
        Vec2 d = p.pos - t.pos;
        float dist = d.length();
        if (dist < nearest && p.vel.length() > 80.0f) {
            nearest = dist;
            pose.look = d;
        }
    }
    if (t.hurtTimer > 0.0f) {
        pose.mood = Mood::OUCH;
        screenPos.x += std::sin(t.hurtTimer * 90.0f) * t.hurtTimer * 5.0f;  // shudder
    } else if (nearest < 240.0f) {
        pose.mood = Mood::SCARED;
    }
    renderTotemBirdGeometry(ren, t.kind, screenPos, t.radius, t.angle, t.hp / t.maxHp, t.hasHelmet, helmRatio,
                            t.hasShield, shieldRatio, t.inZeppelin, balloonRatio, pose);
}

void SnoutAndSiegeGame::renderGoldenTruffle(SDL_Renderer* ren, Vec2 cam) {
    if (!levelTruffle.exists || levelTruffle.collected) return;
    Vec2 screenPos = levelTruffle.pos - cam;
    float pulse = std::sin(levelTruffle.pulseTime * 4.5f);
    float auraRad = levelTruffle.radius * (1.25f + 0.20f * pulse);

    // Radiant golden aura
    drawNGon(ren, screenPos, auraRad * 1.5f, 14, levelTruffle.pulseTime * 0.5f, {255, 215, 0, 40});
    drawNGon(ren, screenPos, auraRad * 1.1f, 12, -levelTruffle.pulseTime * 0.8f, {255, 230, 80, 75});

    // Sprite texture if loaded
    SDL_Texture* tex = assets.useSpriteTextures ? assets.getTexture("sprite.item_truffle") : nullptr;
    if (tex) {
        float sSize = levelTruffle.radius * 2.4f;
        SDL_FRect dst{screenPos.x - sSize * 0.5f, screenPos.y - sSize * 0.5f, sSize, sSize};
        SDL_RenderCopyF(ren, tex, nullptr, &dst);
    } else {
        // Origami geometric golden truffle
        float r = levelTruffle.radius;
        drawNGon(ren, screenPos, r, 8, 0.0f, {218, 165, 32, 255});
        drawNGon(ren, screenPos, r * 0.75f, 8, PI * 0.125f, {255, 215, 0, 255});
        drawNGon(ren, screenPos, r * 0.45f, 6, 0.0f, {255, 245, 140, 255});
    }

    // 4-point rotating golden specular glint flare
    float gSize = 8.0f + 4.0f * pulse;
    drawThickLine(ren, {screenPos.x - gSize, screenPos.y}, {screenPos.x + gSize, screenPos.y}, 2.0f, {255, 248, 180, 230});
    drawThickLine(ren, {screenPos.x, screenPos.y - gSize}, {screenPos.x, screenPos.y + gSize}, 2.0f, {255, 248, 180, 230});
    drawNGon(ren, screenPos, 2.5f, 4, 0.0f, {255, 255, 255, 255});
}

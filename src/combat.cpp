#include "game.h"

void SnoutAndSiegeGame::activateMidAirAbility() {
    if (activePigs.empty()) return;
    PigBody& mainPig = activePigs[0];
    if (!mainPig.launched || mainPig.abilityUsed || !mainPig.active) return;
    mainPig.abilityUsed = true;

    if (mainPig.type == PigType::TROTTER) {
        shotUsedAnvil = true;
        mainPig.vel.x *= 0.22f;
        mainPig.vel.y = 820.0f;
        mainPig.mass *= 1.85f;
        mainPig.invMass = 1.0f / mainPig.mass;
        spawnBurst(mainPig.pos, 18, {238, 155, 130, 255}, 210.0f);
        floatScores.push_back({mainPig.pos, "ANVIL SLAM!", {255, 200, 150, 255}, 1.0f, 1.0f});
        assets.playSound("sound.slam");
        addTrauma(0.65f);
    } else if (mainPig.type == PigType::TURBO) {
        float spd = mainPig.vel.length();
        Vec2 dir = (spd > 10.0f) ? mainPig.vel.normalized() : Vec2(1.0f, -0.1f).normalized();
        mainPig.vel = dir * std::max(880.0f, spd * 2.45f);
        spawnBurst(mainPig.pos, 24, {255, 208, 70, 255}, 260.0f);
        floatScores.push_back({mainPig.pos, "TURBO CHARGE!", {255, 225, 90, 255}, 1.0f, 1.0f});
        assets.playSound("sound.dash");
        addTrauma(0.45f);
    } else if (mainPig.type == PigType::SPLIT) {
        float baseAngle = std::atan2(mainPig.vel.y, mainPig.vel.x);
        float speed = std::max(520.0f, mainPig.vel.length() * 1.08f);
        // Copy the parent before growing activePigs: push_back may reallocate,
        // which would leave the mainPig reference dangling.
        const PigBody parent = mainPig;
        const float offsets[2] = {-0.23f, +0.23f};
        activePigs.reserve(activePigs.size() + 2);
        for (float off : offsets) {
            PigBody clone = parent;
            clone.radius = 15.0f;
            clone.abilityUsed = true;
            clone.pos = parent.pos + Vec2(0.0f, off * 45.0f);
            clone.vel = {std::cos(baseAngle + off) * speed, std::sin(baseAngle + off) * speed};
            activePigs.push_back(clone);
        }
        spawnBurst(parent.pos, 20, {110, 235, 185, 255}, 200.0f);
        floatScores.push_back({parent.pos, "TRI-SQUADRON!", {130, 250, 200, 255}, 1.0f, 1.0f});
        assets.playSound("sound.split");
        addTrauma(0.28f);
    } else if (mainPig.type == PigType::BOMB) {
        triggerBombDetonation(mainPig);
    } else if (mainPig.type == PigType::GLIDER) {
        // Gale the Aeronaut: Aerodynamic Glide Lift & Twin Iron Aerial Bomb Release
        mainPig.vel.y = -45.0f; // lift against gravity
        mainPig.vel.x = std::max(mainPig.vel.x * 1.25f, 320.0f); // forward glide surge
        spawnBurst(mainPig.pos, 18, {180, 230, 255, 255}, 220.0f);
        floatScores.push_back({mainPig.pos, "AERO GLIDE!", {160, 235, 255, 255}, 1.0f, 1.0f});
        assets.playSound("sound.glide");
        addTrauma(0.35f);

        // Drop Twin Iron Aerial Bombs
        AerialBomb b1;
        b1.pos = mainPig.pos + Vec2(-10.0f, 12.0f);
        b1.vel = Vec2(mainPig.vel.x * 0.25f - 25.0f, 75.0f);
        b1.radius = 9.0f;
        b1.blastRadius = 95.0f;
        b1.damage = 120.0f;
        b1.active = true;

        AerialBomb b2;
        b2.pos = mainPig.pos + Vec2(10.0f, 12.0f);
        b2.vel = Vec2(mainPig.vel.x * 0.35f + 25.0f, 55.0f);
        b2.radius = 9.0f;
        b2.blastRadius = 95.0f;
        b2.damage = 120.0f;
        b2.active = true;

        aerialBombs.push_back(b1);
        aerialBombs.push_back(b2);
    } else if (mainPig.type == PigType::GRAVITON) {
        // Atticus the Graviton Boar: Graviton Vortex ability pulling blocks and birds inward
        mainPig.vortexTimer = 1.35f;
        spawnBurst(mainPig.pos, 22, {185, 110, 255, 255}, 220.0f);
        floatScores.push_back({mainPig.pos, "GRAVITON VORTEX!", {210, 140, 255, 255}, 1.1f, 1.1f});
        assets.playSound("sound.vortex");
        addTrauma(0.50f);
    } else if (mainPig.type == PigType::DRILL) {
        // Rusty the Drill Hog: Supersonic rotary bore straight through fortress blocks
        // The bore has a fixed budget that each pierced block wears down (see
        // resolveCircleOBB), so it punches through a wall or two, not a whole map.
        mainPig.drillTimer = 1.0f;
        ++drillBoreId;
        float spd = mainPig.vel.length();
        Vec2 dir = (spd > 10.0f) ? mainPig.vel.normalized() : Vec2(1.0f, 0.0f);
        mainPig.vel = dir * std::max(780.0f, spd * 1.85f);
        spawnBurst(mainPig.pos, 20, {255, 180, 50, 255}, 260.0f);
        floatScores.push_back({mainPig.pos, "SUPERSONIC BORE!", {255, 195, 75, 255}, 1.1f, 1.1f});
        assets.playSound("sound.drill", 0.85f);
        addTrauma(0.55f);
    }
}

void SnoutAndSiegeGame::triggerGravitonImplosion(Vec2 center) {
    fortressAwake = true;
    const float blastRadius = 180.0f;
    Particle ring{};
    ring.pos = center;
    ring.isRing = true;
    ring.ringRadius = 16.0f;
    ring.ringMaxRadius = blastRadius;
    ring.life = 0.45f;
    ring.maxLife = 0.45f;
    ring.color = {175, 95, 255, 255};
    particles.push_back(ring);

    spawnBurst(center, 36, {190, 110, 255, 255}, 360.0f);
    spawnBurst(center, 24, {70, 230, 255, 255}, 280.0f);
    floatScores.push_back({center, "SINGULARITY!", {210, 140, 255, 255}, 1.2f, 1.2f});
    assets.playSound("sound.vortex");
    assets.playSound("sound.explosion");
    addTrauma(0.80f);

    int targetsHit = 0;
    for (auto& b : blocks) {
        if (b.destroyed) continue;
        Vec2 delta = b.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            b.vel += dir * (600.0f * falloff);
            b.angVel += 12.0f * falloff;
            damageBlock(b, 140.0f * falloff);
            targetsHit++;
        }
    }
    for (auto& t : birds) {
        if (t.defeated) continue;
        Vec2 delta = t.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            t.vel += dir * (560.0f * falloff);
            damageBird(t, 160.0f * falloff);
            targetsHit++;
        }
    }
    if (targetsHit >= 3) {
        achievementProgress[static_cast<size_t>(AchievementId::VORTEX_MAESTRO)]++;
        saveProgress();
        if (achievementProgress[static_cast<size_t>(AchievementId::VORTEX_MAESTRO)] >= 1) {
            unlockAchievement(AchievementId::VORTEX_MAESTRO);
        }
    }
}

void SnoutAndSiegeGame::triggerBombDetonation(PigBody& bombPig) {
    if (bombPig.exploded) return;
    fortressAwake = true;
    bombPig.exploded = true;
    bombPig.abilityUsed = true;
    Vec2 center = bombPig.pos;
    const float blastRadius = 172.0f;

    Particle ring{};
    ring.pos = center;
    ring.isRing = true;
    ring.ringRadius = 16.0f;
    ring.ringMaxRadius = blastRadius;
    ring.life = 0.45f;
    ring.maxLife = 0.45f;
    ring.color = {255, 165, 60, 255};
    particles.push_back(ring);

    spawnBurst(center, 42, {255, 130, 45, 255}, 380.0f);
    spawnBurst(center, 24, {90, 95, 110, 255}, 260.0f);
    floatScores.push_back({center, "KABOOM!", {255, 140, 60, 255}, 1.2f, 1.2f});
    assets.playSound("sound.explosion");
    addTrauma(0.85f);

    int blocksBefore = shotBlocksDestroyed;
    for (auto& b : blocks) {
        if (b.destroyed) continue;
        Vec2 delta = b.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            b.vel += dir * (680.0f * falloff * b.invMass);
            b.angVel += ((b.pos.x > center.x) ? 1.0f : -1.0f) * 6.0f * falloff;
            if (b.mat == MaterialTier::TNT) {
                if (!b.ignited) {
                    b.ignited = true;
                    b.tntFuse = 0.08f + ((b.pos.x > center.x) ? 0.04f : 0.01f);
                }
            } else {
                damageBlock(b, 240.0f * falloff);
            }
        }
    }

    int blocksAfter = shotBlocksDestroyed;
    if (blocksAfter - blocksBefore >= 5) {
        unlockAchievement(AchievementId::SEISMIC_ANNIHILATION);
    }

    if (levelTruffle.exists && !levelTruffle.collected) {
        if ((levelTruffle.pos - center).length() < blastRadius) {
            collectGoldenTruffle();
        }
    }

    for (auto& t : birds) {
        if (t.defeated) continue;
        Vec2 delta = t.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            t.vel += dir * (580.0f * falloff);
            damageBird(t, 240.0f * falloff);
        }
    }
}

void SnoutAndSiegeGame::detonateAerialBomb(AerialBomb& ab) {
    if (!ab.active) return;
    ab.active = false;
    fortressAwake = true;
    Vec2 center = ab.pos;
    float blastRadius = ab.blastRadius;

    Particle ring{};
    ring.pos = center;
    ring.isRing = true;
    ring.ringRadius = 10.0f;
    ring.ringMaxRadius = blastRadius;
    ring.life = 0.35f;
    ring.maxLife = 0.35f;
    ring.color = {255, 180, 70, 255};
    particles.push_back(ring);

    spawnBurst(center, 28, {255, 140, 50, 255}, 320.0f);
    spawnBurst(center, 16, {100, 110, 125, 255}, 220.0f);
    floatScores.push_back({center, "AIR STRIKE!", {255, 190, 80, 255}, 1.1f, 1.1f});
    assets.playSound("sound.explosion");
    addTrauma(0.55f);

    for (auto& b : blocks) {
        if (b.destroyed) continue;
        Vec2 delta = b.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            b.vel += dir * (540.0f * falloff * b.invMass);
            b.angVel += ((b.pos.x > center.x) ? 1.0f : -1.0f) * 4.5f * falloff;
            if (b.mat == MaterialTier::TNT) {
                if (!b.ignited) {
                    b.ignited = true;
                    b.tntFuse = 0.05f;
                }
            } else {
                damageBlock(b, ab.damage * falloff);
            }
        }
    }

    if (levelTruffle.exists && !levelTruffle.collected) {
        if ((levelTruffle.pos - center).length() < blastRadius) {
            collectGoldenTruffle();
        }
    }

    for (auto& t : birds) {
        if (t.defeated) continue;
        Vec2 delta = t.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            t.vel += dir * (480.0f * falloff);
            damageBird(t, ab.damage * falloff);
        }
    }
}

void SnoutAndSiegeGame::triggerTntDetonation(BlockBody& tntBlock) {
    if (tntBlock.destroyed) return;
    tntBlock.destroyed = true;
    tntBlock.ignited = false;
    fortressAwake = true;
    Vec2 center = tntBlock.pos;
    const float blastRadius = 210.0f;

    shotTntDetonated++;
    if (shotTntDetonated >= 3) {
        unlockAchievement(AchievementId::POWDER_MAESTRO);
    }

    if (levelTruffle.exists && !levelTruffle.collected) {
        if ((levelTruffle.pos - center).length() < blastRadius) {
            collectGoldenTruffle();
        }
    }

    Particle ring{};
    ring.pos = center;
    ring.isRing = true;
    ring.ringRadius = 18.0f;
    ring.ringMaxRadius = blastRadius;
    ring.life = 0.48f;
    ring.maxLife = 0.48f;
    ring.color = {255, 145, 45, 255};
    particles.push_back(ring);

    spawnBurst(center, 46, {255, 120, 35, 255}, 420.0f);
    spawnBurst(center, 28, {255, 215, 60, 255}, 310.0f);
    spawnSplinters(center, MaterialTier::TNT, 16, 280.0f);

    score += 2500;
    floatScores.push_back({center, "BOOM! +2500", {255, 180, 70, 255}, 1.35f, 1.35f});
    assets.playSound("sound.explosion");
    addTrauma(0.92f);

    for (auto& b : blocks) {
        if (b.destroyed) continue;
        Vec2 delta = b.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            b.vel += dir * (760.0f * falloff * b.invMass);
            b.angVel += ((b.pos.x > center.x) ? 1.0f : -1.0f) * 7.5f * falloff;
            if (b.mat == MaterialTier::TNT) {
                if (!b.ignited) {
                    b.ignited = true;
                    b.tntFuse = 0.08f + ((b.pos.x > center.x) ? 0.04f : 0.01f);
                }
            } else {
                damageBlock(b, 280.0f * falloff);
            }
        }
    }

    for (auto& t : birds) {
        if (t.defeated) continue;
        Vec2 delta = t.pos - center;
        float dist = delta.length();
        if (dist < blastRadius) {
            float falloff = 1.0f - (dist / blastRadius);
            Vec2 dir = (dist > 1.0f) ? (delta * (1.0f / dist)) : Vec2(0.0f, -1.0f);
            t.vel += dir * (650.0f * falloff);
            damageBird(t, 280.0f * falloff);
        }
    }
}

void SnoutAndSiegeGame::spawnBurst(Vec2 pos, int count, SDL_Color col, float maxSpeed) {
    for (int i = 0; i < count; ++i) {
        float a = (2.0f * PI * i) / count + (i % 3) * 0.17f;
        float spd = maxSpeed * (0.35f + 0.65f * ((i * 37) % 100) / 100.0f);
        Particle pt{};
        pt.pos = pos;
        pt.vel = {std::cos(a) * spd, std::sin(a) * spd - 60.0f};
        pt.size = 4.0f + (i % 5) * 1.5f;
        pt.life = 0.45f + (i % 4) * 0.14f;
        pt.maxLife = pt.life;
        pt.color = col;
        pt.kind = ParticleKind::GENERIC;
        particles.push_back(pt);
    }
}

void SnoutAndSiegeGame::spawnSplinters(Vec2 pos, MaterialTier mat, int count, float maxSpeed) {
    for (int i = 0; i < count; ++i) {
        float a = (2.0f * PI * i) / count + ((i * 31) % 100) * 0.015f;
        float spd = maxSpeed * (0.40f + 0.60f * ((i * 47) % 100) / 100.0f);
        Particle pt{};
        pt.pos = pos + Vec2((i % 5 - 2) * 4.0f, (i % 3 - 1) * 4.0f);
        pt.vel = {std::cos(a) * spd, std::sin(a) * spd - 90.0f};
        pt.angVel = ((i % 2 == 0) ? 1.0f : -1.0f) * (8.0f + (i % 7) * 2.5f);
        pt.angle = (i * 0.7f);
        pt.life = 0.65f + (i % 5) * 0.16f;
        pt.maxLife = pt.life;

        if (mat == MaterialTier::TWIG) {
            // Wicker / Twig: thin dry reeds and straw splinters
            pt.kind = ParticleKind::WOOD_SPLINTER;
            pt.size = 4.5f + (i % 4) * 1.4f;
            int cVar = i % 3;
            pt.color = (cVar == 0) ? SDL_Color{235, 198, 118, 255} :
                       (cVar == 1) ? SDL_Color{204, 166, 92, 255} : SDL_Color{245, 222, 160, 255};
        } else if (mat == MaterialTier::TIMBER) {
            // Timber: mixture of flying wood splinters and chunky oak blocks
            pt.kind = (i % 3 == 0) ? ParticleKind::TIMBER_CHUNK : ParticleKind::WOOD_SPLINTER;
            pt.size = 5.5f + (i % 5) * 1.8f;
            int cVar = i % 3;
            pt.color = (cVar == 0) ? SDL_Color{174, 102, 54, 255} :
                       (cVar == 1) ? SDL_Color{128, 72, 38, 255} : SDL_Color{224, 168, 102, 255};
        } else if (mat == MaterialTier::STONE) {
            // Stone: heavy tumbling slate shards & basalt pebbles
            pt.kind = ParticleKind::STONE_SHARD;
            pt.size = 5.0f + (i % 5) * 2.0f;
            int cVar = i % 3;
            pt.color = (cVar == 0) ? SDL_Color{118, 130, 148, 255} :
                       (cVar == 1) ? SDL_Color{72, 80, 96, 255} : SDL_Color{172, 184, 202, 255};
        } else if (mat == MaterialTier::ICE) {
            // Glacial Frost Ice: crystalline diamond shards
            pt.kind = ParticleKind::ICE_CRYSTAL;
            pt.size = 5.0f + (i % 4) * 2.2f;
            int cVar = i % 3;
            pt.color = (cVar == 0) ? SDL_Color{180, 240, 255, 255} :
                       (cVar == 1) ? SDL_Color{225, 250, 255, 255} : SDL_Color{125, 212, 248, 255};
        } else {
            // TNT: charred wood splinters & ember spark fragments
            pt.kind = (i % 2 == 0) ? ParticleKind::TIMBER_CHUNK : ParticleKind::SPARK_GLINT;
            pt.size = 4.5f + (i % 4) * 1.6f;
            int cVar = i % 3;
            pt.color = (cVar == 0) ? SDL_Color{224, 52, 42, 255} :
                       (cVar == 1) ? SDL_Color{255, 210, 68, 255} : SDL_Color{64, 58, 62, 255};
        }
        particles.push_back(pt);
    }

    // Add 2-3 expanding masonry dust / sawdust puffs
    for (int p = 0; p < 3; ++p) {
        Particle puff{};
        puff.pos = pos + Vec2((p - 1) * 8.0f, (p % 2) * -6.0f);
        puff.vel = Vec2((p - 1) * 22.0f, -32.0f - p * 12.0f);
        puff.size = 11.0f + p * 4.0f;
        puff.life = 0.55f + p * 0.12f;
        puff.maxLife = puff.life;
        puff.kind = ParticleKind::SMOKE_PUFF;
        puff.color = (mat == MaterialTier::STONE) ? SDL_Color{165, 175, 192, 150} :
                     (mat == MaterialTier::TIMBER ? SDL_Color{198, 168, 134, 150} :
                     (mat == MaterialTier::ICE    ? SDL_Color{205, 235, 250, 150} :
                     (mat == MaterialTier::TNT    ? SDL_Color{85, 80, 85, 180} :
                                                    SDL_Color{218, 202, 164, 150})));
        particles.push_back(puff);
    }
}

void SnoutAndSiegeGame::spawnFeatherBurst(Vec2 pos, BirdClass kind, int count, float maxSpeed) {
    for (int i = 0; i < count; ++i) {
        float a = (2.0f * PI * i) / count + ((i * 29) % 100) * 0.02f;
        float spd = maxSpeed * (0.35f + 0.65f * ((i * 53) % 100) / 100.0f);
        Particle pt{};
        pt.pos = pos + Vec2((i % 7 - 3) * 3.0f, (i % 5 - 2) * 3.0f);
        pt.vel = {std::cos(a) * spd, std::sin(a) * spd - 110.0f};
        pt.size = 5.0f + (i % 4) * 1.5f;
        pt.life = 1.15f + (i % 6) * 0.22f;
        pt.maxLife = pt.life;
        pt.kind = ParticleKind::FEATHER;
        pt.wobbleFreq = 10.0f + (i % 5) * 2.2f;
        pt.wobblePhase = i * 1.37f;

        if (kind == BirdClass::TOUCAN) {
            int cVar = i % 4;
            pt.color = (cVar == 0) ? SDL_Color{46, 178, 214, 255} :
                       (cVar == 1) ? SDL_Color{28, 126, 185, 255} :
                       (cVar == 2) ? SDL_Color{248, 172, 52, 255} : SDL_Color{224, 72, 56, 255};
        } else if (kind == BirdClass::OWL) {
            int cVar = i % 4;
            pt.color = (cVar == 0) ? SDL_Color{128, 92, 176, 255} :
                       (cVar == 1) ? SDL_Color{64, 68, 122, 255} :
                       (cVar == 2) ? SDL_Color{245, 212, 78, 255} : SDL_Color{192, 202, 235, 255};
        } else {
            int cVar = i % 4;
            pt.color = (cVar == 0) ? SDL_Color{224, 48, 62, 255} :
                       (cVar == 1) ? SDL_Color{245, 248, 255, 255} :
                       (cVar == 2) ? SDL_Color{255, 214, 64, 255} : SDL_Color{182, 32, 44, 255};
        }
        particles.push_back(pt);
    }

    // Add 3-4 golden sparkle glints
    for (int s = 0; s < 4; ++s) {
        Particle sp{};
        sp.pos = pos + Vec2((s - 1.5f) * 16.0f, (s % 2 - 0.5f) * 14.0f);
        sp.vel = Vec2((s - 1.5f) * 35.0f, -50.0f - s * 15.0f);
        sp.size = 6.0f + s * 1.5f;
        sp.life = 0.75f + s * 0.15f;
        sp.maxLife = sp.life;
        sp.kind = ParticleKind::SPARK_GLINT;
        sp.color = {255, 238, 110, 255};
        particles.push_back(sp);
    }

    // Plus expanding feather cloud puff
    for (int p = 0; p < 2; ++p) {
        Particle puff{};
        puff.pos = pos + Vec2((p * 2 - 1) * 6.0f, -4.0f);
        puff.vel = Vec2((p * 2 - 1) * 18.0f, -28.0f);
        puff.size = 14.0f + p * 5.0f;
        puff.life = 0.65f;
        puff.maxLife = puff.life;
        puff.kind = ParticleKind::SMOKE_PUFF;
        puff.color = {235, 240, 250, 140};
        particles.push_back(puff);
    }
}

void SnoutAndSiegeGame::eruptMagmaVent(BlockBody& b) {
    assets.playSound("sound.magma");
    addTrauma(0.24f);
    for (int i = 0; i < 18; ++i) {
        Particle em{};
        em.pos = Vec2(b.pos.x + ((i % 5) - 2) * 5.0f, b.pos.y - b.h * 0.5f - 2.0f);
        float angle = -PI * 0.5f + ((i % 7) - 3) * 0.16f;
        float spd = 130.0f + (i % 6) * 35.0f;
        em.vel = Vec2(std::cos(angle) * spd, std::sin(angle) * spd);
        em.size = 5.0f + (i % 4) * 2.0f;
        em.life = 1.4f + (i % 5) * 0.25f;
        em.maxLife = em.life;
        em.kind = ParticleKind::EMBER;
        em.wobbleFreq = 8.0f + (i % 4) * 2.0f;
        em.wobblePhase = i * 0.8f;
        int cVar = i % 3;
        em.color = (cVar == 0) ? SDL_Color{255, 230, 80, 255} :
                   (cVar == 1) ? SDL_Color{255, 120, 30, 255} : SDL_Color{255, 60, 20, 255};
        particles.push_back(em);
    }
}

void SnoutAndSiegeGame::triggerLightningStrike() {
    Vec2 strikeTarget{WINDOW_W * 0.72f, GROUND_Y};
    float highestY = GROUND_Y;
    BlockBody* targetBlock = nullptr;
    BirdTarget* targetBird = nullptr;

    for (auto& t : birds) {
        if (!t.defeated && t.hasHelmet && t.pos.y < highestY) {
            highestY = t.pos.y;
            strikeTarget = t.pos;
            targetBird = &t;
            targetBlock = nullptr;
        }
    }
    if (!targetBird) {
        for (auto& b : blocks) {
            if (!b.destroyed && (b.mat == MaterialTier::STONE) && b.pos.y < highestY) {
                highestY = b.pos.y;
                strikeTarget = b.pos;
                targetBlock = &b;
            }
        }
    }
    if (!targetBird && !targetBlock) {
        for (auto& b : blocks) {
            if (!b.destroyed && b.pos.y < highestY) {
                highestY = b.pos.y;
                strikeTarget = b.pos;
                targetBlock = &b;
            }
        }
    }

    LightningBolt bolt{};
    bolt.life = 0.24f;
    bolt.maxLife = 0.24f;
    bolt.width = 4.0f;
    bolt.color = SDL_Color{235, 248, 255, 255};
    Vec2 pt(strikeTarget.x + (std::sin(SDL_GetTicks() * 0.01f) * 45.0f), 45.0f);
    bolt.points.push_back(pt);
    int numSegs = 7;
    for (int s = 1; s < numSegs; ++s) {
        float frac = static_cast<float>(s) / numSegs;
        Vec2 basePt = pt + (strikeTarget - pt) * frac;
        float jitter = std::sin((s * 4.3f) + SDL_GetTicks() * 0.02f) * 26.0f;
        bolt.points.push_back(Vec2(basePt.x + jitter, basePt.y));
    }
    bolt.points.push_back(strikeTarget);
    lightningBolts.push_back(bolt);

    skyFlashAlpha = 0.85f;
    addTrauma(0.55f);
    assets.playSound("sound.thunder");

    if (targetBird) {
        damageBird(*targetBird, 65.0f);
    } else if (targetBlock) {
        damageBlock(*targetBlock, 75.0f);
    }

    // Electrostatic spark explosion at strike point
    for (int s = 0; s < 14; ++s) {
        Particle sp{};
        sp.pos = strikeTarget;
        float a = (PI * 2.0f * s) / 14.0f;
        float spd = 120.0f + (s % 4) * 35.0f;
        sp.vel = Vec2(std::cos(a) * spd, std::sin(a) * spd - 40.0f);
        sp.size = 5.0f + (s % 3);
        sp.life = 0.55f;
        sp.maxLife = 0.55f;
        sp.kind = ParticleKind::SPARK_GLINT;
        sp.color = {255, 245, 140, 255};
        particles.push_back(sp);
    }
}

void SnoutAndSiegeGame::damageBlock(BlockBody& b, float dmg) {
    if (b.destroyed || dmg <= 2.5f) return;
    if (b.mat == MaterialTier::MAGMA_VENT) {
        eruptMagmaVent(b);
        // Thermal shockwave melts adjacent ice and ignites adjacent TNT (< 85px)
        for (auto& other : blocks) {
            if (other.destroyed || &other == &b) continue;
            float d = (other.pos - b.pos).length();
            if (d < 85.0f) {
                if (other.mat == MaterialTier::ICE) damageBlock(other, 99.0f);
                if (other.mat == MaterialTier::TNT && !other.ignited) {
                    other.ignited = true;
                    other.tntFuse = 0.08f;
                }
            }
        }
        return;
    }
    b.hp -= dmg;
    if (b.mat == MaterialTier::TNT) {
        if (!b.ignited) {
            b.ignited = true;
            b.tntFuse = (b.hp <= 0.0f) ? 0.05f : 0.14f;
        } else if (b.hp <= 0.0f && b.tntFuse > 0.05f) {
            b.tntFuse = 0.05f;
        }
        return;
    }
    if (b.hp <= 0.0f) {
        b.destroyed = true;
        shotBlocksDestroyed++;
        addTrauma(b.mat == MaterialTier::STONE ? 0.32f :
                 (b.mat == MaterialTier::TIMBER ? 0.20f :
                 (b.mat == MaterialTier::ICE ? 0.18f : 0.12f)));
        int pts = (b.mat == MaterialTier::TWIG) ? 500 :
                  (b.mat == MaterialTier::TIMBER ? 900 :
                  (b.mat == MaterialTier::STONE ? 1500 : 600)); // ICE = 600
        score += pts;
        spawnSplinters(b.pos, b.mat, (b.mat == MaterialTier::STONE ? 20 : (b.mat == MaterialTier::ICE ? 22 : 16)), 240.0f);
        SDL_Color debCol = (b.mat == MaterialTier::TWIG) ? SDL_Color{245, 218, 128, 255} :
                           (b.mat == MaterialTier::TIMBER ? SDL_Color{236, 178, 112, 255} :
                           (b.mat == MaterialTier::STONE ? SDL_Color{195, 215, 238, 255} :
                                                           SDL_Color{160, 240, 255, 255})); // ICE
        floatScores.push_back({b.pos, "+" + std::to_string(pts), debCol, 0.95f, 0.95f});
        assets.playSound(b.mat == MaterialTier::TWIG ? "sound.hit_twig" :
                        (b.mat == MaterialTier::TIMBER ? "sound.hit_timber" :
                        (b.mat == MaterialTier::STONE ? "sound.hit_stone" : "sound.hit_ice")));
    } else if (dmg > 10.0f) {
        spawnSplinters(b.pos, b.mat, 4, 110.0f);
        if (dmg > 45.0f) addTrauma(0.08f);
    }
}

void SnoutAndSiegeGame::damageBird(BirdTarget& t, float dmg) {
    if (t.defeated || dmg <= 2.0f) return;
    if (t.hasShield && t.shieldHp > 0.0f) {
        float absorbed = std::min(t.shieldHp, dmg);
        t.shieldHp -= absorbed;
        dmg -= absorbed;
        if (screen == ScreenState::ENDLESS && absorbed > 0.0f) {
            endlessScore += static_cast<int>(absorbed * 10.0f * endlessComboMultiplier);
            endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.15f);
        }
        assets.playSound("sound.shield");
        for (int s = 0; s < 6; ++s) {
            Particle sp{};
            sp.pos = t.pos + Vec2(-t.radius * 0.9f, (s - 2.5f) * 6.0f);
            sp.vel = Vec2(-100.0f - s * 20.0f, (s - 2.5f) * 50.0f);
            sp.size = 5.0f + s;
            sp.life = 0.5f;
            sp.maxLife = 0.5f;
            sp.kind = ParticleKind::SPARK_GLINT;
            sp.color = {255, 235, 120, 255};
            particles.push_back(sp);
        }
        if (t.shieldHp <= 0.0f) {
            t.hasShield = false;
            t.shieldHp = 0.0f;
            if (screen == ScreenState::ENDLESS) {
                endlessScore += static_cast<int>(1200 * endlessComboMultiplier);
                endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.3f);
            } else {
                score += 1200;
            }
            floatScores.push_back({t.pos + Vec2(-t.radius, 0.0f), "+1200 SHIELD SHATTERED!", {240, 205, 90, 255}, 1.3f, 1.3f});
            assets.playSound("sound.shield");
            for (int p = 0; p < 4; ++p) {
                Particle sPart{};
                sPart.pos = t.pos + Vec2(-t.radius, (p - 1.5f) * 12.0f);
                sPart.vel = Vec2(-120.0f - p * 35.0f, -140.0f + p * 60.0f);
                sPart.size = t.radius * 0.8f;
                sPart.angle = 0.0f;
                sPart.angVel = 8.0f + p * 2.0f;
                sPart.life = 1.8f;
                sPart.maxLife = 1.8f;
                sPart.kind = ParticleKind::SHIELD_DEBRIS;
                sPart.color = {120, 130, 150, 255};
                particles.push_back(sPart);
            }
        }
        if (dmg <= 2.0f) return;
    }

    if (t.inZeppelin && !t.balloonPopped && t.balloonHp > 0.0f) {
        float absorbed = std::min(t.balloonHp, dmg);
        t.balloonHp -= absorbed;
        dmg -= absorbed;
        if (t.balloonHp <= 0.0f) {
            t.balloonHp = 0.0f;
            t.balloonPopped = true;
            t.inZeppelin = false;
            t.vel.y = 320.0f;
            t.mass = 4.8f;
            t.invMass = 1.0f / t.mass;
            if (screen == ScreenState::ENDLESS) {
                endlessScore += static_cast<int>(1500 * endlessComboMultiplier);
                endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.35f);
            } else {
                score += 1500;
            }
            floatScores.push_back({t.pos + Vec2(0.0f, -t.radius * 1.8f), "+1500 ZEPPELIN DOWNED!", {255, 175, 75, 255}, 1.4f, 1.4f});
            assets.playSound("sound.zeppelin");
            for (int b = 0; b < 6; ++b) {
                Particle bPart{};
                bPart.pos = t.pos + Vec2((b - 2.5f) * 12.0f, -t.radius * 1.8f);
                bPart.vel = Vec2((b - 2.5f) * 55.0f, -90.0f - b * 15.0f);
                bPart.size = t.radius * 0.7f;
                bPart.angle = 0.0f;
                bPart.angVel = 5.0f + b;
                bPart.life = 2.2f;
                bPart.maxLife = 2.2f;
                bPart.kind = ParticleKind::BALLOON_SCRAP;
                bPart.color = {185, 140, 90, 255};
                particles.push_back(bPart);
            }
        }
        if (dmg <= 2.0f) return;
    }

    if (t.hasHelmet && t.helmetHp > 0.0f) {
        float absorbed = std::min(t.helmetHp, dmg);
        t.helmetHp -= absorbed;
        dmg -= absorbed;
        if (screen == ScreenState::ENDLESS && absorbed > 0.0f) {
            endlessScore += static_cast<int>(absorbed * 10.0f * endlessComboMultiplier);
            endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.1f);
        }
        assets.playSound("sound.hit_metal");
        // Metallic sparks on kettle impact
        for (int s = 0; s < 5; ++s) {
            Particle sp{};
            sp.pos = t.pos + Vec2((s - 2.0f) * 6.0f, -t.radius * 0.7f);
            sp.vel = Vec2((s - 2.0f) * 45.0f, -70.0f - (s % 3) * 20.0f);
            sp.size = 5.0f + s;
            sp.life = 0.5f + s * 0.1f;
            sp.maxLife = sp.life;
            sp.kind = ParticleKind::SPARK_GLINT;
            sp.color = {255, 220, 90, 255};
            particles.push_back(sp);
        }
        if (t.helmetHp <= 0.0f) {
            t.hasHelmet = false;
            t.helmetHp = 0.0f;
            if (screen == ScreenState::ENDLESS) {
                endlessScore += static_cast<int>(1000 * endlessComboMultiplier);
                endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.25f);
            } else {
                score += 1000;
            }
            floatScores.push_back({t.pos + Vec2(0.0f, -t.radius * 0.8f), "+1000 HELMET SHATTERED!", {255, 230, 110, 255}, 1.3f, 1.3f});
            assets.playSound("sound.hit_metal");
            // Launch tumbling HELMET_DEBRIS piece
            Particle helmPart{};
            helmPart.pos = t.pos + Vec2(0.0f, -t.radius * 0.6f);
            helmPart.vel = Vec2((dmg > 0.0f ? 60.0f : -60.0f), -220.0f);
            helmPart.angle = 0.0f;
            helmPart.angVel = 12.0f;
            helmPart.size = t.radius * 1.3f;
            helmPart.life = 2.0f;
            helmPart.maxLife = 2.0f;
            helmPart.kind = ParticleKind::HELMET_DEBRIS;
            helmPart.color = {140, 150, 165, 255};
            particles.push_back(helmPart);

            // Achievement: CAULDRON_CRACKER (Knock off 10 enemy bird metal helmets)
            achievementProgress[static_cast<size_t>(AchievementId::CAULDRON_CRACKER)]++;
            saveProgress();
            if (achievementProgress[static_cast<size_t>(AchievementId::CAULDRON_CRACKER)] >= 10) {
                unlockAchievement(AchievementId::CAULDRON_CRACKER);
            }
        }
        if (dmg <= 2.0f) return;
    }
    t.hp -= dmg;
    if (screen == ScreenState::ENDLESS && dmg > 0.0f) {
        endlessScore += static_cast<int>(dmg * 10.0f * endlessComboMultiplier);
        endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.1f);
    }
    if (t.hp <= 0.0f) {
        t.defeated = true;
        addTrauma(0.42f);
        int pts = (t.kind == BirdClass::CONDOR) ? 15000 :
                  (t.kind == BirdClass::FALCON ? 12500 :
                  (t.kind == BirdClass::HERON ? 10000 :
                  (t.kind == BirdClass::OWL ? 7500 : 5000)));
        if (screen == ScreenState::ENDLESS) {
            endlessScore += pts * endlessComboMultiplier;
            endlessComboMultiplier = std::min(5.0f, endlessComboMultiplier + 0.5f);
        } else {
            score += pts;
        }
        spawnFeatherBurst(t.pos, t.kind, 28, 260.0f);
        floatScores.push_back({t.pos, "+" + std::to_string(pts), {255, 235, 110, 255}, 1.4f, 1.4f});
        assets.playSound("sound.bird_defeat");

        // Achievement: FIRST_BLOOD (Defeat your first bird enemy)
        unlockAchievement(AchievementId::FIRST_BLOOD);

        // Achievement: MARKSMAN (Hit a bird enemy from over 500 pixels away with a slingshot shot)
        if ((t.pos - shotLaunchPos).length() > 500.0f) {
            unlockAchievement(AchievementId::MARKSMAN);
        }

        // Achievement: ANVIL_EXECUTIONER (Crush a bird directly using Trotter's Anvil Slam ability)
        if (shotUsedAnvil) {
            unlockAchievement(AchievementId::ANVIL_EXECUTIONER);
        }

        // Achievement: ACROBATIC_HOG (Defeat any bird with a pig after bouncing off a trampoline)
        if (shotHitTrampoline) {
            unlockAchievement(AchievementId::ACROBATIC_HOG);
        }

        // Feature Pack E: Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
        bool allDefeated = true;
        for (const auto& otherBird : birds) {
            if (!otherBird.defeated) {
                allDefeated = false;
                break;
            }
        }
        if (allDefeated && enableSloMoCam && !sloMoActive) {
            sloMoActive = true;
            sloMoTimer = 1.35f;
            sloMoFocusPos = t.pos;
        }
    } else if (dmg > 12.0f) {
        spawnFeatherBurst(t.pos, t.kind, 5, 120.0f);
        if (dmg > 30.0f) addTrauma(0.15f);
    }
}

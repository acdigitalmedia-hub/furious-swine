#include "game.h"

std::array<Vec2, 4> SnoutAndSiegeGame::getBlockVerts(const BlockBody& b) {
    float c = std::cos(b.angle), s = std::sin(b.angle);
    float hw = b.w * 0.5f, hh = b.h * 0.5f;
    Vec2 ux(c, s), uy(-s, c);
    return {
        b.pos - ux * hw - uy * hh,
        b.pos + ux * hw - uy * hh,
        b.pos + ux * hw + uy * hh,
        b.pos - ux * hw + uy * hh
    };
}

float SnoutAndSiegeGame::getTargetFlatAngle(const BlockBody& b) {
    if (b.w >= b.h) {
        if (b.angle > PI * 0.5f) return PI;
        if (b.angle < -PI * 0.5f) return -PI;
        return 0.0f;
    } else {
        if (std::abs(b.angle) < 0.22f) return 0.0f;
        return (b.angle >= 0.0f) ? (PI * 0.5f) : (-PI * 0.5f);
    }
}

float SnoutAndSiegeGame::getTiltFromStable(const BlockBody& b) {
    return b.angle - getTargetFlatAngle(b);
}

void SnoutAndSiegeGame::updateGroundedSupportGraph(float subDt) {
    const size_t nBlocks = blocks.size();
    std::vector<float> bMinX(nBlocks, 1e9f), bMaxX(nBlocks, -1e9f);
    std::vector<float> bMinY(nBlocks, 1e9f), bMaxY(nBlocks, -1e9f);
    std::vector<float> supMinX(nBlocks, 1e9f), supMaxX(nBlocks, -1e9f);

    for (size_t i = 0; i < nBlocks; ++i) {
        blocks[i].hasGroundPath = false;
        blocks[i].stablySupported = false;
        if (blocks[i].destroyed) continue;
        auto verts = getBlockVerts(blocks[i]);
        for (const Vec2& v : verts) {
            bMinX[i] = std::min(bMinX[i], v.x);
            bMaxX[i] = std::max(bMaxX[i], v.x);
            bMinY[i] = std::min(bMinY[i], v.y);
            bMaxY[i] = std::max(bMaxY[i], v.y);
            if (v.y >= GROUND_Y - 3.5f) {
                supMinX[i] = std::min(supMinX[i], v.x);
                supMaxX[i] = std::max(supMaxX[i], v.x);
            }
        }
        if (supMinX[i] <= supMaxX[i]) {
            blocks[i].hasGroundPath = true;
            float tiltErr = std::abs(getTiltFromStable(blocks[i]));
            float margin = 2.5f;
            float eMin = supMinX[i] + margin;
            float eMax = supMaxX[i] - margin;
            if (eMin <= eMax && blocks[i].pos.x >= eMin && blocks[i].pos.x <= eMax && tiltErr < 0.12f) {
                blocks[i].stablySupported = true;
            }
        }
    }

    // Propagate support bottom-up across 5 tiers
    for (int pass = 0; pass < 5; ++pass) {
        for (size_t i = 0; i < nBlocks; ++i) {
            if (blocks[i].destroyed) continue;
            for (size_t j = 0; j < nBlocks; ++j) {
                if (i == j || blocks[j].destroyed) continue;
                if (!blocks[j].hasGroundPath) continue;
                if (blocks[j].pos.y <= blocks[i].pos.y + 2.0f) continue;
                if (std::abs(bMaxY[i] - bMinY[j]) < 10.0f) {
                    float ovMinX = std::max(bMinX[i], bMinX[j]);
                    float ovMaxX = std::min(bMaxX[i], bMaxX[j]);
                    if (ovMaxX > ovMinX) {
                        blocks[i].hasGroundPath = true;
                        if (blocks[j].stablySupported) {
                            supMinX[i] = std::min(supMinX[i], ovMinX);
                            supMaxX[i] = std::max(supMaxX[i], ovMaxX);
                        }
                    }
                }
            }
            if (supMinX[i] <= supMaxX[i]) {
                float tiltErr = std::abs(getTiltFromStable(blocks[i]));
                float margin = 2.5f;
                float eMin = supMinX[i] + margin;
                float eMax = supMaxX[i] - margin;
                if (eMin <= eMax && blocks[i].pos.x >= eMin && blocks[i].pos.x <= eMax && tiltErr < 0.12f) {
                    blocks[i].stablySupported = true;
                }
            }
        }
    }

    // Apply overhanging torque or freefall acceleration to non-stably-supported blocks
    for (size_t i = 0; i < nBlocks; ++i) {
        BlockBody& B = blocks[i];
        if (B.destroyed) continue;
        float tiltErr = getTiltFromStable(B);
        if (!B.hasGroundPath) {
            if (B.vel.y < 0.0f) B.vel.y *= 0.5f;
        } else if (!B.stablySupported) {
            if (supMinX[i] <= supMaxX[i]) {
                float pivotX = (B.pos.x < supMinX[i]) ? supMinX[i] :
                               (B.pos.x > supMaxX[i]  ? supMaxX[i] : B.pos.x);
                float leverX = B.pos.x - pivotX;
                if (std::abs(leverX) > 1.0f) {
                    B.angVel += clampf(leverX * 0.28f, -9.5f, 9.5f) * subDt;
                }
            }
            if (std::abs(tiltErr) > 0.06f) {
                float flatTorque = -clampf(tiltErr * 11.0f, -8.5f, 8.5f);
                B.angVel += flatTorque * subDt;
                if (std::abs(tiltErr) > 0.20f && std::abs(B.angVel) < 0.9f) {
                    damageBlock(B, 95.0f * subDt);
                }
            }
            B.vel.y += 110.0f * subDt;
        } else {
            // Stably supported: high-grip horizontal damping + sleep threshold
            B.vel.x *= 0.90f;
            if (std::abs(B.vel.x) < 10.0f) B.vel.x = 0.0f;
            if (std::abs(B.angVel) < 0.25f && std::abs(tiltErr) <= 0.06f) {
                B.angVel = 0.0f;
                B.angle = getTargetFlatAngle(B);
            }
        }
    }
}

void SnoutAndSiegeGame::stepPhysics(float dt) {
    // Feature Pack C: Atmospheric Hazards & Dynamic Weather
    windGustTimer += dt;
    if (currentLevelIdx >= 0 && currentLevelIdx < static_cast<int>(levels.size())) {
        float baseW = levels[currentLevelIdx].wind;
        if (baseW != 0.0f) {
            currentWind = baseW + std::sin(windGustTimer * 1.6f) * 6.0f;
        } else {
            currentWind = 0.0f;
        }
    }

    // Drifting storm clouds
    for (auto& sc : stormClouds) {
        sc.pos.x += sc.speed * dt;
        if (sc.pos.x > WINDOW_W + sc.width * 0.5f) {
            sc.pos.x = -sc.width * 0.5f;
        }
    }

    // Lightning decay and bolts update
    if (skyFlashAlpha > 0.0f) {
        skyFlashAlpha = std::max(0.0f, skyFlashAlpha - 2.8f * dt);
    }
    for (auto& lb : lightningBolts) {
        lb.life -= dt;
    }
    lightningBolts.erase(std::remove_if(lightningBolts.begin(), lightningBolts.end(),
                         [](const LightningBolt& b) { return b.life <= 0.0f; }),
                         lightningBolts.end());

    // Check if any pig is actively launched in the air
    bool anyPigLaunched = false;
    for (const auto& p : activePigs) {
        if (p.launched) { anyPigLaunched = true; break; }
    }

    simTime += dt;
    // Storm clouds exist exactly when the level being played has a storm
    // (campaign, Endless wave or editor test play alike).
    if (!stormClouds.empty()) {
        if (anyPigLaunched) {
            stormStrikeTimer -= dt;
            if (stormStrikeTimer <= 0.0f) {
                triggerLightningStrike();
                // Derived from simulated time (not the wall clock) so a given
                // sequence of shots always plays out identically.
                stormStrikeTimer = 7.0f + ((static_cast<int>(simTime * 1000.0f) % 40) * 0.1f);
            }
        }
    }

    const int subSteps = 10;
    float subDt = dt / subSteps;

    for (int step = 0; step < subSteps; ++step) {
        // 1. Integrate Pigs (always active once launched)
        for (auto& p : activePigs) {
            if (!p.active || !p.launched) continue;
            // Atmospheric wind acceleration on dynamic airborne swine
            if (p.pos.y + p.radius < GROUND_Y - 2.0f) {
                p.vel.x += (currentWind * 0.85f) * subDt;
            }
            p.vel.y += GRAVITY * subDt;
            p.pos += p.vel * subDt;
            p.angle += p.angVel * subDt;
            p.lifeTimer += subDt;

            if (p.fuseTimer > 0.0f) {
                p.fuseTimer -= subDt;
                if (p.fuseTimer <= 0.0f && !p.exploded) {
                    triggerBombDetonation(p);
                }
            }

            if (p.vortexTimer > 0.0f) {
                p.vortexTimer -= subDt;
                fortressAwake = true;
                float vortexRadius = 240.0f;
                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    Vec2 toPig = p.pos - b.pos;
                    float d = toPig.length();
                    if (d > 10.0f && d < vortexRadius) {
                        float factor = (1.0f - d / vortexRadius);
                        Vec2 pull = toPig.normalized() * (450.0f * factor * subDt);
                        b.vel += pull;
                    }
                }
                for (auto& bt : birds) {
                    if (bt.defeated) continue;
                    Vec2 toPig = p.pos - bt.pos;
                    float d = toPig.length();
                    if (d > 10.0f && d < vortexRadius) {
                        float factor = (1.0f - d / vortexRadius);
                        Vec2 pull = toPig.normalized() * (380.0f * factor * subDt);
                        bt.vel += pull;
                    }
                }
                if ((rand() % 4) == 0) {
                    Particle cm{};
                    float a = (rand() % 360) * (PI / 180.0f);
                    float dist = 30.0f + (rand() % 120);
                    cm.pos = p.pos + Vec2(std::cos(a) * dist, std::sin(a) * dist);
                    cm.vel = Vec2(-std::sin(a) * 120.0f - std::cos(a) * 80.0f,
                                  std::cos(a) * 120.0f - std::sin(a) * 80.0f);
                    cm.size = 5.0f;
                    cm.life = 0.5f;
                    cm.maxLife = 0.5f;
                    cm.kind = ParticleKind::COSMIC_MOTE;
                    cm.color = {190, 110, 255, 255};
                    particles.push_back(cm);
                }
                if (p.vortexTimer <= 0.0f) {
                    triggerGravitonImplosion(p.pos);
                }
            }

            if (p.drillTimer > 0.0f) {
                p.drillTimer -= subDt;
                if (p.pos.y >= GROUND_Y - p.radius - 4.0f) {
                    fortressAwake = true;
                    p.vel.x *= 0.94f;
                    for (auto& b : blocks) {
                        if (b.destroyed) continue;
                        if (std::abs(b.pos.x - p.pos.x) < 95.0f && b.pos.y > GROUND_Y - 95.0f) {
                            damageBlock(b, 60.0f * subDt);
                            b.vel.y += 35.0f * subDt;
                        }
                    }
                    if ((rand() % 3) == 0) {
                        spawnBurst(p.pos, 3, {160, 120, 75, 255}, 120.0f);
                    }
                }
            }

            if (p.pos.y + p.radius > GROUND_Y) {
                p.pos.y = GROUND_Y - p.radius;
                if (p.vel.y > 0.0f) {
                    p.vel.y = -p.vel.y * p.restitution;
                    p.vel.x *= 0.80f;
                    p.angVel = p.vel.x / std::max(10.0f, p.radius);
                }
            }
            if (p.pos.x < -120.0f || p.pos.x > WINDOW_W + 160.0f) {
                p.active = false;
            }
        }

        // 2. Check Pig vs Block & Pig vs Bird collisions (wakes fortress upon first contact!)
        for (auto& p : activePigs) {
            if (!p.active || !p.launched) continue;
            for (auto& b : blocks) {
                if (b.destroyed) continue;
                resolveCircleOBB(p, b);
            }
            for (auto& t : birds) {
                if (t.defeated) continue;
                Vec2 d = t.pos - p.pos;
                float dist = d.length();
                float minDist = p.radius + t.radius;
                if (dist < minDist) {
                    fortressAwake = true;
                    Vec2 n = (dist > 1e-4f) ? (d * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                    float pen = minDist - dist;
                    p.pos -= n * (pen * 0.4f);
                    t.pos += n * (pen * 0.6f);
                    Vec2 rv = t.vel - p.vel;
                    float vn = rv.dot(n);
                    if (vn < 0.0f) {
                        float e = (t.hasShield && t.shieldHp > 0.0f && p.pos.x <= t.pos.x) ? 0.75f : 0.35f;
                        float j = -(1.0f + e) * vn / (p.invMass + t.invMass);
                        Vec2 imp = n * j;
                        p.vel -= imp * p.invMass;
                        t.vel += imp * t.invMass;
                        damageBird(t, std::abs(j) * 0.28f);
                        addTrauma(0.32f);
                        if (p.type == PigType::BOMB && p.fuseTimer < 0.0f && !p.exploded) {
                            p.fuseTimer = 0.45f;
                        }
                    }
                }
            }
            for (auto& pend : pendulums) {
                Vec2 ballPos = pend.getBallPos();
                Vec2 d = p.pos - ballPos;
                float dist = d.length();
                float minDist = p.radius + pend.radius;
                if (dist < minDist) {
                    fortressAwake = true;
                    Vec2 n = (dist > 1e-4f) ? (d * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                    float pen = minDist - dist;
                    p.pos += n * pen;
                    Vec2 ballVel = pend.getBallVel();
                    Vec2 rv = p.vel - ballVel;
                    float vn = rv.dot(n);
                    if (vn < 0.0f) {
                        float e = 0.65f;
                        float j = -(1.0f + e) * vn / (p.invMass + 1.0f / pend.mass);
                        Vec2 imp = n * j;
                        p.vel += imp * p.invMass;
                        Vec2 rAnchor = ballPos - pend.anchor;
                        float torque = cross(rAnchor, -imp);
                        pend.angVel += torque / (pend.mass * pend.length * pend.length);
                        assets.playSound("sound.hit_metal");
                        addTrauma(0.35f);
                    }
                }
            }

            // Proximity collection of Golden Truffle by airborne pig
            if (levelTruffle.exists && !levelTruffle.collected) {
                if ((p.pos - levelTruffle.pos).length() < (p.radius + levelTruffle.radius)) {
                    collectGoldenTruffle();
                }
            }
        }

        // Integrate Aerial Bombs
        for (auto& ab : aerialBombs) {
            if (!ab.active) continue;
            ab.vel.y += GRAVITY * subDt;
            ab.pos += ab.vel * subDt;
            ab.lifeTimer += subDt;

            if (step == 0) {
                Particle sp{};
                sp.pos = ab.pos - ab.vel.normalized() * (ab.radius * 0.8f);
                sp.vel = Vec2(-ab.vel.x * 0.1f, -15.0f);
                sp.size = 3.5f;
                sp.life = 0.28f;
                sp.maxLife = 0.28f;
                sp.kind = ParticleKind::SPARK_GLINT;
                sp.color = {255, 205, 80, 220};
                particles.push_back(sp);
            }

            if (ab.pos.y + ab.radius >= GROUND_Y) {
                detonateAerialBomb(ab);
                continue;
            }

            for (auto& t : birds) {
                if (t.defeated) continue;
                if ((t.pos - ab.pos).length() < t.radius + ab.radius) {
                    detonateAerialBomb(ab);
                    break;
                }
            }
            if (!ab.active) continue;

            for (auto& b : blocks) {
                if (b.destroyed) continue;
                Vec2 local = ab.pos - b.pos;
                float c = std::cos(-b.angle), s = std::sin(-b.angle);
                Vec2 rot(local.x * c - local.y * s, local.x * s + local.y * c);
                float hx = b.w * 0.5f, hy = b.h * 0.5f;
                float cx = clampf(rot.x, -hx, hx);
                float cy = clampf(rot.y, -hy, hy);
                if ((rot - Vec2(cx, cy)).lengthSq() < ab.radius * ab.radius) {
                    detonateAerialBomb(ab);
                    break;
                }
            }
        }

        // 3. Before any pig hits the fortress, keep the fortress in 100% static equilibrium
        //    so blocks and perched birds never drift, sink, or rotate a single pixel!
        if (!fortressAwake) {
            continue;
        }

        // Moving Block vs Golden Truffle check
        if (levelTruffle.exists && !levelTruffle.collected) {
            for (const auto& b : blocks) {
                if (b.destroyed) continue;
                if (b.vel.lengthSq() > 100.0f) {
                    float r = std::max(b.w, b.h) * 0.5f;
                    if ((b.pos - levelTruffle.pos).length() < (r + levelTruffle.radius)) {
                        collectGoldenTruffle();
                        break;
                    }
                }
            }
        }

        // 4. Integrate Blocks under Gravity & Update Burning TNT Fuses
        for (size_t bi = 0; bi < blocks.size(); ++bi) {
            BlockBody& b = blocks[bi];
            if (b.destroyed) continue;
            if (b.mat == MaterialTier::TNT && b.ignited && b.tntFuse > 0.0f) {
                b.tntFuse -= subDt;
                if (b.tntFuse <= 0.0f) {
                    triggerTntDetonation(b);
                    continue;
                }
            }
            b.vel.y += GRAVITY * subDt;
            b.pos += b.vel * subDt;
            b.angle += b.angVel * subDt;
            if (b.angle > PI) b.angle -= 2.0f * PI;
            if (b.angle < -PI) b.angle += 2.0f * PI;
            b.vel.x *= 0.992f;
            b.vel.y *= 0.998f;
            b.angVel *= 0.985f;
        }

        // 5. Integrate Birds under Gravity
        for (auto& t : birds) {
            if (t.defeated) continue;
            if (t.inZeppelin && !t.balloonPopped) {
                t.vel.y += (GRAVITY * 0.15f) * subDt;
            } else {
                t.vel.y += GRAVITY * subDt;
            }
            t.pos += t.vel * subDt;
            t.angle += t.angVel * subDt;
            t.vel.x *= 0.985f;
            t.angVel *= 0.92f;
        }

        // 5b. Integrate Pendulums under Harmonic Gravity & Collide with Blocks and Birds
        for (auto& pend : pendulums) {
            float alpha = (-GRAVITY / pend.length) * std::sin(pend.angle) - 0.25f * pend.angVel;
            pend.angVel += alpha * subDt;
            pend.angle += pend.angVel * subDt;

            Vec2 ballPos = pend.getBallPos();
            Vec2 ballVel = pend.getBallVel();
            float ballSpeed = ballVel.length();

            // Pendulum ball vs Blocks (destructive wrecking ball impact)
            for (auto& b : blocks) {
                if (b.destroyed) continue;
                float c = std::cos(-b.angle), s = std::sin(-b.angle);
                Vec2 rel = ballPos - b.pos;
                Vec2 local(rel.x * c - rel.y * s, rel.x * s + rel.y * c);
                float hw = b.w * 0.5f, hh = b.h * 0.5f;
                Vec2 closest(clampf(local.x, -hw, hw), clampf(local.y, -hh, hh));
                Vec2 diff = local - closest;
                float distSq = diff.lengthSq();
                if (distSq < pend.radius * pend.radius) {
                    float dist = std::sqrt(std::max(1e-6f, distSq));
                    Vec2 localN = diff * (1.0f / dist);
                    float wc = std::cos(b.angle), ws = std::sin(b.angle);
                    Vec2 worldN(localN.x * wc - localN.y * ws, localN.x * ws + localN.y * wc);
                    float pen = pend.radius - dist;
                    b.pos -= worldN * (pen * 0.7f);
                    float dmg = (ballSpeed * 0.45f + 25.0f) * (pend.spiked ? 2.5f : 1.5f);
                    damageBlock(b, dmg);
                    if (ballSpeed > 40.0f) {
                        assets.playSound(pend.spiked ? "sound.hit_metal" : "sound.hit_stone");
                        addTrauma(0.25f);
                    }
                    Vec2 rAnchor = ballPos - pend.anchor;
                    Vec2 imp = worldN * (dmg * 1.5f);
                    float torque = cross(rAnchor, imp);
                    pend.angVel += torque / (pend.mass * pend.length * pend.length);
                }
            }

            // Pendulum ball vs Birds
            for (auto& t : birds) {
                if (t.defeated) continue;
                Vec2 d = t.pos - ballPos;
                float dist = d.length();
                float minDist = pend.radius + t.radius;
                if (dist < minDist) {
                    Vec2 n = (dist > 1e-4f) ? (d * (1.0f / dist)) : Vec2(1.0f, 0.0f);
                    float pen = minDist - dist;
                    t.pos += n * pen;
                    float dmg = (ballSpeed * 0.65f + 45.0f) * (pend.spiked ? 2.2f : 1.4f);
                    damageBird(t, dmg);
                    addTrauma(0.35f);
                    assets.playSound("sound.hit_metal");
                }
            }
        }

        // 6. Update Bottom-Up Grounded Support Graph
        updateGroundedSupportGraph(subDt);

        // 7. Iterative Constraint Solver (Ground -> Block-Block -> Ground -> Bird-Block -> Bird-Ground)
        for (int iter = 0; iter < 6; ++iter) {
            for (auto& b : blocks) {
                if (b.destroyed) continue;
                resolveBlockGround(b, true, subDt);
            }

            for (size_t i = 0; i < blocks.size(); ++i) {
                if (blocks[i].destroyed) continue;
                for (size_t j = i + 1; j < blocks.size(); ++j) {
                    if (blocks[j].destroyed) continue;
                    resolveBlockBlock(blocks[i], blocks[j], true, subDt);
                }
            }

            for (auto& b : blocks) {
                if (b.destroyed) continue;
                resolveBlockGround(b, false, subDt);
            }

            for (auto& t : birds) {
                if (t.defeated) continue;
                for (auto& b : blocks) {
                    if (b.destroyed) continue;
                    resolveBirdBlock(t, b, true, subDt);
                }
            }

            for (auto& t : birds) {
                if (t.defeated) continue;
                if (t.pos.y + t.radius > GROUND_Y) {
                    float impactSpeed = t.vel.length();
                    t.pos.y = GROUND_Y - t.radius;
                    if (t.vel.y > 0.0f) {
                        if (iter == 0 && impactSpeed > 85.0f) {
                            damageBird(t, (impactSpeed - 85.0f) * 0.85f);
                        }
                        t.vel.y = -t.vel.y * 0.12f;
                        t.vel.x *= 0.72f;
                        if (std::abs(t.vel.x) < 12.0f) {
                            t.vel.x = 0.0f;
                            t.angVel = 0.0f;
                        } else {
                            t.angVel = t.vel.x / std::max(10.0f, t.radius);
                        }
                    }
                }
            }
        }
    }

    for (auto& p : activePigs) {
        if (p.active && p.launched && p.lifeTimer < 4.5f) {
            if (p.trail.empty() || (p.pos - p.trail.back()).lengthSq() > 220.0f) {
                p.trail.push_back(p.pos);
                if (p.trail.size() > 28) p.trail.erase(p.trail.begin());
            }
            // Record flight arc for ghost trajectory memory
            if (currentFlightTrail.empty() || (p.pos - currentFlightTrail.back()).lengthSq() > 320.0f) {
                if (currentFlightTrail.size() < 45) {
                    currentFlightTrail.push_back(p.pos);
                }
            }
            // Pig faces forward along trajectory in mid-air
            if (p.vel.lengthSq() > 400.0f && p.pos.y + p.radius < GROUND_Y - 4.0f) {
                p.angle = std::atan2(p.vel.y, p.vel.x);
            }
        }
    }

    for (auto& pt : particles) {
        pt.life -= dt;
        if (pt.isRing) {
            float pRatio = 1.0f - (pt.life / pt.maxLife);
            pt.ringRadius = 16.0f + pRatio * (pt.ringMaxRadius - 16.0f);
        } else if (pt.kind == ParticleKind::FEATHER) {
            // Feathers float and flutter with gentle air drag + wind push
            pt.vel.x += currentWind * 1.2f * dt;
            pt.vel.x *= (1.0f - 1.8f * dt);
            pt.vel.y += 110.0f * dt;
            if (pt.vel.y > 68.0f) pt.vel.y = 68.0f;
            pt.pos += pt.vel * dt;
            pt.pos.x += std::sin(pt.life * pt.wobbleFreq + pt.wobblePhase) * 48.0f * dt;
            pt.angle = std::sin(pt.life * pt.wobbleFreq + pt.wobblePhase) * 0.55f;
        } else if (pt.kind == ParticleKind::SMOKE_PUFF) {
            // Soft smoke puffs drift upward and expand + drift with wind
            pt.vel.x += currentWind * 0.7f * dt;
            pt.vel.x *= (1.0f - 1.4f * dt);
            pt.vel.y -= 14.0f * dt;
            pt.pos += pt.vel * dt;
        } else if (pt.kind == ParticleKind::EMBER) {
            // Volcanic embers rise upward with reverse gravity, sinusoidal sway, and wind
            pt.vel.y -= 95.0f * dt; // buoyant upward rise
            pt.vel.x += currentWind * 0.8f * dt;
            pt.vel.x *= (1.0f - 1.2f * dt);
            pt.pos += pt.vel * dt;
            pt.pos.x += std::sin(pt.life * pt.wobbleFreq + pt.wobblePhase) * 22.0f * dt;

            // Embers melt ICE and ignite TNT on contact!
            for (auto& b : blocks) {
                if (b.destroyed) continue;
                if (b.mat == MaterialTier::ICE || b.mat == MaterialTier::TNT) {
                    float hw = b.w * 0.5f, hh = b.h * 0.5f;
                    if (std::abs(pt.pos.x - b.pos.x) < hw + 4.0f && std::abs(pt.pos.y - b.pos.y) < hh + 4.0f) {
                        if (b.mat == MaterialTier::ICE) {
                            damageBlock(b, 20.0f); // melts ice!
                            pt.life = 0.0f; // quench ember
                            break;
                        } else if (b.mat == MaterialTier::TNT && !b.ignited) {
                            b.ignited = true;
                            b.tntFuse = 0.08f; // ignites TNT!
                            pt.life = 0.0f;
                            break;
                        }
                    }
                }
            }
        } else if (pt.kind == ParticleKind::GOLD_SPARKLE) {
            // Radiating golden sparkle with slight atmospheric drag
            pt.vel *= (1.0f - 1.6f * dt);
            pt.pos += pt.vel * dt;
            pt.angle += pt.angVel * dt;
        } else if (pt.kind == ParticleKind::BALLOON_SCRAP) {
            pt.vel.x += currentWind * 0.9f * dt;
            pt.vel.x *= (1.0f - 1.5f * dt);
            pt.vel.y += 85.0f * dt;
            if (pt.vel.y > 55.0f) pt.vel.y = 55.0f;
            pt.pos += pt.vel * dt;
            pt.angle += pt.angVel * dt;
        } else if (pt.kind == ParticleKind::COSMIC_MOTE) {
            pt.vel *= (1.0f - 2.2f * dt);
            pt.pos += pt.vel * dt;
        } else {
            // Debris, splinters, chunks, sparks
            pt.vel.y += GRAVITY * 0.55f * dt;
            pt.pos += pt.vel * dt;
            pt.angle += pt.angVel * dt;
            if (pt.pos.y > GROUND_Y - 2.0f && pt.vel.y > 0.0f) {
                pt.pos.y = GROUND_Y - 2.0f;
                pt.vel.y = -pt.vel.y * 0.32f;
                pt.vel.x *= 0.65f;
            }
        }
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                    [](const Particle& p) { return p.life <= 0.0f; }), particles.end());

    for (auto& fs : floatScores) {
        fs.life -= dt;
        fs.pos.y -= 38.0f * dt;
    }
    floatScores.erase(std::remove_if(floatScores.begin(), floatScores.end(),
                      [](const FloatingScore& f) { return f.life <= 0.0f; }), floatScores.end());

    evaluateTurnAndWinState(dt);
}

void SnoutAndSiegeGame::resolveBlockGround(BlockBody& b, bool allowDamage, float subDt) {
    auto verts = getBlockVerts(b);
    float maxPen = 0.0f;
    Vec2 contactSum{0.0f, 0.0f};
    int contactCount = 0;

    for (const Vec2& v : verts) {
        float pen = v.y - GROUND_Y;
        if (pen > 0.0f) {
            if (pen > maxPen) maxPen = pen;
            contactSum += v;
            contactCount++;
        }
    }
    if (contactCount == 0) return;

    b.pos.y -= maxPen;
    Vec2 contactPt = contactSum * (1.0f / contactCount);
    contactPt.y = GROUND_Y;

    Vec2 r = contactPt - b.pos;
    Vec2 n(0.0f, -1.0f);
    Vec2 vContact = b.vel + cross(b.angVel, r);
    float vn = vContact.dot(n);

    float tilt = getTiltFromStable(b);

    if (vn < 0.0f) {
        float impactSpeed = -vn;
        if (allowDamage && impactSpeed > 205.0f) {
            damageBlock(b, (impactSpeed - 205.0f) * 0.26f);
            addTrauma(0.16f);
        }
        float rn = cross(r, n);
        float denom = b.invMass + rn * rn * b.invInertia;
        float j = -vn / denom;
        Vec2 imp = n * j;
        b.vel += imp * b.invMass;
        b.angVel += cross(r, imp) * b.invInertia;

        // High ground friction at all angles so blocks grip the ground instead of sliding
        Vec2 tangent(1.0f, 0.0f);
        float vt = (b.vel + cross(b.angVel, r)).dot(tangent);
        float rt = cross(r, tangent);
        float tDenom = b.invMass + rt * rt * b.invInertia;
        float jt = clampf(-vt / tDenom, -j * b.friction, j * b.friction);
        Vec2 fImp = tangent * jt;
        b.vel += fImp * b.invMass;
        b.angVel += cross(r, fImp) * b.invInertia;
        b.vel.x *= 0.88f;
        if (std::abs(b.vel.x) < 10.0f) b.vel.x = 0.0f;
    }

    if (contactCount == 1) {
        if (std::abs(tilt) < 0.08f && std::abs(b.angVel) < 0.6f) {
            b.angle -= tilt * 0.35f;
            b.angVel *= 0.65f;
        } else {
            float flatTorque = -clampf(tilt * 12.0f, -9.0f, 9.0f);
            b.angVel += flatTorque * subDt;
            if (std::abs(tilt) > 0.20f && std::abs(b.angVel) < 0.9f) {
                damageBlock(b, 95.0f * subDt);
            }
        }
    }
}

void SnoutAndSiegeGame::resolveCircleOBB(PigBody& p, BlockBody& b) {
    float c = std::cos(-b.angle), s = std::sin(-b.angle);
    Vec2 rel = p.pos - b.pos;
    Vec2 local(rel.x * c - rel.y * s, rel.x * s + rel.y * c);
    float hw = b.w * 0.5f, hh = b.h * 0.5f;
    Vec2 closest(clampf(local.x, -hw, hw), clampf(local.y, -hh, hh));
    Vec2 diff = local - closest;
    float distSq = diff.lengthSq();
    if (distSq >= p.radius * p.radius) return;

    fortressAwake = true;

    float dist = std::sqrt(distSq);
    Vec2 localN;
    float pen = 0.0f;
    if (dist > 1e-4f) {
        localN = diff * (1.0f / dist);
        pen = p.radius - dist;
    } else {
        float dx = hw - std::abs(local.x);
        float dy = hh - std::abs(local.y);
        if (dx < dy) {
            localN = Vec2((local.x >= 0.0f) ? 1.0f : -1.0f, 0.0f);
            pen = p.radius + dx;
        } else {
            localN = Vec2(0.0f, (local.y >= 0.0f) ? 1.0f : -1.0f);
            pen = p.radius + dy;
        }
    }

    float wc = std::cos(b.angle), ws = std::sin(b.angle);
    Vec2 worldN(localN.x * wc - localN.y * ws, localN.x * ws + localN.y * wc);

    if (b.mat == MaterialTier::TRAMPOLINE) {
        shotHitTrampoline = true;
        p.pos += worldN * pen;
        float inSpeed = p.vel.length();
        float vn = p.vel.dot(worldN);
        if (vn < 0.0f) {
            float bounceSpeed = std::max(inSpeed * 1.65f, 420.0f);
            Vec2 refl = p.vel - worldN * (2.0f * vn);
            if (refl.lengthSq() > 1e-4f) {
                p.vel = refl.normalized() * bounceSpeed;
            } else {
                p.vel = worldN * bounceSpeed;
            }
            assets.playSound("sound.boing");
            addTrauma(0.22f);
            // Emerald & gold trampoline spark glints
            for (int s = 0; s < 6; ++s) {
                Particle sp{};
                sp.pos = p.pos;
                sp.vel = worldN * 180.0f + Vec2((s - 2.5f) * 40.0f, -60.0f);
                sp.size = 6.0f + s;
                sp.life = 0.6f;
                sp.maxLife = 0.6f;
                sp.kind = ParticleKind::SPARK_GLINT;
                sp.color = {100, 255, 160, 255};
                particles.push_back(sp);
            }
        }
        return;
    }

    Vec2 contactPt = p.pos - worldN * p.radius;

    float totalInvMass = p.invMass + b.invMass;
    p.pos += worldN * (pen * (p.invMass / totalInvMass));
    b.pos -= worldN * (pen * (b.invMass / totalInvMass));

    Vec2 rB = contactPt - b.pos;
    Vec2 vB = b.vel + cross(b.angVel, rB);
    Vec2 rv = p.vel - vB;
    float vn = rv.dot(worldN);
    if (vn >= 0.0f) return;

    float rnB = cross(rB, worldN);
    float denom = p.invMass + b.invMass + (rnB * rnB) * b.invInertia;
    float e = std::min(p.restitution, b.restitution);
    float j = -(1.0f + e) * vn / denom;
    Vec2 impulse = worldN * j;

    float pigPierceFactor = 1.0f;
    float dmgMultiplier = 0.19f;
    if (p.type == PigType::TURBO && p.abilityUsed && b.mat != MaterialTier::STONE) {
        pigPierceFactor = 0.65f;
        dmgMultiplier = 0.35f;
    } else if (p.type == PigType::TROTTER && p.abilityUsed) {
        dmgMultiplier = 0.34f;
    } else if (p.type == PigType::SPLIT && (b.mat == MaterialTier::TWIG || b.mat == MaterialTier::ICE)) {
        dmgMultiplier = 0.38f;
    } else if (p.type == PigType::DRILL && p.drillTimer > 0.0f) {
        // Stone bites back: the bit slows more and wears out faster on it.
        const bool hard = (b.mat == MaterialTier::STONE);
        pigPierceFactor = hard ? 0.45f : 0.22f;
        dmgMultiplier = 0.68f;
        if (b.drillStamp != drillBoreId) {
            b.drillStamp = drillBoreId;
            shotDrillPierces++;
            if (shotDrillPierces >= 3) {
                unlockAchievement(AchievementId::DEEP_BORE);
            }
            const float wear = hard ? 0.40f : (b.mat == MaterialTier::TIMBER ? 0.18f : 0.08f);
            p.drillTimer = std::max(0.0f, p.drillTimer - wear);
        }
        assets.playSound("sound.drill", 0.55f);
        for (int k = 0; k < 4; ++k) {
            Particle sp{};
            sp.pos = contactPt;
            sp.vel = Vec2(-p.vel.x * 0.4f + (k - 1.5f) * 50.0f, -80.0f - k * 30.0f);
            sp.size = 5.0f;
            sp.life = 0.4f;
            sp.maxLife = 0.4f;
            sp.kind = ParticleKind::SPARK_GLINT;
            sp.color = {255, 180, 50, 255};
            particles.push_back(sp);
        }
    }
    if (b.mat == MaterialTier::ICE) {
        dmgMultiplier = std::max(dmgMultiplier, 0.44f);
        pigPierceFactor = std::min(pigPierceFactor, 0.55f);
    }

    p.vel += impulse * (p.invMass * pigPierceFactor);
    b.vel -= impulse * b.invMass;
    b.angVel -= cross(rB, impulse) * b.invInertia;
    p.angVel = p.vel.x / std::max(12.0f, p.radius);

    damageBlock(b, std::abs(j) * dmgMultiplier);
    addTrauma(clampf(std::abs(j) * 0.0006f, 0.08f, 0.38f));

    if (p.type == PigType::BOMB && p.fuseTimer < 0.0f && !p.exploded) {
        p.fuseTimer = 0.42f;
    }
    if (b.mat == MaterialTier::TNT && !b.ignited) {
        b.ignited = true;
        b.tntFuse = 0.10f;
    }
}

void SnoutAndSiegeGame::resolveBirdBlock(BirdTarget& t, BlockBody& b, bool allowDamage, float subDt) {
    float c = std::cos(-b.angle), s = std::sin(-b.angle);
    Vec2 rel = t.pos - b.pos;
    Vec2 local(rel.x * c - rel.y * s, rel.x * s + rel.y * c);
    float hw = b.w * 0.5f, hh = b.h * 0.5f;
    Vec2 closest(clampf(local.x, -hw, hw), clampf(local.y, -hh, hh));
    Vec2 diff = local - closest;
    float distSq = diff.lengthSq();
    if (distSq >= t.radius * t.radius) return;

    if (t.pos.y < b.pos.y && !b.stablySupported && !t.balloonPopped && t.mass <= 3.0f) {
        if (allowDamage && (t.vel - b.vel).length() > 60.0f) {
            damageBird(t, 42.0f);
        }
        return;
    }

    float dist = std::sqrt(distSq);
    Vec2 localN;
    float pen = 0.0f;
    if (dist > 1e-4f) {
        localN = diff * (1.0f / dist);
        pen = t.radius - dist;
    } else {
        float dx = hw - std::abs(local.x);
        float dy = hh - std::abs(local.y);
        if (dx < dy) {
            localN = Vec2((local.x >= 0.0f) ? 1.0f : -1.0f, 0.0f);
            pen = t.radius + dx;
        } else {
            localN = Vec2(0.0f, (local.y >= 0.0f) ? 1.0f : -1.0f);
            pen = t.radius + dy;
        }
    }

    float wc = std::cos(b.angle), ws = std::sin(b.angle);
    Vec2 worldN(localN.x * wc - localN.y * ws, localN.x * ws + localN.y * wc);
    Vec2 contactPt = t.pos - worldN * t.radius;

    if (worldN.y > 0.25f) {
        t.pos.x += (t.pos.x >= b.pos.x ? 1.0f : -1.0f) * pen * 0.4f;
        if (allowDamage) {
            damageBird(t, b.mass * 4.2f);
        }
    } else {
        if (std::abs(local.x) > hw - 3.0f && worldN.y < -0.2f) {
            float slipDir = (t.pos.x >= b.pos.x) ? 1.0f : -1.0f;
            t.vel.x += slipDir * 90.0f * subDt;
            t.vel.y += 60.0f * subDt;
        } else if (b.stablySupported && worldN.y < -0.7f) {
            // Perched on a stable deck: lift the bird 100% out of penetration without pushing the deck down!
            t.pos += worldN * pen;
        } else {
            float totalInv = t.invMass + b.invMass;
            float corr = std::max(0.0f, pen - 0.05f) * 0.65f / totalInv;
            t.pos += worldN * (corr * t.invMass);
            b.pos -= worldN * (corr * b.invMass);
        }
    }

    Vec2 rB = contactPt - b.pos;
    Vec2 vB = b.vel + cross(b.angVel, rB);
    Vec2 rv = t.vel - vB;
    float vn = rv.dot(worldN);
    if (vn >= 0.0f) return;

    float rnB = cross(rB, worldN);
    float denom = t.invMass + b.invMass + rnB * rnB * b.invInertia;
    float j = -vn / denom;
    Vec2 imp = worldN * j;
    t.vel += imp * t.invMass;
    if (worldN.y <= 0.25f && !b.stablySupported) {
        b.vel -= imp * b.invMass;
        b.angVel -= cross(rB, imp) * b.invInertia;
    }

    // Prevent perched birds from slowly rotating on top of stable beams
    if (b.stablySupported && worldN.y < -0.7f && std::abs(rv.x) < 28.0f) {
        t.vel.x = b.vel.x;
        t.angVel = 0.0f;
        t.angle *= 0.80f;
    } else {
        float mu = (std::abs(worldN.y) > 0.85f && std::abs(local.x) <= hw - 3.0f) ? 0.65f : 0.0f;
        Vec2 tangent(-worldN.y, worldN.x);
        float vt = rv.dot(tangent);
        float jt = clampf(-vt / denom, -j * mu, j * mu);
        t.vel += tangent * (jt * t.invMass);
        t.angVel = t.vel.x / std::max(10.0f, t.radius);
    }

    float relSpeed = -vn;
    if (allowDamage && relSpeed > 58.0f) {
        damageBird(t, (relSpeed - 58.0f) * 0.88f * std::max(1.0f, b.mass * 0.50f));
        if (t.balloonPopped || t.mass > 3.0f) {
            damageBlock(b, (relSpeed - 58.0f) * 1.15f * t.mass);
            spawnSplinters(b.pos, b.mat, 6, 160.0f);
        }
    }
}

void SnoutAndSiegeGame::resolveBlockBlock(BlockBody& A, BlockBody& B, bool allowDamage, float /*subDt*/) {
    auto vertsA = getBlockVerts(A);
    auto vertsB = getBlockVerts(B);

    float cA = std::cos(A.angle), sA = std::sin(A.angle);
    float cB = std::cos(B.angle), sB = std::sin(B.angle);
    Vec2 axes[4] = {
        {cA, sA}, {-sA, cA},
        {cB, sB}, {-sB, cB}
    };

    float minOverlap = 1e9f;
    Vec2 bestAxis{0.0f, 0.0f};

    for (int i = 0; i < 4; ++i) {
        Vec2 axis = axes[i];
        float minA = 1e9f, maxA = -1e9f;
        float minB = 1e9f, maxB = -1e9f;
        for (const Vec2& v : vertsA) {
            float p = v.dot(axis);
            minA = std::min(minA, p);
            maxA = std::max(maxA, p);
        }
        for (const Vec2& v : vertsB) {
            float p = v.dot(axis);
            minB = std::min(minB, p);
            maxB = std::max(maxB, p);
        }
        float overlap = std::min(maxA, maxB) - std::max(minA, minB);
        if (overlap <= 0.0f) return;
        if (overlap < minOverlap) {
            minOverlap = overlap;
            bestAxis = axis;
        }
    }

    Vec2 delta = B.pos - A.pos;
    Vec2 n = bestAxis;
    if (delta.dot(n) < 0.0f) {
        n = n * -1.0f;
    }

    // For vertically stacked blocks, align normal with true vertical axis without ad-hoc horizontal flings
    float tiltA = std::abs(getTiltFromStable(A));
    float tiltB = std::abs(getTiltFromStable(B));
    if (std::abs(delta.y) > (A.h + B.h) * 0.25f && std::abs(n.y) > 0.55f && tiltA < 0.08f && tiltB < 0.08f) {
        n = Vec2(0.0f, (delta.y > 0.0f) ? 1.0f : -1.0f);
    }

    auto pointInOBB = [](Vec2 pt, const BlockBody& box, float margin) -> bool {
        float c = std::cos(-box.angle), s = std::sin(-box.angle);
        Vec2 rel = pt - box.pos;
        float lx = std::abs(rel.x * c - rel.y * s);
        float ly = std::abs(rel.x * s + rel.y * c);
        return (lx <= box.w * 0.5f + margin) && (ly <= box.h * 0.5f + margin);
    };

    std::vector<Vec2> contactPts;
    for (const Vec2& v : vertsB) {
        if (pointInOBB(v, A, 2.0f)) {
            contactPts.push_back(v);
        }
    }
    for (const Vec2& v : vertsA) {
        if (pointInOBB(v, B, 2.0f)) {
            contactPts.push_back(v);
        }
    }

    if (contactPts.empty()) {
        float aMinX = 1e9f, aMaxX = -1e9f, aMinY = 1e9f, aMaxY = -1e9f;
        float bMinX = 1e9f, bMaxX = -1e9f, bMinY = 1e9f, bMaxY = -1e9f;
        for (int i = 0; i < 4; ++i) {
            aMinX = std::min(aMinX, vertsA[i].x); aMaxX = std::max(aMaxX, vertsA[i].x);
            aMinY = std::min(aMinY, vertsA[i].y); aMaxY = std::max(aMaxY, vertsA[i].y);
            bMinX = std::min(bMinX, vertsB[i].x); bMaxX = std::max(bMaxX, vertsB[i].x);
            bMinY = std::min(bMinY, vertsB[i].y); bMaxY = std::max(bMaxY, vertsB[i].y);
        }
        contactPts.push_back(Vec2(0.5f * (std::max(aMinX, bMinX) + std::min(aMaxX, bMaxX)),
                                  0.5f * (std::max(aMinY, bMinY) + std::min(aMaxY, bMaxY))));
    } else if (contactPts.size() > 2) {
        Vec2 tangent(-n.y, n.x);
        float minProj = 1e9f, maxProj = -1e9f;
        Vec2 pMin = contactPts[0], pMax = contactPts[0];
        for (const Vec2& pt : contactPts) {
            float proj = pt.dot(tangent);
            if (proj < minProj) { minProj = proj; pMin = pt; }
            if (proj > maxProj) { maxProj = proj; pMax = pt; }
        }
        contactPts.clear();
        contactPts.push_back(pMin);
        if (maxProj - minProj > 1.0f) {
            contactPts.push_back(pMax);
        }
    }

    // Directional Support-Aware Positional Correction:
    // If one block is resting on top of a stably-supported lower block, lift the upper block
    // 100% out of overlap without pushing or tilting the stably-supported lower block!
    const float slop = 0.01f;
    float pen = std::max(0.0f, minOverlap - slop);
    if (n.y < -0.7f && A.stablySupported) {
        // B is above A, and A is stably supported
        B.pos += n * pen;
        if (B.stablySupported) {
            if (B.vel.y > A.vel.y) B.vel.y = A.vel.y;
            B.vel.x *= 0.85f;
            if (std::abs(B.vel.x) < 10.0f) B.vel.x = 0.0f;
            B.angVel = 0.0f;
            B.angle = getTargetFlatAngle(B);
            return;
        }
    } else if (n.y > 0.7f && B.stablySupported) {
        // A is above B, and B is stably supported
        A.pos -= n * pen;
        if (A.stablySupported) {
            if (A.vel.y > B.vel.y) A.vel.y = B.vel.y;
            A.vel.x *= 0.85f;
            if (std::abs(A.vel.x) < 10.0f) A.vel.x = 0.0f;
            A.angVel = 0.0f;
            A.angle = getTargetFlatAngle(A);
            return;
        }
    } else {
        float totalInv = A.invMass + B.invMass;
        float corr = pen * 0.55f / totalInv;
        Vec2 dPosA = n * (-corr * A.invMass);
        Vec2 dPosB = n * ( corr * B.invMass);
        if (!A.hasGroundPath && dPosA.y < 0.0f) dPosA.y = 0.0f;
        if (!B.hasGroundPath && dPosB.y < 0.0f) dPosB.y = 0.0f;
        A.pos += dPosA;
        B.pos += dPosB;
    }

    float numPts = static_cast<float>(contactPts.size());
    float mu = 0.5f * (A.friction + B.friction);
    Vec2 tangent(-n.y, n.x);

    for (const Vec2& pt : contactPts) {
        Vec2 rA = pt - A.pos;
        Vec2 rB = pt - B.pos;
        Vec2 vA = A.vel + cross(A.angVel, rA);
        Vec2 vB = B.vel + cross(B.angVel, rB);
        Vec2 rv = vB - vA;
        float vn = rv.dot(n);
        if (vn >= 0.0f) continue;

        float rnA = cross(rA, n);
        float rnB = cross(rB, n);
        float denom = A.invMass + B.invMass + rnA * rnA * A.invInertia + rnB * rnB * B.invInertia;
        if (denom < 1e-6f) continue;

        // Resting restitution: zero restitution when relative speed is low (< 30.0f)
        float e = (std::abs(vn) < 30.0f) ? 0.0f : std::min(A.restitution, B.restitution);
        float j = -(1.0f + e) * vn / denom;
        j /= numPts;
        Vec2 imp = n * j;

        Vec2 dVelA = imp * (-A.invMass);
        Vec2 dVelB = imp * ( B.invMass);
        if (!B.hasGroundPath && dVelA.y < 0.0f) dVelA.y = 0.0f;
        if (!A.hasGroundPath && dVelB.y < 0.0f) dVelB.y = 0.0f;

        A.vel += dVelA;
        A.angVel -= cross(rA, imp) * A.invInertia;
        B.vel += dVelB;
        B.angVel += cross(rB, imp) * B.invInertia;

        // High block-on-block friction so stacked pieces grip realistically instead of skating
        Vec2 vA2 = A.vel + cross(A.angVel, rA);
        Vec2 vB2 = B.vel + cross(B.angVel, rB);
        Vec2 rv2 = vB2 - vA2;
        float vt = rv2.dot(tangent);
        float rtA = cross(rA, tangent);
        float rtB = cross(rB, tangent);
        float tDenom = A.invMass + B.invMass + rtA * rtA * A.invInertia + rtB * rtB * B.invInertia;
        if (tDenom > 1e-6f) {
            float jt = clampf(-vt / tDenom, -j * mu, j * mu);
            Vec2 fImp = tangent * jt;

            A.vel -= fImp * A.invMass;
            A.angVel -= cross(rA, fImp) * A.invInertia;
            B.vel += fImp * B.invMass;
            B.angVel += cross(rB, fImp) * B.invInertia;
        }

        float relSpd = -vn;
        if (allowDamage && relSpd > 190.0f) {
            float dmg = (relSpd - 190.0f) * 0.22f / numPts;
            damageBlock(A, dmg);
            damageBlock(B, dmg);
        }
    }
}

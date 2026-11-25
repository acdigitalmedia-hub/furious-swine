// level_lab: developer tool for designing and checking campaign levels.
//
//   level_lab stable  <N|all>          idle-stability check (same rules as --self-test)
//   level_lab solve   <N|all> [fine] [notramp] [robust]
//                                      greedy shot search: can the level be cleared,
//                                      with how many pigs, and do good shots use
//                                      the level's trampolines? "notramp" swaps
//                                      trampolines for stone to measure that.
//                                      Also reports "skilled": pigs needed when
//                                      each shot is judged by its worst nearby
//                                      result (+-2.5 deg, +-10% power); "robust"
//                                      prints that plan instead of the perfect one.
//   level_lab render  <N> <out.bmp> [shots] [freeze_seconds]
//                                      picture of level N; with shots (e.g. "1"),
//                                      plays that many solver shots first and
//                                      shows the last shot's flight path
//                                      (optionally frozen mid-flight).
//
// N is the 1-based level number; levels load from assets/levels/ (or the
// directory in $LEVELS_DIR, handy for experiments). Run from the
// project root. Gameplay side effects (save files) are redirected to a
// temporary directory so the player's progress is never touched.
#include "game.h"
#include "text.h"

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <thread>
#include <unistd.h>

namespace {

constexpr float kDt = 1.0f / 60.0f;

struct Shot {
    float angleDeg = 45.0f;
    float power = 1.0f;        // fraction of max pull
    float abilityAt = -1.0f;   // seconds after launch; < 0 = never
};

struct Outcome {
    int birdsBefore = 0;
    int birdsAfter = 0;
    int score = 0;
    bool usedTrampoline = false;
    bool complete = false;
    bool failed = false;
};

int aliveBirds(const SnoutAndSiegeGame& g) {
    int n = 0;
    for (const auto& b : g.birds) if (!b.defeated) ++n;
    return n;
}

bool pigReady(const SnoutAndSiegeGame& g) {
    return !g.activePigs.empty() && !g.activePigs[0].launched;
}

// Fires one shot and simulates until the next pig is ready or the level ends.
Outcome fire(SnoutAndSiegeGame& g, const Shot& s, int maxFrames = 60 * 30) {
    Outcome o;
    o.birdsBefore = aliveBirds(g);
    float a = s.angleDeg * PI / 180.0f;
    float len = MAX_PULL * s.power;
    g.launchCurrentPig(Vec2(-std::cos(a) * len, std::sin(a) * len));
    bool abilityDone = false;
    for (int f = 0; f < maxFrames; ++f) {
        float t = f * kDt;
        if (!abilityDone && s.abilityAt >= 0.0f && t >= s.abilityAt) {
            g.activateMidAirAbility();
            abilityDone = true;
        }
        g.updateGame(kDt);
        o.usedTrampoline |= g.shotHitTrampoline;
        if (g.levelComplete || g.levelFailed) break;
        if (f > 10 && pigReady(g)) break;
    }
    o.birdsAfter = aliveBirds(g);
    o.score = g.score;
    o.complete = g.levelComplete;
    o.failed = g.levelFailed;
    return o;
}

void startLevel(SnoutAndSiegeGame& g, int idx) {
    srand(12345);
    g.loadLevel(idx);
    g.enableSloMoCam = false;
}

struct GridDims { int angles = 0, powers = 0, abilities = 0; };

std::vector<Shot> shotGrid(bool fine, GridDims* dims = nullptr) {
    std::vector<Shot> grid;
    float angStep = fine ? 1.25f : 2.5f;
    std::vector<float> powers = fine ? std::vector<float>{0.5f, 0.6f, 0.68f, 0.76f, 0.84f, 0.92f, 1.0f}
                                     : std::vector<float>{0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
    std::vector<float> abilities = {-1.0f, 0.4f, 0.7f, 1.0f, 1.3f};
    int na = 0;
    for (float a = -5.0f; a <= 75.01f; a += angStep, ++na)
        for (float p : powers)
            for (float ab : abilities) grid.push_back({a, p, ab});
    if (dims) *dims = {na, static_cast<int>(powers.size()), static_cast<int>(abilities.size())};
    return grid;
}

struct Worker {
    SnoutAndSiegeGame game;
};

// Runs all candidate shots after replaying `prefix`, in parallel.
std::vector<Outcome> sweep(std::vector<std::unique_ptr<Worker>>& workers, int idx,
                           const std::vector<Shot>& prefix, const std::vector<Shot>& grid) {
    std::vector<Outcome> out(grid.size());
    std::atomic<size_t> next{0};
    std::vector<std::thread> threads;
    for (auto& w : workers) {
        threads.emplace_back([&, wp = w.get()] {
            for (size_t i = next++; i < grid.size(); i = next++) {
                startLevel(wp->game, idx);
                for (const auto& s : prefix) fire(wp->game, s);
                out[i] = fire(wp->game, grid[i]);
            }
        });
    }
    for (auto& t : threads) t.join();
    return out;
}

bool better(const Outcome& a, const Outcome& b) {
    if (a.complete != b.complete) return a.complete;
    int ka = a.birdsBefore - a.birdsAfter, kb = b.birdsBefore - b.birdsAfter;
    if (ka != kb) return ka > kb;
    return a.score > b.score;
}

const char* pigName(PigType t) {
    static const char* names[] = {"trotter", "turbo", "split", "bomb", "glider", "graviton", "drill"};
    return names[static_cast<int>(t)];
}

struct SolveResult {
    bool solved = false;
    int pigsUsed = 0;
    int pigsTotal = 0;
    int birdsLeft = 0;
    int score = 0;
    int stars = 0;
    float firstShotHitRate = 0.0f;  // fraction of pig-1 shots killing >= 1 bird
    int firstShotBest = 0;          // most birds killed by one pig-1 shot
    int tramplinesInLevel = 0;
    float trampolineShareOfGood = 0.0f;  // of pig-1 shots killing >= max(1,best-1), share using trampoline
    int trampolineShotsUseful = 0;       // pig-1 shots that bounce AND kill >= 1 bird
    std::vector<Shot> shots;
    std::vector<Outcome> outcomes;
};

// Kills a shot can be relied on for: the worst result among its neighbours
// (one grid step of angle and/or power, same ability timing). Models a
// skilled player who can't repeat an exact pixel-perfect pull.
std::vector<int> robustKills(const std::vector<Outcome>& outs, const GridDims& d) {
    std::vector<int> r(outs.size());
    auto at = [&](int a, int p, int b) { return (a * d.powers + p) * d.abilities + b; };
    for (int a = 0; a < d.angles; ++a)
        for (int p = 0; p < d.powers; ++p)
            for (int b = 0; b < d.abilities; ++b) {
                int worst = 1 << 30;
                for (int da = -1; da <= 1; ++da)
                    for (int dp = -1; dp <= 1; ++dp) {
                        int na = a + da, np = p + dp;
                        if (na < 0 || na >= d.angles || np < 0 || np >= d.powers) continue;
                        const Outcome& o = outs[at(na, np, b)];
                        worst = std::min(worst, o.complete ? 99 : o.birdsBefore - o.birdsAfter);
                    }
                r[at(a, p, b)] = worst;
            }
    return r;
}

SolveResult solve(std::vector<std::unique_ptr<Worker>>& workers, int idx, bool fine, bool verbose,
                  bool robust = false) {
    SolveResult r;
    GridDims dims;
    auto grid = shotGrid(fine, &dims);
    SnoutAndSiegeGame& g0 = workers[0]->game;
    startLevel(g0, idx);
    r.pigsTotal = static_cast<int>(g0.remainingPigs.size()) + (pigReady(g0) ? 1 : 0);
    for (const auto& b : g0.blocks) if (b.mat == MaterialTier::TRAMPOLINE) r.tramplinesInLevel++;

    for (int pig = 0; pig < r.pigsTotal; ++pig) {
        auto outs = sweep(workers, idx, r.shots, grid);
        size_t best = 0;
        if (robust) {
            auto rk = robustKills(outs, dims);
            for (size_t i = 1; i < outs.size(); ++i) {
                if (rk[i] > rk[best] || (rk[i] == rk[best] && better(outs[i], outs[best]))) best = i;
            }
        } else {
            for (size_t i = 1; i < outs.size(); ++i) if (better(outs[i], outs[best])) best = i;
        }
        if (pig == 0) {
            int hits = 0, good = 0, goodTramp = 0;
            for (const auto& o : outs) {
                int k = o.birdsBefore - o.birdsAfter;
                r.firstShotBest = std::max(r.firstShotBest, k);
                if (k >= 1) hits++;
                if (k >= 1 && o.usedTrampoline) r.trampolineShotsUseful++;
            }
            int goodBar = std::max(1, r.firstShotBest - 1);
            for (const auto& o : outs) {
                if (o.birdsBefore - o.birdsAfter >= goodBar) {
                    good++;
                    if (o.usedTrampoline) goodTramp++;
                }
            }
            r.firstShotHitRate = static_cast<float>(hits) / outs.size();
            r.trampolineShareOfGood = good ? static_cast<float>(goodTramp) / good : 0.0f;
        }
        r.shots.push_back(grid[best]);
        r.outcomes.push_back(outs[best]);
        const Outcome& o = outs[best];
        if (verbose) {
            std::printf("  pig %d (%s): angle %.1f power %.2f ability %s -> birds %d->%d score %d%s%s\n",
                        pig + 1, pigName(g0.levels[idx].pigQueue[pig]), grid[best].angleDeg, grid[best].power,
                        grid[best].abilityAt < 0 ? "none" : std::to_string(grid[best].abilityAt).substr(0, 3).c_str(),
                        o.birdsBefore, o.birdsAfter, o.score, o.usedTrampoline ? " [TRAMPOLINE]" : "",
                        o.complete ? " CLEARED" : "");
        }
        r.birdsLeft = o.birdsAfter;
        r.score = o.score;
        if (o.complete) { r.solved = true; r.pigsUsed = pig + 1; break; }
        if (o.failed) break;
    }
    // Score with remaining-pig bonus as the game computes it.
    if (r.solved) {
        startLevel(g0, idx);
        for (const auto& s : r.shots) fire(g0, s);
        r.score = g0.score;
        r.stars = g0.earnedStars;
    }
    return r;
}

bool stable(SnoutAndSiegeGame& g, int idx, std::string& why) {
    g.loadLevel(idx);
    g.fortressAwake = true;
    auto b0 = g.blocks;
    auto t0 = g.birds;
    for (int f = 0; f < 300; ++f) g.stepPhysics(kDt);
    float tilt = 0, disp = 0, brot = 0, bdisp = 0;
    for (size_t i = 0; i < g.blocks.size(); ++i) {
        tilt = std::max(tilt, std::abs(g.blocks[i].angle - b0[i].angle));
        disp = std::max(disp, (g.blocks[i].pos - b0[i].pos).length());
    }
    for (size_t i = 0; i < g.birds.size(); ++i) {
        brot = std::max(brot, std::abs(g.birds[i].angle));
        bdisp = std::max(bdisp, (g.birds[i].pos - t0[i].pos).length());
    }
    for (const auto& p : g.pendulums) tilt = std::max(tilt, std::abs(p.angle));
    char buf[256];
    std::snprintf(buf, sizeof buf, "tilt=%.3f disp=%.2f birdRot=%.3f birdDisp=%.2f", tilt, disp, brot, bdisp);
    why = buf;
    bool ok = tilt <= 0.01f && disp <= 1.5f && brot <= 0.01f && bdisp <= 1.5f;
    if (!ok) {
        for (size_t i = 0; i < g.blocks.size(); ++i) {
            float d = (g.blocks[i].pos - b0[i].pos).length();
            if (d > 0.5f) {
                std::snprintf(buf, sizeof buf, "\n    block %zu (%.0f,%.0f %gx%g) moved %.1fpx", i,
                              b0[i].pos.x, b0[i].pos.y, b0[i].w, b0[i].h, d);
                why += buf;
            }
        }
        for (size_t i = 0; i < g.birds.size(); ++i) {
            float d = (g.birds[i].pos - t0[i].pos).length();
            if (d > 0.5f) {
                std::snprintf(buf, sizeof buf, "\n    bird %zu (%.0f,%.0f) moved %.1fpx", i, t0[i].pos.x, t0[i].pos.y, d);
                why += buf;
            }
        }
    }
    return ok;
}

std::vector<int> parseLevels(const std::string& arg) {
    std::vector<int> v;
    if (arg == "all") { for (int i = 0; i < NUM_LEVELS; ++i) v.push_back(i); return v; }
    // comma list and ranges: 1,5,10-20
    size_t pos = 0;
    while (pos < arg.size()) {
        size_t comma = arg.find(',', pos);
        std::string part = arg.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        size_t dash = part.find('-');
        int a = std::atoi(part.c_str());
        int b = dash == std::string::npos ? a : std::atoi(part.c_str() + dash + 1);
        for (int i = a; i <= b; ++i) if (i >= 1 && i <= NUM_LEVELS) v.push_back(i - 1);
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return v;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: level_lab stable|solve|render <levels> [...]\n");
        return 2;
    }
    std::string cmd = argv[1];
    std::vector<int> lv = parseLevels(argv[2]);
    if (lv.empty()) { std::fprintf(stderr, "no valid levels in '%s'\n", argv[2]); return 2; }

    setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;

    unsigned nThreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::unique_ptr<Worker>> workers;
    for (unsigned i = 0; i < (cmd == "solve" ? nThreads : 1u); ++i) {
        workers.push_back(std::make_unique<Worker>());
        workers.back()->game.persistProgress = false;
        workers.back()->game.tutorialHints = false;
        const char* dir = std::getenv("LEVELS_DIR");  // default: assets/levels
        if (!workers.back()->game.initLevels(dir ? dir : "assets/levels")) return 1;
    }
    if (cmd == "render") {
        win = SDL_CreateWindow("level_lab", 0, 0, WINDOW_W, WINDOW_H, 0);
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        workers[0]->game.assets.loadAll(ren);
        textLoadFont();  // before we leave the project directory
    }

    // Keep progress/achievement saves away from the real save file.
    std::string tmpl = (std::filesystem::temp_directory_path() / "level_lab_XXXXXX").string();
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    const char* tmpDir = mkdtemp(buf.data());
    if (!tmpDir || chdir(tmpDir) != 0) { std::perror("level_lab: temp dir"); return 1; }

    int rc = 0;
    if (cmd == "stable") {
        for (int idx : lv) {
            std::string why;
            bool ok = stable(workers[0]->game, idx, why);
            if (!ok) rc = 1;
            std::printf("level %3d: %s  %s\n", idx + 1, ok ? "stable  " : "UNSTABLE", why.c_str());
        }
    } else if (cmd == "solve") {
        bool fine = false, noTramp = false, robustOnly = false;
        for (int a = 3; a < argc; ++a) {
            if (std::string(argv[a]) == "fine") fine = true;
            if (std::string(argv[a]) == "robust") robustOnly = true;
            if (std::string(argv[a]) == "notramp") noTramp = true;
        }
        if (noTramp) {
            // Same level with every trampoline turned into plain stone: shows
            // how much the level actually depends on bouncing.
            for (auto& w : workers) {
                for (auto& L : w->game.levels) {
                    for (auto& b : L.blocks) {
                        if (b.mat == MaterialTier::TRAMPOLINE)
                            b = SnoutAndSiegeGame::makeBlock(MaterialTier::STONE, b.pos.x, b.pos.y, b.w, b.h);
                    }
                }
            }
        }
        bool verbose = lv.size() == 1;
        for (int idx : lv) {
            const LevelSpec& L = workers[0]->game.levels[idx];
            if (verbose) std::printf("level %d: %s\n", idx + 1, L.name.c_str());
            SolveResult r = solve(workers, idx, fine, verbose && !robustOnly);
            SolveResult h = solve(workers, idx, fine, verbose && robustOnly, true);
            std::printf("level %3d: %s pigs %d/%d skilled %s birdsLeft %d score %6d stars %d (2*=%d 3*=%d) | "
                        "pig1 hit%% %3.0f best %d | tramps %d trampGood%% %3.0f trampUseful %d\n",
                        idx + 1, r.solved ? "SOLVED  " : "UNSOLVED", r.pigsUsed, r.pigsTotal,
                        h.solved ? std::to_string(h.pigsUsed).c_str() : "X", r.birdsLeft,
                        r.score, r.stars, L.star2Threshold, L.star3Threshold, r.firstShotHitRate * 100.0f,
                        r.firstShotBest, r.tramplinesInLevel, r.trampolineShareOfGood * 100.0f,
                        r.trampolineShotsUseful);
            std::fflush(stdout);
        }
    } else if (cmd == "render") {
        if (argc < 4) { std::fprintf(stderr, "render needs an output path\n"); return 2; }
        std::string outPath = argv[3];
        if (outPath[0] != '/') outPath = std::string(getenv("PWD") ? getenv("PWD") : ".") + "/" + outPath;
        int shots = argc > 4 ? std::atoi(argv[4]) : 0;
        SnoutAndSiegeGame& g = workers[0]->game;
        int idx = lv[0];
        std::vector<Shot> plan;
        if (shots > 0) {
            // Reuse the solver for the shot plan (single worker; slower but fine).
            for (unsigned i = 1; i < nThreads; ++i) {
                workers.push_back(std::make_unique<Worker>());
                workers.back()->game.persistProgress = false;
                workers.back()->game.tutorialHints = false;
                workers.back()->game.levels = g.levels;
            }
            plan = solve(workers, idx, false, true).shots;
            if (static_cast<int>(plan.size()) > shots) plan.resize(shots);
        }
        startLevel(g, idx);
        // Optional 5th argument: freeze the last shot that many seconds after launch.
        float freezeAt = argc > 5 ? static_cast<float>(std::atof(argv[5])) : -1.0f;
        for (size_t i = 0; i < plan.size(); ++i) {
            bool last = (i + 1 == plan.size());
            fire(g, plan[i], (last && freezeAt > 0.0f) ? static_cast<int>(freezeAt * 60.0f) : 60 * 30);
        }
        // Hide the results card so the battlefield stays visible.
        g.levelComplete = false;
        g.levelFailed = false;
        // Show the last shot's path as the ghost trajectory.
        if (!g.currentFlightTrail.empty()) g.ghostTrajectory = g.currentFlightTrail;
        g.floatScores.clear();
        g.particles.clear();
        g.render(ren);
        SDL_RenderPresent(ren);
        SDL_Surface* snap = SDL_CreateRGBSurfaceWithFormat(0, WINDOW_W, WINDOW_H, 32, SDL_PIXELFORMAT_ARGB8888);
        SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_ARGB8888, snap->pixels, snap->pitch);
        SDL_RWops* rw = SDL_RWFromFile(outPath.c_str(), "wb");
        if (rw) SDL_SaveBMP_RW(snap, rw, 1);
        SDL_FreeSurface(snap);
        std::printf("wrote %s\n", outPath.c_str());
    } else {
        std::fprintf(stderr, "unknown command %s\n", cmd.c_str());
        rc = 2;
    }
    std::filesystem::remove_all(tmpDir);
    return rc;
}

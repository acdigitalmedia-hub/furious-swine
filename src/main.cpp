// ============================================================================
// SNOUT & SIEGE: RISE OF THE SWINE
// A C++17 2D Physics Artillery Game for Linux (Zero -lSDL2 Linker Dependency)
//
// Why this compiles out-of-the-box on any Linux distro:
//   - Compiles with standard glibc flags only:
//       g++ -std=c++17 -O2 src/*.cpp -o furious_swine -ldl -lm -lpthread
//   - At runtime, dynamically loads `libSDL2-2.0.so.0` via dlopen() from
//     /usr/lib64, /usr/lib, or /usr/lib/x86_64-linux-gnu (no SDL2-devel needed).
//   - If SDL2 runtime is not installed at all, automatically falls back to
//     native X11 (`libX11.so.6`) with a built-in 32-bit ARGB software renderer!
// ============================================================================

#include "game.h"
#include "self_test.h"

// ============================================================================
// MAIN ENTRY POINT (Interactive Linux Desktop Loop + CLI Flags)
// ============================================================================
int main(int argc, char** argv) {
    bool selfTest = false;
    bool exportShots = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--self-test") selfTest = true;
        if (arg == "--export-screenshots") { selfTest = true; exportShots = true; }
    }
    if (selfTest) {
        return runSelfTestAndExportScreenshots(exportShots);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Failed to initialize video/audio backend: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Snout & Siege: Rise of the Swine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Failed to create window (ensure X11/Wayland display is active): "
                  << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer, WINDOW_W, WINDOW_H);

    SnoutAndSiegeGame game;
    game.appWindow = window;
    game.assets.loadAll(renderer);
    if (!game.initLevels()) {
        std::cerr << "Failed to load level data from assets/levels/. "
                     "Run the game from the project root directory.\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    game.loadProgress();
    game.loadLevel(0);
    game.screen = ScreenState::TITLE;

    bool running = true;
    uint32_t lastTicks = SDL_GetTicks();

    while (running) {
        uint32_t now = SDL_GetTicks();
        float frameTime = (now - lastTicks) / 1000.0f;
        lastTicks = now;
        if (frameTime > 0.10f) frameTime = 0.10f;
        if (frameTime < 0.0f) frameTime = 0.0f;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            bool isPlayingMode = (game.screen == ScreenState::PLAYING || game.screen == ScreenState::ENDLESS ||
                                 (game.screen == ScreenState::EDITOR && game.isTestPlaying));

            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_MOUSEMOTION) {
                game.mouseX = static_cast<float>(ev.motion.x);
                game.mouseY = static_cast<float>(ev.motion.y);
                if (isPlayingMode && game.isManualPanning) {
                    float dx = game.mouseX - game.lastPanMouseX;
                    game.camTarget.x = clampf(game.camTarget.x - dx, 0.0f, 420.0f);
                    game.lastPanMouseX = game.mouseX;
                }
                if (isPlayingMode && game.draggingSling) {
                    Vec2 mPos(game.mouseX + game.camPos.x, game.mouseY + game.camPos.y);
                    Vec2 pull = mPos - Vec2(SLING_X, SLING_Y);
                    if (pull.length() > MAX_PULL) pull = pull.normalized() * MAX_PULL;
                    game.slingPullPos = Vec2(SLING_X, SLING_Y) + pull;
                    if (!game.activePigs.empty() && !game.activePigs[0].launched) {
                        game.activePigs[0].pos = game.slingPullPos;
                        if (pull.lengthSq() > 36.0f) {
                            game.activePigs[0].angle = std::atan2(-pull.y, -pull.x);
                        }
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                float mx = static_cast<float>(ev.button.x);
                float my = static_cast<float>(ev.button.y);
                game.mouseX = mx;
                game.mouseY = my;

                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    if (isPlayingMode) {
                        game.isManualPanning = true;
                        game.manualPanTimer = 0.0f;
                        game.lastPanMouseX = mx;
                    }
                } else if (ev.button.button == SDL_BUTTON_LEFT) {
                    // First check clickable UI buttons on any screen or modal
                    bool clickedUI = false;
                    auto buttonsCopy = game.activeButtons;
                    for (const auto& btn : buttonsCopy) {
                        if (btn.contains(mx, my)) {
                            clickedUI = game.handleButtonClick(btn.id, running);
                            break;
                        }
                    }

                    // If in playing state and not clicking a UI button, handle slingshot or Pig ability
                    if (!clickedUI && isPlayingMode &&
                        !game.levelComplete && !game.levelFailed && !game.endlessGameOver) {
                        Vec2 worldMouse(mx + game.camPos.x, my + game.camPos.y);
                        if (!game.activePigs.empty() && !game.activePigs[0].launched) {
                            if ((worldMouse - game.activePigs[0].pos).length() < 58.0f) {
                                game.draggingSling = true;
                                game.isManualPanning = false;
                                game.manualPanTimer = 0.0f;
                                game.camTarget = {0.0f, 0.0f};
                                game.simSpeed = 1.0f;
                            }
                        } else if (!game.activePigs.empty() && game.activePigs[0].launched) {
                            game.activateMidAirAbility();
                        }
                    } else if (!clickedUI && game.screen == ScreenState::EDITOR && !game.isTestPlaying) {
                        game.handleEditorCanvasClick(mx, my);
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONUP) {
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    if (game.isManualPanning) {
                        game.manualPanTimer = 2.2f;
                    }
                } else if (ev.button.button == SDL_BUTTON_LEFT) {
                    if (isPlayingMode && game.draggingSling) {
                        game.draggingSling = false;
                        game.launchCurrentPig(game.slingPullPos - Vec2(SLING_X, SLING_Y));
                    }
                }
            } else if (ev.type == SDL_KEYDOWN) {
                int sym = ev.key.keysym.sym;

                // Ignore OS key auto-repeat for everything except camera/page
                // panning, so holding a toggle key doesn't flip it every frame.
                const bool isPanKey = (sym == SDLK_LEFT || sym == SDLK_RIGHT || sym == SDLK_a || sym == SDLK_d);
                if (ev.key.repeat && !isPanKey) continue;

                if (sym == SDLK_F11 || sym == SDLK_f) {
                    game.toggleFullscreen();
                    continue;
                }

                if (sym == SDLK_c) {
                    game.enableSloMoCam = !game.enableSloMoCam;
                    game.floatScores.push_back({{WINDOW_W * 0.5f, 110.0f},
                                                game.enableSloMoCam ? "CINEMATIC SLO-MO CAM: ENABLED" : "CINEMATIC SLO-MO CAM: DISABLED",
                                                {255, 220, 100, 255}, 1.5f, 1.4f});
                    continue;
                }

                if (sym == SDLK_TAB && isPlayingMode) {
                    game.simSpeed = (game.simSpeed > 1.0f) ? 1.0f : 2.5f;
                    continue;
                }

                if (sym == SDLK_ESCAPE) {
                    if (game.screen == ScreenState::TITLE) running = false;
                    else if (game.screen == ScreenState::EDITOR && game.isTestPlaying) game.returnToEditor();
                    else game.screen = ScreenState::TITLE;
                } else if (sym == SDLK_m) {
                    game.assets.musicMuted = !game.assets.musicMuted;
                    game.saveProgress();
                } else if (sym == SDLK_s && game.screen != ScreenState::EDITOR) {
                    game.assets.sfxMuted = !game.assets.sfxMuted;
                    game.saveProgress();
                } else if (sym == SDLK_v) {
                    game.assets.useSpriteTextures = !game.assets.useSpriteTextures;
                } else if (sym == SDLK_F5) {
                    game.assets.loadAll(renderer);
                    game.floatScores.push_back({{WINDOW_W * 0.5f, 110.0f},
                                                "ASSETS HOT-RELOADED!", {140, 255, 190, 255}, 1.5f, 1.5f});
                } else if (isPlayingMode) {
                    if (sym == SDLK_r) {
                        if (game.screen == ScreenState::EDITOR) game.startEditorTestPlay();
                        else if (game.screen == ScreenState::ENDLESS) game.startEndlessMode();
                        else game.loadLevel(game.currentLevelIdx);
                    } else if (sym == SDLK_n && game.screen == ScreenState::PLAYING) {
                        int next = game.currentLevelIdx + 1;
                        if (next < NUM_LEVELS && game.isLevelUnlocked(next)) game.loadLevel(next);
                    } else if (sym == SDLK_p && game.screen == ScreenState::PLAYING) {
                        int prev = game.currentLevelIdx - 1;
                        if (prev >= 0 && game.isLevelUnlocked(prev)) game.loadLevel(prev);
                    } else if (sym == SDLK_RIGHT || sym == SDLK_d) {
                        game.isManualPanning = true;
                        game.manualPanTimer = 3.0f;
                        game.camTarget.x = std::min(420.0f, game.camTarget.x + 140.0f);
                    } else if (sym == SDLK_LEFT || sym == SDLK_a) {
                        game.isManualPanning = true;
                        game.manualPanTimer = 3.0f;
                        game.camTarget.x = std::max(0.0f, game.camTarget.x - 140.0f);
                    } else if (sym == SDLK_SPACE && !game.levelComplete && !game.levelFailed && !game.endlessGameOver) {
                        game.activateMidAirAbility();
                    }
                } else if (game.screen == ScreenState::EDITOR && !game.isTestPlaying) {
                    if (sym == SDLK_s) game.saveCustomLevel(game.customSlot);
                    else if (sym == SDLK_l) game.loadCustomLevel(game.customSlot);
                    else if (sym == SDLK_t) game.startEditorTestPlay();
                    else if (sym == SDLK_g) game.editorSnapGrid = !game.editorSnapGrid;
                    else if (sym == SDLK_DELETE || sym == SDLK_BACKSPACE) {
                        game.customLevel.blocks.clear();
                        game.customLevel.birds.clear();
                        game.customLevel.pendulums.clear();
                        game.customLevel.hasTruffle = false;
                        game.floatScores.push_back({{WINDOW_W * 0.5f, 120.0f}, "FORTRESS CLEARED!", {255, 140, 140, 255}, 1.4f, 1.4f});
                    }
                } else if (game.screen == ScreenState::LEVEL_SELECT) {
                    if (sym == SDLK_LEFT || sym == SDLK_a) {
                        if (game.levelSelectPage > 0) game.levelSelectPage--;
                    } else if (sym == SDLK_RIGHT || sym == SDLK_d) {
                        if (game.levelSelectPage < LEVEL_SELECT_VAULT_PAGE) game.levelSelectPage++;
                    }
                }
            }
        }

        game.updateGame(frameTime);
        game.render(renderer);
        SDL_RenderPresent(renderer);
        uint32_t frameElapsed = SDL_GetTicks() - now;
        if (frameElapsed < 15) {
            SDL_Delay(15 - frameElapsed);
        }
    }

    game.assets.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

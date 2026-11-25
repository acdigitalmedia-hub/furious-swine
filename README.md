# Snout & Siege: Rise of the Swine (C++17 / SDL2 for Linux)

A role-reversed 2D physics artillery game for Linux written in C++17 with SDL2. Launch heroic **Origami/Woodblock Pigs** from a timber catapult to demolish multi-tiered fortresses and knock out the aristocratic **Totem Birds** perched inside.

## Key Features
- **7 Heroic Pig Classes with Mid-Air Abilities (`SPACE` or `Left-Click` while airborne)**:
  - **Trotter (Terracotta Vanguard)**: Executes an **Anvil Slam** vertical dive to crush horizontal roof beams.
  - **Bramble (Ochre Turbo Boar)**: Triggers a **Supersonic Charge** (2.45x speed boost) that pierces through Wicker, Timber, and Brittle Ice.
  - **Clover (Jade Tri-Piglet)**: Splits mid-air into a **3-Way Scatter Squadron** for wide structural coverage.
  - **Vulcan (Obsidian Kaboom Hog)**: Detonates a **Seismic Shockwave** that shatters heavy Carved Slate blocks and triggers TNT chain reactions.
  - **Gale (Aeronaut Glider Swine)**: Deploys origami sky-blue glider wings for **Aerodynamic Updraft Lift** and drops **Twin Aerial Iron Bombs** that detonate on impact with radial blast damage.
  - **Atticus (The Graviton Boar)**: Opens a **Cosmic Singularity Vortex** that exerts continuous inward pull on blocks and birds within 210px, collapsing in a violent radial implosion shockwave.
  - **Rusty (The Drill Hog)**: Engages a **Supersonic Rotary Bore** ($\ge 780$ px/s) piercing straight through fortress walls ($0.22\times$ drag, $0.68\times$ damage) and triggering ground subsidence cave-ins.
- **Interactive Fortress Architect & Sandbox Mode**:
  - Full-screen custom fortress level editor accessible directly from the Title Screen.
  - Multi-category tool dock: Blocks (Timber, Stone, Ice, TNT, Trampoline in 5 size presets), Birds (Sentry Toucan, Warden Owl, Archduke Heron, Commander Falcon with Tower Shield, Baron Von Condor with Zeppelin, with optional Iron Helmets), Swinging Wrecking Ball Pendulums, Secret Golden Truffles, and Demolition Eraser.
  - 20px snap-to-grid toggle, Chapter Theme selector (Chapters 1–6), dynamic crosswind control ($\pm 120$ MPH), and cumulonimbus lightning storm toggle.
  - 3 dedicated save/load configuration slots (`custom_siege_1.cfg` – `3`).
  - 1-click **Test Siege** playtesting with full swine arsenal, and instant return snapshot restoration.
- **Endless Siege Gauntlet Mode**:
  - Procedural survival gauntlet with escalating wave difficulty (fortress height, multi-bay width, durable materials, cauldron-helmed sentries, spiked wrecking balls, and crosswinds).
  - Swine reinforcements awarded upon clearing waves.
  - Dynamic combo score multiplier (up to $5.0\times$) and persistent best wave tracking (`endless.best_wave`).
- **Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")**:
  - Automatically triggers dramatic bullet-time slow motion ($0.22\times$ speed) and dynamic widescreen letterbox bars (`** AVIAN RECKONING **`) when the final living bird in a fortress is struck down.
  - Toggle on/off at any time with hotkey **`[C]`**.
- **7 Material Tiers & Defensive Apparatus**:
  - **Tier 1 — Wicker / Reed (`MAT_TWIG`)**: 68 HP, low density, shatters into straw splinters.
  - **Tier 2 — Seasoned Timber (`MAT_TIMBER`)**: 170 HP, medium density, displays progressive fracture cracks at `<66%` and `<33%` HP.
  - **Tier 3 — Carved Slate (`MAT_STONE`)**: 350 HP, high density, resists standard impacts; vulnerable to Vulcan's blast and Trotter's Anvil Slam.
  - **Tier 4 — Brittle Glacial Ice (`MAT_ICE`)**: 38 HP, ultra-low friction (`0.04`), shatters into crystalline diamond shards with acoustic glass shatter sound.
  - **Tier 5 — Volatile TNT Gunpowder Kegs (`MAT_TNT`)**: 35 HP, massive radial blast damage (`240px` radius) with camera trauma, staggered domino chain reactions, and spark fuse countdowns.
  - **Tier 6 — Bouncy Trampoline Deflector (`MAT_TRAMPOLINE`)**: High restitution (`1.65x`), launches pigs and debris skyward over front palisades with acoustic spring twangs (`sound.boing`) and chevron directional guides.
  - **Tier 7 — Volcanic Magma Vents (`MAT_MAGMA` / `MaterialTier::MAGMA_VENT`)**: Heavy indestructible basalt vents (9999 HP) spewing buoyant fiery embers that melt Brittle Ice and ignite TNT gunpowder kegs on contact.
- **Atmospheric Hazards & Dynamic Weather Systems**:
  - **Gusting Crosswinds (`currentWind`)**: Handcrafted directional wind vectors ($W_x \in [-180, +180]$ px/s²) applying continuous lateral acceleration to dynamic airborne pigs, fluttering feathers, smoke, and embers.
  - **Wind-Curved Aiming Trajectory Dots**: Real-time parabolic aiming dots dynamically integrate lateral wind velocity to reveal the true flight path before release.
  - **Animated Weather Card & Wind Gauge HUD**: Translucent header widget displaying atmospheric condition (`CALM`, `BREEZE`, `GALE`, `STORM`) and animated directional wind arrows with real-time airspeed readouts (e.g., `<< -65 MPH`, `+110 MPH >>`).
  - **Cumulonimbus Storm Clouds & Lightning Strikes**: Drifting dark storm clouds discharge electrostatic lightning into the highest stone pillars and iron cauldron helmets, delivering 160 blast damage with full-screen whiteout flashes, screen trauma, and rolling acoustic thunder (`sound.thunder`).
  - **Volcanic Magma Vents & Buoyant Embers**: Chapter 5 geothermal vents erupt with bubbling molten audio (`sound.magma`), launching glowing buoyant embers that dissolve ice barriers and detonate TNT caches.
- **Swinging Wrecking Ball Pendulums (`PendulumBody`)**:
  - Ceiling-anchored iron chains and massive spherical wrecking balls with real angular harmonic physics ($\alpha = -\frac{g}{L}\sin\theta - c\omega$).
  - Transfer collision impulses and momentum to smash through block walls, with spiked wrecking ball variants dealing devastating demolition damage.
- **Cauldron-Helmed Birds (`hasHelmet`)**:
  - Sentries equipped with iron kettle cauldron helmets absorbing +70 to +140 damage with metallic clangs (`sound.hit_metal`) and sparks.
  - Shattering helmet pop off into spinning debris (`HELMET_DEBRIS`) with `+1000 HELMET SHATTERED!` bonus score.
- **Simulation Fast-Forward Toggle (`2.5x Speed`)**:
  - Press **`[TAB]`** or click **`[SPEED: 1.0X]` / `[SPEED: 2.5X]`** on the top HUD to quickly resolve settling debris and rolling blocks.
  - Automatically resets to 1.0x when aiming or when a new pig is loaded onto the slingshot.
- **5 Perched Totem Bird Classes**:
  - **Sentry Toucan** (78 HP, 5,000 pts)
  - **Warden Owl** (140 HP, 7,500 pts)
  - **Archduke Heron** (210 HP, 10,000 pts)
  - **Commander Falcon** (150 HP, 12,500 pts): Equipped with a 120 HP **Directional Tower Shield** deflecting frontal swine hits with high restitution ($e=0.75$) until shattered (+1,200 pts).
  - **Baron Von Condor** (240 HP, 15,000 pts): Hovering in a 70 HP **Suspended Steampunk Zeppelin** with buoyant lift ($0.15\times$ gravity); popping the envelope (+1,500 pts) sends the 4.8kg iron gondola plummeting downward at 320 px/s to crush fortress blocks beneath.
- **55-to-150-Level Campaign Expansion Across 14 Thematic Chapters & 10 Secret Truffle Vaults**:
  - **Chapter I (Levels 1–10)**: Dawn Meadow & The Wicker Outposts
  - **Chapter II (Levels 11–20)**: Autumn Woodlands & Timber Palisades (Level 12 introduces Helmed Birds, Level 14 introduces Headwinds, Level 18 introduces Pendulum Wrecking Ball)
  - **Chapter III (Levels 21–30)**: Basalt Canyon & Red Mesa Viaducts (Level 22 introduces Canyon Tailwinds, Level 26 introduces Dual Stepping Trampolines)
  - **Chapter IV (Levels 31–40)**: Frostpeak Aurora Highlands & Glacial Pagodas (Level 35 features Blizzard Headwind, Level 39 features Thunder Peak Sanctuary with storm clouds and lightning)
  - **Chapter V (Levels 41–50)**: Obsidian Imperial Citadel & Emperor's High Eyrie (Level 42 features Spiked Emperor Pendulum, Level 44 features Volcanic Casemate with Magma Vents, Level 50 combines storm clouds, lightning strikes, crosswinds, and magma vents)
  - **Chapter VI (Levels 51–60)**: The Bamboo Steppes & Gilded Pagodas (Flared tiered eaves, bamboo shrines, and elevated gatehouses)
  - **Chapter VII (Levels 61–70)**: Sunken Megalithic Ruins (Heavy foundation plinths, massive stone steles, and wide lintel bridges)
  - **Chapter VIII (Levels 71–80)**: Zephyr Highlands & Cloud Viaducts (High-altitude gale crosswinds, suspension bridges, and zeppelin roosts)
  - **Chapter IX (Levels 81–90)**: Glacial Bastion & Frost Spire Citadel (Brittle ice casemates, frozen ziggurats, and slippery bridge spans)
  - **Chapter X (Levels 91–100)**: Magma Foundry & Obsidian Gantries (Industrial crane girders, magma vents, and molten iron cauldrons)
  - **Chapter XI (Levels 101–110)**: Thunderhead Peaks & Tempest Eyrie (Electrified storm clouds, lightning-attracting stanchions, and gale tailwinds)
  - **Chapter XII (Levels 111–120)**: Solstice Sanctuary & Sunken Pagodas (Stepped ziggurat platforms, wide megalithic porticos, and dual-layer roofs)
  - **Chapter XIII (Levels 121–130)**: The Emperor's Colosseum & Grand Championship (Gladiatorial multi-bay arenas defended by High Marshalls and Royal Guards)
  - **Chapter XIV (Levels 131–140)**: Sovereign High Sanctum & Emperor's Zenith (Definitive campaign finale combining all 7 swine, dynamic weather, and multi-tier imperial citadels)
  - **Chapter XV (Levels 141–150)**: The 10 Secret Truffle Vaults (Powder Crypt, Trampoline Gauntlet, Pendulum Cathedral, Thunder Ridge, Obsidian Treasury, Gilded Sanctum, etc.)
  - **450 Total Stars**, 7-page Level Select menu with direct tab navigation, Arrow/A/D keyboard page cycling, and persistent save progress in `save_progress.cfg`.
- **Architectural Variety & Differently Sized/Shaped Blocks**:
  - Dynamic procedural architectural builders: Megalithic Bays (`addMonolithBay`), Tiered Pagodas (`addPagodaBay`), Stepped Ziggurats (`addPyramidBay`), Fortified Bomb-Proof Casemates (`addFortifiedCasemate`), and Industrial Crane Gantries (`addClockworkGantry`).
  - Architectural rendering embellishments:
    - Stone Wide Lintels: Horizontal masonry relief grooves, vertical mortar seams, under-arch bevel highlights.
    - Stone Columns: Capital/base rim collars, dual vertical fluting grooves.
    - Stone Megaliths: Dressed ashlar inset panel relief borders.
    - Timber Heavy Crates: Reinforced diagonal cross-brace "X" with 4 iron corner rivets.
    - Timber Girders & Posts: Longitudinal wood grain lines, iron bolt pegs, and iron collar bands.
    - Twig / Bamboo Slabs: Segment nodes and rattan wrap bindings.
- **35 Hidden Golden Origami Truffles**:
  - Collectible golden origami truffles hidden in tricky fortress niches across the campaign (2–3 per chapter).
  - Collected by pig proximity, moving block impact, or blast/shockwave radius (+2,500 bonus points, sparkling particles, and celestial chime).
  - Collecting truffles unlocks the 10 Secret Truffle Vault challenge levels!
- **Royal Swine Trophy Hall & Achievement System**:
  - Dedicated full-screen Trophy Hall accessible from the Title Screen tracking 12 distinct achievements (including `VORTEX_MAESTRO`, `DEEP_BORE`, `TRUFFLE_CONNOISSEUR`, and `VAULT_CONQUEROR`).
  - Live animated sliding toast banners and fanfare sound when unlocking achievements in-game.
- **Visceral Camera & Visual Juice**:
  - Dynamic camera framing with projectile tracking and rubble centroid tracking.
  - Manual camera panning via **Right-Click Drag** or **`[A]` / `[D]`** keys.
  - Trauma-based non-linear screen shake, feather bursts with distress expressions, and trajectory ghost dots.
- **Modern Physical Acoustic Audio Engine & Asset Pipeline (`assets/manifest.cfg`)**:
  - 16-channel polyphonic mixer with continuous exponential BGM ducking, dry sub-bass routing, and master `tanh` saturation limiter.
  - **Modern In-Game Acoustic Groove (`bgm_battle.wav`, 32 Bars, ~70s Loop @ 110 BPM)**: Warm acoustic upright walking bass with 2-pole Butterworth low-pass filtering (<240 Hz), warm physical acoustic guitar strums with soundboard resonance (<2.6 kHz), rosewood marimba modal synthesis, wooden cajón kick, and swinging studio shaker. Dynamic arrangement with wide groove breaks that breathe and eliminate ear fatigue.
  - **Contemporary Acoustic Indie Title Theme (`bgm_title.wav`, 16 Bars, ~46s Loop @ 84 BPM)**: Intimate fingerpicked acoustic guitar arpeggios, warm upright bass foundation, glockenspiel bells, warm electric piano chords, and separated Schroeder room reverb.
  - **Refined Organic Sound Effects (25 Sounds, `v16` Pipeline)**:
    - **Drill Hog Audio Overhaul**: Replaced harsh high-pitched screeching with a warm, low-passed mechanical rotary engine hum (`tanh(1.8f * sin(p1))`), deep sub-bass body (`sin(p1 * 0.5f)`), and gentle mechanical gear churning at calibrated `0.70f` gain, with pierce gain softened to `0.55f`.
    - Layered physical transients, authentic material resonances (oak, stone, ice, iron, rubber), comical cartoon pops, and cinematic sub-bass impact sweeps.
  - Auto-bootstraps 23 custom BMP sprites and 25 synthesized WAV sound effects (`v16` pipeline).
  - Press **`F5`** at any time in-game to hot-reload assets.

## Building & Running on Linux

Requires a C++17 compiler and Make (or CMake 3.12+). Install the SDL2 runtime
for graphics and audio; SDL2 development headers are not required. If SDL2 is
unavailable, the game falls back to X11 with software rendering.

Run commands from the project root so the game can find `assets/`. Saves,
custom levels, and exported screenshots are written to the current directory.

### Using Make
```bash
make -j"$(nproc)"
./furious_swine
```

Object files go to `build/obj/`; only the files you change (and files
that include a header you change) are rebuilt.

### Using CMake
```bash
cmake -S . -B build
cmake --build build
./build/furious_swine
```

### Running Self-Test Suite
```bash
./furious_swine --self-test
# Or export fresh verification screenshots:
./furious_swine --export-screenshots
```

The self-test runs headlessly and needs no desktop session. It temporarily
overwrites `save_progress.cfg` and custom level slot 2, and restores both when
it exits, including when a check fails.

### Level design tool (`level_lab`)

`make tools` builds `level_lab`, a developer tool for checking campaign levels
(run it from the project root; it never touches your save file):

```bash
./level_lab stable 1-150          # does each level stand still when untouched?
./level_lab solve 26              # search shots: pigs needed with perfect aim and
                                  #   with "skilled" (+-2.5 deg, +-10% power) aim
./level_lab solve 26 notramp      # same, with trampolines swapped for stone
./level_lab render 26 out.bmp 1   # picture of level 26 after the solver's first shot
```

Set `LEVELS_DIR=/some/dir` to test edited copies of the level files.

## Project Layout

- `src/main.cpp`: entry point, window setup, and the input/event loop.
- `src/sdl_runtime.h/.cpp`: SDL2 ABI declarations, `dlopen()` loader, and
  the X11 + software-renderer fallback (no SDL2 headers needed at build time).
- `src/common.h`: shared constants, `Vec2` math, enums, and entity structs.
- `src/draw.h/.cpp`: bitmap font, drawing primitives, origami pig/bird models.
- `src/assets.h/.cpp`: asset manifest, sprite/WAV loading, 16-voice audio mixer.
- `src/asset_bootstrap.cpp`: procedural generation of default sprites, sounds, and music.
- `src/game.h`: the `SnoutAndSiegeGame` class, implemented across:
  - `game.cpp`: core state, achievements, level flow, saves, camera, UI clicks
  - `levels.cpp`: loads the campaign levels and computes star thresholds
  - `editor.cpp`: Fortress Architect editor and custom level files
  - `endless.cpp`: Endless Siege Gauntlet wave generation
  - `combat.cpp`: pig abilities, explosions, damage, particle effects
  - `physics.cpp`: rigid-body simulation and collision resolution
  - `render_world.cpp` / `render_ui.cpp`: world rendering / menus, editor UI, HUD
- `src/level_format.h/.cpp`: the level text format shared by campaign levels
  and editor save slots (documented at the top of `level_format.h`).
- `src/self_test.h/.cpp`: headless self-test suite (`--self-test`).
- `tools/level_lab.cpp`: level design/analysis tool (`make tools`).
- `assets/levels/level_001.cfg` … `level_150.cfg`: the campaign (1–140) and
  Secret Vault (141–150) levels as editable text. Restart the game after
  editing; `--self-test` checks every level loads and settles stably.
- `assets/`: sprite, audio, and asset configuration files.
- `Makefile` / `CMakeLists.txt`: Linux build definitions.
- `CHANGELOG.md`: development history.
- `local/`: ignored archive of the original executable, player save, and screenshots.

Build outputs, player saves, custom levels, and generated screenshots are ignored
by Git. To restore your archived progress, copy `local/save_progress.cfg` to the
project root before launching the game.

## Controls
- **Mouse Left-Click + Drag on Slingshot**: Aim and adjust launch tension
- **Release Mouse Left-Click**: Launch Pig
- **`SPACE` or `Left-Click` (while Pig is airborne)**: Activate Pig's mid-air ability (Anvil Slam, Supersonic Dash, 3-Way Split, Seismic Blast, Aero Glide & Bombs, Graviton Vortex, or Supersonic Drill)
- **`[C]`**: Toggle Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")
- **`[TAB]`**: Toggle simulation speed between 1.0x and 2.5x fast-forward
- **Mouse Right-Click + Drag** or **`[A]` / `[D]` / Arrow Keys**: Pan camera to scout the battlefield
- **`R`**: Restart current level / test play / endless gauntlet
- **`N` / `P`**: Next / Previous Level
- **`M`**: Mute / Unmute music
- **`S`**: Mute / Unmute sound effects (when not in editor)
- **`F11`**: Toggle fullscreen
- **`V`**: Toggle custom BMP sprite textures vs. pure Origami vector geometry
- **`F5`**: Hot-reload `assets/manifest.cfg`, sprites, and sounds
- **`ESC`**: Return to Title / Quit
- **Fortress Architect Controls**:
  - **`[LMB]` (Canvas)**: Place selected block / bird / pendulum / truffle, or demolish with Eraser
  - **`[G]`**: Toggle 20px Snap-to-Grid
  - **`[T]`**: Immediate Test Siege playtest
  - **`[S]` / `[L]`**: Save / Load custom fortress slot
  - **`[DEL]` / `[BACKSPACE]`**: Clear custom fortress canvas

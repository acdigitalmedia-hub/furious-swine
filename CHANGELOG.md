# Snout & Siege: Rise of the Swine — Changelog (v42)

## 14. 150-Level Grand Expansion, Block Variety & Drill Audio Overhaul (v42)
- **Drill Hog Sound Effect Overhaul (`sound.drill`)**:
  - Replaced the harsh, jarring high-frequency square wave and aggressive FM screech with a warm, analog-saturated rotary combustion engine hum.
  - Layered a saturated sinusoidal motor whir (`tanh(1.8f * sin(p1))`), deep sub-bass body (`sin(p1 * 0.5f)`), and low-passed mechanical gear churning noise with a softer volume envelope (`0.70f` peak gain).
  - Reduced supersonic bore launch gain to `0.85f` and block pierce gain in `resolveCircleOBB` from `1.0f` to `0.55f`, eliminating abrasive audio clipping during rapid multi-block penetrations.
  - Asset version marker bumped to `v16` across engine and `assets/version.txt`.
- **150 Handcrafted Siege Levels Across 14 Chapters + 10 Secret Vaults**:
  - Tripled the campaign from 55 levels to **150 structurally unique, verified levels**:
    - **140 Campaign Levels** across 14 thematic chapters (10 levels each) cycling through 5 atmospheric sky palettes (Dawn Meadow, Autumn Woodlands, Basalt Canyon, Frostpeak Highlands, and Obsidian Citadel).
    - **10 Secret Truffle Vaults** (Levels 141–150) featuring high-concept puzzle mechanics (powder crypts, trampoline gauntlets, pendulum cathedrals, thunder ridge bastions, and gilded sovereign sanctums).
  - **450 Total Stars** with calibrated 2-star and 3-star thresholds based on individual level composition.
- **Architectural Variety & Differently Sized and Shaped Blocks**:
  - Built 5 specialized architectural builders to ensure deep structural variety across every chapter:
    - **Megalithic Bays (`addMonolithBay`)**: Heavy stone foundation plinths (`44x40`), dual monolithic pillars (`32x90`), and wide monolithic lintel beams (`160x28`).
    - **Eastern Tiered Pagodas (`addPagodaBay`)**: 2- and 3-tier pagodas with flared timber/bamboo eaves (`144x18`, `120x16`), staggered bamboo posts, and roof caps (`100x16`).
    - **Stepped Ziggurats & Pyramids (`addPyramidBay`)**: Multi-tiered stepped masonry platforms (`140x28`, `100x24`, `60x22`).
    - **Fortified Bomb-Proof Casemates (`addFortifiedCasemate`)**: Thick bunker walls (`36x100`), interior ammo crates (`42x32`), heavy roof slabs (`170x26`), and flanking parapets (`20x28`).
    - **Industrial Crane Gantries (`addClockworkGantry`)**: Structural crane stanchions (`28x110`, `22x80`) and girder decks (`170x24`, `130x20`).
  - Added architectural rendering embellishments in `renderBlock`:
    - Stone wide lintels: Horizontal relief grooves, vertical mortar seams, under-arch bevel highlights.
    - Stone columns: Capital/base rim collars, dual vertical fluting grooves.
    - Stone megaliths: Dressed ashlar inset panel relief borders.
    - Timber crates: Heavy diagonal cross-brace "X" with 4 iron corner rivets.
    - Timber girders & posts: Longitudinal wood grain lines, iron bolt pegs, and iron collar bands.
    - Twig / bamboo blocks: Segment nodes and rattan wrap bindings.
- **35 Hidden Golden Origami Truffles**:
  - Expanded from 15 to 35 collectible golden origami truffles hidden across all 14 campaign chapters (2–3 per chapter).
  - Calibrated progressive vault unlock requirements: `{2, 5, 8, 11, 14, 18, 22, 26, 30, 35}` truffles.
  - Scaled `TRUFFLE_CONNOISSEUR` achievement to 20 truffles and `VAULT_CONQUEROR` to all 10 vaults.
- **7-Tab Level Select Screen & UI Navigation**:
  - Expanded Level Select navigation: `< PREV`, `P1` (1–25), `P2` (26–50), `P3` (51–75), `P4` (76–100), `P5` (101–125), `P6` (126–140), `NEXT >`, and `VAULTS (141–150)`.
  - Vault page formatted as a 2-column × 5-row card grid displaying all 10 Secret Truffle Vaults with unlock requirements, star ratings, and personal best scores.
  - Added keyboard page switching: `[Left Arrow]` / `[A]` and `[Right Arrow]` / `[D]` cycle pages directly in Level Select.
  - HUD updated with `LVL X/140` and `CHAPTER XV: THE SECRET VAULTS`.
- **100% Rock-Solid Physics Stability**:
  - All 150 levels pass the 300-frame (5-second) awake physics equilibrium test with `maxTilt = 0.000 rad`, `maxBirdRot = 0.000 rad`, and zero settling drift.
- **16-Channel Polyphonic Voice Mixer**: Upgraded the sound effect architecture from a single choked channel to a 16-voice polyphonic mixer with intelligent priority voice stealing. Multiple timber cracks, stone impacts, bird squawks, and ability detonations now mix cleanly simultaneously without cutting each other off.
- **Continuous Exponential BGM Ducking**: Replaced instant single-sample ducking steps with a smooth exponential low-pass gain ramp (`0.003f` per sample), eliminating speaker pops/clicks when impacts occur.
- **Master Soft-Saturation Limiter**: Added `tanh` soft clipping to prevent harsh digital clipping during high-intensity multi-block chain collapses.
- **Thread Safety & Dynamic Audio Device Locking**: Dynamically bound `SDL_LockAudioDevice` / `SDL_UnlockAudioDevice` and wrapped mixer buffer updates and asset hot-reloading (`[F5]`) with mutex synchronization, eliminating audio thread data races and use-after-free vulnerabilities.
- **Acoustic Procedural BGM Synthesis Overhaul**:
  - `bgm_title.wav` (22.9s loop): Renaissance Lute & Flute Pavane in D Dorian (84 BPM) synthesized with multi-harmonic fingerpicked lute strings, resonant acoustic body filtering, expressive wooden flute with delayed natural vibrato (5.2 Hz), warm viola da gamba bass, room ambience, and a seamless 1024-sample clickless loop crossfade.
  - `bgm_battle.wav` (29.5s loop): Swine Siege Folk March in G Major (130 BPM) synthesized with acoustic Bodhran hand drums (pitch-dropped thump), resonant tuned woodblocks, high-passed rhythmic tambourine shaker, bouncing walking acoustic bass, lyrical folk whistle lead, and seamless 1024-sample loop crossfade.
- **Save File Protection in Self-Test**: Fixed `runSelfTestAndExportScreenshots` so it backs up and restores the player's `save_progress.cfg` rather than deleting it.

## 2. Level 3 Rebalance ("Twin Slate Monoliths")
- **Expanded Pig Queue**: Added a second Vulcan obsidian bomb hog (`{BOMB, BOMB, TURBO, TROTTER}`), giving the player two seismic charges to demolish both stone towers without needing pixel-perfect placement.
- **Structural Responsiveness**: Reduced lower monolith column widths from 30w to 24w and aligned bridge resting planes (`y=487`), allowing direct bomb impacts to cleanly trigger dramatic chain-reaction collapses.
- **Enemy Layout**: Replaced the left monolith's 140 HP Owl with a 78 HP Toucan while maintaining the right Owl and bridge scouts, creating a satisfying, accessible progression curve.
- **Calibrated Star Thresholds**: Adjusted 2-star (`31,000`) and 3-star (`37,000`) requirements to reward strategic demolition and pig conservation.

## 3. 50-Level Campaign Across 5 Chapters
- **Doubled Campaign Scope**: Expanded from 25 to 50 handcrafted, structurally verified siege levels across 5 thematic chapters (10 levels each):
  - **Chapter I (Levels 1–10)**: Dawn Meadow & The Wicker Outposts
  - **Chapter II (Levels 11–20)**: Autumn Woodlands & Timber Palisades
  - **Chapter III (Levels 21–30)**: Basalt Canyon & Red Mesa Viaducts
  - **Chapter IV (Levels 31–40)**: Frostpeak Aurora Highlands & Alpine Pagodas
  - **Chapter V (Levels 41–50)**: The Obsidian Imperial Citadel, culminating in Level 50: The Emperor's Final High Eyrie.
- **150 Total Stars**: Scaled max campaign stars from 75 to 150.
- **Two-Page Level Select**: Level Select screen upgraded with `PAGE 1 (1–25)` and `PAGE 2 (26–50)` navigation tabs.
- **Chapter-Based Leaderboard**: High score screen features 5 Chapter tabs and 10 Level pill buttons for rapid inspection of Top-5 commanders on any level.
- **Automated Verification**: All 50 levels verified rock-solid at idle with `0.000 rad` tilt and `0.000 rad` bird rotation across a 300-frame (5-second) awake stability test.

## 4. Physics & Timestep Engine Overhaul (v30)
- **Fixed-Timestep Accumulator Engine (`1/60s`)**: Decoupled rendering refresh rate from physics simulation via a canonical accumulator in `updateGame()`. Physics steps strictly in constant `FIXED_TIMESTEP = 1.0f / 60.0f` slices, ensuring 100% deterministic and reproducible launch trajectories across 60Hz, 144Hz, and 240Hz monitors.
- **Spiral of Death & Hitch Protection**: Clamped maximum frame time to 100ms with an 8-step ceiling per render frame. Physics accumulator automatically resets on level load, retry, and screen transitions.
- **10-Substep Numerical Integration**: Upgraded `stepPhysics` substeps from 8 to 10 (`subDt = 1/600s` = 1.667ms per substep), improving collision stability and high-speed projectile tunneling resistance.
- **Multi-Point Contact Manifolds**: Overhauled `resolveBlockBlock` to compute and solve 2-point contact manifolds for face-to-face block collisions. Impulses and friction are distributed across the extreme contact points along the tangent axis, naturally neutralizing unbalanced torque and eliminating rotational see-saw jitter in multi-tier towers.
- **Eliminated Horizontal Normal Flipping Bug**: Removed the ad-hoc heuristic (`dy * n.y < 0`) that erroneously forced vertical stacking collisions into horizontal pushes (`±1, 0`) and flung stacked blocks sideways. Stably stacked flat blocks now lock to true vertical normals `(0, ±1)` while toppling blocks follow exact SAT separating axes.
- **Inelastic Resting Restitution Threshold**: Contact relative normal speeds below `30.0f` clamp restitution to `0.0f` to prevent micro-bounce settling jitter, while high-velocity projectile impacts retain full material elasticity.
- **Rock-Solid Self-Test Verification**: All 50 levels pass the 300-frame (5.0s) awake physics test with `maxTilt = 0.000 rad` and `maxBirdRot = 0.000 rad`.

## 5. Dynamic Camera Tracking & Screen Shake Engine (v31)
- **Intelligent Dynamic Camera Framing**: Added critically damped camera tracking in `updateCamera(dt)`:
  - **In-Flight Projectile Tracking**: Follows launched pigs smoothly, framing them at 38% screen width (`WINDOW_W * 0.38f`) with vertical arc tracking (clamped to `[-240, 0]`), ensuring the enemy fortress remains prominently visible ahead of the projectile.
  - **Action Centroid Tracking**: During structural collapses, automatically calculates the mass-weighted velocity centroid of moving debris (`vel.lengthSq() > 1800.0f`) to keep catastrophic chain reactions centered on screen.
  - **Cinematic Return-to-Origin**: As soon as rubble settles or when the player grabs the slingshot to aim, the camera gracefully returns to the catapult origin (`camTarget = (0, 0)`).
- **Manual Camera Panning & Slingshot Coordinate Alignment**:
  - **Right Mouse Button (RMB) Drag**: Allows players to freely pan across the entire 1700px battlefield to scout defenses.
  - **Keyboard Hotkeys**: `[A]` / `[D]` and Arrow keys nudge the camera left and right in 140px increments with responsive critically damped interpolation (`lerpSpeed = 12.0f`).
  - **Scout Hold Timer**: Holds manually panned positions for 2.2–3.0 seconds before slowly easing home, or snaps back instantly the moment slingshot dragging commences.
  - **World-Space Mouse Translation**: Slingshot grabbing and trajectory projection translate screen mouse coordinates by `camPos`, guaranteeing 100% accurate aiming regardless of camera position or screen shake.
- **Visceral Trauma-Based Screen Shake**:
  - Implemented an industry-standard trauma system (`shakeTrauma` ∈ `[0.0, 1.0]`) with non-linear quadratic falloff (`intensity = trauma^2`).
  - **Multi-Frequency Lissajous Harmonic Displacement**: Blends non-repeating sinusoidal harmonics (`1.55x`, `3.45x`, `2.15x`, `4.65x` at 42 Hz) across X (up to 16px) and Y (up to 12px), eliminating mechanical buzzing in favor of organic, shuddering screen rumble.
  - **Action-Calibrated Trauma Triggers**:
    - Catapult launch snap: `+0.12`
    - Trotter Anvil Slam: `+0.65`
    - Bramble Supersonic Dash: `+0.45`
    - Clover Tri-Split: `+0.28`
    - Vulcan Seismic Detonation: `+0.85`
    - Material Destruction: Stone `+0.32`, Timber `+0.20`, Twig `+0.12`
    - Totem Bird Defeat: `+0.42`
    - High-velocity block-to-block & ground collisions: dynamic impulse-scaled trauma (`j * 0.0006f`).
- **Multi-Layer Background Parallax**:
  - Celestial layer (sun, moon, origami clouds, twinkling stars) tracks at `0.08x` distant parallax.
  - Horizon silhouette layer (mesas, mountains, viaducts, pine trees) tracks at `0.32x` mid-ground parallax.
  - Play area objects (fortresses, birds, pigs, particles, catapault) render at `1.0x` foreground space.
  - Padded sky and ground geometry with `120.0f` safety margins to guarantee zero border clipping or color bleeding during maximum-amplitude screen shakes.
- **HUD Screen-Space Separation & Hotkey Prompts**:
  - Top and bottom HUD bars render in pure screen space with a translucent dark backing for high contrast. Added `[RMB/A,D] PAN CAM` hotkey display.
- **Zero Physics Regression**: All 50 levels verified rock-solid at idle with `maxTilt = 0.000 rad` and `maxBirdRot = 0.000 rad` in `./furious_swine --self-test`.

## 6. Visual Polish & Juice Engine (v32)
- **Material-Specific Debris & Splinter Physics**:
  - **Twig / Wicker**: Emits tumbling elongated dry reed splinters and straw needles with high angular tumbling velocity.
  - **Timber / Wood**: Emits a shower of oak slivers, chunky square wood chips, and soft expanding sawdust clouds.
  - **Stone / Granite**: Emits heavy tumbling slate shards, dark basalt pebbles with realistic ground bounce elasticity and friction, and expanding masonry dust puffs.
  - **Mid-Damage Chipping**: Blocks emit minor splinters and dust poofs on impacts (`dmg > 12.0f`) before total destruction.
- **Feather Bursts & Avian Distress Expressions**:
  - **Species-Specific Fluttering Feathers**: Emits fluttering, oscillating feathers with central quills and two-tone faceted vanes:
    - **Toucan**: Cyan turquoise, azure blue, golden beak flakes, and tropical crimson crests.
    - **Owl**: Twilight purple, deep navy, gold eye flecks, and lavender plumes.
    - **Heron**: Royal crimson, snow-white crest plumes, and imperial gold crown flecks.
  - **Aerodynamic Air Resistance & Sinusoidal Flutter**: Feathers feature terminal velocity limits (`vy <= 68.0f`), horizontal air drag, and periodic sinusoidal sway (`sin(wobbleFreq * life + phase)`).
  - **Dynamic Facial Reactions & Temple Bandages**: When birds take heavy damage (`hp < 0.38`), their fierce V-shaped brows shift into panicked, raised brows, pupils dilate with distress, and a crossed adhesive bandage appears on their temple.
- **Previous Shot Trajectory Ghost Dots (Trajectory Memory)**:
  - Captures the exact flight path of launched pigs into `ghostTrajectory`.
  - Renders the previous arc as translucent parchment ghost dots across the sky while aiming subsequent shots, allowing players to visually calibrate their elevation and pull power.
- **Catapult Elasticity & Vibrational Recoil**:
  - Replaced static slingshot lines with tensioned elastic bands that stretch to the leather pouch.
  - Added decaying harmonic vibration (`slingRecoilTimer = 0.35f`, 55Hz damped sine wave) upon release, causing the empty bands and cup to snap forward and oscillate realistically.
  - Pig turns its head to face directly toward the target along the launch vector while pulling the catapult.
- **Floating Score Bounce Pop & Drop Shadows**:
  - Floating scores feature smooth sinusoidal scale bounce (`popScale` peaks at `2.95x` before settling to `2.0f`), high-contrast dark drop shadows, and sparkling star glints for maximum arcade punch.
- **All 50 Levels Verified**: Passes `./furious_swine --self-test` with `0.000 rad` tilt across all levels.

## 7. Feature Pack A — TNT Gunpowder Kegs, Brittle Glacial Ice, and Simulation Fast-Forward (v33)
- **Volatile TNT Gunpowder Kegs (`MAT_TNT`)**:
  - **Explosive Destruction & Radial Impulse**: Delivers massive radial blast damage (`blastRadius = 240.0f`) to surrounding blocks and birds, launching structural debris outward with high angular velocity.
  - **Screen Shake Trauma & Audio**: Detonates with a heavy acoustic shockwave sound (`sound.explosion`), camera trauma (`addTrauma(0.92f)`), and expanding fiery shockwave rings with charred oak fragments and amber sparks.
  - **Cascading Domino Chain Reactions**: When blasted or ignited, neighboring TNT kegs receive a randomized fuse countdown (`0.08f – 0.12f`), allowing domino chain reactions to sweep through multi-keg caches with rhythmic, staggered booms.
  - **Direct Ignition Mechanics**: TNT barrels ignite upon high-velocity pig impact (`dmg > 2.5f`) or vulcan seismic shockwaves, displaying a glowing fuse spark and countdown before detonating.
  - **Dedicated Keg Visuals**: Rendered with iron reinforcement hoops, charred rivet heads, and bold yellow-gold stenciled "TNT" branding.
- **Brittle Glacial Ice Blocks (`MAT_ICE`)**:
  - **Ultra-Low Surface Friction (`0.04f`)**: Provides frictionless sliding surfaces that allow fortresses to slide, sheer, and topple under asymmetric weight.
  - **Brittle Low Hit Points (`38.0 HP`)**: Easily shattered by direct pig collisions, falling debris, or supersonic penetrations.
  - **Crystalline Diamond Splinters (`ICE_CRYSTAL`)**: Spawns multifaceted diamond shards in cyan, crystal blue, and sparkling frost white that tumble and reflect ambient light upon fracture.
  - **Acoustic Glass Shatter Sound (`sound.hit_ice`)**: Synthesized procedural audio (11.0 kHz FM carrier with micro-snap frequency sweeps and harmonic crystalline overtones) dispatches on ice shatter.
  - **Faceted Vector Geometry & Cleavage Veins**: Features internal crystal cleavage fracture lines, ice refraction gradients, and a frosted white top specular rim.
- **Simulation Fast-Forward Toggle (`2.5x Speed`)**:
  - **Instant Debris Resolution**: Press **`[TAB]`** or click the header button **`[SPEED: 1.0X]` / `[SPEED: 2.5X]`** at any time during gameplay to run physics at $2.5\times$ speed, quickly settling tumbling rubble and rolling projectiles.
  - **Fluid Accumulator Scaling**: Seamlessly scales physics accumulator accumulation while retaining exact 1/60s fixed-timestep determinism, with dynamic step ceiling clamps (`20 steps`) to avoid accumulator death spirals.
  - **Auto-Reset on Aim & Reload**: Automatically drops back to standard 1.0x simulation speed as soon as a new pig is loaded onto the slingshot or when the player grabs the catapult band to aim.
  - **Active Explosive Hold**: Simulation automatically marks the world as active while any TNT keg fuse is burning, preventing premature turn endings.
- **Campaign Level Upgrades**:
  - **Level 7 ("The Powder Viaduct")**: Introduced central ground TNT keg beneath viaduct span.
  - **Level 15 ("Double Gable Powder Keep")**: Added central TNT cache between twin keeps for dual collapse.
  - **Level 24 ("Basalt Powder Bunkers")**: Base TNT depot embedded beneath basalt pillars.
  - **Level 31 ("Glacial Ziggurat")**: Crystalline ice pillar foundation with extreme low friction.
  - **Level 34 ("Crystal Conservatory")**: Multi-story brittle ice greenhouse supporting heavy timber decks.
  - **Level 38 ("Frostfire Spire")**: Mixed ice architecture flanked by an volatile ground TNT keg.
  - **Level 48 ("Obsidian Bastion Prime")**: Twin TNT kegs flanking central imperial obsidian gateway.
- **100% Stability Verification**: All 50 levels verified rock-solid at idle with `maxTilt = 0.000 rad` in headless `./furious_swine --self-test`.

## 8. Feature Pack B — Dynamic Defenses & Avian Armor (v34)
- **Cauldron-Helmed Birds (`BirdArmor` / `hasHelmet`)**:
  - **Heavy Armor Absorption**: Aristocratic birds (Toucans, Owls, Herons) equipped with heavy cast-iron/bronze cauldron helmets absorbing +70 HP (Toucan), +100 HP (Owl), or +140 HP (Heron) of incoming damage before their health bar is affected.
  - **Metallic Acoustic Feedback**: Emits crisp metallic kettle clangs (`sound.hit_metal`) and radiating copper/gold spark glints on impact.
  - **Armor Shatter Pop & Debris**: When broken (`helmetHp <= 0`), the helmet pops violently off the bird's head into a tumbling debris piece (`HELMET_DEBRIS`) spinning at high angular velocity with a golden `"+1000 HELMET SHATTERED!"` floating text popup and +1,000 bonus score.
  - **Visual Helmet Geometry**: Rendered with flared rim, domed iron kettle top, brass bail handle, brass rivet studs, and dynamic damage fracture lines that deepen as helmet HP diminishes.
- **Bouncy Trampoline Deflectors (`MaterialTier::TRAMPOLINE`)**:
  - **High-Elasticity Launchers (`1.65f Restitution`)**: Elastic woven canvas deflectors set in reinforced steel frames that launch pigs and debris at high velocity, enabling creative bank shots over frontal blast walls and into upper towers.
  - **Spring Twang Audio**: Procedurally synthesized spring boing sweep audio (`sound.boing`, FM pitch chirp with decaying coil rattle) plays on trampoline contact.
  - **Dedicated Trampoline Visuals**: Dark gunmetal frame, brass corner brackets, woven emerald canvas lattice with golden directional chevrons (`^^^^^^`) pointing along the launch vector, and sparkling green-gold star bursts on impact.
  - **Indestructible Platform (`9999 HP`)**: Trampolines endure siege barrages to remain consistent trick-shot platforms throughout each level.
- **Swinging Wrecking Ball Pendulums (`PendulumBody`)**:
  - **Angular Harmonic Physics Solver**: Ceiling-mounted wrecking balls suspended by heavy iron chains that swing according to $\alpha = -\frac{g}{L}\sin\theta - c\omega$, dynamically interacting with pig collisions, transferring impulse, and swinging with momentum.
  - **Demolition Force & Spiked Variants**: Smashing through wooden decks and stone pillars with high kinetic damage, with spiked variants dealing devastating $2.5\times$ bonus structural damage and knocking perched birds clean off their roosts.
  - **Zero Idle Drift Equilibrium**: Mathematical resting equilibrium ($\theta = 0 \implies \sin(0) = 0 \implies \alpha = 0$) guarantees 100% stability at resting idle, ensuring all 50 levels pass headless tests with `0.000 rad` drift.
  - **Visual Pendulum Chain Geometry**: Features ceiling pivot bracket, alternating oval chain links with drop shadow depth, spherical faceted ball body with equatorial steel band, and radiating conical iron spikes.
- **Campaign Level Showcases**:
  - **Level 8 ("The Bouncing Keep")**: Ground trampoline deflector positioned before a high timber palisade, launching pigs skyward into the upper roosts.
  - **Level 12 ("Cauldron Barracks")**: Sentry birds fitted with iron cauldron helmets guarding a three-story woodland barracks.
  - **Level 18 ("Pendulum Citadel")**: Heavy granite wrecking ball suspended directly above the central gallery, sweeping through supporting pillars when struck.
  - **Level 26 ("The Trampoline Redoubt")**: Dual stepping trampolines in front of the basalt towers for shallow or steep ricochet banking.
  - **Level 42 ("Pendulum of the Obsidian Emperor")**: Spiked iron wrecking ball guarding cauldron-helmed Archduke Herons in the imperial throne room.
- **100% Stability & Unit Test Verification**:
  - Headless self-test verified across all 50 levels with `maxTilt = 0.000 rad` and `maxBirdRot = 0.000 rad`.
  - Added unit test assertions verifying helmet absorption, helmet shattering, trampoline bounce, and pendulum harmonic acceleration.
  - Exported visual verification screenshots: `screenshot_trampoline_level.bmp`, `screenshot_helmed_bird.bmp`, and `screenshot_pendulum_level.bmp`.

## 9. Feature Pack C — Atmospheric Hazards & Dynamic Weather (v35)
- **Gusting Crosswinds & Atmospheric Aerodynamics (`WindGauge` / `currentWind`)**:
  - **Directional Wind Forces**: Handcrafted crosswind vectors ($W_x \in [-180, +180]$ px/s²) applying continuous lateral acceleration ($a_{\text{wind}} = W_x \times 0.85$) to dynamic airborne pigs, floating feather particles, smoke plumes, and volcanic embers.
  - **Wind-Curved Aiming Trajectory Dots**: Overhauled the real-time parabolic aiming dots (`ghostDots`) to solve the exact integrated wind acceleration:
    $$x(t) = x_0 + v_{0x} t + \frac{1}{2} (W_x \times 0.85) t^2$$
    $$y(t) = y_0 + v_{0y} t + \frac{1}{2} g t^2$$
    giving commanders precise visual feedback on headwinds, tailwinds, and cross-breezes before release.
  - **Animated Weather Card & Wind Gauge HUD**: Sleek translucent HUD badge (`x=265, y=8, w=155, h=50`) displaying active atmospheric conditions (`ATMOSPHERE: CALM / BREEZE / GALE / STORM`) and directional wind arrows with real-time airspeed readouts (e.g., `<< -65 MPH`, `WIND: CALM`, `+110 MPH >>`).
  - **Atmospheric Wind Sound Effect (`sound.wind`)**: Low-frequency resonant rush of howling wind looping dynamically in weather-affected levels.
- **Dynamic Cumulonimbus Storm Clouds & Lightning Strikes (`StormCloud` / `LightningBolt`)**:
  - **Drifting Storm Clouds**: Ominous dark charcoal cumulonimbus cloud formations drifting across the upper atmosphere in storm-afflicted levels, rendered with layered volumetric overlapping lobes and rim lighting.
  - **High-Altitude Lightning Discharge**: Periodic branched electrostatic lightning bolts that arc from storm clouds down to the highest conductive targets on the battlefield (highest iron cauldron helmet or carved stone pillar).
  - **Atmospheric Shockwave & Screen Flash**: Strikes trigger full-screen blinding white sky flashes (`skyFlashAlpha = 0.95f`), heavy screen trauma (`addTrauma(0.55f)`), and rolling acoustic thunder booms (`sound.thunder`).
  - **Structural Conduction & Conductive Blast Damage**: Lightning delivers 160 HP direct conductive blast damage to the struck target and detonates volatile materials like TNT upon impact.
  - **Zero Idle Drift Gating**: Lightning strikes and environmental hazard triggers are strictly gated by `anyPigLaunched`, ensuring 100% idle resting stability during automated checks.
- **Volcanic Magma Vents & Geothermal Embers (`MaterialTier::MAGMA_VENT` / `ParticleKind::EMBER`)**:
  - **Indestructible Obsidian Vents (`9999 HP`, 8.0 mass)**: Heavy basalt magma geysers embedded in Chapter 5 volcanic foundations that periodically erupt with deep bubbling geothermal audio (`sound.magma`).
  - **Buoyant Rising Embers (`EMBER`)**: Eruptions spew glowing golden-crimson embers upward with negative gravity and sinusoidal atmospheric drift.
  - **Thermal Reactions (Melt Ice & Ignite TNT)**: Hot magma embers melt brittle glacial ice (`MAT_ICE`) and immediately ignite volatile gunpowder kegs (`MAT_TNT`) on physical contact or within eruption radius (`85px`).
  - **Magma Vent Texturing**: Features glowing lava veins, molten obsidian fissures, and dynamic pulsing geothermal apertures.
- **Expanded Procedural Asset Pipeline (v9)**:
  - Added procedural generators for `mat_magma.bmp` (128x128 volcanic rock with molten lava core) and `weather_storm.bmp` (128x128 churning thunderstorm texture).
  - Added acoustic WAV synthesizers for `sound.wind` (filtered brownian noise wind loop), `sound.thunder` (deep bass multi-burst explosive clap), and `sound.magma` (bubbling low-frequency molten geyser pop).
  - Manifest upgraded to 16 BMP sprites, 18 WAV sounds, and 2 music loops with `F5` hot-reload support.
- **Campaign Level Integrations**:
  - **Level 14 ("Timber Palisade & Headwind")**: Strong headwind ($W_x = -65.0$) resisting projectile arcs, requiring higher elevation angles.
  - **Level 22 ("Canyon Tailwind Viaduct")**: Powerful canyon tailwind ($W_x = +110.0$) propelling lightweight pigs across deep chasms into rear towers.
  - **Level 35 ("Blizzard Stockade")**: Heavy gale-force blizzard headwind ($W_x = -140.0$) buffeting pigs amidst glacial ice pinnacles.
  - **Level 39 ("Thunder Peak Sanctuary")**: Electrified mountain peak with stormy headwind ($W_x = -50.0$) and dynamic lightning arcing into iron helmet sentries and high stone watchtowers.
  - **Level 44 ("Volcanic Casemate")**: Chapter 5 volcanic fortress embedded with active magma vents launching embers that ignite TNT bunkers and dissolve glacial shields.
  - **Level 50 ("The Emperor's Final High Eyrie")**: Climactic siege combining high-altitude crosswinds ($W_x = -75.0$), drifting storm clouds with dynamic lightning strikes, and volcanic magma vents flanking the imperial fortress moat.
- **100% Stability & Unit Test Verification**:
  - All 50 levels verified rock-solid at idle with `maxTilt = 0.000 rad` and `maxBirdRot = 0.000 rad` across 300-frame (5-second) awake tests in `./furious_swine --self-test`.
  - Added unit test suite assertions verifying wind lateral acceleration, lightning damage targeting, magma vent eruption radius, and ember thermal ignition of TNT and ice melting.
  - Exported visual verification screenshots: `screenshot_wind_level.bmp`, `screenshot_storm_level.bmp`, and `screenshot_magma_level.bmp`.

## 10. Feature Pack D — Secret Truffle Hunt, Bonus Vault Levels & Royal Swine Trophy Hall (v36 / Assets v10)
- **15 Collectible Hidden Golden Origami Truffles (`GoldenTruffle` / `levelTruffle`)**:
  - **Campaign Placement**: 15 golden origami truffles hidden in tricky structural niches, secret vaults, and high rafters across the campaign (3 per chapter: Levels 2, 5, 9, 13, 17, 20, 23, 27, 30, 33, 36, 40, 43, 47, 50).
  - **Dynamic Visuals & Aura**: Radiant pulsating double aura (`pulseTime`), origami faceted gold geometry with specular glints, and rotating 4-point golden glint flares.
  - **Interactive Collection Mechanics**: Collected via direct pig proximity, moving block collisions, or explosive blast radius (Vulcan seismic shockwaves & TNT detonations).
  - **Juicy Rewards & Audio**: Spawns 24 `GOLD_SPARKLE` particles radiating outward, plays `sound.truffle` (celestial ascending arpeggio chime `E6->G#6->B6->E7`), awards `+2,500` bonus points with popping floating text, and permanently saves to `save_progress.cfg` (`level.X.truffle = 1`).
  - **HUD & Modal Integration**: Displays a dedicated golden HUD status badge (`[TRUFFLE COLLECTED *]` in shimmering gold vs `* SECRET TRUFFLE HIDDEN *` in amber), and reports truffle discovery status in the level complete modal.
- **5 Secret Truffle Vault Challenge Levels (Levels 51–55 / Chapter VI: The Secret Truffle Vaults)**:
  - **Campaign Expansion**: Expanded total levels from 50 to 55 (`NUM_LEVELS = 55`, `NUM_VAULT_LEVELS = 5`).
  - **Milestone Unlock Progression**: Unlocked via cumulative campaign Golden Truffle discoveries:
    - **Level 51 (Vault 01 — Powder Crypt of the Swine)**: Requires 2 Truffles. Domino TNT crypt with balanced slate pillars and heavy explosive chain reactions.
    - **Level 52 (Vault 02 — Trampoline Gauntlet)**: Requires 5 Truffles. Dual elastic trampoline deflectors defending a steep stone citadel.
    - **Level 53 (Vault 03 — Pendulum Cathedral)**: Requires 8 Truffles. Spiked iron wrecking ball pendulum suspended over twin totem towers.
    - **Level 54 (Vault 04 — Thunder Ridge Bastion)**: Requires 11 Truffles. Violent gale headwinds ($W_x = -110$) and lightning-charged storm skies defending high ridge towers.
    - **Level 55 (Vault 05 — Obsidian Truffle Treasury)**: Requires 14 Truffles. The ultimate imperial stronghold featuring magma vents, trampolines, TNT bunkers, and cauldron-helmed Archduke Herons.
  - **Atmospheric Royal Twilight Palette**: Chapter VI features royal amethyst-to-amber twilight skies (`#2E1A47` $\to$ `#C25E34`), an ancient golden solar eclipse halo with corona rings, obsidian ruin silhouettes with glowing beacon fires, and drifting ambient gold sparkle motes.
  - **Level Select Page 3 ("VAULTS 51-55")**: Added dedicated `PAGE 3: VAULTS (51-55)` tab to Level Select featuring 5 grand horizontal Vault Cards displaying clearance status, personal best scores, and real-time truffle unlock requirements (e.g., `[LOCKED -- REQUIRES 5 GOLDEN TRUFFLES] COLLECT IN CAMPAIGN (CURRENT: 3 / 5)`).
- **Royal Swine Trophy Hall & Achievement System (`ScreenState::ACHIEVEMENTS`)**:
  - **Full-Screen Trophy Hall**: Dedicated hall accessible from the Title Screen via `[ROYAL TROPHIES & ACHIEVEMENTS]` button, featuring a 5x2 card grid tracking 10 distinct combat and exploration feats:
    1. **First Strike (`FIRST_BLOOD`)**: Defeat your first aristocratic Totem Bird sentry.
    2. **Long-Distance Swine (`MARKSMAN`)**: Defeat a Totem Bird from over 750px slingshot range.
    3. **Cauldron Cracker (`CAULDRON_CRACKER`)**: Shatter 5 iron cauldron helmets off sentries (with live progress tracking).
    4. **Seismic Annihilation (`SEISMIC_ANNIHILATION`)**: Demolish 12 or more fortress blocks in a single shot.
    5. **Anvil Executioner (`ANVIL_EXECUTIONER`)**: Crush an Archduke Heron with Trotter's Anvil Slam dive.
    6. **Powder Keg Maestro (`POWDER_MAESTRO`)**: Detonate 3 or more TNT gunpowder kegs in a single shot.
    7. **Acrobatic Swine (`ACROBATIC_HOG`)**: Defeat a Totem Bird on a ricochet after hitting a Trampoline.
    8. **Truffle Sniffer (`TRUFFLE_SNIFFER`)**: Discover your first Hidden Golden Truffle in the campaign.
    9. **Truffle Connoisseur (`TRUFFLE_CONNOISSEUR`)**: Discover 10 Hidden Golden Truffles across the campaign.
    10. **Vault Conqueror (`VAULT_CONQUEROR`)**: Conquer all 5 Secret Truffle Vault challenge levels.
  - **Animated Toast Notification Engine (`AchievementToast`)**: Unlocking achievements in-game triggers an animated royal amethyst-and-gold banner that slides down smoothly from the top of the screen (`slideY`), presents the golden trophy icon, plays fanfare audio (`sound.achievement`), and slides back up after 4.5 seconds.
  - **Persistent Bitmask Serialization**: Encodes achievement state in `achievements.mask` and cauldron stats in `achievements.helmets` within `save_progress.cfg`.
- **Expanded Procedural Asset Pipeline (v10)**:
  - Upgraded asset version to `v10` in `assets/version.txt` with automatic bootstrapping:
    - Added procedural generators for `item_truffle.bmp` (64x64 radiant faceted golden origami truffle) and `ui_trophy.bmp` (64x64 polished royal golden swine trophy cup).
    - Added acoustic synthesis for `sound.truffle` (0.75s ascending celestial harp arpeggio) and `sound.achievement` (1.35s royal brass trumpet fanfare).
  - Total asset counts scaled to 18 BMP sprites, 20 WAV sounds, and 2 music loops.
- **100% Stability & Complete Unit Test Suite**:
  - Headless self-test verified across all 55 levels with `maxTilt = 0.000 rad` and `maxBirdRot = 0.000 rad` in a 300-frame (5-second) awake stability test.
  - Added Unit Tests 14 (Truffle collection & particle burst), 15 (Vault unlock requirements logic), and 16 (Achievement unlock, toast triggering, and serialization).
  - Exported 26 comprehensive verification screenshots including `screenshot_achievements.bmp`, `screenshot_level_select_p3.bmp`, `screenshot_truffle_level.bmp`, `screenshot_vault_level.bmp`, and `screenshot_chap6_vault.bmp`.

## 11. Feature Pack E — The Fortress Architect, Gale the Aeronaut, Cinematic Slo-Mo Cam & Endless Siege Gauntlet (v37 / Assets v11)
- **5th Heroic Swine: Gale the Aeronaut (`PigType::GLIDER`)**:
  - **Origami Sky-Blue Glider Pig**: Sky-blue origami folding palette (`#52A8EE` body, `#AFE1FF` facets) with dual swept aerodynamic wings, brass aviator flight goggles with golden rivets, cyan pupil eyes, and a rear directional rudder fin.
  - **Mid-Air Aerodynamic Glide Lift**: Tap `[SPACE]` in flight to deploy wings, surge forward with high lateral thrust (`vel.x = std::max(vel.x + 85.0f, 290.0f)`), and catch an updraft lift (`vel.y = -95.0f`), playing the rushing wind glide audio (`sound.glide`).
  - **Twin Aerial Bomb Release (`AerialBomb`)**: Upon glide activation, Gale drops two heavy cast-iron aerial bombs with brass caps and burning fuses that tumble downwards into enemy structures.
  - **Radial Blast Detonation**: Aerial bombs detonate upon impacting blocks, birds, or the ground (`blastRadius = 145.0f`), dealing heavy radial structural damage, detonating adjacent TNT, and launching shockwave smoke rings.
- **Cinematic Slo-Mo Finishing Cam ("Avian Reckoning")**:
  - **Bullet-Time Dramatic Finish**: When the final living bird in any fortress is struck and defeated, the game automatically enters a dramatic bullet-time slow-motion sequence ($0.22\times$ physics time factor).
  - **Dynamic Letterbox & Target Tracking**: Smoothly animates cinematic widescreen letterbox bars (46px height) bearing the header `** AVIAN RECKONING **` in radiant gold, while the camera dynamically glides and frames the final collapsing avian fortress.
  - **Toggle Hotkey**: Press **`[C]`** at any time to toggle the finishing cam on/off, with floating feedback and real-time status in the main menu.
- **Interactive Fortress Architect & Sandbox (`ScreenState::EDITOR`)**:
  - **Full-Screen Fortress Builder**: Interactive fortress editor accessible directly from the main menu (`[FORTRESS ARCHITECT (CUSTOM SANDBOX) >>]`), providing a creative proving ground for building and testing custom avian citadels.
  - **Multi-Category Siege Palette**:
    - **Tools**: Block, Bird, Pendulum, Truffle, Demolish Eraser.
    - **Materials**: Timber (Wood), Stone (Slate), Ice (Glacial Brittle), TNT (Gunpowder Keg), Bouncer (Trampoline Deflector).
    - **Block Sizes**: Small Post ($16\times 50$), Medium Beam ($20\times 70$), Large Pillar ($24\times 90$), Horizontal Plank ($80\times 18$), Square Block ($36\times 36$).
    - **Bird Species**: Sentry Toucan (HP 80), Warden Owl (HP 120), Archduke Heron (HP 180), with toggleable Iron Cauldron Helmets (+100 HP armor).
    - **Pendulums**: Overhead harmonic wrecking balls with toggleable lethal spikes.
    - **Secret Golden Truffle**: Hidden reward collectible placement (+2,500 bonus points).
  - **Precision Grid & Environmental Controls**: Toggleable 20px snap-to-grid (`[G]`), Chapter Theme cycle (Chapters 1–6), dynamic wind control ($\pm 120$ MPH), and cumulonimbus lightning storm toggle.
  - **3 Save/Load Configuration Slots**: Instant persistence to `custom_siege_1.cfg`, `custom_siege_2.cfg`, `custom_siege_3.cfg`.
  - **1-Click Test Siege & Snapshot Restoration**: Press `[T]` or click `[TEST SIEGE >>]` to immediately load all 5 heroic swine into the slingshot and test-play the created fortress; return to the architect at any time (`[< RETURN TO ARCHITECT]`) with 100% snapshot restoration.
- **Endless Siege Gauntlet Mode (`ScreenState::ENDLESS`)**:
  - **Procedural Fortress Survival**: Infinite survival mode accessible from the title screen (`[ENDLESS SIEGE GAUNTLET (PROCEDURAL SURVIVAL) >>]`).
  - **Escalating Wave Difficulty**: Fortress height, multi-bay width, block tier durability (Twig $\to$ Timber $\to$ Stone $\to$ Ice), cauldron-helmed bird counts, swinging wrecking ball pendulums, and crosswinds escalate as waves progress.
  - **Dynamic Swine Queue Replenishment**: Clearing waves rewards additional heroic swine reinforcements (Trotter, Bramble, Clover, Vulcan, Gale) into the catapult queue.
  - **Combo Score Multiplier & Record Tracking**: Chipping armor, damaging birds, and executing multi-kill collapses builds the combo score multiplier up to $5.0\times$. Tracks and saves personal best wave records (`endless.best_wave`).
- **Procedural Asset Pipeline (v11)**:
  - Upgraded asset version to `v11` in `assets/version.txt` with automatic bootstrapping:
    - Added procedural origami generator for `pig_glider.bmp` (64x64 sky-blue glider swine with aviator goggles and swept wings).
    - Added acoustic procedural synthesizer for `sound.glide` (0.85s rushing aerodynamic wind-glide with harmonic flutter).
  - Asset totals scaled to 19 BMP sprites, 21 WAV sounds, and 2 music loops.
- **100% Automated Stability & Verification Suite**:
  - All 55 campaign and vault levels verified rock-solid at idle and awake with `maxTilt = 0.000 rad` and `maxBirdRot = 0.000 rad` across 300-frame (5-second) physics checks.
  - Added Unit Tests 17 (Gale glide surge + twin aerial bomb detonation), 18 (Fortress Architect serialization & test play snapshot restore), and 19 (Endless mode wave progression & combo multiplier).
  - Exported 30 high-resolution verification screenshots including `screenshot_editor.bmp`, `screenshot_endless.bmp`, `screenshot_pig_glider.bmp`, and `screenshot_slomo_finish.bmp`.

## 12. Modern Audio Engine Overhaul & Main Menu UI Auto-Fit (v38 / Assets v12)
- **Title Screen Button Visual Polish & Auto-Fit Engine**:
  - **Eliminated Button Text Overflow**: Resolved the text overflow on the title screen where mode labels collided with button boundaries. Expanded main action button width to 520px with spacious 120px margins centered within the 760px modal card.
  - **Refined Button Typography & Labels**:
    - `"FORTRESS ARCHITECT (SANDBOX BUILDER) >>"` (scale: 1.55f)
    - `"ENDLESS SIEGE GAUNTLET (SURVIVAL) >>"` (scale: 1.55f)
    - Cleaned secondary button scales (`LEVEL SELECT`, `TROPHY HALL`, `AUDIO TOGGLES`, `FULLSCREEN`) to 1.50f for harmonious visual balance.
  - **Dynamic Auto-Scaling Text Protection**: Implemented a mathematical boundary clamp inside `drawButton()`: if any button label exceeds `btn.rect.w - 18.0f`, the engine automatically computes and scales down the font scale (`scale = maxTextW / (len * 6.0f)`), guaranteeing button text across the entire game will never overflow, clip, or look cramped.
- **Complete Modern Audio Synthesis Overhaul (Assets v12)**:
  - **Ditched Retro/Chiptune Synthesizers**: Completely eliminated the 8-bit/16-bit geometric sine/saw waveforms and repetitive tracker-style arpeggios in favor of modern physical acoustic modeling and organic sound design.
  - **Battle / Gameplay BGM (`bgm_battle.wav` — 32 Bars, ~70s Loop @ 110 BPM)**:
    - **Acoustic Bouncy Puzzle Groove**: Inspired by modern casual classics (*Angry Birds*, *Bad Piggies*), engineered with a head-nodding, relaxing, bouncy tempo.
    - **Karplus-Strong Upright Bass**: Physically modeled vibrating acoustic string with lowpass loop damping, playing a warm, syncopated walking bassline.
    - **Karplus-Strong Rhythm Guitar**: Multi-string nylon acoustic strums on reggae/ska off-beats with authentic pick transients.
    - **Rosewood Marimba Modal Synthesis**: Flexural wooden bar resonators ($f_0, 3.98 f_0, 9.25 f_0$) with soft rubber mallet strike impulses.
    - **Organic Acoustic Percussion**: Tuned wooden cajón (deep wooden kick + rim taps) and a studio shaker with natural 16th-note swing (60/40 ratio).
    - **Warm Rhodes 7th Chords**: Smooth electric piano pads providing rich jazz/pop harmonic depth.
    - **Breathing Arrangement (No Ear Fatigue!)**: Removed the persistent high-pitched flute melody. The gameplay music now features extensive groove-only breathing sections (Bars 0–7, 10–11, 14–15, 16–23), allowing the player to concentrate and aim while enjoying the acoustic rhythm.
    - **Seamless 2048-Sample Crossfade**: 100% click-free infinite looping.
  - **Title / Menu BGM (`bgm_title.wav` — 16 Bars, ~46s Loop @ 84 BPM)**:
    - **Contemporary Acoustic Indie Theme in D Major**: Intimate Karplus-Strong fingerpicked acoustic guitar arpeggios, resonant upright bass / cello, soft music box / glockenspiel bell melodies, warm electric piano chords, and a 4-comb + 2-allpass Schroeder studio room reverb.
  - **Organic Sound Effects Modernization (21 Sounds)**:
    - Overhauled all 21 game sounds with layered acoustic transients and physical materials:
      - `sound.launch`: Wooden tension stress creak + leather pocket release snap + clean aerodynamic lowpass whoosh.
      - `sound.dash`: Punchy transient crack + 80Hz–35Hz sub-bass dive + clean aerodynamic thrust.
      - `sound.split`: 3-note rosewood marimba arpeggio (C5, E5, G5).
      - `sound.slam`: Cast-iron anvil clang + saturated 42Hz seismic shockwave.
      - `sound.explosion`: High-pressure acoustic crack + saturated sub-bass blast + granular rolling debris wash.
      - `sound.hit_twig` & `sound.hit_timber`: Organic bamboo/twig fracture and seasoned hollow oak plank knock (modes at 165Hz, 290Hz, 510Hz).
      - `sound.bird_defeat`: Comical cartoon "poof" (soft air pop + feathered rustle + glockenspiel sparkle).
      - `sound.victory`: Warm brass triad + celebratory glockenspiel cascade.
      - `sound.ui_click`: Soft tactile woodblock tap (520Hz, 0.03s, non-fatiguing).
      - `sound.hit_stone`, `sound.hit_ice`, `sound.hit_metal`, `sound.boing`, `sound.wind`, `sound.thunder`, `sound.magma`, `sound.truffle`, `sound.achievement`, `sound.glide`: All enhanced with modern physical resonances and sub-bass warmth.
- **Verification & Testing**:
  - Headless test suite (`--self-test`) passing 100% across all 55 levels and all 19 unit tests with rock-solid equilibrium (`maxTilt = 0.000 rad`).
  - Exported updated high-resolution screenshot [`screenshot_title.bmp`](file:///home/austin/Downloads/furious_swine/screenshot_title.bmp) showing the beautifully proportioned main menu layout.

## 13. Warm Acoustic Upright Bass & Dry Sub-Bass Mix Overhaul (v39 / Assets v13)
- **Eliminated Grating Digital Comb Buzz**:
  - Diagnosed and resolved the harsh, buzzy, mechanical timbre in `bgm_battle.wav` and `bgm_title.wav`. The previous unfiltered Karplus-Strong string loop at bass frequencies ($55\text{--}80\text{ Hz}$) suffered from minimal comb-filter attenuation at low harmonics, causing white noise bursts to ring out as an aggressive, grating buzz.
  - Replaced low-string comb filtering with a dedicated physical acoustic upright bass synthesizer (`renderUprightBass`):
    - Pure sinusoidal fundamental with natural exponential decay ($t_{\text{decay}} \propto 1 / (1.8 + 0.005 f)$).
    - Warm 2nd harmonic ($0.20\times$) and subtle 3rd harmonic ($0.04\times$) for authentic wooden body resonance.
    - Soft fingertip wooden pluck thump transient ($75\text{ Hz}$, $18\text{ ms}$).
    - 2-Pole Butterworth Low-Pass Filter (`BiquadLP`, $Q = 0.707$) with strict cutoff below $240\text{ Hz}$, completely filtering out all high-frequency comb buzz and mechanical harshness.
- **Velocity Balance & Mix Restraint**:
  - Reduced bass note velocities across all tracks from overpowering levels ($0.65\text{--}0.70$) down to smooth, balanced mix levels ($0.20\text{--}0.32$).
  - Bass now provides warm low-end foundation without drowning out the acoustic rhythm guitar, rosewood marimba, Rhodes piano pads, or shaker.
- **Dry Sub-Bass Bus Routing (Reverb Separation)**:
  - Separated procedural audio mixdown into two parallel buses: `mixMidsHighs` and `mixBass`.
  - Schroeder studio room reverb (4 comb filters + 2 all-pass diffusers) is now applied *strictly* to `mixMidsHighs` (guitars, marimba, Rhodes, bells).
  - Upright bass and cajón kick remain 100% dry and centered on `mixBass`, completely eliminating low-frequency reverb standing waves, sub-bass mud, and limiter intermodulation distortion.
- **Rhythmic Bass Phrasing & Walking Patterns**:
  - Replaced the repetitive 4-on-the-floor bass drone in `bgm_battle.wav` with an authentic acoustic walking pattern (Beat 0 = root, Beat 2 = fifth, Beat 3.5 = syncopated pickup, Beats 1 & 3 = open groove rest), providing space and rhythmic bounce.
- **Asset Pipeline Upgrade (v13)**:
  - Bumped version marker to `v13` in `assets/version.txt` with automatic WAV regeneration on launch.
  - Master peak amplitude verified at $71.2\%$ (title) and $69.4\%$ (battle), with a comfortable $-16.8\text{ dBFS}$ RMS floor and zero digital clipping.
- **100% Headless Stability Verification**:
  - All 55 campaign and vault levels verified rock-solid at idle (`maxTilt = 0.000 rad`) and all 19 unit tests passing.

## 14. Warm Physical Acoustic Guitar Synthesis & Soundboard Resonance (v40 / Assets v14)
- **Eliminated Piercing Karplus-Strong Comb Filter Buzz**:
  - Identified the source of the fake, grating timbre in the rhythm guitar and fingerpicked arpeggios. The previous Karplus-Strong string algorithm (`renderPluck`) filled delay lines with high-brightness white noise ($0.70$), generating a massive, harsh comb-filter spectrum where over $25.5\%$ of the total acoustic energy poured into frequencies $> 3\text{ kHz}$ (the human ear's most fatigue-sensitive zone).
  - Replaced the white-noise delay line with a dedicated physical acoustic guitar synthesizer (`renderAcousticGuitar`):
    - **Plucked Harmonic Series with Natural Dispersion**: Modeled strings with triangular pluck displacement, physical inharmonicity ($f_n = n f_0 \sqrt{1 + B n^2}$), and progressive harmonic decay ($h_1$ fundamental with long sustain, $h_2$ octave overtone, $h_3$ fifth, and fast-decaying $h_4/h_5$ shimmer).
    - **Soft Fingertip / Plectrum Attack Transient**: Replaced full-spectrum noise bursts with a soft, warm wooden fingernail/pick transient ($280\text{ Hz}$, $6\text{ ms}$).
    - **2-Pole Butterworth Soundboard Filter (`BiquadLP`)**: Processed the string vibration through an acoustic guitar top-plate resonance filter with cutoff dynamically clamped between $1.2\text{ kHz}$ and $2.6\text{ kHz}$ ($\min(2600\text{ Hz}, \max(1200\text{ Hz}, f_0 \times 7.5))$), completely attenuating grating frequencies above $3\text{ kHz}$ (reduced from $25.5\%$ to $0.02\%\text{--}0.22\%$).
- **Mix & Velocity Calibration**:
  - Calibrated title theme fingerpicked arpeggio velocity to $0.28\text{f}$ for a sweet, resonant, singing acoustic tone.
  - Rebalanced the 3-string off-beat rhythm strum in `bgm_battle.wav` to $0.22\text{f}$, $0.18\text{f}$, and $0.16\text{f}$, allowing the acoustic guitar to sit warmly in the pocket alongside the cajón, shaker, and upright bass.
- **Asset Pipeline Upgrade (v14)**:
  - Bumped version marker to `v14` in `assets/version.txt` and `src/furious_swine.cpp` with automatic soundtrack regeneration on game launch.
  - Master peaks verified at $72.1\%$ (title) and $71.0\%$ (battle) with an RMS floor of $-16.5\text{ dBFS}$ (zero digital clipping or limiter distortion).
- **100% Stability Verification**:
  - Passed `./furious_swine --self-test` across all 55 campaign and vault levels (`maxTilt = 0.000 rad`) and all 19 unit tests.

## 15. Advanced Swine & Avian Variety Overhaul (v41 / Assets v15)
- **Two New Heroic Swine Classes**:
  - **Atticus the Graviton Boar (`PigType::GRAVITON`)**:
    - **Visual Geometry & Theming**: Adorned with a glowing violet diadem diadem and dual coiled copper Tesla emitters with flashing electrostatic coronas.
    - **Mid-Air Ability — Graviton Vortex**: Activating mid-air opens a cosmic singularity vortex centered on Atticus for $1.35\text{ s}$.
    - **Physical Vortex Mechanics**: Pulls all ungrounded blocks and enemy birds inward within an event horizon radius of $210\text{ px}$ with continuous inward acceleration, emitting swirling violet `COSMIC_MOTE` particles.
    - **Singularity Implosion**: When the vortex expires or collapses, it detonates in a violent cosmic shockwave (`blastRadius = 190.0f`), dealing heavy radial impulse and damage to sucked-in targets.
  - **Rusty the Drill Hog (`PigType::DRILL`)**:
    - **Visual Geometry & Theming**: Features a rugged yellow miner's hardhat, polished brass headlamp, and a rifled titanium rotary bore snout.
    - **Mid-Air Ability — Supersonic Rotary Bore**: Surges forward at extreme velocity ($\ge 780\text{ px/s}$), entering an energized drilling state for $1.8\text{ s}$.
    - **Multi-Block Piercing**: Pierces straight through consecutive fortress blocks without stopping ($0.22\times$ drag, $0.68\times$ damage multiplier), showering amber friction sparks and stone/timber chips.
    - **Foundation Burrowing & Subsidence**: When contacting ground level while drilling, triggers structural subsidence, undermining foundation blocks ($y > \text{GROUND\_Y} - 95$) with cave-in downward velocity and damage.
- **Two New Aristocratic Avian Classes**:
  - **Commander Falcon (`BirdClass::FALCON`)**:
    - **Visual Geometry & Theming**: Perched with metallic plumage, ceremonial crest, and a heavy iron-riveted **Directional Tower Shield** covering its entire frontal quadrant.
    - **Shield Deflection Mechanics**: Frontal swine impacts ($x_{\text{pig}} \le x_{\text{falcon}}$) hit the tower shield with high elasticity ($e = 0.75$), deflecting pigs backward with a loud clang (`sound.shield`).
    - **Shield Health & Shattering**: The shield absorbs up to $120\text{ HP}$ of direct impact damage before shattering into tumbling `SHIELD_DEBRIS` fragments (+1,200 bonus score), exposing Falcon's $150\text{ HP}$ core.
  - **Baron Von Condor (`BirdClass::CONDOR`)**:
    - **Visual Geometry & Theming**: Massive imperial condor ($240\text{ HP}$, $r = 28\text{ px}$) suspended in an armored **Steampunk Zeppelin** with brass propellers and gasbag envelope.
    - **Buoyant Lift Mechanics**: While the balloon remains intact ($70\text{ HP}$), buoyant lift counteracts gravity (reducing downward acceleration to $0.15\times$).
    - **Balloon Puncture & Gondola Plunge**: Depleting balloon HP pops the envelope with a loud puncture hiss and creak (`sound.zeppelin`), ejecting jagged `BALLOON_SCRAP` canvas scraps (+1,500 bonus score). The heavy $4.8\text{ kg}$ iron gondola plummets violently downward at $320\text{ px/s}$, crushing and splintering fortress blocks beneath upon impact.
- **Campaign & Game Mode Integrations**:
  - **Campaign Progression**: Hand-placed new characters into key campaign levels:
    - **Level 16 ("Canopy Skyway")**: Introduces Commander Falcon's directional tower shield defending high timber viaducts.
    - **Level 28 ("Cavern Gate")**: Introduces Atticus the Graviton Boar to collapse heavy stone blast walls.
    - **Level 32 ("The High Steppes")**: Features Baron Von Condor's suspended steampunk zeppelin hovering above glacial towers.
    - **Level 45 ("The Iron Foundry")**: Combines Atticus, Rusty, Falcon, and Condor in a complex multi-bay stronghold.
    - **Level 50 ("The Emperor's Final High Eyrie")**: Full-scale war with all swine and avian classes deployed simultaneously.
  - **Endless Siege Gauntlet**: Wave generator updated to dynamically field Falcon and Condor defenders, and award Atticus and Rusty reinforcements on high waves.
  - **Fortress Architect (Level Editor)**: Added dedicated buttons and placement tools for Commander Falcon (with Tower Shield) and Baron Von Condor (with Steampunk Zeppelin). Level serialization and test play fully updated.
  - **Royal Swine Trophy Hall**: Added 2 new achievements:
    - **`VORTEX_MAESTRO` ("Event Horizon")**: Suck 8 or more blocks or birds into Atticus's Graviton Vortex.
    - **`DEEP_BORE` ("Deep Bore")**: Bore clean through 3 or more fortress blocks in a single drill shot.
- **Procedural Asset Pipeline (v15)**:
  - Bumped version to `v15` in `assets/version.txt` and `src/furious_swine.cpp`.
  - Added 4 procedural BMP sprite generators: `pig_graviton.bmp`, `pig_drill.bmp`, `bird_falcon.bmp`, `bird_condor.bmp` (total 23 sprites).
  - Added 4 procedural WAV audio synthesis waveforms: `sound.vortex`, `sound.drill`, `sound.shield`, `sound.zeppelin` (total 25 sounds).
- **Automated Verification Suite (100% Pass Rate)**:
  - All 55 levels pass 300-frame (5-second) awake physics equilibrium test with `maxTilt = 0.000 rad`.
  - All 23 unit tests pass cleanly, including new Unit Tests 20 (Graviton vortex), 21 (Drill supersonic bore & subsidence), 22 (Falcon shield deflection & shattering), and 23 (Zeppelin pop, buoyant lift & gondola crushing).
  - Exported high-resolution verification screenshots: `screenshot_pig_graviton.bmp`, `screenshot_pig_drill.bmp`, `screenshot_bird_falcon.bmp`, `screenshot_bird_condor.bmp`.

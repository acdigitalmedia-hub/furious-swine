// Text format for levels, shared by the campaign (assets/levels/*.cfg) and
// the Fortress Architect editor's custom_siege_N.cfg save slots.
//
// One "key = value" per line; lines starting with '#' are comments.
//
//   name     = LEVEL 1: WICKER WATCHTOWER
//   subtitle = Pull the catapult & launch the Trotter!
//   pigs     = trotter turbo split bomb glider graviton drill
//   wind     = -60                  (lateral wind, px/s^2; default 0)
//   storm    = 1                    (storm clouds + lightning; default 0)
//   truffle  = 850 380              (hidden golden truffle position; optional)
//   star2    = 21000                (optional; computed from contents if absent)
//   star3    = 26000
//   theme    = 2                    (editor only: backdrop 0-14, one per chapter)
//
//   block    = <material> <x> <y> <w> <h> [field=value ...]
//              material: twig timber stone ice tnt trampoline magma
//   bird     = <kind> <x> <y> [helmet] [shield] [zeppelin] [field=value ...]
//              kind: toucan owl heron falcon condor
//              Stats come from SnoutAndSiegeGame::makeBird(); falcons always
//              get a shield and condors a zeppelin. field=value overrides a
//              stat, e.g. helmetHp=180 maxHelmetHp=180.
//   pendulum = <anchorX> <anchorY> <length> [spiked] [field=value ...]
//              fields: angle angVel radius mass (defaults: 0 0 24 4.5)
//
// Positions are world pixels (ground top is y = 630, catapult at x = 215).
// The editor's older numeric format ("block = 1 780 580 24 100",
// "bird = 2 840 610 1", "pendulum = x y len 0|1") is still accepted.
#pragma once

#include "common.h"

#include <string>

struct ParsedLevel {
    LevelSpec spec;
    bool hasPigs = false;
    bool hasStarThresholds = false;
    int theme = -1;             // -1 = not specified
    bool legacyFormat = false;  // pre-"format = 2" editor file
};

// Parses level text. Malformed or out-of-range entries are skipped and
// described in *warnings (if given). Returns false only if nothing usable
// could be read at all.
bool parseLevelText(const std::string& text, ParsedLevel& out, std::string* warnings = nullptr);

// Serializes a level. theme < 0 omits the theme line; starThresholds controls
// whether star2/star3 are written.
std::string serializeLevel(const LevelSpec& L, int theme = -1, bool starThresholds = false);

bool readTextFile(const std::string& path, std::string& out);
// Writes to a temp file and renames it over the target, so a crash or a full
// disk mid-write can never leave a truncated file behind.
bool writeTextFileAtomically(const std::string& path, const std::string& contents);

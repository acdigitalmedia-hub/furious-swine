// Procedural generation of the default sprite and sound/music assets
// written into assets/ on first run (or when assets/version.txt is stale).
#include "assets.h"

void AssetPipeline::renderUprightBass(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                     float duration, float vel) {
    size_t totalSamples = static_cast<size_t>(duration * sampleRate);
    BiquadLP lp;
    lp.setParams(std::min(240.0f, freq * 3.2f), 0.707f, sampleRate);

    float p1 = 0.0f;
    for (size_t i = 0; i < totalSamples; ++i) {
        size_t dstIdx = startSample + i;
        if (dstIdx >= dest.size()) break;
        float t = static_cast<float>(i) / sampleRate;

        float attack = std::min(1.0f, t / 0.008f);
        float decayRate = 1.8f + freq * 0.005f;
        float env = attack * std::exp(-t * decayRate);

        p1 += 2.0f * PI * freq / sampleRate;
        float fundamental = std::sin(p1);
        float secondHarm  = 0.20f * std::sin(p1 * 2.0f) * std::exp(-t * 4.0f);
        float thirdHarm   = 0.04f * std::sin(p1 * 3.0f) * std::exp(-t * 8.0f);
        float thump = (t < 0.018f) ? (1.0f - t / 0.018f) * std::sin(2.0f * PI * 75.0f * t) * 0.22f : 0.0f;

        float raw = (fundamental + secondHarm + thirdHarm + thump) * env * vel;
        float warm = lp.process(raw);
        dest[dstIdx] += warm;
    }
}

void AssetPipeline::renderAcousticGuitar(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                        float duration, float vel) {
    float cutoff = std::min(2600.0f, std::max(1200.0f, freq * 7.5f));
    BiquadLP lp;
    lp.setParams(cutoff, 0.707f, sampleRate);
    size_t totalSamples = static_cast<size_t>(duration * sampleRate);
    float p1 = 0.0f;
    float decayRate = 2.4f + freq * 0.003f;
    for (size_t i = 0; i < totalSamples; ++i) {
        size_t dstIdx = startSample + i;
        if (dstIdx >= dest.size()) break;
        float t = static_cast<float>(i) / sampleRate;
        float attack = std::min(1.0f, t / 0.004f);
        float env = attack * std::exp(-t * decayRate);

        p1 += 2.0f * PI * freq / sampleRate;
        float h1 = std::sin(p1);
        float h2 = 0.38f * std::sin(p1 * 2.0004f) * std::exp(-t * 5.5f);
        float h3 = 0.16f * std::sin(p1 * 3.0009f) * std::exp(-t * 11.0f);
        float h4 = 0.06f * std::sin(p1 * 4.0016f) * std::exp(-t * 20.0f);
        float h5 = 0.02f * std::sin(p1 * 5.0025f) * std::exp(-t * 32.0f);

        float thump = (t < 0.006f) ? (1.0f - t / 0.006f) * std::sin(2.0f * PI * 280.0f * t) * 0.20f : 0.0f;
        float raw = (h1 + h2 + h3 + h4 + h5 + thump) * env * vel;
        dest[dstIdx] += lp.process(raw);
    }
}

void AssetPipeline::renderPluck(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                float duration, float vel, uint32_t& /*rng*/, float /*decay*/, float /*damp*/, float /*brightness*/) {
    renderAcousticGuitar(dest, startSample, freq, sampleRate, duration, vel);
}

void AssetPipeline::renderMarimba(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                  float duration, float vel, uint32_t& rng) {
    size_t totalSamples = static_cast<size_t>(duration * sampleRate);
    for (size_t i = 0; i < totalSamples; ++i) {
        size_t dstIdx = startSample + i;
        if (dstIdx >= dest.size()) break;
        float t = static_cast<float>(i) / sampleRate;
        rng = rng * 1664525u + 1013904223u;
        float white = (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
        float mallet = (t < 0.0035f) ? (1.0f - t / 0.0035f) * white * 0.18f : 0.0f;
        float m1 = std::sin(2.0f * PI * freq * t) * std::exp(-t * (2.8f + freq * 0.0025f));
        float m2 = std::sin(2.0f * PI * (freq * 3.98f) * t) * std::exp(-t * 26.0f) * 0.28f;
        float m3 = std::sin(2.0f * PI * (freq * 9.25f) * t) * std::exp(-t * 60.0f) * 0.10f;
        dest[dstIdx] += (m1 + m2 + m3 + mallet) * vel;
    }
}

void AssetPipeline::renderRhodesChord(std::vector<float>& dest, size_t startSample, const std::vector<float>& freqs,
                                      float sampleRate, float duration, float vel) {
    size_t totalSamples = static_cast<size_t>(duration * sampleRate);
    for (size_t i = 0; i < totalSamples; ++i) {
        size_t dstIdx = startSample + i;
        if (dstIdx >= dest.size()) break;
        float t = static_cast<float>(i) / sampleRate;
        float chordVal = 0.0f;
        float lfo = 1.0f + 0.08f * std::sin(2.0f * PI * 0.55f * t);
        float env = std::min(1.0f, t / 0.035f) * std::exp(-t * 1.15f);
        for (float f : freqs) {
            float p1 = 2.0f * PI * f * t;
            float p2 = 2.0f * PI * (f * 2.002f) * t;
            chordVal += (std::sin(p1) + 0.20f * std::sin(p2) * std::exp(-t * 3.0f));
        }
        dest[dstIdx] += chordVal * env * lfo * vel;
    }
}

void AssetPipeline::renderCajon(std::vector<float>& dest, size_t startSample, float sampleRate, float vel, bool isBass) {
    size_t totalSamples = static_cast<size_t>(0.24f * sampleRate);
    float p = 0.0f;
    for (size_t i = 0; i < totalSamples; ++i) {
        size_t dstIdx = startSample + i;
        if (dstIdx >= dest.size()) break;
        float t = static_cast<float>(i) / sampleRate;
        if (isBass) {
            float f = 82.0f * std::exp(-t * 22.0f) + 44.0f;
            p += 2.0f * PI * f / sampleRate;
            float env = std::exp(-t * 13.0f);
            float tap = (t < 0.005f) ? (1.0f - t / 0.005f) * 0.30f : 0.0f;
            dest[dstIdx] += (std::tanh(1.4f * std::sin(p)) + tap) * env * vel;
        } else {
            float f = 280.0f * std::exp(-t * 35.0f) + 140.0f;
            p += 2.0f * PI * f / sampleRate;
            float env = std::exp(-t * 28.0f);
            dest[dstIdx] += std::sin(p) * env * vel;
        }
    }
}

void AssetPipeline::renderShaker(std::vector<float>& dest, size_t startSample, float sampleRate, float vel, uint32_t& rng, ResonantBP& bpShaker) {
    size_t totalSamples = static_cast<size_t>(0.045f * sampleRate);
    for (size_t i = 0; i < totalSamples; ++i) {
        size_t dstIdx = startSample + i;
        if (dstIdx >= dest.size()) break;
        float t = static_cast<float>(i) / sampleRate;
        rng = rng * 1664525u + 1013904223u;
        float white = (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
        float s = bpShaker.process(white);
        float env = std::exp(-t * 60.0f) * std::min(1.0f, t / 0.003f);
        dest[dstIdx] += s * env * vel;
    }
}

void AssetPipeline::writeSynthesizedWav(const std::string& filename, int type, bool forceOverwrite) {
    if (!forceOverwrite) {
        std::ifstream check(filename, std::ios::binary);
        if (check.good()) return;
    }

    const int sampleRate = 44100;
    float duration = 0.35f;
    if (type == 0) duration = 0.38f; // Catapult Launch
    if (type == 1) duration = 0.40f; // Turbo Boar Dash
    if (type == 2) duration = 0.36f; // Jade Tri-Piglet Split
    if (type == 3) duration = 0.52f; // Trotter Anvil Slam
    if (type == 4) duration = 0.78f; // Vulcan Seismic Detonation
    if (type == 5) duration = 0.32f; // Wicker / Reed Shatter
    if (type == 6) duration = 0.38f; // Seasoned Timber Impact
    if (type == 7) duration = 0.50f; // Bird Defeat
    if (type == 8) duration = 1.25f; // Victory Fanfare
    if (type == 9) duration = 0.42f; // Stone / Slate Impact
    if (type == 10) duration = 0.06f; // UI Click
    if (type == 12) duration = 0.35f; // Glacial Ice / Crystal Shatter
    if (type == 13) duration = 0.40f; // Iron Cauldron / Helmet Clang
    if (type == 14) duration = 0.44f; // Trampoline Spring Boing
    if (type == 15) duration = 1.25f; // Gusting Crosswind whistling sweep
    if (type == 16) duration = 1.45f; // Atmospheric Thunder crack & low rumble
    if (type == 17) duration = 0.85f; // Geothermal Magma Vent eruption & sizzle
    if (type == 18) duration = 0.75f; // Golden Truffle Discovery Chime
    if (type == 19) duration = 1.40f; // Royal Swine Achievement Fanfare
    if (type == 20) duration = 45.714285f; // Title BGM (16 bars @ 84 BPM in D major)
    if (type == 21) duration = 69.818182f; // Battle BGM (32 bars @ 110 BPM modern acoustic groove)
    if (type == 22) duration = 0.65f; // Aeronaut Glider Flight (sound.glide)
    if (type == 23) duration = 0.85f; // Graviton Vortex hum & cosmic implosion sweep
    if (type == 24) duration = 0.55f; // Drill Hog supersonic rotary bore
    if (type == 25) duration = 0.45f; // Falcon Directional Tower Shield clang & ricochet
    if (type == 26) duration = 0.70f; // Zeppelin balloon puncture hiss & crash creak

    const int numSamples = static_cast<int>(sampleRate * duration);
    std::vector<int16_t> pcm(numSamples, 0);

    uint32_t rng = 0x9E3779B9u ^ (static_cast<uint32_t>(type) * 1013904223u);
    auto randFloat = [&]() -> float {
        rng = rng * 1664525u + 1013904223u;
        return (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
    };

    if (type == 20) {
        // --- TITLE BGM: Contemporary Acoustic Indie Theme in D Major (84 BPM, 16 Bars) ---
        const float beatLen = 60.0f / 84.0f;
        std::vector<float> mixMidsHighs(numSamples, 0.0f);
        std::vector<float> mixBass(numSamples, 0.0f);

        static const float chordRoots[16] = {
            146.83f, 123.47f,  98.00f, 110.00f, // D  -> Bm -> G  -> A
            185.00f, 123.47f,  82.41f, 110.00f, // F#m-> Bm -> Em -> A
             98.00f, 110.00f, 185.00f, 123.47f, // G  -> A  -> F#m-> Bm
             82.41f, 110.00f, 146.83f, 146.83f  // Em -> A  -> D  -> D
        };
        static const float chordThirds[16] = {
            185.00f, 146.83f, 123.47f, 146.83f,
            220.00f, 146.83f,  98.00f, 138.59f,
            123.47f, 138.59f, 220.00f, 146.83f,
             98.00f, 138.59f, 185.00f, 185.00f
        };
        static const float chordFifths[16] = {
            220.00f, 185.00f, 146.83f, 164.81f,
            277.18f, 185.00f, 123.47f, 164.81f,
            146.83f, 164.81f, 277.18f, 185.00f,
            123.47f, 164.81f, 220.00f, 220.00f
        };
        static const float chordSevenths[16] = {
            277.18f, 220.00f, 185.00f, 196.00f,
            329.63f, 220.00f, 146.83f, 196.00f,
            185.00f, 196.00f, 329.63f, 220.00f,
            146.83f, 196.00f, 277.18f, 277.18f
        };

        static const float melodyNotes[16] = {
            739.99f, 880.00f, 739.99f, 659.25f, // F#5, A5, F#5, E5
            587.33f, 739.99f, 659.25f, 0.0f,    // D5,  F#5, E5,  rest
            783.99f, 880.00f, 739.99f, 587.33f, // G5,  A5,  F#5, D5
            659.25f, 739.99f, 587.33f, 0.0f     // E5,  F#5, D5,  cadence
        };

        for (int bar = 0; bar < 16; ++bar) {
            float barStart = bar * 4.0f * beatLen;
            size_t sBar = static_cast<size_t>(barStart * sampleRate);

            // 1. Warm Rhodes / Electric Piano Pad
            std::vector<float> chord{chordRoots[bar], chordThirds[bar], chordFifths[bar], chordSevenths[bar]};
            renderRhodesChord(mixMidsHighs, sBar, chord, sampleRate, 4.0f * beatLen, 0.28f);

            // 2. Warm Upright Bass on root and 5th (warm, deep, balanced)
            renderUprightBass(mixBass, sBar, chordRoots[bar] * 0.5f, sampleRate, beatLen * 2.8f, 0.30f);
            size_t sBeat2 = static_cast<size_t>((barStart + 2.0f * beatLen) * sampleRate);
            renderUprightBass(mixBass, sBeat2, chordFifths[bar] * 0.5f, sampleRate, beatLen * 1.8f, 0.24f);

            // 3. Contemporary Warm Fingerpicked Acoustic Guitar Arpeggio (8th notes)
            for (int sub = 0; sub < 8; ++sub) {
                float tSub = barStart + sub * (beatLen * 0.5f);
                size_t sSub = static_cast<size_t>(tSub * sampleRate);
                float fArp = (sub == 0 || sub == 4) ? chordRoots[bar] :
                             ((sub == 1 || sub == 5) ? chordThirds[bar] :
                             ((sub == 2 || sub == 6) ? chordFifths[bar] : chordSevenths[bar]));
                if (sub >= 4) fArp *= 2.0f;
                renderAcousticGuitar(mixMidsHighs, sSub, fArp, sampleRate, beatLen * 1.2f, 0.28f);
            }

            // 4. Glockenspiel / Music Box Melody
            if (melodyNotes[bar] > 20.0f) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), melodyNotes[bar], sampleRate, beatLen * 2.5f, 0.42f, rng);
            }
        }

        // Apply Studio Room Reverb to mids/highs (bass stays dry, centered, and tight)
        StudioReverb rev; rev.init(sampleRate);
        std::vector<float> master(numSamples, 0.0f);
        for (int i = 0; i < numSamples; ++i) {
            float dry = mixMidsHighs[i];
            float wet = rev.process(dry);
            master[i] = dry + wet * 0.16f + mixBass[i];
        }

        // Seamless clickless crossfade on last 2048 samples
        const int crossfade = 2048;
        for (int i = 0; i < crossfade; ++i) {
            float alpha = static_cast<float>(i) / crossfade;
            int endIdx = numSamples - crossfade + i;
            master[endIdx] = master[endIdx] * (1.0f - alpha) + master[i] * alpha;
        }

        for (int i = 0; i < numSamples; ++i) {
            float s = std::tanh(master[i] * 1.05f);
            pcm[i] = static_cast<int16_t>(clampf(s * 25000.0f, -32000.0f, 32000.0f));
        }
    } else if (type == 21) {
        // --- BATTLE BGM: Modern Bouncy Acoustic Groove (110 BPM, 32 Bars, ~70s) ---
        const float beatLen = 60.0f / 110.0f;
        std::vector<float> mixMidsHighs(numSamples, 0.0f);
        std::vector<float> mixBass(numSamples, 0.0f);
        ResonantBP bpShaker; bpShaker.setParams(4800.0f, 2.2f, sampleRate);

        static const float chordRoots[32] = {
            130.81f, 110.00f, 146.83f,  98.00f, 164.81f, 110.00f, 146.83f,  98.00f, // C -> Am -> Dm -> G -> Em -> Am -> Dm -> G
            174.61f,  98.00f, 164.81f, 110.00f, 146.83f,  98.00f, 130.81f, 130.81f, // F -> G  -> Em -> Am -> Dm -> G  -> C  -> C7
            110.00f, 164.81f, 110.00f, 146.83f, 174.61f, 164.81f, 146.83f,  98.00f, // Am-> E7 -> Am -> D7 -> F  -> Em -> Dm -> G
            130.81f, 164.81f, 174.61f,  98.00f, 110.00f, 146.83f,  98.00f, 130.81f  // C -> Em -> F  -> G  -> Am -> Dm -> G  -> C
        };
        static const float chordThirds[32] = {
            164.81f, 130.81f, 174.61f, 123.47f, 196.00f, 130.81f, 174.61f, 123.47f,
            220.00f, 123.47f, 196.00f, 130.81f, 174.61f, 123.47f, 164.81f, 164.81f,
            130.81f, 207.65f, 130.81f, 185.00f, 220.00f, 196.00f, 174.61f, 123.47f,
            164.81f, 196.00f, 220.00f, 123.47f, 130.81f, 174.61f, 123.47f, 164.81f
        };
        static const float chordFifths[32] = {
            196.00f, 164.81f, 220.00f, 146.83f, 246.94f, 164.81f, 220.00f, 146.83f,
            261.63f, 146.83f, 246.94f, 164.81f, 220.00f, 146.83f, 196.00f, 196.00f,
            164.81f, 246.94f, 164.81f, 220.00f, 261.63f, 246.94f, 220.00f, 146.83f,
            196.00f, 246.94f, 261.63f, 146.83f, 164.81f, 220.00f, 146.83f, 196.00f
        };

        for (int bar = 0; bar < 32; ++bar) {
            float barStart = bar * 4.0f * beatLen;
            size_t sBar = static_cast<size_t>(barStart * sampleRate);

            // 1. Warm Rhodes / Electric Piano Pad
            std::vector<float> chord{chordRoots[bar], chordThirds[bar], chordFifths[bar]};
            renderRhodesChord(mixMidsHighs, sBar, chord, sampleRate, 4.0f * beatLen, (bar >= 16 && bar <= 23 ? 0.30f : 0.20f));

            for (int beat = 0; beat < 4; ++beat) {
                float tBeat = barStart + beat * beatLen;
                size_t sBeat = static_cast<size_t>(tBeat * sampleRate);

                // 2. Wooden Cajon
                if (bar >= 16 && bar <= 23) {
                    renderCajon(mixMidsHighs, sBeat, sampleRate, 0.24f, false);
                } else {
                    if (beat == 0 || beat == 2) {
                        renderCajon(mixBass, sBeat, sampleRate, (beat == 0 ? 0.48f : 0.34f), true);
                    } else {
                        renderCajon(mixMidsHighs, sBeat, sampleRate, 0.30f, false);
                    }
                }

                // 3. Studio Shaker with 16th-note swing
                for (int sub16 = 0; sub16 < 4; ++sub16) {
                    float swingOffset = (sub16 % 2 == 1) ? 0.025f : 0.0f;
                    float tShaker = tBeat + (sub16 * 0.25f + swingOffset) * beatLen;
                    size_t sShaker = static_cast<size_t>(tShaker * sampleRate);
                    float sVel = (sub16 == 0 ? 0.20f : (sub16 == 2 ? 0.32f : 0.12f));
                    renderShaker(mixMidsHighs, sShaker, sampleRate, sVel, rng, bpShaker);
                }

                // 4. Warm Upright Bass Walking Pattern (Bouncy, spacious, zero harsh buzz)
                if (beat == 0) {
                    renderUprightBass(mixBass, sBeat, chordRoots[bar] * 0.5f, sampleRate, beatLen * 1.4f, 0.32f);
                } else if (beat == 2) {
                    renderUprightBass(mixBass, sBeat, chordFifths[bar] * 0.5f, sampleRate, beatLen * 1.2f, 0.26f);
                } else if (beat == 3) {
                    size_t sPickup = static_cast<size_t>((tBeat + beatLen * 0.5f) * sampleRate);
                    renderUprightBass(mixBass, sPickup, chordRoots[bar] * 0.5f * 1.05946f, sampleRate, beatLen * 0.7f, 0.20f);
                }

                // 5. Warm Acoustic Guitar Strum on Off-Beats (Rich, organic, zero harsh comb buzz)
                size_t sOff = static_cast<size_t>((tBeat + beatLen * 0.5f) * sampleRate);
                renderAcousticGuitar(mixMidsHighs, sOff, chordThirds[bar], sampleRate, beatLen * 0.85f, 0.22f);
                renderAcousticGuitar(mixMidsHighs, sOff + 160, chordFifths[bar], sampleRate, beatLen * 0.85f, 0.18f);
                renderAcousticGuitar(mixMidsHighs, sOff + 320, chordRoots[bar] * 2.0f, sampleRate, beatLen * 0.85f, 0.16f);
            }

            // 6. Rosewood Marimba Melody (Section 2 & 4 with breathing pauses!)
            if (bar == 8) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.2f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.5f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.2f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.5f * beatLen * sampleRate), 880.00f, sampleRate, beatLen * 1.5f, 0.48f, rng);
            } else if (bar == 9) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 587.33f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.0f * beatLen * sampleRate), 523.25f, sampleRate, beatLen * 1.5f, 0.45f, rng);
            } else if (bar == 12) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), 587.33f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.5f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.5f * beatLen * sampleRate), 698.46f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.5f * beatLen * sampleRate), 880.00f, sampleRate, beatLen * 1.5f, 0.48f, rng);
            } else if (bar == 13) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 698.46f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.0f * beatLen * sampleRate), 587.33f, sampleRate, beatLen * 1.5f, 0.45f, rng);
            } else if (bar == 24) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 523.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.45f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(3.0f * beatLen * sampleRate), 1046.50f, sampleRate, beatLen * 1.5f, 0.48f, rng);
            } else if (bar == 25) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 987.77f, sampleRate, beatLen * 1.1f, 0.44f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.0f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.0f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.5f, 0.45f, rng);
            } else if (bar == 28) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.5f * beatLen * sampleRate), 880.00f, sampleRate, beatLen * 1.1f, 0.44f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(1.5f * beatLen * sampleRate), 783.99f, sampleRate, beatLen * 1.1f, 0.42f, rng);
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(2.5f * beatLen * sampleRate), 659.25f, sampleRate, beatLen * 1.5f, 0.45f, rng);
            } else if (bar == 31) {
                renderMarimba(mixMidsHighs, sBar + static_cast<size_t>(0.0f * beatLen * sampleRate), 523.25f, sampleRate, beatLen * 2.5f, 0.42f, rng);
            }
        }

        // Apply Studio Room Reverb to mids/highs (bass stays clean, punchy, and dry)
        StudioReverb rev; rev.init(sampleRate);
        std::vector<float> master(numSamples, 0.0f);
        for (int i = 0; i < numSamples; ++i) {
            float dry = mixMidsHighs[i];
            float wet = rev.process(dry);
            master[i] = dry + wet * 0.15f + mixBass[i];
        }

        // Seamless clickless crossfade on last 2048 samples
        const int crossfade = 2048;
        for (int i = 0; i < crossfade; ++i) {
            float alpha = static_cast<float>(i) / crossfade;
            int endIdx = numSamples - crossfade + i;
            master[endIdx] = master[endIdx] * (1.0f - alpha) + master[i] * alpha;
        }

        for (int i = 0; i < numSamples; ++i) {
            float s = std::tanh(master[i] * 1.05f);
            pcm[i] = static_cast<int16_t>(clampf(s * 25000.0f, -32000.0f, 32000.0f));
        }
    } else {
        // --- MODERN SOUND EFFECTS (types 0 to 19, 22) ---
        float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
        float lpNoise = 0.0f, lpNoise2 = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float attack = std::min(1.0f, t / 0.003f);
            float sample = 0.0f;
            float wn = randFloat();
            lpNoise = 0.88f * lpNoise + 0.12f * wn;
            lpNoise2 = 0.65f * lpNoise2 + 0.35f * wn;

            if (type == 0) {
                // Catapult Launch: Organic wood tension stress creak + leather release click + aerodynamic whoosh
                float fCreak = 120.0f + 260.0f * std::sin(t * 120.0f);
                float fWhoosh = 420.0f * std::exp(-t * 8.0f) + 110.0f;
                p1 += 2.0f * PI * fCreak / sampleRate;
                p2 += 2.0f * PI * fWhoosh / sampleRate;
                float creak = (t < 0.04f) ? std::sin(p1) * (1.0f - t / 0.04f) * 0.45f : 0.0f;
                float snap = (t >= 0.035f && t < 0.045f) ? wn * 0.85f : 0.0f;
                float whoosh = lpNoise2 * std::sin(p2) * std::sin(PI * (t / duration)) * 0.55f;
                float body = std::sin(2.0f * PI * 130.0f * t) * std::exp(-t * 14.0f) * 0.40f;
                sample = (creak + snap + whoosh + body) * attack;
            } else if (type == 1) {
                // Turbo Boar Dash: Clean propulsion whoosh with sub-bass punch & stereo-phased rush
                float fSub = 85.0f * std::exp(-t * 6.0f) + 35.0f;
                p1 += 2.0f * PI * fSub / sampleRate;
                p2 += 2.0f * PI * (fSub * 1.5f) / sampleRate;
                float sub = std::tanh(2.2f * std::sin(p1)) * std::exp(-t * 7.5f) * 0.65f;
                float rush = (wn - lpNoise) * std::exp(-t * 5.0f) * 0.50f;
                float pop = (t < 0.015f) ? wn * (1.0f - t / 0.015f) * 0.55f : 0.0f;
                sample = (sub + rush + pop) * attack;
            } else if (type == 2) {
                // Tri-Piglet Split: 3-tone rosewood marimba arpeggio (C5 - E5 - G5)
                int nIdx = (t < 0.08f) ? 0 : (t < 0.16f ? 1 : 2);
                float lt = t - nIdx * 0.08f;
                float f0 = (nIdx == 0 ? 523.25f : (nIdx == 1 ? 659.25f : 783.99f));
                float m1 = std::sin(2.0f * PI * f0 * lt) * std::exp(-lt * 18.0f);
                float m2 = std::sin(2.0f * PI * (f0 * 3.98f) * lt) * std::exp(-lt * 45.0f) * 0.25f;
                float tap = (lt < 0.004f) ? wn * 0.20f : 0.0f;
                sample = (m1 + m2 + tap) * std::min(1.0f, lt / 0.002f);
            } else if (type == 3) {
                // Trotter Anvil Slam: Heavy cast-iron anvil clang + saturated 42Hz seismic shockwave
                float fSub = 42.0f * std::exp(-t * 3.0f);
                p1 += 2.0f * PI * fSub / sampleRate;
                p2 += 2.0f * PI * 880.0f / sampleRate;
                p3 += 2.0f * PI * 1420.0f / sampleRate;
                float metal = (0.35f * std::sin(p2) + 0.25f * std::sin(p3)) * std::exp(-t * 22.0f);
                float sub = std::tanh(2.8f * std::sin(p1)) * std::exp(-t * 5.5f) * 0.85f;
                sample = (metal + sub) * attack;
            } else if (type == 4) {
                // Vulcan Seismic Detonation: Modern cinematic explosion (crack transient + sub-bass + rubble wash)
                float fSub = 65.0f * std::exp(-t * 4.0f) + 28.0f;
                p1 += 2.0f * PI * fSub / sampleRate;
                float crack = (t < 0.015f) ? wn * (1.0f - t / 0.015f) * 1.2f : 0.0f;
                float subWave = std::tanh(3.0f * std::sin(p1)) * std::exp(-t * 3.5f) * 0.85f;
                float rubble = lpNoise * std::exp(-t * 2.5f) * 0.70f;
                sample = (crack + subWave + rubble) * attack;
            } else if (type == 5) {
                // Wicker / Reed Shatter: Organic bamboo & twig fracture crack
                float snapEnv = std::exp(-t * 36.0f) + ((t > 0.03f) ? 0.6f * std::exp(-(t - 0.03f) * 45.0f) : 0.0f);
                sample = (0.40f * (wn - lpNoise) + 0.35f * lpNoise2) * snapEnv * 1.1f * attack;
            } else if (type == 6) {
                // Seasoned Timber Impact: Resonant hollow oak plank knock (modes at 165Hz, 290Hz, 510Hz)
                p1 += 2.0f * PI * 165.0f / sampleRate;
                p2 += 2.0f * PI * 290.0f / sampleRate;
                p3 += 2.0f * PI * 510.0f / sampleRate;
                float knock = 0.55f * std::sin(p1) * std::exp(-t * 14.0f) +
                              0.30f * std::sin(p2) * std::exp(-t * 24.0f) +
                              0.18f * std::sin(p3) * std::exp(-t * 36.0f);
                float crack = (t < 0.008f) ? wn * 0.40f : 0.0f;
                sample = (knock + crack) * attack * 1.15f;
            } else if (type == 7) {
                // Avian Vanquished: Comical organic air pop + feathered rush + glockenspiel sparkle
                float fPop = 420.0f * std::exp(-t * 25.0f) + 140.0f;
                p1 += 2.0f * PI * fPop / sampleRate;
                float pop = std::sin(p1) * std::exp(-t * 18.0f) * 0.65f;
                float feathers = (wn - lpNoise) * std::exp(-t * 8.0f) * 0.45f;
                float sparkle = (t > 0.08f) ? std::sin(2.0f * PI * 1318.5f * t) * std::exp(-(t - 0.08f) * 14.0f) * 0.25f : 0.0f;
                sample = (pop + feathers + sparkle) * attack;
            } else if (type == 8) {
                // Victory Fanfare: Warm brass triad + cascading glockenspiel chimes
                int st = (t < 0.22f) ? 0 : (t < 0.44f ? 1 : (t < 0.66f ? 2 : 3));
                float stT = t - st * 0.22f;
                float fLead = (st == 0 ? 523.25f : (st == 1 ? 659.25f : (st == 2 ? 783.99f : 1046.50f)));
                p1 += 2.0f * PI * fLead / sampleRate;
                float env = std::exp(-stT * (st == 3 ? 2.5f : 5.5f)) * std::min(1.0f, stT / 0.008f);
                float brass = (0.55f * std::sin(p1) + 0.25f * std::sin(p1 * 2.0f) + 0.12f * std::sin(p1 * 3.0f)) * env;
                sample = brass * 0.95f;
            } else if (type == 9) {
                // Stone / Slate Impact: Heavy mineral thud (85Hz) + stone crumble
                float fThud = 95.0f * std::exp(-t * 12.0f) + 42.0f;
                p1 += 2.0f * PI * fThud / sampleRate;
                float thud = std::tanh(1.8f * std::sin(p1)) * std::exp(-t * 15.0f) * 0.75f;
                float grit = lpNoise2 * std::exp(-t * 30.0f) * 0.35f;
                sample = (thud + grit) * attack;
            } else if (type == 11) {
                // High Score Fanfare: Shimmering celesta & glockenspiel arpeggio
                static const float chNotes[7] = {523.25f, 659.25f, 783.99f, 987.77f, 1046.50f, 1318.51f, 1567.98f};
                float chVal = 0.0f;
                for (int n = 0; n < 7; ++n) {
                    float nStart = n * 0.08f;
                    if (t >= nStart) {
                        float nt = t - nStart;
                        float f = chNotes[n];
                        float env = std::exp(-nt * (n == 6 ? 2.2f : 5.5f)) * std::min(1.0f, nt / 0.003f);
                        chVal += std::sin(2.0f * PI * f * nt) * env * 0.28f;
                    }
                }
                sample = chVal * attack;
            } else if (type == 12) {
                // Glacial Ice Shatter: Shimmering glass overtones with crisp micro-fracture crackle
                p1 += 2.0f * PI * 2240.0f / sampleRate;
                p2 += 2.0f * PI * 3480.0f / sampleRate;
                p3 += 2.0f * PI * 4920.0f / sampleRate;
                float ring = (0.50f * std::sin(p1) + 0.32f * std::sin(p2) + 0.18f * std::sin(p3)) * std::exp(-t * 12.0f);
                float crack = (wn - lpNoise) * std::exp(-t * 35.0f) * 0.65f;
                sample = (ring + crack) * attack;
            } else if (type == 13) {
                // Iron Cauldron Clang: Heavy cast iron bell chime with acoustic damping
                p1 += 2.0f * PI * 540.0f / sampleRate;
                p2 += 2.0f * PI * 1280.0f / sampleRate;
                p3 += 2.0f * PI * 2450.0f / sampleRate;
                float clang = 0.45f * std::sin(p1) * std::exp(-t * 10.0f) +
                              0.35f * std::sin(p2) * std::exp(-t * 16.0f) +
                              0.20f * std::sin(p3) * std::exp(-t * 26.0f);
                sample = clang * attack * 1.1f;
            } else if (type == 14) {
                // Trampoline Bounce: Organic rubber tension bend with elastic pitch scoop
                float sweep = 110.0f + 240.0f * (1.0f - std::exp(-t * 14.0f)) * std::exp(-t * 5.0f);
                p1 += 2.0f * PI * sweep / sampleRate;
                float twang = std::sin(p1) * std::exp(-t * 6.5f) * 0.85f;
                sample = twang * attack;
            } else if (type == 15) {
                // Wind Gust: Smooth atmospheric breeze with gentle frequency-swept pink noise
                float mod = std::sin(2.0f * PI * 1.5f * t);
                float gust = lpNoise * (0.8f + 0.35f * mod) * std::sin(PI * (t / duration));
                sample = gust * 1.1f;
            } else if (type == 16) {
                // Thunder: Lightning crack + rolling subterranean sub-bass rumble
                float fSub = 58.0f * std::exp(-t * 1.5f) + 24.0f;
                p1 += 2.0f * PI * fSub / sampleRate;
                float crack = wn * std::exp(-t * 26.0f) * 1.1f;
                float sub = std::tanh(2.8f * std::sin(p1)) * std::exp(-t * 1.3f) * 0.85f;
                float roll = lpNoise * std::exp(-t * 1.0f) * 0.65f;
                sample = (crack + sub + roll) * attack;
            } else if (type == 17) {
                // Magma: Geothermal vent pop + bubbling molten rock sizzle
                float fPop = 130.0f * std::exp(-t * 7.0f) + 36.0f;
                p1 += 2.0f * PI * fPop / sampleRate;
                float pop = std::sin(p1) * std::exp(-t * 10.0f) * 0.75f;
                float hiss = (wn - lpNoise) * std::exp(-t * 3.0f) * 0.55f;
                sample = (pop + hiss) * attack;
            } else if (type == 18) {
                // Golden Truffle: Sparkling 4-note ascending celestial music box chime (E6-G#6-B6-E7)
                int nIdx = clampf(static_cast<int>(t / 0.14f), 0, 3);
                float lt = t - nIdx * 0.14f;
                static const float arpF[4] = {1318.51f, 1661.22f, 1975.53f, 2637.02f};
                float f = arpF[nIdx];
                p1 += 2.0f * PI * f / sampleRate;
                float sparkle = std::sin(p1) * std::exp(-lt * 10.0f) * std::min(1.0f, lt / 0.003f);
                sample = sparkle * attack * 1.25f;
            } else if (type == 19) {
                // Achievement: Triumphant modern brass & chime fanfare
                int fIdx = (t < 0.22f) ? 0 : (t < 0.44f ? 1 : (t < 0.66f ? 2 : 3));
                float lt = (fIdx < 3) ? (t - fIdx * 0.22f) : (t - 0.66f);
                static const float brF[4] = {587.33f, 739.99f, 880.00f, 1174.66f};
                float targetF = brF[fIdx];
                p1 += 2.0f * PI * targetF / sampleRate;
                float bEnv = std::exp(-lt * (fIdx < 3 ? 5.0f : 2.2f)) * std::min(1.0f, lt / 0.012f);
                float brass = (0.55f * std::sin(p1) + 0.28f * std::sin(p1 * 2.0f)) * bEnv;
                sample = brass * attack * 1.25f;
            } else if (type == 22) {
                // Aeronaut Glider Flight: Soaring aerodynamic wind rush with gentle wing flutter
                float fGlide = 280.0f + 240.0f * (1.0f - std::exp(-t * 6.0f));
                p1 += 2.0f * PI * fGlide / sampleRate;
                float flutter = 1.0f + 0.20f * std::sin(2.0f * PI * 16.0f * t);
                float windRush = (wn - lpNoise) * 0.50f;
                float wing = std::sin(p1) * flutter * 0.40f;
                sample = (windRush + wing) * std::exp(-t * 3.5f) * attack * 1.2f;
            } else if (type == 23) {
                // Graviton Vortex: Deep cosmic sub-oscillator sweep + reverse vortex suction + implosion pop
                float fVortex = 160.0f * (1.0f - (t / duration)) + 45.0f;
                p1 += 2.0f * PI * fVortex / sampleRate;
                float ringMod = std::sin(2.0f * PI * 28.0f * t);
                float subHum = std::tanh(2.5f * std::sin(p1)) * (0.75f + 0.25f * ringMod);
                float suction = lpNoise * (t / duration) * 0.85f;
                float pop = (t > duration - 0.12f) ? (wn * std::exp(-(t - (duration - 0.12f)) * 40.0f)) : 0.0f;
                sample = (subHum * 0.75f + suction + pop) * attack * 1.25f;
            } else if (type == 24) {
                // Drill Bore: Warm mechanical rotary engine whir & resonant tungsten bore
                float fDrill = 165.0f + 25.0f * std::sin(2.0f * PI * 14.0f * t);
                p1 += 2.0f * PI * fDrill / sampleRate;
                float motor = std::tanh(1.8f * std::sin(p1));
                float subRumble = 0.35f * std::sin(p1 * 0.5f);
                float churn = lpNoise * (0.30f + 0.15f * std::sin(2.0f * PI * 28.0f * t));
                sample = (motor * 0.55f + subRumble + churn) * std::exp(-t * 3.4f) * attack * 0.70f;
            } else if (type == 25) {
                // Directional Tower Shield: Resonant tempered steel deflection ping & ricochet
                p1 += 2.0f * PI * 880.0f / sampleRate;
                p2 += 2.0f * PI * 1760.0f / sampleRate;
                float ring = (0.7f * std::sin(p1) + 0.35f * std::sin(p2)) * std::exp(-t * 14.0f);
                float thud = std::sin(2.0f * PI * 110.0f * t) * std::exp(-t * 22.0f) * 0.8f;
                sample = (ring + thud) * attack * 1.4f;
            } else if (type == 26) {
                // Zeppelin Puncture: High-pressure canvas tearing hiss & groaning deflate
                float canvasHiss = (wn - lpNoise) * std::exp(-t * 4.5f) * 1.1f;
                float groan = std::sin(2.0f * PI * (90.0f - t * 40.0f) * t) * std::exp(-t * 2.5f) * 0.65f;
                sample = (canvasHiss + groan) * attack * 1.25f;
            } else {
                // Tactile Woodblock UI Click (type == 10)
                float fClick = 520.0f * std::exp(-t * 40.0f) + 260.0f;
                p1 += 2.0f * PI * fClick / sampleRate;
                sample = std::sin(p1) * std::exp(-t * 55.0f) * attack * 0.85f;
            }

            int32_t val = static_cast<int32_t>(clampf(sample, -1.0f, 1.0f) * 26000.0f);
            pcm[i] = static_cast<int16_t>(val);
        }
    }

    std::ofstream out(filename, std::ios::binary);
    uint32_t dataBytes = static_cast<uint32_t>(numSamples * sizeof(int16_t));
    uint32_t riffSize = 36 + dataBytes;
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t byteRate = sampleRate * numChannels * sizeof(int16_t);
    uint16_t blockAlign = numChannels * sizeof(int16_t);
    uint16_t bitsPerSample = 16;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&riffSize), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    out.write(reinterpret_cast<const char*>(&fmtSize), 4);
    out.write(reinterpret_cast<const char*>(&audioFormat), 2);
    out.write(reinterpret_cast<const char*>(&numChannels), 2);
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataBytes), 4);
    out.write(reinterpret_cast<const char*>(pcm.data()), dataBytes);
}

void AssetPipeline::bootstrapDefaultAssets() {
    ensureDir("assets");
    ensureDir("assets/sprites");
    ensureDir("assets/sounds");

    ensureDir("assets/music");
    // Check asset version marker (v15 includes Graviton, Drill, Falcon, Condor characters)
    bool needUpgrade = true;
    {
        std::ifstream vf("assets/version.txt");
        std::string ver;
        if (vf.good() && std::getline(vf, ver) && ver == "v16") {
            needUpgrade = false;
        }
    }

    std::ifstream mfCheck("assets/manifest.cfg");
    if (!mfCheck.good() || needUpgrade) {
        std::ofstream mf("assets/manifest.cfg");
        mf << "# Snout & Siege Custom Asset Pipeline Manifest (v5)\n";
        mf << "# Replace any BMP sprite or WAV sound file and press F5 in-game to hot-reload.\n\n";
        mf << "sprite.pig_trotter   = assets/sprites/pig_trotter.bmp\n";
        mf << "sprite.pig_turbo     = assets/sprites/pig_turbo.bmp\n";
        mf << "sprite.pig_split     = assets/sprites/pig_split.bmp\n";
        mf << "sprite.pig_bomb      = assets/sprites/pig_bomb.bmp\n";
        mf << "sprite.pig_glider    = assets/sprites/pig_glider.bmp\n";
        mf << "sprite.pig_graviton  = assets/sprites/pig_graviton.bmp\n";
        mf << "sprite.pig_drill     = assets/sprites/pig_drill.bmp\n";
        mf << "sprite.bird_toucan   = assets/sprites/bird_toucan.bmp\n";
        mf << "sprite.bird_owl      = assets/sprites/bird_owl.bmp\n";
        mf << "sprite.bird_heron    = assets/sprites/bird_heron.bmp\n";
        mf << "sprite.bird_falcon   = assets/sprites/bird_falcon.bmp\n";
        mf << "sprite.bird_condor   = assets/sprites/bird_condor.bmp\n";
        mf << "sprite.mat_twig      = assets/sprites/mat_twig.bmp\n";
        mf << "sprite.mat_timber    = assets/sprites/mat_timber.bmp\n";
        mf << "sprite.mat_stone     = assets/sprites/mat_stone.bmp\n";
        mf << "sprite.mat_ice       = assets/sprites/mat_ice.bmp\n";
        mf << "sprite.mat_tnt       = assets/sprites/mat_tnt.bmp\n";
        mf << "sprite.mat_trampoline= assets/sprites/mat_trampoline.bmp\n";
        mf << "sprite.metal_helm    = assets/sprites/metal_helm.bmp\n";
        mf << "sprite.mat_magma     = assets/sprites/mat_magma.bmp\n";
        mf << "sprite.weather_storm = assets/sprites/weather_storm.bmp\n";
        mf << "sprite.item_truffle  = assets/sprites/item_truffle.bmp\n";
        mf << "sprite.ui_trophy     = assets/sprites/ui_trophy.bmp\n\n";
        mf << "sound.launch         = assets/sounds/launch.wav\n";
        mf << "sound.dash           = assets/sounds/ability_dash.wav\n";
        mf << "sound.split          = assets/sounds/ability_split.wav\n";
        mf << "sound.slam           = assets/sounds/ability_slam.wav\n";
        mf << "sound.explosion      = assets/sounds/explosion.wav\n";
        mf << "sound.hit_twig       = assets/sounds/hit_twig.wav\n";
        mf << "sound.hit_timber     = assets/sounds/hit_timber.wav\n";
        mf << "sound.hit_stone      = assets/sounds/hit_stone.wav\n";
        mf << "sound.hit_ice        = assets/sounds/hit_ice.wav\n";
        mf << "sound.hit_metal      = assets/sounds/hit_metal.wav\n";
        mf << "sound.boing          = assets/sounds/boing.wav\n";
        mf << "sound.wind           = assets/sounds/wind.wav\n";
        mf << "sound.thunder        = assets/sounds/thunder.wav\n";
        mf << "sound.magma          = assets/sounds/magma.wav\n";
        mf << "sound.truffle        = assets/sounds/truffle.wav\n";
        mf << "sound.achievement    = assets/sounds/achievement.wav\n";
        mf << "sound.bird_defeat    = assets/sounds/bird_defeat.wav\n";
        mf << "sound.victory        = assets/sounds/victory.wav\n";
        mf << "sound.glide          = assets/sounds/glide.wav\n";
        mf << "sound.vortex         = assets/sounds/vortex.wav\n";
        mf << "sound.drill          = assets/sounds/drill.wav\n";
        mf << "sound.shield         = assets/sounds/shield.wav\n";
        mf << "sound.zeppelin       = assets/sounds/zeppelin.wav\n";
        mf << "sound.ui_click       = assets/sounds/ui_click.wav\n";
        mf << "sound.highscore      = assets/sounds/highscore.wav\n";
        mf << "sound.knock_wood     = assets/sounds/knock_wood.wav\n";
        mf << "sound.knock_stone    = assets/sounds/knock_stone.wav\n";
        mf << "sound.knock_ice      = assets/sounds/knock_ice.wav\n";
        mf << "sound.oink           = assets/sounds/oink.wav\n";
        mf << "sound.bird_hurt      = assets/sounds/bird_hurt.wav\n\n";
        mf << "music.title          = assets/music/bgm_title.wav\n";
        mf << "music.battle         = assets/music/bgm_battle.wav\n";
        mf << "music.frost          = assets/music/bgm_frost.wav\n";
        mf << "music.volcano        = assets/music/bgm_volcano.wav\n";
        mf << "music.mystic         = assets/music/bgm_mystic.wav\n";
        mf << "music.heroic         = assets/music/bgm_heroic.wav\n";
    }
    mfCheck.close();

    writeSynthesizedWav("assets/sounds/launch.wav", 0, needUpgrade);
    writeSynthesizedWav("assets/sounds/ability_dash.wav", 1, needUpgrade);
    writeSynthesizedWav("assets/sounds/ability_split.wav", 2, needUpgrade);
    writeSynthesizedWav("assets/sounds/ability_slam.wav", 3, needUpgrade);
    writeSynthesizedWav("assets/sounds/explosion.wav", 4, needUpgrade);
    writeSynthesizedWav("assets/sounds/hit_twig.wav", 5, needUpgrade);
    writeSynthesizedWav("assets/sounds/hit_timber.wav", 6, needUpgrade);
    writeSynthesizedWav("assets/sounds/bird_defeat.wav", 7, needUpgrade);
    writeSynthesizedWav("assets/sounds/victory.wav", 8, needUpgrade);
    writeSynthesizedWav("assets/sounds/hit_stone.wav", 9, needUpgrade);
    writeSynthesizedWav("assets/sounds/hit_ice.wav", 12, needUpgrade);
    writeSynthesizedWav("assets/sounds/hit_metal.wav", 13, needUpgrade);
    writeSynthesizedWav("assets/sounds/boing.wav", 14, needUpgrade);
    writeSynthesizedWav("assets/sounds/wind.wav", 15, needUpgrade);
    writeSynthesizedWav("assets/sounds/thunder.wav", 16, needUpgrade);
    writeSynthesizedWav("assets/sounds/magma.wav", 17, needUpgrade);
    writeSynthesizedWav("assets/sounds/truffle.wav", 18, needUpgrade);
    writeSynthesizedWav("assets/sounds/achievement.wav", 19, needUpgrade);
    writeSynthesizedWav("assets/sounds/glide.wav", 22, needUpgrade);
    writeSynthesizedWav("assets/sounds/vortex.wav", 23, needUpgrade);
    writeSynthesizedWav("assets/sounds/drill.wav", 24, needUpgrade);
    writeSynthesizedWav("assets/sounds/shield.wav", 25, needUpgrade);
    writeSynthesizedWav("assets/sounds/zeppelin.wav", 26, needUpgrade);
    writeSynthesizedWav("assets/sounds/ui_click.wav", 10, needUpgrade);
    writeSynthesizedWav("assets/sounds/highscore.wav", 11, needUpgrade);
    writeSynthesizedWav("assets/music/bgm_title.wav", 20, needUpgrade);
    writeSynthesizedWav("assets/music/bgm_battle.wav", 21, needUpgrade);

    auto saveSpriteBmp = [&](const std::string& path, int w, int h, auto drawFn) {
        if (!needUpgrade) {
            std::ifstream f(path, std::ios::binary);
            if (f.good()) return;
        }
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
        if (!surf) return;
        uint32_t* px = static_cast<uint32_t*>(surf->pixels);
        int pitchPx = surf->pitch / 4;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                px[y * pitchPx + x] = drawFn(x, y, w, h);
            }
        }
        SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "wb");
        if (rw) SDL_SaveBMP_RW(surf, rw, 1);
        SDL_FreeSurface(surf);
    };

    auto packARGB = [](uint8_t a, uint8_t r, uint8_t g, uint8_t b) -> uint32_t {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               static_cast<uint32_t>(b);
    };

    // Rich v2 Wicker/Reed texture with bamboo segments and diagonal bindings
    saveSpriteBmp("assets/sprites/mat_twig.bmp", 64, 64, [&](int x, int y, int w, int h) {
        if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 88, 62, 24);
        bool topLeftBevel = (x < 5 || y < 5);
        int cane = (y / 8);
        int caneOffset = (cane % 2) * 16;
        int nodeDist = std::abs(((x + caneOffset) % 32) - 16);
        float tubeShade = std::cos(((y % 8) - 3.5f) * 0.42f);
        int baseR = static_cast<int>(195 + 38 * tubeShade + (topLeftBevel ? 18 : 0));
        int baseG = static_cast<int>(156 + 34 * tubeShade + (topLeftBevel ? 15 : 0));
        int baseB = static_cast<int>(78  + 22 * tubeShade);
        if (nodeDist < 2) { baseR -= 42; baseG -= 36; baseB -= 24; }
        if (y % 8 == 0)   { baseR -= 35; baseG -= 30; baseB -= 20; }
        return packARGB(255,
                        static_cast<uint8_t>(clampf(baseR, 0, 255)),
                        static_cast<uint8_t>(clampf(baseG, 0, 255)),
                        static_cast<uint8_t>(clampf(baseB, 0, 255)));
    });

    // Rich v2 Seasoned Timber texture with wood grain, bevels, and iron corner brackets
    saveSpriteBmp("assets/sprites/mat_timber.bmp", 64, 64, [&](int x, int y, int w, int h) {
        if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 52, 28, 14);
        // Iron corner straps & brass rivets
        bool inCorner = (x < 10 || x >= w - 10) && (y < 10 || y >= h - 10);
        if (inCorner) {
            bool rivet = ((x == 5 || x == w - 6) && (y == 5 || y == h - 6));
            if (rivet) return packARGB(255, 228, 188, 96);
            return packARGB(255, 68, 72, 82);
        }
        float grain = std::sin(y * 0.52f + std::sin(x * 0.14f) * 1.8f) * 12.0f +
                      std::cos(y * 1.3f - x * 0.06f) * 5.0f;
        int bevel = (y < 6 || x < 6) ? 18 : ((y > h - 6 || x > w - 6) ? -18 : 0);
        int r = static_cast<int>(clampf(158.0f + grain + bevel, 0.0f, 255.0f));
        int g = static_cast<int>(clampf(94.0f  + grain * 0.6f + bevel * 0.7f, 0.0f, 255.0f));
        int b = static_cast<int>(clampf(52.0f  + grain * 0.35f + bevel * 0.5f, 0.0f, 255.0f));
        return packARGB(255, r, g, b);
    });

    // Rich v2 Carved Slate / Basalt texture with 3D beveled ashlar blocks & runes
    saveSpriteBmp("assets/sprites/mat_stone.bmp", 64, 64, [&](int x, int y, int w, int h) {
        if (x < 3 || x >= w - 3 || y < 3 || y >= h - 3) return packARGB(255, 36, 42, 52);
        int bevel = (x < 8 || y < 8) ? 24 : ((x >= w - 8 || y >= h - 8) ? -22 : 0);
        bool runeGroove = ((x == 16 || x == w - 17) && y >= 14 && y <= h - 15) ||
                          ((y == 16 || y == h - 17) && x >= 14 && x <= w - 15);
        if (runeGroove) bevel = -28;
        int noise = (((x * 73856093) ^ (y * 19349663)) & 0xF) - 8;
        int base = static_cast<int>(clampf(108 + bevel + noise, 30, 220));
        return packARGB(255,
                        static_cast<uint8_t>(base - 8),
                        static_cast<uint8_t>(base),
                        static_cast<uint8_t>( std::min(255, base + 16)));
    });

    // Glacial Frost Ice texture with crystalline refraction & diagonal cleavage veins
    saveSpriteBmp("assets/sprites/mat_ice.bmp", 64, 64, [&](int x, int y, int w, int h) {
        if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 110, 185, 225);
        bool topBevel = (x < 5 || y < 5);
        int diag = (x + y) % 24;
        int crossDiag = std::abs((x - y) % 32 - 16);
        float vein = (diag == 0 || diag == 1) ? 35.0f : ((crossDiag < 2) ? 25.0f : 0.0f);
        int baseR = static_cast<int>(clampf(165.0f + vein + (topBevel ? 45.0f : 0.0f), 0.0f, 255.0f));
        int baseG = static_cast<int>(clampf(225.0f + vein * 0.8f + (topBevel ? 20.0f : 0.0f), 0.0f, 255.0f));
        int baseB = static_cast<int>(clampf(250.0f + vein * 0.4f, 0.0f, 255.0f));
        return packARGB(255, static_cast<uint8_t>(baseR), static_cast<uint8_t>(baseG), static_cast<uint8_t>(baseB));
    });

    // Volatile Gunpowder Keg / Crate texture with iron reinforcing bands and bold TNT stenciling
    saveSpriteBmp("assets/sprites/mat_tnt.bmp", 64, 64, [&](int x, int y, int w, int h) {
        if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 42, 18, 16);
        bool inBand = (y >= 8 && y <= 16) || (y >= h - 17 && y <= h - 9);
        if (inBand) {
            bool rivet = ((x == 6 || x == w - 7 || x == w / 2) && (y == 12 || y == h - 13));
            if (rivet) return packARGB(255, 235, 195, 95);
            return packARGB(255, 52, 56, 64);
        }
        int plank = (y / 16);
        int baseR = 192, baseG = 45, baseB = 38;
        if (plank % 2 == 1) { baseR -= 18; baseG -= 8; baseB -= 6; }
        if (y % 16 == 0)   { baseR -= 36; baseG -= 14; baseB -= 10; }
        bool isT1 = (y >= 26 && y <= 38 && x >= 14 && x <= 22) && (y <= 29 || (x >= 17 && x <= 19));
        bool isN  = (y >= 26 && y <= 38 && x >= 26 && x <= 37) && (x <= 28 || x >= 35 || std::abs((x - 26) - (y - 26)) <= 1);
        bool isT2 = (y >= 26 && y <= 38 && x >= 41 && x <= 49) && (y <= 29 || (x >= 44 && x <= 46));
        if (isT1 || isN || isT2) {
            return packARGB(255, 255, 230, 95);
        }
        return packARGB(255, static_cast<uint8_t>(baseR), static_cast<uint8_t>(baseG), static_cast<uint8_t>(baseB));
    });

    // High-resolution 192x192 faceted origami base sprites with radial paper-fold shading (v3)
    auto makeHiResOrigamiSprite = [&](uint8_t br, uint8_t bg, uint8_t bb,
                                      uint8_t sr, uint8_t sg, uint8_t sb, bool isBird) {
        return [=](int x, int y, int w, int h) -> uint32_t {
            float nx = (x - w * 0.5f) / (w * 0.44f);
            float ny = (y - h * 0.5f) / (h * 0.44f);
            float dodec = std::max({std::abs(nx), std::abs(ny),
                                    (std::abs(nx) + std::abs(ny)) * 0.71f,
                                    (std::abs(nx) * 0.866f + std::abs(ny) * 0.5f),
                                    (std::abs(nx) * 0.5f + std::abs(ny) * 0.866f)});
            if (dodec > 1.0f) return packARGB(0, 0, 0, 0);
            if (dodec > 0.91f) return packARGB(255, 24, 20, 22);
            // Multi-facet angular paper-fold shading + subtle paper grain
            float ang = std::atan2(ny, nx);
            int sector = static_cast<int>((ang + PI) / (PI / 4.0f)) % 8;
            float sectorShade = (sector == 0 || sector == 1 || sector == 7) ? 1.08f :
                                ((sector == 3 || sector == 4) ? 0.84f : 0.96f);
            float innerBev = (dodec < 0.62f) ? 1.05f : 0.97f;
            int grain = (((x * 374761393) ^ (y * 668265263)) & 0x7) - 3;
            float mult = sectorShade * innerBev;
            uint8_t r = static_cast<uint8_t>(clampf(br * mult + grain, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(clampf(bg * mult + grain, 0.0f, 255.0f));
            uint8_t b = static_cast<uint8_t>(clampf(bb * mult + grain, 0.0f, 255.0f));
            if (!isBird && std::abs(nx - 0.22f) < 0.34f && std::abs(ny - 0.10f) < 0.26f) {
                return packARGB(255, sr, sg, sb);
            }
            if (isBird && ny > -0.15f && ny < 0.30f && std::abs(nx) < (0.30f - ny) * 0.8f) {
                return packARGB(255, sr, sg, sb);
            }
            return packARGB(255, r, g, b);
        };
    };

    saveSpriteBmp("assets/sprites/pig_trotter.bmp", 192, 192,
                  makeHiResOrigamiSprite(209, 102, 78, 238, 155, 130, false));
    saveSpriteBmp("assets/sprites/pig_turbo.bmp", 192, 192,
                  makeHiResOrigamiSprite(230, 168, 52, 250, 208, 112, false));
    saveSpriteBmp("assets/sprites/pig_split.bmp", 192, 192,
                  makeHiResOrigamiSprite(76, 168, 135, 142, 219, 190, false));
    saveSpriteBmp("assets/sprites/pig_bomb.bmp", 192, 192,
                  makeHiResOrigamiSprite(68, 74, 86, 214, 94, 48, false));
    saveSpriteBmp("assets/sprites/pig_glider.bmp", 192, 192,
                  makeHiResOrigamiSprite(82, 168, 238, 175, 225, 255, false));
    saveSpriteBmp("assets/sprites/pig_graviton.bmp", 192, 192,
                  makeHiResOrigamiSprite(110, 52, 185, 210, 165, 255, false));
    saveSpriteBmp("assets/sprites/pig_drill.bmp", 192, 192,
                  makeHiResOrigamiSprite(175, 102, 32, 245, 195, 120, false));

    saveSpriteBmp("assets/sprites/bird_toucan.bmp", 192, 192,
                  makeHiResOrigamiSprite(38, 122, 138, 242, 132, 62, true));
    saveSpriteBmp("assets/sprites/bird_owl.bmp", 192, 192,
                  makeHiResOrigamiSprite(78, 82, 126, 218, 176, 78, true));
    saveSpriteBmp("assets/sprites/bird_heron.bmp", 192, 192,
                  makeHiResOrigamiSprite(228, 222, 206, 246, 194, 52, true));
    saveSpriteBmp("assets/sprites/bird_falcon.bmp", 192, 192,
                  makeHiResOrigamiSprite(48, 68, 88, 230, 175, 45, true));
    saveSpriteBmp("assets/sprites/bird_condor.bmp", 192, 192,
                  makeHiResOrigamiSprite(34, 34, 40, 240, 185, 50, true));

    // Bouncy Trampoline Deflector texture with dark steel rim, brass corner studs, and emerald woven spring mat
    saveSpriteBmp("assets/sprites/mat_trampoline.bmp", 64, 64, [&](int x, int y, int w, int h) {
        if (x < 3 || x >= w - 3 || y < 3 || y >= h - 3) return packARGB(255, 32, 38, 48); // steel border
        // Brass corner studs
        bool corner = (x < 8 || x >= w - 8) && (y < 8 || y >= h - 8);
        if (corner) {
            bool stud = (x >= 4 && x <= 6 && y >= 4 && y <= 6) ||
                        (x >= w - 7 && x <= w - 5 && y >= 4 && y <= 6) ||
                        (x >= 4 && x <= 6 && y >= h - 7 && y <= h - 5) ||
                        (x >= w - 7 && x <= w - 5 && y >= h - 7 && y <= h - 5);
            if (stud) return packARGB(255, 235, 195, 80);
            return packARGB(255, 55, 62, 74);
        }
        // Woven emerald canvas lattice & spring chevrons
        int weave = ((x / 4) + (y / 4)) % 2;
        int r = (weave == 0) ? 36 : 28;
        int g = (weave == 0) ? 148 : 122;
        int b = (weave == 0) ? 84 : 68;
        // Golden center spring chevron
        int cx = w / 2, cy = h / 2;
        int dy = y - cy;
        int chevronX = std::abs(x - cx);
        if (std::abs(dy + 6 - chevronX / 2) <= 1 || std::abs(dy - 6 - chevronX / 2) <= 1) {
            return packARGB(255, 245, 215, 85);
        }
        return packARGB(255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    });

    // Cauldron Helmet Icon texture (iron kettle with flared brim, rivets, and top handle)
    saveSpriteBmp("assets/sprites/metal_helm.bmp", 64, 64, [&](int x, int y, int w, int h) {
        float nx = (x - w * 0.5f) / (w * 0.42f);
        float ny = (y - h * 0.52f) / (h * 0.40f);
        // Top brass bail handle
        if (y >= 8 && y <= 16 && std::abs(x - w / 2) <= 10) {
            if (y <= 11 || std::abs(x - w / 2) >= 8) return packARGB(255, 215, 175, 75);
        }
        // Helmet dome
        if (ny < 0.0f) {
            float rDome = std::sqrt(nx * nx + ny * ny);
            if (rDome > 1.0f) return packARGB(0, 0, 0, 0);
            float shade = 0.75f + 0.45f * (1.0f - nx * 0.7f - ny * 0.5f);
            int base = static_cast<int>(clampf(110.0f * shade, 0.0f, 255.0f));
            // Central ridge line
            if (std::abs(nx) < 0.08f) base += 35;
            return packARGB(255, static_cast<uint8_t>(std::max(0, base - 5)), static_cast<uint8_t>(base), static_cast<uint8_t>(std::min(255, base + 15)));
        } else if (ny <= 0.55f && std::abs(nx) <= 1.08f) {
            // Lower cylinder & flared brim
            float shade = 0.8f + 0.35f * (1.0f - nx * 0.6f);
            int base = static_cast<int>(clampf(95.0f * shade, 0.0f, 255.0f));
            // Rivet row
            if (y >= h / 2 && y <= h / 2 + 3 && (x % 10 == 0)) {
                return packARGB(255, 235, 195, 85);
            }
            return packARGB(255, static_cast<uint8_t>(std::max(0, base - 5)), static_cast<uint8_t>(base), static_cast<uint8_t>(std::min(255, base + 12)));
        } else if (ny > 0.55f && ny <= 0.85f && std::abs(nx) <= 1.25f) {
            // Flared brim
            return packARGB(255, 65, 72, 85);
        }
        return packARGB(0, 0, 0, 0);
    });

    // Volcanic Magma Vent texture: Obsidian basalt with jagged glowing molten lava veins
    saveSpriteBmp("assets/sprites/mat_magma.bmp", 64, 64, [&](int x, int y, int w, int h) {
        if (x < 2 || x >= w - 2 || y < 2 || y >= h - 2) return packARGB(255, 30, 20, 24);
        // Lava vent aperture at top
        if (y <= 10 && x >= 18 && x <= w - 19) {
            float distTop = std::abs(x - w / 2) / 14.0f;
            if (distTop < 1.0f) {
                int r = 255;
                int g = static_cast<int>(180 + 75 * (1.0f - distTop));
                int b = static_cast<int>(30 + 50 * (1.0f - distTop));
                return packARGB(255, r, static_cast<uint8_t>(clampf(g, 0, 255)), static_cast<uint8_t>(clampf(b, 0, 255)));
            }
        }
        // Molten fissures branching down
        float fissure1 = std::abs((x - 32) - std::sin(y * 0.22f) * 10.0f);
        float fissure2 = std::abs((x - 16) - std::cos(y * 0.18f) * 8.0f);
        float fissure3 = std::abs((x - 48) + std::sin(y * 0.19f) * 9.0f);
        float minFissure = std::min({fissure1, fissure2, fissure3});
        if (minFissure < 2.0f) {
            return packARGB(255, 255, static_cast<uint8_t>(200 - minFissure * 40), 40);
        } else if (minFissure < 4.5f) {
            return packARGB(255, 220, 75, 20);
        }
        // Dark basalt rock texture with noise and bevel
        int bevel = (x < 6 || y < 6) ? 14 : ((x >= w - 6 || y >= h - 6) ? -16 : 0);
        int noise = (((x * 614689) ^ (y * 982451)) & 0xF) - 8;
        int baseR = static_cast<int>(clampf(42 + bevel + noise, 18, 90));
        int baseG = static_cast<int>(clampf(34 + bevel + noise, 14, 80));
        int baseB = static_cast<int>(clampf(38 + bevel + noise, 16, 85));
        return packARGB(255, static_cast<uint8_t>(baseR), static_cast<uint8_t>(baseG), static_cast<uint8_t>(baseB));
    });

    // Storm Cloud & Lightning Icon texture: Dark ominous storm cloud with electric lightning fork
    saveSpriteBmp("assets/sprites/weather_storm.bmp", 64, 64, [&](int x, int y, int w, int h) {
        float nx = (x - w * 0.5f) / (w * 0.44f);
        float ny = (y - h * 0.42f) / (h * 0.35f);
        // Cloud puffs (3 overlapping lobes)
        float c1 = std::sqrt((nx + 0.35f) * (nx + 0.35f) + (ny + 0.05f) * (ny + 0.05f));
        float c2 = std::sqrt(nx * nx + (ny - 0.15f) * (ny - 0.15f));
        float c3 = std::sqrt((nx - 0.38f) * (nx - 0.38f) + (ny + 0.08f) * (ny + 0.08f));
        float cBase = std::sqrt(nx * nx * 0.8f + (ny - 0.35f) * (ny - 0.35f) * 2.5f);
        bool inCloud = (c1 < 0.48f || c2 < 0.62f || c3 < 0.44f || (cBase < 0.85f && ny >= 0.0f && ny <= 0.55f));

        // Electric lightning fork
        bool bolt1 = (y >= 26 && y <= 40 && std::abs((x - 30) - (y - 26) * 0.3f) <= 1.5f);
        bool bolt2 = (y >= 38 && y <= 58 && std::abs((x - 36) + (y - 38) * 0.4f) <= 1.5f);
        bool boltStud = (y >= 38 && y <= 41 && x >= 28 && x <= 38);
        if (bolt1 || bolt2 || boltStud) {
            return packARGB(255, 255, 248, 120); // bright electric yellow
        }
        if (inCloud) {
            float shade = 0.85f - (ny + 0.5f) * 0.4f;
            int base = static_cast<int>(clampf(70.0f * shade, 25.0f, 130.0f));
            return packARGB(255, static_cast<uint8_t>(base), static_cast<uint8_t>(base + 4), static_cast<uint8_t>(base + 14));
        }
        return packARGB(0, 0, 0, 0);
    });

    // Golden Truffle Icon texture: Radiant origami truffle with faceted lobes and golden ambient glints
    saveSpriteBmp("assets/sprites/item_truffle.bmp", 64, 64, [&](int x, int y, int w, int h) {
        float nx = (x - w * 0.5f) / (w * 0.44f);
        float ny = (y - h * 0.52f) / (h * 0.42f);
        float dist = std::sqrt(nx * nx + ny * ny);
        if (dist > 1.0f) return packARGB(0, 0, 0, 0);

        float angle = std::atan2(ny, nx);
        float sector = std::fmod(angle + PI * 2.0f, PI / 3.0f) / (PI / 3.0f);
        float shade = 0.75f + 0.35f * (1.0f - nx * 0.5f - ny * 0.6f);
        if (sector < 0.15f || sector > 0.85f) shade *= 0.80f; // origami crease
        int r = static_cast<int>(clampf(248.0f * shade + 12.0f, 0.0f, 255.0f));
        int g = static_cast<int>(clampf(198.0f * shade, 0.0f, 255.0f));
        int b = static_cast<int>(clampf(58.0f * shade, 0.0f, 255.0f));

        // Diamond sparkle star at top-left
        float gx = (x - 22) / 7.0f, gy = (y - 18) / 7.0f;
        float gDist = std::sqrt(gx * gx + gy * gy);
        if (gDist < 1.0f && (std::abs(gx) < 0.28f || std::abs(gy) < 0.28f)) {
            return packARGB(255, 255, 255, 235);
        }
        return packARGB(255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    });

    // Royal Trophy Cup Icon texture: Gleaming gold chalice with handles, pedestal, and star
    saveSpriteBmp("assets/sprites/ui_trophy.bmp", 64, 64, [&](int x, int y, int w, int h) {
        float nx = (x - w * 0.5f) / (w * 0.44f);
        float ny = (y - h * 0.50f) / (h * 0.44f);

        // 1. Pedestal base (mahogany plinth at bottom)
        if (y >= 52 && y <= 60 && std::abs(x - w / 2) <= 20) {
            int woodR = (y <= 54) ? 140 : 85;
            return packARGB(255, woodR, 45, 28);
        }
        // 2. Trophy stem & flared foot
        if (y >= 44 && y < 52 && std::abs(x - w / 2) <= (5 + (y - 44) * 1.4f)) {
            return packARGB(255, 225, 175, 55);
        }
        // 3. Side handles (curved loops)
        if (y >= 16 && y <= 36 && std::abs(nx) >= 0.55f && std::abs(nx) <= 0.95f) {
            float hy = (y - 26) / 10.0f;
            float hx = (std::abs(nx) - 0.75f) / 0.20f;
            if (hx * hx + hy * hy <= 1.0f && hx * hx + hy * hy >= 0.40f) {
                return packARGB(255, 215, 160, 45);
            }
        }
        // 4. Main golden cup bowl
        if (y >= 10 && y <= 44 && std::abs(nx) <= 0.65f) {
            float cupWidth = 0.65f * (0.6f + 0.4f * (1.0f - (y - 10) / 34.0f));
            if (std::abs(nx) <= cupWidth) {
                float shade = 0.8f + 0.35f * (1.0f - nx * 0.7f - ny * 0.3f);
                if (y <= 13) shade += 0.25f; // rim highlight
                if (std::abs(x - w / 2) <= 4 && std::abs(y - 26) <= 4) {
                    return packARGB(255, 255, 255, 220); // star emblem
                }
                int r = static_cast<int>(clampf(245.0f * shade, 0.0f, 255.0f));
                int g = static_cast<int>(clampf(190.0f * shade, 0.0f, 255.0f));
                int b = static_cast<int>(clampf(50.0f * shade, 0.0f, 255.0f));
                return packARGB(255, static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
            }
        }
        return packARGB(0, 0, 0, 0);
    });

    if (needUpgrade) {
        std::ofstream vf("assets/version.txt");
        vf << "v16\n";
    }
}

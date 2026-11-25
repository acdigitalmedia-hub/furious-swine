// Asset pipeline: manifest parsing, sprite/sound loading, and the
// 16-voice polyphonic software audio mixer.
#pragma once

#include "common.h"

// ============================================================================
// CUSTOM ASSET PIPELINE (Manifest + Custom BMP Sprites + Synthesized WAV Audio)
// ============================================================================
class AssetPipeline {
public:
    std::map<std::string, std::string> manifest;
    std::map<std::string, SDL_Texture*> textures;
    // Decoded audio, always converted to the mixer's native format
    // (44.1 kHz, mono, signed 16-bit) so user-supplied WAVs of any common
    // layout play at the correct pitch and speed.
    struct WavSound {
        std::vector<int16_t> pcm;
    };
    static constexpr int MIXER_RATE = 44100;

    // Converts an SDL-decoded WAV buffer (U8 / S16 / S32 / F32, any channel
    // count, any sample rate) to 44.1 kHz mono S16. Returns false on an
    // unsupported format.
    static bool convertToMixerFormat(const SDL_AudioSpec& spec, const uint8_t* buf, uint32_t len,
                                     std::vector<int16_t>& out);

    // Loads a WAV file from disk and converts it. Returns false on failure.
    static bool loadWavFile(const std::string& path, WavSound& ws);
    std::map<std::string, WavSound> sounds;
    std::map<std::string, WavSound> musicTracks;
    std::string currentMusicKey;
    size_t bgmCursor = 0;
    bool musicEnabled = true;
    bool musicMuted = false;
    bool sfxMuted = false;

    // 16-channel polyphonic sound effect mixer with dynamic voice stealing
    struct SfxVoice {
        const int16_t* samples = nullptr;
        size_t length = 0;
        size_t cursor = 0;
        float gain = 1.0f;
        int priority = 0;
        bool active = false;
    };
    static constexpr size_t MAX_SFX_VOICES = 16;
    std::array<SfxVoice, MAX_SFX_VOICES> sfxVoices{};
    std::mutex audioMutex;

    float currentBgmGain = 0.32f;
    float targetBgmGain = 0.32f;
    uint32_t lastHitSoundTicks = 0;

    uint32_t audioDevice = 0;
    bool audioEnabled = true;
    bool useSpriteTextures = true;
    int loadedSpriteCount = 0;
    int loadedSoundCount = 0;

    std::vector<int16_t> mixScratch;

    static void ensureDir(const std::string& path);

    static void audioMixerCallback(void* userdata, uint8_t* stream, int len);

    static int getSoundPriority(const std::string& key);

    void playMusic(const std::string& key);

    void playSound(const std::string& key, float gain = 1.0f);

    struct OnePoleLP {
        float a = 0.0f;
        float z = 0.0f;
        void setCutoff(float fc, float fs) {
            float x = 2.0f * PI * fc / fs;
            a = clampf(x / (x + 1.0f), 0.001f, 0.999f);
        }
        float process(float in) {
            z += a * (in - z);
            return z;
        }
    };

    struct OnePoleHP {
        float a = 0.0f, z = 0.0f;
        void setCutoff(float fc, float fs) {
            float x = 2.0f * PI * fc / fs;
            a = clampf(1.0f / (1.0f + x), 0.001f, 0.999f);
        }
        float process(float in) {
            float out = a * (in - z);
            z = in;
            return out;
        }
    };

    struct ResonantBP {
        float b0 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
        void setParams(float fc, float Q, float fs) {
            float w0 = 2.0f * PI * fc / fs;
            float alpha = std::sin(w0) / (2.0f * Q);
            float a0 = 1.0f + alpha;
            b0 = alpha / a0;
            b2 = -alpha / a0;
            a1 = (-2.0f * std::cos(w0)) / a0;
            a2 = (1.0f - alpha) / a0;
        }
        float process(float in) {
            float out = b0 * in + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    struct StudioReverb {
        struct Comb {
            std::vector<float> buf;
            size_t idx = 0;
            float feedback = 0.72f;
            float lp = 0.0f;
            float damp = 0.35f;
            void init(int len, float fb, float d) {
                buf.assign(len, 0.0f); idx = 0; feedback = fb; damp = d; lp = 0.0f;
            }
            float process(float in) {
                float out = buf[idx];
                lp = out * (1.0f - damp) + lp * damp;
                buf[idx] = in + lp * feedback;
                idx = (idx + 1) % buf.size();
                return out;
            }
        };
        struct AllPass {
            std::vector<float> buf;
            size_t idx = 0;
            float g = 0.5f;
            void init(int len, float gain) {
                buf.assign(len, 0.0f); idx = 0; g = gain;
            }
            float process(float in) {
                float bufOut = buf[idx];
                float out = -g * in + bufOut;
                buf[idx] = in + g * bufOut;
                idx = (idx + 1) % buf.size();
                return out;
            }
        };
        Comb combs[4];
        AllPass allpasses[2];
        void init(float sampleRate) {
            combs[0].init(static_cast<int>(sampleRate * 0.0297f), 0.74f, 0.32f);
            combs[1].init(static_cast<int>(sampleRate * 0.0371f), 0.72f, 0.34f);
            combs[2].init(static_cast<int>(sampleRate * 0.0413f), 0.70f, 0.36f);
            combs[3].init(static_cast<int>(sampleRate * 0.0437f), 0.68f, 0.38f);
            allpasses[0].init(static_cast<int>(sampleRate * 0.0051f), 0.5f);
            allpasses[1].init(static_cast<int>(sampleRate * 0.0017f), 0.5f);
        }
        float process(float in) {
            float combSum = combs[0].process(in) + combs[1].process(in) +
                            combs[2].process(in) + combs[3].process(in);
            float ap1 = allpasses[0].process(combSum * 0.25f);
            return allpasses[1].process(ap1);
        }
    };

    struct BiquadLP {
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
        void setParams(float fc, float Q, float fs) {
            float w0 = 2.0f * PI * fc / fs;
            float alpha = std::sin(w0) / (2.0f * Q);
            float a0 = 1.0f + alpha;
            b0 = ((1.0f - std::cos(w0)) / 2.0f) / a0;
            b1 = (1.0f - std::cos(w0)) / a0;
            b2 = b0;
            a1 = (-2.0f * std::cos(w0)) / a0;
            a2 = (1.0f - alpha) / a0;
        }
        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    static void renderUprightBass(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                 float duration, float vel);

    static void renderAcousticGuitar(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                                    float duration, float vel);

    static void renderPluck(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                            float duration, float vel, uint32_t& /*rng*/, float /*decay*/ = 0.994f, float /*damp*/ = 0.5f, float /*brightness*/ = 0.7f);

    static void renderMarimba(std::vector<float>& dest, size_t startSample, float freq, float sampleRate,
                              float duration, float vel, uint32_t& rng);

    static void renderRhodesChord(std::vector<float>& dest, size_t startSample, const std::vector<float>& freqs,
                                  float sampleRate, float duration, float vel);

    static void renderCajon(std::vector<float>& dest, size_t startSample, float sampleRate, float vel, bool isBass);

    static void renderShaker(std::vector<float>& dest, size_t startSample, float sampleRate, float vel, uint32_t& rng, ResonantBP& bpShaker);

    static void writeSynthesizedWav(const std::string& filename, int type, bool forceOverwrite);

    void bootstrapDefaultAssets();

    bool loadAll(SDL_Renderer* ren);

    SDL_Texture* getTexture(const std::string& key) const;

    void shutdown();
};

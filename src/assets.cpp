#include "assets.h"

bool AssetPipeline::convertToMixerFormat(const SDL_AudioSpec& spec, const uint8_t* buf, uint32_t len,
                                         std::vector<int16_t>& out) {
    out.clear();
    if (!buf || len == 0) return false;
    uint16_t fmt = spec.format ? spec.format : 0x8010; // fallback loader leaves format unset
    int channels = spec.channels ? spec.channels : 1;
    int rate = spec.freq > 0 ? spec.freq : MIXER_RATE;
    int bytesPerSample;
    switch (fmt) {
        case 0x0008: bytesPerSample = 1; break; // AUDIO_U8
        case 0x8010: bytesPerSample = 2; break; // AUDIO_S16LSB
        case 0x8020: bytesPerSample = 4; break; // AUDIO_S32LSB
        case 0x8120: bytesPerSample = 4; break; // AUDIO_F32LSB
        default: return false;
    }
    size_t frameBytes = static_cast<size_t>(bytesPerSample) * channels;
    size_t frames = len / frameBytes;
    if (frames == 0) return false;

    auto sampleAt = [&](size_t frame, int ch) -> float {
        const uint8_t* p = buf + frame * frameBytes + static_cast<size_t>(ch) * bytesPerSample;
        switch (fmt) {
            case 0x0008: return (static_cast<int>(p[0]) - 128) / 128.0f;
            case 0x8010: { int16_t v; std::memcpy(&v, p, 2); return v / 32768.0f; }
            case 0x8020: { int32_t v; std::memcpy(&v, p, 4); return static_cast<float>(v / 2147483648.0); }
            default:     { float v; std::memcpy(&v, p, 4); return std::isfinite(v) ? v : 0.0f; }
        }
    };
    auto monoAt = [&](size_t frame) -> float {
        float sum = 0.0f;
        for (int c = 0; c < channels; ++c) sum += sampleAt(frame, c);
        return sum / channels;
    };

    size_t outFrames = (rate == MIXER_RATE) ? frames
                     : static_cast<size_t>(static_cast<double>(frames) * MIXER_RATE / rate);
    if (outFrames == 0) return false;
    out.resize(outFrames);
    double step = static_cast<double>(rate) / MIXER_RATE;
    for (size_t i = 0; i < outFrames; ++i) {
        float v;
        if (rate == MIXER_RATE) {
            v = monoAt(i);
        } else {
            double srcPos = i * step;
            size_t i0 = static_cast<size_t>(srcPos);
            size_t i1 = std::min(i0 + 1, frames - 1);
            float frac = static_cast<float>(srcPos - i0);
            v = monoAt(i0) * (1.0f - frac) + monoAt(i1) * frac;
        }
        out[i] = static_cast<int16_t>(clampf(v, -1.0f, 1.0f) * 32767.0f);
    }
    return true;
}

bool AssetPipeline::loadWavFile(const std::string& path, WavSound& ws) {
    SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb");
    if (!rw) return false;
    SDL_AudioSpec spec{};
    uint8_t* raw = nullptr;
    uint32_t rawLen = 0;
    if (!SDL_LoadWAV_RW(rw, 1, &spec, &raw, &rawLen)) return false;
    bool ok = convertToMixerFormat(spec, raw, rawLen, ws.pcm);
    SDL_FreeWAV(raw);
    if (!ok) {
        std::cerr << "[ASSETS] Unsupported WAV format in " << path << " (format 0x"
                  << std::hex << spec.format << std::dec << ")\n";
    }
    return ok && ws.pcm.size() >= 2;
}

void AssetPipeline::ensureDir(const std::string& path) {
    mkdir(path.c_str(), 0755);
}

void AssetPipeline::audioMixerCallback(void* userdata, uint8_t* stream, int len) {
    auto* pipe = static_cast<AssetPipeline*>(userdata);
    if (!pipe) {
        std::memset(stream, 0, len);
        return;
    }
    int16_t* out = reinterpret_cast<int16_t*>(stream);
    int numSamples = len / sizeof(int16_t);

    if (!pipe->audioEnabled) {
        std::memset(stream, 0, len);
        return;
    }

    std::lock_guard<std::mutex> lock(pipe->audioMutex);

    const int16_t* bgmBuf = nullptr;
    size_t bgmTotal = 0;
    if (!pipe->musicMuted && pipe->musicEnabled && !pipe->currentMusicKey.empty()) {
        auto it = pipe->musicTracks.find(pipe->currentMusicKey);
        if (it != pipe->musicTracks.end() && it->second.pcm.size() >= 2) {
            bgmBuf = it->second.pcm.data();
            bgmTotal = it->second.pcm.size();
        }
    }

    // Auto-duck BGM smoothly when high-priority sounds (explosions, abilities, victory) are active
    bool heavySoundActive = false;
    if (!pipe->sfxMuted) {
        for (const auto& v : pipe->sfxVoices) {
            if (v.active && v.priority >= 3) {
                heavySoundActive = true;
                break;
            }
        }
    }
    pipe->targetBgmGain = heavySoundActive ? 0.22f : 0.32f;

    for (int i = 0; i < numSamples; ++i) {
        // Smooth continuous exponential ducking (zero clicks or pops)
        pipe->currentBgmGain += (pipe->targetBgmGain - pipe->currentBgmGain) * 0.003f;

        float s = 0.0f;
        if (bgmBuf && bgmTotal > 0) {
            s += bgmBuf[pipe->bgmCursor] * pipe->currentBgmGain;
            pipe->bgmCursor++;
            if (pipe->bgmCursor >= bgmTotal) {
                pipe->bgmCursor = 0;
            }
        }

        // Mix all 16 polyphonic voice channels simultaneously if SFX not muted
        if (!pipe->sfxMuted) {
            for (auto& v : pipe->sfxVoices) {
                if (v.active && v.samples && v.cursor < v.length) {
                    // Linear interpolation so voices can play slightly sharp/flat.
                    size_t i0 = static_cast<size_t>(v.cursor);
                    float frac = static_cast<float>(v.cursor - static_cast<double>(i0));
                    float a = v.samples[i0];
                    float b = (i0 + 1 < v.length) ? v.samples[i0 + 1] : 0.0f;
                    s += (a + (b - a) * frac) * v.gain;
                    v.cursor += v.rate;
                    if (v.cursor >= v.length) {
                        v.active = false;
                        v.samples = nullptr;
                    }
                }
            }
        }

        // Soft saturation limiter to avoid harsh digital clipping when multiple blocks break
        float scaled = s / 32768.0f;
        float limited = std::tanh(scaled * 1.05f) * 0.95f;
        out[i] = static_cast<int16_t>(clampf(limited * 32767.0f, -32760.0f, 32760.0f));
    }
}

int AssetPipeline::getSoundPriority(const std::string& key) {
    if (key == "sound.highscore" || key == "sound.achievement") return 5;
    if (key == "sound.victory" || key == "sound.truffle")       return 5;
    if (key == "sound.explosion" || key == "sound.thunder") return 4;
    if (key == "sound.slam" || key == "sound.dash" ||
        key == "sound.split" || key == "sound.launch" || key == "sound.boing" || key == "sound.magma" || key == "sound.glide") return 3;
    if (key == "sound.bird_defeat" || key == "sound.hit_metal" || key == "sound.wind") return 2;
    if (key == "sound.oink" || key == "sound.bird_hurt") return 2;
    if (key == "sound.hit_stone" || key == "sound.hit_timber" ||
        key == "sound.hit_twig" || key == "sound.hit_ice") return 1;
    if (key == "sound.knock_wood" || key == "sound.knock_stone" || key == "sound.knock_ice") return 0;
    return 0;
}

void AssetPipeline::playMusic(const std::string& key) {
    if (currentMusicKey == key) return;
    if (audioDevice != 0) SDL_LockAudioDevice(audioDevice);
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        currentMusicKey = key;
        bgmCursor = 0;
    }
    if (audioDevice != 0) SDL_UnlockAudioDevice(audioDevice);
}

void AssetPipeline::playSound(const std::string& key, float gain) {
    if (!audioEnabled || sfxMuted || audioDevice == 0) return;
    auto it = sounds.find(key);
    if (it == sounds.end() || it->second.pcm.size() < 2) return;

    uint32_t now = SDL_GetTicks();
    int prio = getSoundPriority(key);

    // De-bounce rapid repeats of the same debris sound to prevent clutter.
    if (prio <= 1) {
        uint32_t& last = lastPlayTicks[key];
        if (last != 0 && (now - last) < 40) return;
        last = now;
    }

    // Small random pitch/volume variation so repeated sounds don't sound
    // machine-gunned. Musical stings stay in tune.
    auto rand01 = [&]() {
        variationRng ^= variationRng << 13;
        variationRng ^= variationRng >> 17;
        variationRng ^= variationRng << 5;
        return (variationRng & 0xFFFFFF) / static_cast<float>(0x1000000);
    };
    bool musical = (key == "sound.victory" || key == "sound.achievement" || key == "sound.truffle" ||
                    key == "sound.highscore");
    float pitchSpread = musical ? 0.0f : (prio <= 2 ? 0.07f : 0.04f);
    double rate = 1.0 + (rand01() * 2.0f - 1.0f) * pitchSpread;
    float gainJitter = musical ? 1.0f : 0.9f + 0.2f * rand01();

    const int16_t* pcm = it->second.pcm.data();
    size_t len = it->second.pcm.size();

    if (audioDevice != 0) SDL_LockAudioDevice(audioDevice);
    {
        std::lock_guard<std::mutex> lock(audioMutex);

        // Find an idle voice channel, or steal lowest priority / furthest progressed channel
        int bestIdx = -1;
        int lowestPrio = 999;
        double maxCursor = 0.0;

        for (size_t i = 0; i < MAX_SFX_VOICES; ++i) {
            if (!sfxVoices[i].active) {
                bestIdx = static_cast<int>(i);
                break;
            }
            if (sfxVoices[i].priority < lowestPrio ||
                (sfxVoices[i].priority == lowestPrio && sfxVoices[i].cursor > maxCursor)) {
                lowestPrio = sfxVoices[i].priority;
                maxCursor = sfxVoices[i].cursor;
                bestIdx = static_cast<int>(i);
            }
        }

        if (bestIdx >= 0) {
            if (!sfxVoices[bestIdx].active || prio >= sfxVoices[bestIdx].priority) {
                sfxVoices[bestIdx].samples = pcm;
                sfxVoices[bestIdx].length = len;
                sfxVoices[bestIdx].cursor = 0.0;
                sfxVoices[bestIdx].rate = rate;
                sfxVoices[bestIdx].gain = gain * gainJitter * 0.90f;
                sfxVoices[bestIdx].priority = prio;
                sfxVoices[bestIdx].active = true;
            }
        }
    }
    if (audioDevice != 0) SDL_UnlockAudioDevice(audioDevice);
}

bool AssetPipeline::loadAll(SDL_Renderer* ren) {
    bootstrapDefaultAssets();
    manifest.clear();
    std::ifstream mf("assets/manifest.cfg");
    std::string line;
    while (std::getline(mf, line)) {
        auto hashPos = line.find('#');
        if (hashPos != std::string::npos) line = line.substr(0, hashPos);
        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        auto trim = [](std::string s) {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
            return s;
        };
        std::string key = trim(line.substr(0, eqPos));
        std::string val = trim(line.substr(eqPos + 1));
        if (!key.empty() && !val.empty()) manifest[key] = val;
    }

    for (auto& kv : textures) {
        if (kv.second) SDL_DestroyTexture(kv.second);
    }
    textures.clear();

    if (audioDevice != 0) SDL_LockAudioDevice(audioDevice);
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        for (auto& v : sfxVoices) {
            v.active = false;
            v.samples = nullptr;
            v.cursor = 0;
        }
        bgmCursor = 0;
        currentMusicKey.clear();
        sounds.clear();
        musicTracks.clear();
    }
    if (audioDevice != 0) SDL_UnlockAudioDevice(audioDevice);

    loadedSpriteCount = 0;
    loadedSoundCount = 0;

    for (const auto& kv : manifest) {
        if (kv.first.rfind("sprite.", 0) == 0 && ren) {
            SDL_RWops* rw = SDL_RWFromFile(kv.second.c_str(), "rb");
            if (rw) {
                SDL_Surface* surf = SDL_LoadBMP_RW(rw, 1);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
                    if (tex) {
                        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                        textures[kv.first] = tex;
                        loadedSpriteCount++;
                    }
                    SDL_FreeSurface(surf);
                }
            }
        } else if (kv.first.rfind("sound.", 0) == 0) {
            WavSound ws{};
            if (loadWavFile(kv.second, ws)) {
                sounds[kv.first] = std::move(ws);
                loadedSoundCount++;
            }
        } else if (kv.first.rfind("music.", 0) == 0) {
            WavSound ws{};
            if (loadWavFile(kv.second, ws)) {
                musicTracks[kv.first] = std::move(ws);
            }
        }
    }

    if (audioDevice == 0) {
        SDL_AudioSpec desired{};
        desired.freq = 44100;
        desired.format = 0x8010; // AUDIO_S16LSB
        desired.channels = 1;
        desired.samples = 1024;
        desired.callback = audioMixerCallback;
        desired.userdata = this;
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
        if (audioDevice != 0) {
            SDL_PauseAudioDevice(audioDevice, 0);
        }
    }
    return (loadedSpriteCount > 0 && loadedSoundCount > 0);
}

SDL_Texture* AssetPipeline::getTexture(const std::string& key) const {
    auto it = textures.find(key);
    return (it != textures.end()) ? it->second : nullptr;
}

void AssetPipeline::shutdown() {
    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, 1);
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
    for (auto& kv : textures) {
        if (kv.second) SDL_DestroyTexture(kv.second);
    }
    textures.clear();
    sounds.clear();
    musicTracks.clear();
}

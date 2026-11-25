#!/usr/bin/env python3
"""Sound designer for Snout & Siege: renders every sound effect to
assets/sounds/*.wav (44.1 kHz, mono, 16-bit).

Developer tool (needs numpy). The game only reads the WAV files.

    python3 tools/make_sounds.py            # render everything
    python3 tools/make_sounds.py boing hit_ice   # render a few

Each sound is built from a few physical-ish ingredients: modal resonators
(decaying sinusoids at a material's partials), filtered noise, pitch
sweeps, gentle saturation, and a short synthetic room reverb.
"""
import math
import os
import sys
import wave

import numpy as np

SR = 44100
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'assets', 'sounds')
rng = np.random.default_rng(1234)


# ----------------------------------------------------------------- basics

def t_axis(dur):
    return np.arange(int(dur * SR)) / SR


def silence(dur):
    return np.zeros(int(dur * SR))


def noise(dur):
    return rng.standard_normal(int(dur * SR))


def place(dst, src, at, gain=1.0):
    """Mix src into dst starting at `at` seconds (grows dst if needed)."""
    i = int(at * SR)
    end = i + len(src)
    if end > len(dst):
        dst = np.concatenate([dst, np.zeros(end - len(dst))])
    dst[i:end] += src * gain
    return dst


def env_exp(dur, decay, attack=0.002):
    t = t_axis(dur)
    a = np.clip(t / max(attack, 1e-5), 0, 1)
    return a * np.exp(-t / decay)


def env_adsr(dur, a, d, s, r):
    t = t_axis(dur)
    e = np.where(t < a, t / max(a, 1e-5),
                 np.where(t < a + d, 1 - (1 - s) * (t - a) / max(d, 1e-5), s))
    rel = np.clip((dur - t) / max(r, 1e-5), 0, 1)
    return e * rel


def osc(freq, dur, shape='sine', phase=0.0):
    """Oscillator; freq may be a scalar or a per-sample array (sweeps)."""
    n = int(dur * SR)
    f = np.broadcast_to(np.asarray(freq, dtype=float), (n,))
    ph = 2 * np.pi * np.cumsum(f) / SR + phase
    if shape == 'sine':
        return np.sin(ph)
    frac = (ph / (2 * np.pi)) % 1.0
    if shape == 'saw':
        return 2 * frac - 1
    if shape == 'square':
        return np.where(frac < 0.5, 1.0, -1.0)
    if shape == 'tri':
        return 4 * np.abs(frac - 0.5) - 1
    if shape == 'pulse25':
        return np.where(frac < 0.25, 1.0, -0.33)
    raise ValueError(shape)


def sweep(f0, f1, dur, curve='exp'):
    t = np.linspace(0, 1, int(dur * SR), endpoint=False)
    if curve == 'exp':
        return f0 * (f1 / f0) ** t
    return f0 + (f1 - f0) * t


def modal(partials, dur, base=1.0, rand_phase=True):
    """Sum of decaying sinusoids: [(freq, amp, decay_s), ...]."""
    t = t_axis(dur)
    out = np.zeros_like(t)
    for f, a, d in partials:
        ph = rng.uniform(0, 2 * np.pi) if rand_phase else 0.0
        out += a * np.sin(2 * np.pi * f * base * t + ph) * np.exp(-t / d)
    return out


# ----------------------------------------------------------------- filters

def svf(x, cutoff, q=0.707, mode='lp'):
    """TPT state-variable filter; cutoff may be an array (time-varying)."""
    n = len(x)
    fc = np.broadcast_to(np.asarray(cutoff, dtype=float), (n,))
    g = np.tan(np.pi * np.clip(fc, 10, SR * 0.45) / SR)
    k = 1.0 / q
    a1 = 1.0 / (1.0 + g * (g + k))
    a2 = g * a1
    a3 = g * a2
    ic1 = ic2 = 0.0
    out = np.empty(n)
    xs = x.tolist()
    a1l, a2l, a3l = a1.tolist(), a2.tolist(), a3.tolist()
    kl = k
    if mode == 'lp':
        for i in range(n):
            v3 = xs[i] - ic2
            v1 = a1l[i] * ic1 + a2l[i] * v3
            v2 = ic2 + a2l[i] * ic1 + a3l[i] * v3
            ic1 = 2 * v1 - ic1
            ic2 = 2 * v2 - ic2
            out[i] = v2
    elif mode == 'bp':
        for i in range(n):
            v3 = xs[i] - ic2
            v1 = a1l[i] * ic1 + a2l[i] * v3
            v2 = ic2 + a2l[i] * ic1 + a3l[i] * v3
            ic1 = 2 * v1 - ic1
            ic2 = 2 * v2 - ic2
            out[i] = v1 * kl  # unity gain at the centre
    elif mode == 'hp':
        for i in range(n):
            v3 = xs[i] - ic2
            v1 = a1l[i] * ic1 + a2l[i] * v3
            v2 = ic2 + a2l[i] * ic1 + a3l[i] * v3
            ic1 = 2 * v1 - ic1
            ic2 = 2 * v2 - ic2
            out[i] = xs[i] - kl * v1 - v2
    return out


def lp(x, fc, q=0.707):
    return svf(x, fc, q, 'lp')


def hp(x, fc, q=0.707):
    return svf(x, fc, q, 'hp')


def bp(x, fc, q=1.0):
    return svf(x, fc, q, 'bp')


def dc_block(x):
    return hp(x, 25)


def sat(x, drive=1.5):
    return np.tanh(x * drive) / np.tanh(drive)


def reverb(x, size=0.6, mix=0.18, damp=5000, predelay=0.012):
    """Convolution with a synthetic exponentially decaying noise tail."""
    n = int(size * SR)
    t = np.arange(n) / SR
    ir = rng.standard_normal(n) * np.exp(-t * 6.9 / size)
    ir = lp(ir, damp)
    ir[: int(predelay * SR)] = 0
    ir /= np.sqrt((ir ** 2).sum()) + 1e-9
    m = len(x) + n
    nfft = 1 << (m - 1).bit_length()
    wet = np.fft.irfft(np.fft.rfft(x, nfft) * np.fft.rfft(ir, nfft), nfft)[:m]
    dry = np.concatenate([x, np.zeros(n)])
    out = dry * (1 - mix) + wet * mix * 2.2
    # trim trailing near-silence
    thresh = np.abs(out).max() * 1e-3
    last = np.nonzero(np.abs(out) > thresh)[0]
    return out[: (last[-1] + 1 if len(last) else len(out))]


def grains(dur, rate, grain_fn, jitter=1.0, decay=None):
    """Scatter short grains (from grain_fn(i)) randomly over `dur` seconds."""
    out = silence(dur + 0.3)
    t = 0.0
    i = 0
    while t < dur:
        g = grain_fn(i)
        amp = 1.0 if decay is None else math.exp(-t / decay)
        out = place(out, g, t, amp * rng.uniform(0.5, 1.0))
        t += rng.exponential(1.0 / rate) * jitter
        i += 1
    return out


# ----------------------------------------------------------------- output

def finish(x, peak_db=-1.0, fade_ms=8, target_rms_db=None):
    x = dc_block(np.asarray(x, dtype=float))
    if target_rms_db is not None:
        rms = np.sqrt(np.mean(x ** 2)) + 1e-12
        x = x * (10 ** (target_rms_db / 20) / rms)
    pk = np.abs(x).max() + 1e-12
    lim = 10 ** (peak_db / 20)
    if pk > lim:
        # soft-knee limiting instead of a hard rescale keeps the punch
        x = np.tanh(x / lim * 0.9) / np.tanh(0.9) * lim if pk > lim * 1.6 else x * (lim / pk)
    f = int(fade_ms / 1000 * SR)
    if f > 0 and len(x) > 2 * f:
        x[-f:] *= np.linspace(1, 0, f)
    x[:4] *= np.linspace(0, 1, 4)
    return x


def write(name, x):
    pcm = np.clip(x * 32767, -32767, 32767).astype(np.int16)
    path = os.path.join(OUT, name + '.wav')
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    rms = 20 * np.log10(np.sqrt(np.mean(x ** 2)) + 1e-12)
    print(f'  {name:16s} {len(x) / SR:5.2f}s  rms {rms:6.1f} dB')


SOUNDS = {}


def sound(name):
    def wrap(fn):
        SOUNDS[name] = fn
        return fn
    return wrap


# ----------------------------------------------------------------- materials

def wood_knock(base=1.0, dur=0.35, bright=1.0):
    modes = [(190, 1.0, 0.07), (410, 0.7, 0.05), (705, 0.45, 0.035), (1130, 0.3, 0.02), (1760, 0.18, 0.012)]
    body = modal(modes, dur, base)
    click = bp(noise(0.03) * env_exp(0.03, 0.004), 2200 * bright, 0.8) * 0.6
    return place(body, click, 0)


def stone_thud(base=1.0, dur=0.45):
    t = t_axis(dur)
    thump = np.sin(2 * np.pi * np.cumsum(sweep(120 * base, 55 * base, dur)) / SR) * np.exp(-t / 0.08)
    grit = lp(noise(dur), 1800) * env_exp(dur, 0.05) * 0.5
    return thump + grit


def glass_tink(f, dur=0.5, decay=0.12):
    ratios = [(1.0, 1.0), (2.32, 0.5), (4.25, 0.3), (6.63, 0.15)]
    return modal([(f * r, a, decay / (1 + i * 0.6)) for i, (r, a) in enumerate(ratios)], dur)


def metal_ring(f, dur=0.9, decay=0.35):
    ratios = [(1.0, 1.0), (2.76, 0.6), (5.40, 0.35), (8.93, 0.2), (13.3, 0.1)]
    out = modal([(f * r, a, decay / (1 + i * 0.5)) for i, (r, a) in enumerate(ratios)], dur)
    out += modal([(f * 1.004, 0.5, decay)], dur)  # slight beating
    return out


# ----------------------------------------------------------------- impacts

@sound('hit_twig')
def hit_twig():
    # Wicker snapping: a few dry cracks and a light rattle.
    out = silence(0.4)
    for i in range(5):
        crack = bp(noise(0.03) * env_exp(0.03, 0.003 + 0.002 * i), rng.uniform(1600, 3200), 1.6)
        out = place(out, crack, i * rng.uniform(0.012, 0.03), 1.0 - i * 0.13)
    rattle = grains(0.22, 90, lambda i: wood_knock(rng.uniform(2.2, 3.4), 0.05), decay=0.08) * 0.25
    out = place(out, rattle, 0.02)
    out = place(out, wood_knock(1.7, 0.18) * 0.5, 0)
    return finish(reverb(lp(out, 7000), 0.35, 0.12), target_rms_db=-19)


@sound('hit_timber')
def hit_timber():
    # A beam cracking apart: woody knock, a splintering crack, debris.
    out = wood_knock(1.0, 0.45) * 1.0
    crack = bp(noise(0.12) * env_exp(0.12, 0.025), sweep(2600, 900, 0.12), 1.2)
    out = place(out, crack, 0.004, 0.9)
    debris = grains(0.3, 45, lambda i: wood_knock(rng.uniform(1.4, 2.4), 0.08), decay=0.12) * 0.35
    out = place(out, debris, 0.05)
    return finish(reverb(sat(lp(out, 6500), 1.3), 0.45, 0.14), target_rms_db=-17)


@sound('hit_stone')
def hit_stone():
    # Masonry crumbling: heavy thud then gravel trickling down.
    out = stone_thud(1.0, 0.5)
    crunch = lp(noise(0.08) * env_exp(0.08, 0.018), 3000) * 0.8
    out = place(out, crunch, 0)
    gravel = grains(0.45, 70, lambda i: lp(noise(0.02) * env_exp(0.02, 0.004), rng.uniform(1200, 3500)),
                    decay=0.18) * 0.55
    out = place(out, gravel, 0.03)
    return finish(reverb(sat(out, 1.4), 0.5, 0.16, damp=3500), target_rms_db=-16)


@sound('hit_ice')
def hit_ice():
    # Ice shattering: a crisp crack and a shower of glassy tinkles.
    crack = hp(noise(0.05) * env_exp(0.05, 0.006), 1500) * 0.7
    out = place(silence(0.6), crack, 0)
    out = place(out, glass_tink(1650, 0.4, 0.09), 0, 0.6)
    tinkles = grains(0.4, 45, lambda i: glass_tink(rng.uniform(2200, 4600), 0.25, 0.05), decay=0.15) * 0.35
    out = place(out, tinkles, 0.015)
    return finish(reverb(lp(out, 9500), 0.6, 0.2, damp=8000), target_rms_db=-19)


@sound('hit_metal')
def hit_metal():
    # Iron helmet clang.
    out = metal_ring(460, 0.9, 0.3) * 0.8
    out = place(out, bp(noise(0.02) * env_exp(0.02, 0.003), 3000, 0.9), 0, 0.6)
    out = place(out, stone_thud(1.6, 0.15) * 0.4, 0)
    return finish(reverb(lp(out, 8000), 0.6, 0.15), target_rms_db=-18)


@sound('knock_wood')
def knock_wood():
    return finish(reverb(wood_knock(1.15, 0.25, 0.8), 0.3, 0.1), target_rms_db=-22)


@sound('knock_stone')
def knock_stone():
    out = place(stone_thud(1.3, 0.25), lp(noise(0.03) * env_exp(0.03, 0.006), 2000), 0, 0.4)
    return finish(reverb(out, 0.35, 0.1, damp=3000), target_rms_db=-21)


@sound('knock_ice')
def knock_ice():
    out = place(glass_tink(2100, 0.3, 0.07) * 0.7, hp(noise(0.02) * env_exp(0.02, 0.003), 2500), 0, 0.3)
    return finish(reverb(lp(out, 9000), 0.4, 0.15, damp=8000), target_rms_db=-23)


# ----------------------------------------------------------------- pigs

@sound('launch')
def launch():
    # Elastic twang + whoosh as the pig leaves the sling.
    t = t_axis(0.5)
    twang_f = 95 * (1 + 0.25 * np.exp(-t / 0.05) * np.sin(2 * np.pi * 22 * t))
    twang = osc(twang_f, 0.5, 'tri') * np.exp(-t / 0.12)
    thwip = bp(noise(0.25) * env_exp(0.25, 0.06), sweep(700, 2400, 0.25), 2.5)
    whoosh = bp(noise(0.5) * env_adsr(0.5, 0.05, 0.1, 0.6, 0.3), sweep(500, 1400, 0.5), 0.9) * 0.6
    out = twang * 0.9
    out = place(out, thwip, 0.0, 0.8)
    out = place(out, whoosh, 0.03)
    return finish(reverb(out, 0.3, 0.1), target_rms_db=-17)


@sound('oink')
def oink():
    # Cartoon grunt: nasal pulse wave through two formants, pitch falls.
    dur = 0.3
    f = sweep(260, 170, dur) * (1 + 0.04 * np.sin(2 * np.pi * 30 * t_axis(dur)))
    src = osc(f, dur, 'pulse25')
    voc = bp(src, 650, 3.0) * 1.0 + bp(src, 1350, 4.0) * 0.6 + lp(src, 400) * 0.4
    e = env_adsr(dur, 0.012, 0.06, 0.7, 0.12)
    out = sat(voc * e, 1.6)
    return finish(reverb(out, 0.25, 0.08), target_rms_db=-19)


@sound('ability_dash')
def ability_dash():
    # Bramble's jet boost: ignition pop and a rising jet roar.
    dur = 0.7
    roar = bp(noise(dur), sweep(400, 2600, dur), 1.4) * env_adsr(dur, 0.03, 0.1, 0.8, 0.35)
    hum = osc(sweep(80, 220, dur), dur, 'saw')
    hum = lp(hum, 900) * env_adsr(dur, 0.02, 0.1, 0.6, 0.3) * 0.5
    pop = stone_thud(1.4, 0.2) * 0.8
    out = place(roar + hum, pop, 0)
    return finish(reverb(sat(out, 1.3), 0.4, 0.12), target_rms_db=-15)


@sound('ability_split')
def ability_split():
    # Clover splitting into three: three quick bubbly pops, rising.
    out = silence(0.5)
    for i, f in enumerate([520, 690, 900]):
        dur = 0.12
        pop = osc(sweep(f * 0.6, f * 1.6, dur), dur, 'sine') * env_exp(dur, 0.035)
        out = place(out, pop, i * 0.055, 0.9)
    sparkle = grains(0.3, 40, lambda i: osc(rng.uniform(3000, 5200), 0.05) * env_exp(0.05, 0.012), decay=0.15) * 0.15
    out = place(out, sparkle, 0.06)
    return finish(reverb(out, 0.4, 0.18), target_rms_db=-16)


@sound('ability_slam')
def ability_slam():
    # Trotter's anvil slam: falling whistle into a heavy clank and boom.
    whistle = osc(sweep(1400, 500, 0.22), 0.22, 'sine') * env_adsr(0.22, 0.03, 0.05, 0.8, 0.05) * 0.35
    out = place(silence(0.9), whistle, 0)
    clank = metal_ring(310, 0.7, 0.22) * 0.6
    boom = stone_thud(0.7, 0.6) * 1.0
    out = place(out, clank, 0.2)
    out = place(out, boom, 0.2)
    return finish(reverb(sat(out, 1.5), 0.6, 0.18), target_rms_db=-14)


@sound('explosion')
def explosion():
    # Big kaboom: sub thump, roaring noise body, crackling debris, tail.
    dur = 1.6
    t = t_axis(dur)
    sub = np.sin(2 * np.pi * np.cumsum(sweep(75, 28, dur)) / SR) * np.exp(-t / 0.35)
    body = lp(noise(dur), sweep(3200, 180, dur)) * env_exp(dur, 0.4, 0.004) * 1.4
    snap = lp(noise(0.06) * env_exp(0.06, 0.01), 6000) * 0.9
    crackle = grains(0.9, 60, lambda i: bp(noise(0.015) * env_exp(0.015, 0.003), rng.uniform(800, 2600), 1.2),
                     decay=0.35) * 0.4
    out = sub * 1.2 + body
    out = place(out, snap, 0)
    out = place(out, crackle, 0.03)
    return finish(reverb(sat(out, 2.0), 1.2, 0.22, damp=2500), target_rms_db=-12)


@sound('glide')
def glide():
    # Gale levelling out: a soft papery swoosh with flutter.
    dur = 0.8
    t = t_axis(dur)
    flutter = 0.7 + 0.3 * np.sin(2 * np.pi * 17 * t)
    air = bp(noise(dur), sweep(900, 1600, dur), 0.8) * env_adsr(dur, 0.08, 0.1, 0.7, 0.4) * flutter
    hum = osc(sweep(180, 240, dur), dur, 'tri') * env_adsr(dur, 0.1, 0.1, 0.4, 0.3) * 0.15
    return finish(reverb(air + hum, 0.4, 0.12), target_rms_db=-19)


@sound('vortex')
def vortex():
    # Atticus' graviton vortex: warbling descending swirl into a deep whum.
    dur = 1.3
    t = t_axis(dur)
    f = sweep(900, 140, dur) * (1 + 0.08 * np.sin(2 * np.pi * 7 * t))
    tone = (osc(f, dur, 'sine') + 0.4 * osc(f * 1.5, dur, 'sine')) * env_adsr(dur, 0.05, 0.1, 0.8, 0.4) * 0.5
    swirl = bp(noise(dur), 600 + 450 * np.sin(2 * np.pi * 3.2 * t), 4.0) * env_adsr(dur, 0.1, 0.1, 0.8, 0.5) * 0.6
    whum = np.sin(2 * np.pi * np.cumsum(sweep(70, 40, 0.6)) / SR) * env_exp(0.6, 0.2)
    out = place(tone + swirl, whum, 0.7, 0.9)
    return finish(reverb(out, 0.9, 0.25), target_rms_db=-15)


@sound('drill')
def drill():
    # Rusty's drill: motor whine plus a gritty boring grind.
    dur = 0.6
    t = t_axis(dur)
    motor_f = sweep(140, 230, dur) * (1 + 0.02 * np.sin(2 * np.pi * 40 * t))
    motor = lp(osc(motor_f, dur, 'saw') + 0.5 * osc(motor_f * 2.01, dur, 'square'), 2200)
    grind = bp(noise(dur), 1400, 1.5) * (0.6 + 0.4 * np.sin(2 * np.pi * 33 * t))
    out = (motor * 0.6 + grind * 0.5) * env_adsr(dur, 0.02, 0.05, 0.85, 0.15)
    return finish(reverb(sat(out, 1.6), 0.3, 0.08), target_rms_db=-17)


# ----------------------------------------------------------------- birds & world

@sound('bird_defeat')
def bird_defeat():
    # Indignant squawk then a feathery poof.
    dur = 0.32
    t = t_axis(dur)
    f = sweep(1150, 700, dur) * (1 + 0.06 * np.sin(2 * np.pi * 28 * t))
    src = osc(f, dur, 'saw')
    squawk = (bp(src, 1600, 3.0) + 0.7 * bp(src, 2700, 4.0) + 0.3 * lp(src, 900)) * env_adsr(dur, 0.01, 0.05, 0.7, 0.12)
    poof = lp(noise(0.35) * env_exp(0.35, 0.08), sweep(3500, 900, 0.35)) * 0.55
    pop = osc(sweep(300, 120, 0.08), 0.08) * env_exp(0.08, 0.02) * 0.7
    out = place(sat(squawk, 1.5) * 0.9, pop, 0.16)
    out = place(out, poof, 0.17)
    return finish(reverb(out, 0.4, 0.14), target_rms_db=-16)


@sound('bird_hurt')
def bird_hurt():
    # Short indignant "eep!" when a bird is hit but survives.
    dur = 0.16
    t = t_axis(dur)
    f = sweep(1500, 1900, dur * 0.4).tolist() + sweep(1900, 1200, dur * 0.6 + 1 / SR).tolist()
    f = np.array(f[: int(dur * SR)]) * (1 + 0.05 * np.sin(2 * np.pi * 35 * t))
    src = osc(f, dur, 'saw')
    eep = (bp(src, 2000, 3.0) + 0.5 * bp(src, 3100, 4.0) + 0.25 * lp(src, 1000)) * env_adsr(dur, 0.006, 0.03, 0.7, 0.06)
    return finish(reverb(sat(eep, 1.4), 0.3, 0.1), target_rms_db=-19)


@sound('boing')
def boing():
    # Trampoline: classic wobbly spring.
    dur = 0.75
    t = t_axis(dur)
    f = 210 * (1 + 0.35 * np.exp(-t / 0.25) * np.sin(2 * np.pi * 15 * t)) * (1 + 0.25 * t)
    tone = osc(f, dur, 'sine') + 0.35 * osc(f * 2, dur, 'sine') + 0.15 * osc(f * 3, dur, 'tri')
    out = tone * env_adsr(dur, 0.005, 0.1, 0.6, 0.45)
    thud = stone_thud(1.2, 0.15) * 0.5
    out = place(out, thud, 0)
    return finish(reverb(out, 0.35, 0.1), target_rms_db=-15)


@sound('shield')
def shield():
    # Falcon's tower shield deflecting a hit: dull clunk + metallic ring.
    out = metal_ring(620, 0.7, 0.18) * 0.5
    out = place(out, stone_thud(1.5, 0.2) * 0.8, 0)
    out = place(out, wood_knock(0.8, 0.2) * 0.5, 0)
    return finish(reverb(out, 0.45, 0.14), target_rms_db=-17)


@sound('zeppelin')
def zeppelin():
    # Condor's balloon: pop, then a comic deflating raspberry.
    pop = (lp(noise(0.05) * env_exp(0.05, 0.008), 5000) + osc(sweep(400, 90, 0.05), 0.05) * env_exp(0.05, 0.015))
    dur = 0.9
    t = t_axis(dur)
    f = sweep(190, 70, dur) * (1 + 0.1 * np.sin(2 * np.pi * 23 * t))
    rasp = lp(osc(f, dur, 'pulse25'), 1500) * env_adsr(dur, 0.03, 0.1, 0.8, 0.3) * 0.6
    hiss = bp(noise(dur), 2500, 0.8) * env_adsr(dur, 0.02, 0.1, 0.5, 0.4) * 0.25
    out = place(pop * 1.0, rasp + hiss, 0.04)
    return finish(reverb(out, 0.4, 0.12), target_rms_db=-15)


@sound('wind')
def wind():
    # A gust: wandering band-passed noise that swells and fades.
    dur = 2.2
    t = t_axis(dur)
    fc = 500 + 300 * np.sin(2 * np.pi * 0.7 * t) + 150 * np.sin(2 * np.pi * 1.9 * t + 1)
    g = bp(noise(dur), fc, 1.5) + 0.5 * lp(noise(dur), 300)
    swell = np.sin(np.pi * t / dur) ** 1.5 * (0.8 + 0.2 * np.sin(2 * np.pi * 1.3 * t))
    return finish(g * swell, target_rms_db=-22, fade_ms=40)


@sound('thunder')
def thunder():
    # Sharp crack, then a long rolling rumble.
    dur = 3.2
    t = t_axis(dur)
    crack = hp(noise(0.12) * env_exp(0.12, 0.03), 800) * 0.9
    roll = np.abs(lp(noise(dur), 3.0)) * 25 + 0.3
    rumble = lp(noise(dur), sweep(900, 120, dur)) * env_exp(dur, 0.9, 0.05) * roll * 1.3
    sub = lp(noise(dur), 60) * env_exp(dur, 1.2) * 6
    out = place(rumble + sub, crack, 0)
    return finish(reverb(sat(out, 1.4), 1.5, 0.25, damp=2000), target_rms_db=-13, fade_ms=200)


@sound('magma')
def magma():
    # Magma vent erupting: whoosh of fire over gloopy bubbles.
    dur = 1.1
    roar = lp(noise(dur), sweep(2500, 500, dur)) * env_adsr(dur, 0.04, 0.2, 0.6, 0.5)
    bubbles = grains(0.9, 18, lambda i: osc(sweep(rng.uniform(120, 220), rng.uniform(350, 600), 0.07), 0.07)
                     * env_exp(0.07, 0.025), decay=0.5) * 0.7
    out = place(roar * 0.8, bubbles, 0.05)
    out = place(out, stone_thud(0.8, 0.4) * 0.7, 0)
    return finish(reverb(sat(out, 1.4), 0.7, 0.18, damp=3000), target_rms_db=-15)


# ----------------------------------------------------------------- musical stings

NOTE = {n: 440 * 2 ** ((i - 9) / 12) for i, n in enumerate(['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'])}


def hz(name):
    """'C5' -> frequency."""
    n, o = name[:-1], int(name[-1])
    return NOTE[n] * 2 ** (o - 4)


def bell(f, dur=1.2, decay=0.5, bright=1.0):
    parts = [(1.0, 1.0, decay), (2.0, 0.35 * bright, decay * 0.6), (3.0, 0.15 * bright, decay * 0.4),
             (4.2, 0.08 * bright, decay * 0.25)]
    return modal([(f * r, a, d) for r, a, d in parts], dur) * env_adsr(dur, 0.003, 0.0, 1.0, 0.05)


def pluck(f, dur=0.6, decay=0.25):
    tone = osc(f, dur, 'tri') + 0.3 * osc(f * 2, dur, 'sine')
    return lp(tone, 4000) * env_exp(dur, decay, 0.003)


def brass(f, dur=0.6):
    e = env_adsr(dur, 0.03, 0.08, 0.75, 0.12)
    src = osc(f * (1 + 0.004 * np.sin(2 * np.pi * 5.5 * t_axis(dur))), dur, 'saw') + 0.5 * osc(f * 1.002, dur, 'saw')
    return lp(src, 600 + 2600 * e) * e * 0.5


@sound('truffle')
def truffle():
    # Golden truffle: sparkling rising bell arpeggio.
    out = silence(1.2)
    for i, n in enumerate(['C6', 'E6', 'G6', 'C7']):
        out = place(out, bell(hz(n), 0.9, 0.35), i * 0.07, 0.6)
    shimmer = grains(0.6, 50, lambda i: osc(rng.uniform(4000, 7000), 0.04) * env_exp(0.04, 0.01), decay=0.3) * 0.12
    out = place(out, shimmer, 0.1)
    return finish(reverb(out, 1.0, 0.28, damp=9000), target_rms_db=-17)


@sound('achievement')
def achievement():
    # Short brass fanfare: G-C-E then a held C major chord.
    out = silence(1.8)
    for i, n in enumerate(['G4', 'C5', 'E5']):
        out = place(out, brass(hz(n), 0.16), i * 0.12, 0.8)
    for n in ['C5', 'E5', 'G5', 'C4']:
        out = place(out, brass(hz(n), 0.9), 0.36, 0.45)
    out = place(out, bell(hz('C7'), 0.9, 0.3) * 0.15, 0.36)
    return finish(reverb(out, 1.1, 0.25), target_rms_db=-15)


@sound('victory')
def victory():
    # Level cleared: bouncy pluck melody resolving on a bright chord.
    out = silence(2.0)
    melody = [('C5', 0.0), ('E5', 0.11), ('G5', 0.22), ('C6', 0.33), ('A5', 0.5), ('B5', 0.61), ('C6', 0.72)]
    for n, at in melody:
        out = place(out, pluck(hz(n), 0.5, 0.2), at, 0.7)
    for n in ['C5', 'E5', 'G5']:
        out = place(out, brass(hz(n), 0.8), 0.72, 0.3)
        out = place(out, bell(hz(n) * 2, 1.0, 0.4), 0.72, 0.12)
    return finish(reverb(out, 1.0, 0.22), target_rms_db=-15)


@sound('highscore')
def highscore():
    out = silence(1.0)
    for i, n in enumerate(['E6', 'G6', 'B6', 'E7']):
        out = place(out, bell(hz(n), 0.6, 0.22), i * 0.05, 0.5)
    return finish(reverb(out, 0.8, 0.25, damp=9000), target_rms_db=-18)


@sound('ui_click')
def ui_click():
    dur = 0.06
    blip = osc(sweep(1300, 700, dur), dur) * env_exp(dur, 0.015, 0.001)
    tick = bp(noise(0.01) * env_exp(0.01, 0.002), 3500, 1.0) * 0.3
    return finish(place(blip, tick, 0), target_rms_db=-22, fade_ms=4)


# ----------------------------------------------------------------- main

def main():
    names = sys.argv[1:] or list(SOUNDS)
    os.makedirs(OUT, exist_ok=True)
    for n in names:
        if n not in SOUNDS:
            sys.exit(f'unknown sound {n}; have: {", ".join(SOUNDS)}')
        write(n, SOUNDS[n]())


if __name__ == '__main__':
    main()

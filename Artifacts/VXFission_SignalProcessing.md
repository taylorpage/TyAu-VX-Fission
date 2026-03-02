# VX-Fission Signal Processing Documentation

This document describes the complete signal processing chain and design decisions for the VX-Fission audio plugin.

## Signal Flow Overview

```
Input Signal (mono or stereo)
    │
    ▼
[Write to ring buffer]
    │
    ▼
[Haas Delay — Wet Bus]
  Positive knob → delay right channel by 0–50 ms
  Negative knob → delay left channel by 0–50 ms
    │
    ▼
[Chorus — on wet bus]
  15 ms base delay ± 5 ms LFO depth (0.8 Hz sine)
  25% blend into wet bus
    │
    ▼
[Reverb — on wet bus]
  Freeverb-style: 8 comb filters + 4 all-pass filters per channel
  3% blend into wet bus
    │
    ▼
[Master dry/wet blend — gain-compensated parallel]
  Dry held at full level; wet added on top; sum normalised by (1 + masterMix)
  masterMix = sqrt(busAmount), busAmount = abs(delayMs) / 50
    │
    ▼
[Light compression]
  Feed-forward peak compressor: 2:1 above -6 dBFS
  Attack 10 ms / Release 120 ms
  Depth scales with busAmount (silent at centre, full at max deflection)
    │
    ▼
Output Signal (stereo)
```

---

## Parameters

| Parameter  | Range       | Default | Description                            |
|------------|-------------|---------|----------------------------------------|
| Delay Time | −50…+50 ms  | 0 ms    | Signed: negative delays L, positive delays R |
| Bypass     | Boolean     | Off     | Full signal bypass                     |

All effects (Haas, chorus, reverb, dry/wet, compression) are derived algorithmically from the single **Delay Time** knob. No additional parameters are exposed.

---

## 1. Haas Delay (Stereo Widening)

**Purpose:** Create psychoacoustic stereo width by delaying one channel relative to the other.

**Implementation:**
- Two 50 ms ring buffers (one per channel), written every sample
- `delayTime > 0` → right channel is delayed, left is dry
- `delayTime < 0` → left channel is delayed, right is dry
- `delayTime ≈ 0` → pass-through (both channels dry)

**Smoothing:**
```
smoothingCoeff = 1 − exp(−1 / (sampleRate × 0.020))
smoothedDelay += smoothingCoeff × (targetDelay − smoothedDelay)
```
~20 ms time constant prevents read-head jumps and crackling when the knob moves.

**Why:**
- Sub-40 ms inter-channel delays create the Haas (precedence) effect — a strong, natural-sounding stereo image without pitch artifacts
- Writing both channels every sample avoids stale-data clicks when the knob crosses zero and the delayed channel switches

---

## 2. Chorus

**Purpose:** Add modulated pitch shimmer and thickness to the wet signal.

**Implementation:**
- LFO: 0.8 Hz sine wave, phase continuous
- Chorus delay: `15 ms + sin(LFO) × 5 ms` (10–20 ms range)
- Linear interpolation between adjacent samples for smooth, artifact-free pitch modulation
- Reads from the same ring buffers as the Haas delay (no extra memory)
- Blended into the wet bus at **25%**

```
chorusDelayMs    = 15.0 + sin(lfoPhase) × 5.0
chorusDelaySamps = chorusDelayMs × sampleRate / 1000   [fractional]

// Linear interpolation
cL = bufL[floor] + frac × (bufL[floor−1] − bufL[floor])
cR = bufR[floor] + frac × (bufR[floor−1] − bufR[floor])

wetL = wetL × 0.75 + cL × 0.25
wetR = wetR × 0.75 + cR × 0.25
```

**Why 15 ms base:**
- Chorus delays below ~10 ms risk flamming; above ~25 ms they read into the Haas delay zone
- 15 ms ± 5 ms sits squarely in the classic chorus sweet spot

**Why 25%:**
- Reduced from 40% to keep the chorus subtle — audible shimmer without pulling focus from the Haas image

---

## 3. Reverb (Freeverb-Style)

**Purpose:** Add spacious, ambient decay to the wet signal.

**Algorithm:** Freeverb (public domain, Jezar at Dreampoint)
- 8 parallel comb filters per channel (stereo spread via +23 sample offset on R)
- 4 series all-pass filters per channel (diffusion)
- Mono-summed input fed to both comb banks; stereo image comes from the different delay times

**Comb filter delay times (ms, sample-rate scaled from Freeverb's 44100 Hz constants):**

| # | Left (ms) | Right (ms) |
|---|-----------|------------|
| 1 | 25.31     | 25.83      |
| 2 | 26.94     | 27.46      |
| 3 | 28.96     | 29.48      |
| 4 | 30.75     | 31.27      |
| 5 | 32.24     | 32.76      |
| 6 | 33.81     | 34.33      |
| 7 | 35.31     | 35.83      |
| 8 | 36.66     | 37.18      |

**All-pass filter delay times (ms):** 12.61, 10.00, 7.73, 5.10 (same spread applied)

**Key parameters:**
- Feedback: `0.94` (~4–5 second decay tail)
- Damping: `0.15` (bright, airy character)
- All-pass feedback: `0.5` (standard Freeverb value)

**Blend into wet bus: 3%**

```
revOut = allPass(sum(combs(monoIn)) × 0.125)

wetL = wetL × 0.97 + revL × 0.03
wetR = wetR × 0.97 + revR × 0.03
```

**Why 3%:** A barely-there hint of space and glue. The long decay tail (feedback 0.94) means even 3% adds meaningful room without swamping the Haas or chorus. Tuned down from 10% → 6% → 3% through listening.

---

## 4. Master Dry/Wet Blend (Gain-Compensated Parallel)

**Purpose:** Introduce the processed signal while preserving the full weight of the dry signal throughout the knob range.

**Implementation:**
```
busAmount = abs(smoothedDelayMs) / 50.0    // 0.0 at centre, 1.0 at max
masterMix = sqrt(busAmount)                // convex curve: hits harder early

outL = (inL + masterMix × wetL) / (1.0 + masterMix)
outR = (inR + masterMix × wetR) / (1.0 + masterMix)
```

**Why parallel instead of crossfade:**
- The original crossfade model (`dry × (1−mix) + wet × mix`) faded the dry signal as the knob turned, causing a noticeable loss of weight and body — the track felt thinner as the effect increased
- The parallel model keeps the dry signal at full amplitude; wet is added on top
- Dividing by `(1 + masterMix)` is the automatic gain compensation: as the parallel wet increases, the sum is normalised back to unity — no level creep, no thinning

**Blend at key positions:**

| Knob position | busAmount | masterMix (sqrt) | Dry weight |
|---------------|-----------|------------------|------------|
| 0% (centre)   | 0.00      | 0%               | 100%       |
| 25%           | 0.25      | 50%              | 100%       |
| 50%           | 0.50      | 71%              | 100%       |
| 100% (full)   | 1.00      | 100%             | 50/50 at unity |

---

## 5. Light Compression

**Purpose:** Add gentle glue and dynamic control to the mixed output as the effect increases.

**Algorithm:** Feed-forward peak compressor with one-pole envelope follower.

```
peakIn = max(|outL|, |outR|)
coeff  = (peakIn > env) ? attackCoeff : releaseCoeff
env   += coeff × (peakIn − env)

// Gain computer: 2:1 above -6 dBFS (0.5 linear)
if env > 0.5:
    reduced    = 0.5 + (env − 0.5) × 0.5
    targetGain = reduced / env
else:
    targetGain = 1.0

// Depth scales with knob
gr   = 1.0 − busAmount × (1.0 − targetGain)
outL = outL × gr
outR = outR × gr
```

**Key parameters:**
- Threshold: `-6 dBFS` (0.5 linear)
- Ratio: `2:1`
- Attack: `10 ms`
- Release: `120 ms`
- Depth: scales with `busAmount` — zero compression at centre knob, full compression at max deflection

**Why:** The gain-compensated parallel blend naturally adds some energy when the wet signal reinforces the dry. The compressor catches any peaks, adds subtle glue, and keeps the plugin from ever feeling louder than the dry signal.

---

## Technical Specifications

| Property             | Value                                  |
|----------------------|----------------------------------------|
| Sample Rate          | Host-dependent (initialized per-instance) |
| Channel Config       | Mono-in → Stereo-out (1→2, also 2→2)  |
| Additional Latency   | 0 samples                              |
| Max Frame Count      | 1024                                   |
| Ring Buffer Size     | 50 ms × sampleRate + 1                 |
| Thread Safety        | Render-thread safe (no allocations in process loop) |

---

## Design Iterations

### Pre-chorus (original)
- Haas delay only, 100% wet crossfade at all knob positions
- No dry/wet blend

### v0.2 — Add chorus
- Chorus added as additive blend on top of existing output
- Chorus depth and wet amount both scaled with knob

### v0.3 — Wet bus model
- Restructured to a parallel dry/wet bus
- All effects (Haas + chorus) live on the wet bus at full strength
- Master mix controlled by knob via crossfade + sqrt curve

### v0.4 — Add reverb
- Freeverb-style reverb added to wet bus
- Initial blend: 55% → too dominant
- Reduced to 15% → still slightly thick
- Reduced to 10% → correct presence without swamping

### v0.4.1 — Reverb tuning
- Feedback raised from 0.90 → 0.94 for longer, lusher tail
- Blend held at 10%

### v0.5 — Blend model overhaul + compression
- Chorus blend reduced 40% → 25% (subtle shimmer, not prominent)
- Reverb blend reduced 10% → 6% → 3% (barely-there space)
- Dry/wet blend changed from crossfade to gain-compensated parallel model
  — dry signal no longer fades as effect increases; preserves track weight
- Light feed-forward peak compressor added post-blend (2:1, -6 dBFS, 10/120 ms)
  — depth scales with busAmount for zero impact at centre knob

---

## Future Enhancements

- Expose reverb decay and wet as separate parameters
- Add a second LFO voice (detuned) for richer stereo chorus
- Tempo-sync option for the Haas delay time
- Pre-delay control before the reverb stage
- High-pass filter on reverb input to reduce low-end muddiness

---

**Last Updated:** March 2026
**Version:** 0.5
**Author:** Taylor Page with Claude Sonnet 4.6

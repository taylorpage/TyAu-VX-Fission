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
  40% blend into wet bus
    │
    ▼
[Reverb — on wet bus]
  Freeverb-style: 8 comb filters + 4 all-pass filters per channel
  10% blend into wet bus
    │
    ▼
[Master dry/wet blend]
  sqrt(busAmount) curve, 0 at centre → 1.0 at full deflection
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

All effects (Haas, chorus, reverb, dry/wet) are derived algorithmically from the single **Delay Time** knob. No additional parameters are exposed.

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
- Blended into the wet bus at **40%**

```
chorusDelayMs    = 15.0 + sin(lfoPhase) × 5.0
chorusDelaySamps = chorusDelayMs × sampleRate / 1000   [fractional]

// Linear interpolation
cL = bufL[floor] + frac × (bufL[floor−1] − bufL[floor])
cR = bufR[floor] + frac × (bufR[floor−1] − bufR[floor])

wetL = wetL × 0.60 + cL × 0.40
wetR = wetR × 0.60 + cR × 0.40
```

**Why 15 ms base:**
- Chorus delays below ~10 ms risk flamming; above ~25 ms they read into the Haas delay zone
- 15 ms ± 5 ms sits squarely in the classic chorus sweet spot

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

**Blend into wet bus: 10%**

```
revOut = allPass(sum(combs(monoIn)) × 0.125)

wetL = wetL × 0.90 + revL × 0.10
wetR = wetR × 0.90 + revR × 0.10
```

**Why 10%:** The reverb tail is long and diffuse. A low blend level adds space and glue without swamping the Haas and chorus. The lush character comes from the high feedback (0.94), not the mix level.

---

## 4. Master Dry/Wet Blend (Bus Model)

**Purpose:** Gradually introduce the full processed signal (Haas + chorus + reverb) as the knob moves away from centre.

**Implementation:**
```
busAmount = abs(smoothedDelayMs) / 50.0    // 0.0 at centre, 1.0 at max
masterMix = sqrt(busAmount)                // convex curve: hits harder early

outL = inL + masterMix × (wetL − inL)
outR = inR + masterMix × (wetR − inR)
```

**Blend curve at key positions:**

| Knob position | busAmount | masterMix (sqrt) |
|---------------|-----------|------------------|
| 0% (centre)   | 0.00      | 0%               |
| 10%           | 0.10      | 32%              |
| 25%           | 0.25      | 50%              |
| 50%           | 0.50      | 71%              |
| 75%           | 0.75      | 87%              |
| 100% (full)   | 1.00      | 100%             |

**Why sqrt:**
- Linear blend felt imperceptible near centre
- Square-root curve gives an immediate sense of the effect even at small deflections, easing naturally toward full wet at the extremes

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
- Haas delay only, 100% wet at all knob positions
- No dry/wet blend

### v0.2 — Add chorus
- Chorus added as additive blend on top of existing output
- Chorus depth and wet amount both scaled with knob

### v0.3 — Wet bus model
- Restructured to a parallel dry/wet bus
- All effects (Haas + chorus) live on the wet bus at full strength
- Master mix controlled by knob via sqrt curve

### v0.4 — Add reverb
- Freeverb-style reverb added to wet bus
- Initial blend: 55% → too dominant
- Reduced to 15% → still slightly thick
- Reduced to 10% → correct presence without swamping

### v0.4.1 — Reverb tuning
- Feedback raised from 0.90 → 0.94 for longer, lusher tail
- Blend held at 10%

---

## Future Enhancements

- Expose reverb decay and wet as separate parameters
- Add a second LFO voice (detuned) for richer stereo chorus
- Tempo-sync option for the Haas delay time
- Pre-delay control before the reverb stage
- High-pass filter on reverb input to reduce low-end muddiness

---

**Last Updated:** March 2026
**Version:** 0.4.1
**Author:** Taylor Page with Claude Sonnet 4.6

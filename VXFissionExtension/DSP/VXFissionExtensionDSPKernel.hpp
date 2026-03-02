//
//  VXFissionExtensionDSPKernel.hpp
//  VXFissionExtension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <algorithm>
#import <cmath>
#import <vector>
#import <span>

#import "VXFissionExtensionParameterAddresses.h"

// ─── Reverb building blocks (Freeverb-style, public domain) ──────────────────

struct CombFilter {
    std::vector<float> buf;
    int   head     = 0;
    float feedback = 0.90f;
    float damp     = 0.15f;  // one-pole LP damping: 0 = bright, 1 = dark
    float store    = 0.0f;   // LP filter state

    void init(int delaySamples, float fb, float d) {
        buf.assign(delaySamples, 0.0f);
        head = 0; feedback = fb; damp = d; store = 0.0f;
    }
    float process(float in) {
        float out = buf[head];
        store     = out * (1.0f - damp) + store * damp;
        buf[head] = in + store * feedback;
        head      = (head + 1 < (int)buf.size()) ? head + 1 : 0;
        return out;
    }
};

struct AllPassFilter {
    std::vector<float> buf;
    int   head     = 0;
    float feedback = 0.5f;

    void init(int delaySamples) {
        buf.assign(delaySamples, 0.0f);
        head = 0;
    }
    float process(float in) {
        float out = buf[head];
        buf[head] = in + out * feedback;
        head      = (head + 1 < (int)buf.size()) ? head + 1 : 0;
        return out - in;
    }
};

// ─────────────────────────────────────────────────────────────────────────────

/*
 VXFissionExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.

 Stereo Haas delay: a single signed knob controls both channel and amount.
   delayTime < 0 → delay left channel by abs(delayTime) ms
   delayTime > 0 → delay right channel by abs(delayTime) ms
   delayTime = 0 → pass-through (no delay)
 */
class VXFissionExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
        // Allocate enough for 50 ms at the current sample rate, plus one extra
        // sample so a delay of exactly 50 ms never wraps onto itself.
        int maxDelaySamples = (int)(mSampleRate * 0.050) + 1;
        mDelayBufferL.assign(maxDelaySamples, 0.0f);
        mDelayBufferR.assign(maxDelaySamples, 0.0f);
        mWriteHead = 0;
        mSmoothedDelayTimeMs = 0.0f;
        // One-pole smoothing: ~20 ms time constant eliminates read-head jumps.
        mSmoothingCoeff = 1.0f - std::exp(-1.0f / (float)(inSampleRate * 0.020));
        // Chorus LFO: 0.8 Hz sine wave.
        mLFOPhase = 0.0f;
        mLFOPhaseIncrement = (2.0f * (float)M_PI * 0.8f) / (float)inSampleRate;
        // Compressor: gentle 2:1 ratio above -6 dBFS, depth scales with busAmount.
        mCompEnv          = 0.0f;
        mCompAttackCoeff  = 1.0f - std::exp(-1.0f / (float)(inSampleRate * 0.010)); // 10 ms
        mCompReleaseCoeff = 1.0f - std::exp(-1.0f / (float)(inSampleRate * 0.120)); // 120 ms
        // Reverb: Freeverb-style comb + all-pass filters, sample-rate scaled.
        // Delay times derived from Freeverb's tuned 44100 Hz constants (public domain).
        static const float kCombMs[8] = { 25.31f, 26.94f, 28.96f, 30.75f,
                                          32.24f, 33.81f, 35.31f, 36.66f };
        static const float kApMs[4]   = { 12.61f, 10.00f,  7.73f,  5.10f };
        int spread = (int)(0.521f * (float)inSampleRate / 1000.0f); // ~23 samples at 44100
        for (int i = 0; i < 8; ++i) {
            int dL = (int)(kCombMs[i] * (float)inSampleRate / 1000.0f);
            mCombL[i].init(dL,          0.94f, 0.15f);
            mCombR[i].init(dL + spread, 0.94f, 0.15f);
        }
        for (int i = 0; i < 4; ++i) {
            int dL = (int)(kApMs[i] * (float)inSampleRate / 1000.0f);
            mAllPassL[i].init(dL);
            mAllPassR[i].init(dL + spread);
        }
    }

    void deInitialize() {
        mDelayBufferL.clear();
        mDelayBufferR.clear();
    }

    // MARK: - Bypass
    bool isBypassed() {
        return mBypassed;
    }

    void setBypass(bool shouldBypass) {
        mBypassed = shouldBypass;
    }

    // MARK: - Parameter Getter / Setter
    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case VXFissionExtensionParameterAddress::delayTime:
                mDelayTimeMs = value;
                break;
            case VXFissionExtensionParameterAddress::bypass:
                mBypassed = (value >= 0.5f);
                break;
            default:
                break;
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        switch (address) {
            case VXFissionExtensionParameterAddress::delayTime:
                return (AUValue)mDelayTimeMs;
            case VXFissionExtensionParameterAddress::bypass:
                return (AUValue)(mBypassed ? 1.0f : 0.0f);
            default:
                return 0.f;
        }
    }

    // MARK: - Max Frames
    AUAudioFrameCount maximumFramesToRender() const {
        return mMaxFramesToRender;
    }

    void setMaximumFramesToRender(const AUAudioFrameCount &maxFrames) {
        mMaxFramesToRender = maxFrames;
    }

    // MARK: - Musical Context
    void setMusicalContextBlock(AUHostMusicalContextBlock contextBlock) {
        mMusicalContextBlock = contextBlock;
    }

    // MARK: - Internal Process
    void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers, AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {

        int numIn  = (int)inputBuffers.size();
        int numOut = (int)outputBuffers.size();

        if (numIn == 0 || numOut == 0) return;

        const int bufSize = (int)mDelayBufferL.size();
        if (bufSize == 0) return;

        if (mBypassed) {
            for (int ch = 0; ch < numOut; ++ch) {
                int srcCh = std::min(ch, numIn - 1);
                for (UInt32 f = 0; f < frameCount; ++f) {
                    outputBuffers[ch][f] = inputBuffers[srcCh][f];
                }
            }
            return;
        }

        for (UInt32 f = 0; f < frameCount; ++f) {
            // Smooth target → current one sample at a time.
            // This moves the read head gradually, avoiding discontinuities.
            mSmoothedDelayTimeMs += mSmoothingCoeff * (mDelayTimeMs - mSmoothedDelayTimeMs);

            // Input samples with mono upmix.
            float inL = inputBuffers[0][f];
            float inR = inputBuffers[std::min(1, numIn - 1)][f];

            // Always write both channels so the buffers are current for
            // whichever channel becomes the delayed one (avoids stale-data clicks
            // when the sign flips and the delayed channel switches).
            mDelayBufferL[mWriteHead] = inL;
            mDelayBufferR[mWriteHead] = inR;

            float absDelayMs = std::abs(mSmoothedDelayTimeMs);
            int delaySamples = std::min(
                (int)(absDelayMs * (float)mSampleRate / 1000.0f),
                bufSize - 1
            );

            int readHead = mWriteHead - delaySamples;
            if (readHead < 0) readHead += bufSize;

            // === Wet bus: full Haas delay ===
            float wetL, wetR;
            if (mSmoothedDelayTimeMs > 0.001f) {
                wetL = inL;
                wetR = mDelayBufferR[readHead];
            } else if (mSmoothedDelayTimeMs < -0.001f) {
                wetL = mDelayBufferL[readHead];
                wetR = inR;
            } else {
                wetL = inL;
                wetR = inR;
            }

            // === Chorus on the wet bus ===
            // LFO advances every sample for continuous phase.
            mLFOPhase += mLFOPhaseIncrement;
            if (mLFOPhase >= 2.0f * (float)M_PI) mLFOPhase -= 2.0f * (float)M_PI;

            // Bus send amount: 0 at centre, 1.0 at full deflection.
            float busAmount = absDelayMs / 50.0f;

            if (busAmount > 0.001f) {
                float lfo = std::sin(mLFOPhase);

                // Chorus: 15 ms base ± 5 ms depth (fixed within the wet bus).
                float chorusDelayMs    = 15.0f + lfo * 5.0f;
                float chorusDelaySampF = chorusDelayMs * (float)mSampleRate / 1000.0f;
                chorusDelaySampF       = std::max(1.0f, std::min(chorusDelaySampF, (float)(bufSize - 2)));

                int   d0  = (int)chorusDelaySampF;
                float frc = chorusDelaySampF - (float)d0;
                int   rh0 = mWriteHead - d0;
                if (rh0 < 0) rh0 += bufSize;
                int   rh1 = rh0 - 1;
                if (rh1 < 0) rh1 += bufSize;

                float cL = mDelayBufferL[rh0] + frc * (mDelayBufferL[rh1] - mDelayBufferL[rh0]);
                float cR = mDelayBufferR[rh0] + frc * (mDelayBufferR[rh1] - mDelayBufferR[rh0]);

                // Blend chorus into the wet bus at a fixed 25 % ratio.
                wetL = wetL * 0.75f + cL * 0.25f;
                wetR = wetR * 0.75f + cR * 0.25f;
            }

            // === Reverb on the wet bus ===
            if (busAmount > 0.001f) {
                // Classic Freeverb approach: mono-sum into comb bank, stereo spread
                // comes from the slightly different delay times in L vs R combs.
                float monoIn = (wetL + wetR) * 0.5f;
                float revL = 0.0f, revR = 0.0f;
                for (int i = 0; i < 8; ++i) {
                    revL += mCombL[i].process(monoIn);
                    revR += mCombR[i].process(monoIn);
                }
                revL *= 0.125f;  // scale by 1/8
                revR *= 0.125f;
                for (int i = 0; i < 4; ++i) {
                    revL = mAllPassL[i].process(revL);
                    revR = mAllPassR[i].process(revR);
                }
                // Blend reverb into the wet bus at 3%.
                wetL = wetL * 0.97f + revL * 0.03f;
                wetR = wetR * 0.97f + revR * 0.03f;
            }

            // === Master dry/wet blend (gain-compensated parallel) ===
            // Dry stays at full level; wet is added on top and the sum is
            // normalised by (1 + masterMix) to compensate for the added energy.
            // Preserves the weight of the dry signal across the full knob range.
            float masterMix = std::sqrt(busAmount);
            float outL = (inL + masterMix * wetL) / (1.0f + masterMix);
            float outR = (inR + masterMix * wetR) / (1.0f + masterMix);

            // === Light compression on mixed output ===
            // Feed-forward peak compressor: 2:1 above -6 dBFS, depth scales with busAmount.
            {
                float peakIn = std::max(std::abs(outL), std::abs(outR));
                float coeff  = (peakIn > mCompEnv) ? mCompAttackCoeff : mCompReleaseCoeff;
                mCompEnv    += coeff * (peakIn - mCompEnv);

                const float kThreshold = 0.5f;  // -6 dBFS
                float targetGain = 1.0f;
                if (mCompEnv > kThreshold) {
                    float reduced = kThreshold + (mCompEnv - kThreshold) * 0.5f; // 2:1
                    targetGain    = reduced / mCompEnv;
                }
                // Blend depth: no effect at centre knob, full at max deflection.
                float gr = 1.0f - busAmount * (1.0f - targetGain);
                outL *= gr;
                outR *= gr;
            }

            if (numOut > 0) outputBuffers[0][f] = outL;
            if (numOut > 1) outputBuffers[1][f] = outR;

            mWriteHead = (mWriteHead + 1 < bufSize) ? mWriteHead + 1 : 0;
        }
    }

    void handleOneEvent(AUEventSampleTime now, AURenderEvent const *event) {
        switch (event->head.eventType) {
            case AURenderEventParameter: {
                handleParameterEvent(now, event->parameter);
                break;
            }
            default:
                break;
        }
    }

    void handleParameterEvent(AUEventSampleTime now, AUParameterEvent const& parameterEvent) {
        setParameter(parameterEvent.parameterAddress, parameterEvent.value);
    }

    // MARK: Member Variables
    AUHostMusicalContextBlock mMusicalContextBlock;

    double mSampleRate           = 44100.0;
    float  mDelayTimeMs          = 0.0f;   // target: signed ms (<0=delay L, >0=delay R, 0=dry)
    float  mSmoothedDelayTimeMs  = 0.0f;   // one-pole smoothed value used by render thread
    float  mSmoothingCoeff       = 0.0f;   // computed in initialize()
    bool   mBypassed             = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;

    std::vector<float> mDelayBufferL;  // ring buffer — left channel
    std::vector<float> mDelayBufferR;  // ring buffer — right channel
    int mWriteHead = 0;

    float mLFOPhase          = 0.0f;  // current LFO phase (radians)
    float mLFOPhaseIncrement = 0.0f;  // per-sample phase step (set in initialize())

    float mCompEnv          = 0.0f;  // compressor envelope follower state
    float mCompAttackCoeff  = 0.0f;  // set in initialize()
    float mCompReleaseCoeff = 0.0f;  // set in initialize()

    CombFilter    mCombL[8];
    CombFilter    mCombR[8];
    AllPassFilter mAllPassL[4];
    AllPassFilter mAllPassR[4];
};

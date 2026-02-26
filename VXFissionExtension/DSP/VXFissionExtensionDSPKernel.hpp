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

            float outL, outR;
            if (mSmoothedDelayTimeMs > 0.001f) {
                // Positive → delay right channel.
                outL = inL;
                outR = mDelayBufferR[readHead];
            } else if (mSmoothedDelayTimeMs < -0.001f) {
                // Negative → delay left channel.
                outL = mDelayBufferL[readHead];
                outR = inR;
            } else {
                // Near zero → pass-through.
                outL = inL;
                outR = inR;
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
};

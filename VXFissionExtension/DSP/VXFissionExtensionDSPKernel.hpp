//
//  VXFissionExtensionDSPKernel.hpp
//  VXFissionExtension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <algorithm>
#import <vector>
#import <span>

#import "VXFissionExtensionParameterAddresses.h"

/*
 VXFissionExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.

 Stereo delay: delays one side (L or R) of the stereo output by a
 user-specified number of milliseconds. The other side passes through dry.
 */
class VXFissionExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
        // Allocate enough for 50 ms at the current sample rate, plus one extra
        // sample so a delay of exactly 50 ms never wraps onto itself.
        int maxDelaySamples = (int)(mSampleRate * 0.050) + 1;
        mDelayBuffer.assign(maxDelaySamples, 0.0f);
        mWriteHead = 0;
    }

    void deInitialize() {
        mDelayBuffer.clear();
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
            case VXFissionExtensionParameterAddress::delayChannel:
                mDelayChannel = (int)(value + 0.5f); // round to 0 (L) or 1 (R)
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
            case VXFissionExtensionParameterAddress::delayChannel:
                return (AUValue)mDelayChannel;
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

        const int bufSize = (int)mDelayBuffer.size();

        // When bypassed, or delay is zero, just copy dry.
        if (mBypassed || mDelayTimeMs <= 0.0f || bufSize == 0) {
            for (int ch = 0; ch < numOut; ++ch) {
                int srcCh = std::min(ch, numIn - 1);
                for (UInt32 f = 0; f < frameCount; ++f) {
                    outputBuffers[ch][f] = inputBuffers[srcCh][f];
                }
            }
            return;
        }

        // Clamp delay to buffer capacity.
        int delaySamples = std::min(
            (int)(mDelayTimeMs * (float)mSampleRate / 1000.0f),
            bufSize - 1
        );

        // The channel that will be delayed (0 = L, 1 = R).
        int delayedCh = std::min(mDelayChannel, numOut - 1);
        // The input channel that feeds the delayed output.
        int delaySrcCh = std::min(delayedCh, numIn - 1);

        for (UInt32 f = 0; f < frameCount; ++f) {
            // Write the input sample (for the delayed channel) into the ring buffer.
            mDelayBuffer[mWriteHead] = inputBuffers[delaySrcCh][f];

            // Read back delaySamples in the past.
            int readHead = mWriteHead - delaySamples;
            if (readHead < 0) readHead += bufSize;
            float delayedSample = mDelayBuffer[readHead];

            mWriteHead = (mWriteHead + 1 < bufSize) ? mWriteHead + 1 : 0;

            // Write all output channels: dry pass-through except the delayed one.
            for (int ch = 0; ch < numOut; ++ch) {
                int srcCh = std::min(ch, numIn - 1);
                outputBuffers[ch][f] = (ch == delayedCh)
                    ? delayedSample
                    : inputBuffers[srcCh][f];
            }
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

    double mSampleRate      = 44100.0;
    float  mDelayTimeMs     = 0.0f;   // delay in milliseconds (0–500)
    int    mDelayChannel    = 0;      // 0 = delay left, 1 = delay right
    bool   mBypassed        = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;

    std::vector<float> mDelayBuffer;
    int mWriteHead = 0;
};

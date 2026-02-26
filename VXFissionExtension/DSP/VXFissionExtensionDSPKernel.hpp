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

 Simple gain: applies a linear gain factor to all channels.
 */
class VXFissionExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
    }

    void deInitialize() {}

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
            case VXFissionExtensionParameterAddress::outputGain:
                mGain = value;
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
            case VXFissionExtensionParameterAddress::outputGain:
                return (AUValue)mGain;
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

        float gainToApply = mBypassed ? 1.0f : mGain;

        for (int ch = 0; ch < numOut; ++ch) {
            int srcCh = std::min(ch, numIn - 1);
            for (UInt32 frame = 0; frame < frameCount; ++frame) {
                outputBuffers[ch][frame] = inputBuffers[srcCh][frame] * gainToApply;
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

    double mSampleRate    = 44100.0;
    float  mGain          = 1.0f;
    bool   mBypassed      = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;
};

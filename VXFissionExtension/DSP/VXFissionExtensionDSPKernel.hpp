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
 */
class VXFissionExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
        initializeEQ(inSampleRate);
    }

    // Initialize pre-distortion EQ filters
    void initializeEQ(double sampleRate) {
        // Simple biquad filter coefficients for fixed EQ curve
        // Low shelf: cut below 75Hz
        // Presence boost: +1.5dB around 6-7kHz
        // Gentle high cut above 10kHz

        // For simplicity, using a single high-pass + presence shelf
        // High-pass at 75Hz (removes low rumble before distortion)
        float fc = 75.0f / sampleRate;
        float Q = 0.707f;
        float w0 = 2.0f * M_PI * fc;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * Q);

        // High-pass coefficients
        mHPF_b0 = (1.0f + cosw0) / 2.0f;
        mHPF_b1 = -(1.0f + cosw0);
        mHPF_b2 = (1.0f + cosw0) / 2.0f;
        mHPF_a0 = 1.0f + alpha;
        mHPF_a1 = -2.0f * cosw0;
        mHPF_a2 = 1.0f - alpha;

        // Normalize
        mHPF_b0 /= mHPF_a0;
        mHPF_b1 /= mHPF_a0;
        mHPF_b2 /= mHPF_a0;
        mHPF_a1 /= mHPF_a0;
        mHPF_a2 /= mHPF_a0;
    }

    // Apply fixed pre-distortion EQ to shape tone
    float applyPreEQ(float input, int channel) {
        // High-pass filter at 75Hz (removes rumble/mud before distortion)
        float hpf = mHPF_b0 * input + mHPF_b1 * mHPF_x1[channel] + mHPF_b2 * mHPF_x2[channel]
                    - mHPF_a1 * mHPF_y1[channel] - mHPF_a2 * mHPF_y2[channel];

        mHPF_x2[channel] = mHPF_x1[channel];
        mHPF_x1[channel] = input;
        mHPF_y2[channel] = mHPF_y1[channel];
        mHPF_y1[channel] = hpf;

        return hpf;
    }
    
    void deInitialize() {
    }
    
    // MARK: - Bypass
    bool isBypassed() {
        return mBypassed;
    }
    
    void setBypass(bool shouldBypass) {
        mBypassed = shouldBypass;
    }
    
    // MARK: - Oversampling Helpers

    // Simple 4-point linear interpolation upsampler (4x oversampling, per-channel)
    void upsample4x(float input, float out[4], int channel) {
        // Create 4 samples from 1 using linear interpolation
        float prev = mLastSample[channel];
        out[0] = prev + (input - prev) * 0.25f;
        out[1] = prev + (input - prev) * 0.50f;
        out[2] = prev + (input - prev) * 0.75f;
        out[3] = input;
        mLastSample[channel] = input;
    }

    // Simple 4-point averaging downsampler with DC blocker (per-channel)
    float downsample4x(float sample1, float sample2, float sample3, float sample4, int channel) {
        // Average all 4 samples
        float downsampled = (sample1 + sample2 + sample3 + sample4) * 0.25f;

        // DC blocker (high-pass at ~5Hz)
        float dcBlocked = downsampled - mDcBlockerZ1[channel] + 0.995f * mDcBlockerOutput[channel];
        mDcBlockerZ1[channel] = downsampled;
        mDcBlockerOutput[channel] = dcBlocked;

        return dcBlocked;
    }

    // Clipping function extracted for reuse
    float applyClipping(float sample) {
        // Pure hard clipping with asymmetry for crunch and clarity
        // Aggressive thresholds for cutting through mix

        // Asymmetric thresholds (tighter on positive for bite)
        // Scale mTubeGain (0.0-2.0) to drive amount (0.0-1.0)
        float driveAmount = (mTubeGain - 1.0f) * 0.5f;  // 1.0 gain = 0.0 drive, 2.0 gain = 0.5 drive
        driveAmount = (driveAmount > 0.0f) ? driveAmount : 0.0f;  // Clamp to positive
        float positiveThreshold = 0.7f - (driveAmount * 0.60f);
        float negativeThreshold = 0.8f - (driveAmount * 0.60f);

        // Hard clip with asymmetry
        if (sample > positiveThreshold) {
            return positiveThreshold;
        } else if (sample < -negativeThreshold) {
            return -negativeThreshold;
        } else {
            return sample;
        }
    }

    // MARK: - Parameter Getter / Setter
    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case VXFissionExtensionParameterAddress::tubegain:
                mTubeGain = value;
                break;
            case VXFissionExtensionParameterAddress::bypass:
                mBypassed = (value >= 0.5f);
                break;
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        // Return the goal. It is not thread safe to return the ramping value.

        switch (address) {
            case VXFissionExtensionParameterAddress::tubegain:
                return (AUValue)mTubeGain;
            case VXFissionExtensionParameterAddress::bypass:
                return (AUValue)(mBypassed ? 1.0f : 0.0f);

            default: return 0.f;
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
    
    /**
     MARK: - Internal Process
     
     This function does the core siginal processing.
     Do your custom DSP here.
     */
    void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers, AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {
        /*
         Note: For an Audio Unit with 'n' input channels to 'n' output channels, remove the assert below and
         modify the check in [VXFissionExtensionAudioUnit allocateRenderResourcesAndReturnError]
         */
        assert(inputBuffers.size() == outputBuffers.size());
        
        if (mBypassed) {
            // Pass the samples through
            for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                std::copy_n(inputBuffers[channel], frameCount, outputBuffers[channel]);
            }
            return;
        }
        
        // Use this to get Musical context info from the Plugin Host,
        // Replace nullptr with &memberVariable according to the AUHostMusicalContextBlock function signature
        /*
         if (mMusicalContextBlock) {
         mMusicalContextBlock(nullptr, 	// currentTempo
         nullptr, 	// timeSignatureNumerator
         nullptr, 	// timeSignatureDenominator
         nullptr, 	// currentBeatPosition
         nullptr, 	// sampleOffsetToNextBeat
         nullptr);	// currentMeasureDownbeatPosition
         }
         */
        
        // Perform per sample dsp on the incoming float in before assigning it to out
        for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                float input = inputBuffers[channel][frameIndex];

                // Apply fixed pre-distortion EQ to shape tone
                float eqed = applyPreEQ(input, channel);

                // Apply tube gain (0.0 to 2.0)
                float gained = eqed * mTubeGain;

                // 4x Oversampling:
                // 1. Upsample to 4x sample rate (creates 4 samples from 1)
                float upsampled[4];
                upsample4x(gained, upsampled, channel);

                // 2. Apply clipping to all 4 oversampled samples (adds tube warmth)
                float clipped1 = applyClipping(upsampled[0]);
                float clipped2 = applyClipping(upsampled[1]);
                float clipped3 = applyClipping(upsampled[2]);
                float clipped4 = applyClipping(upsampled[3]);

                // 3. Downsample back to original rate
                float clipped = downsample4x(clipped1, clipped2, clipped3, clipped4, channel);

                // Output with subtle makeup gain
                float makeupGain = 1.0f + ((mTubeGain - 1.0f) * 0.2f);  // Gentle compensation
                float output = clipped * makeupGain;

                outputBuffers[channel][frameIndex] = output;
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

    double mSampleRate = 44100.0;
    float mTubeGain = 1.0f;  // 0.0 to 2.0
    bool mBypassed = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;

    // Oversampling state variables (per-channel, max 8 channels)
    float mLastSample[8] = {0.0f};
    float mDcBlockerZ1[8] = {0.0f};
    float mDcBlockerOutput[8] = {0.0f};

    // Pre-distortion EQ filter state (per-channel)
    // High-pass filter coefficients
    float mHPF_b0 = 0.0f, mHPF_b1 = 0.0f, mHPF_b2 = 0.0f;
    float mHPF_a0 = 1.0f, mHPF_a1 = 0.0f, mHPF_a2 = 0.0f;
    // High-pass filter state
    float mHPF_x1[8] = {0.0f}, mHPF_x2[8] = {0.0f};
    float mHPF_y1[8] = {0.0f}, mHPF_y2[8] = {0.0f};
};

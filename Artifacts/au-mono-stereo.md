# AUv3 Mono→Stereo in Logic Pro

## The Core Problem

Logic Pro caches Audio Unit channel capabilities using the `version` integer
in `Info.plist` as the cache key. If you change `channelCapabilities` in code
but don't bump the version, Logic silently serves the old cached capabilities
forever — no error, no warning, every rescan is a no-op.

## Required Code Changes

### 1. `VXFissionExtension/Info.plist`

Bump `version` whenever `channelCapabilities` changes:

```xml
<key>version</key>
<integer>67073</integer>   <!-- increment this each time -->
```

Version format: `0xMMmmpp` (major, minor, patch as hex).
`67073` = `0x10601` = version 1.6.1.

### 2. `VXFissionExtensionAudioUnit.swift` — Bus Setup

Input bus must default to **mono**; output bus must be **stereo**:

```swift
let monoFormat   = AVAudioFormat(standardFormatWithSampleRate: 44_100, channels: 1)!
let stereoFormat = AVAudioFormat(standardFormatWithSampleRate: 44_100, channels: 2)!

outputBus = try AUAudioUnitBus(format: stereoFormat)
outputBus?.maximumChannelCount = 2

inputBus.initialize(monoFormat, 2)   // default 1ch, max 2ch
```

### 3. `VXFissionExtensionAudioUnit.swift` — `channelCapabilities`

```swift
override var channelCapabilities: [NSNumber]? {
    return [1, 1, 1, 2, 2, 2] as [NSNumber]
    // [1,1] = Mono→Mono
    // [1,2] = Mono→Stereo  ← this is what shows in Logic's format picker
    // [2,2] = Stereo→Stereo
}
```

### 4. `VXFissionExtensionDSPKernel.hpp` — DSP Upmix

The kernel already handles upmix correctly:

```cpp
for (int ch = 0; ch < numOut; ++ch) {
    int srcCh = std::min(ch, numIn - 1);   // clamps to last input channel
    for (UInt32 frame = 0; frame < frameCount; ++frame) {
        outputBuffers[ch][frame] = inputBuffers[srcCh][frame] * gainToApply;
    }
}
// mono in (numIn=1), stereo out (numOut=2):
//   ch=0 → srcCh=0, ch=1 → srcCh=min(1,0)=0  → both channels get input[0]
```

### 5. `VXFissionExtensionAUProcessHelper.hpp` — Null Buffer Fallback

Safe fallback when host passes null output pointers (clamps index for mono→stereo):

```cpp
if (outAudioBufferList->mBuffers[0].mData == nullptr) {
    UInt32 numInBufs = inAudioBufferList->mNumberBuffers;
    for (UInt32 i = 0; i < outAudioBufferList->mNumberBuffers; ++i) {
        UInt32 srcIdx = (i < numInBufs) ? i : (numInBufs - 1);
        outAudioBufferList->mBuffers[i].mData = inAudioBufferList->mBuffers[srcIdx].mData;
    }
}
```

---

## Build & Deploy Procedure

Every time you change channel capabilities:

```bash
# 1. Bump version in Info.plist (see above)

# 2. Build
bash build.sh

# 3. Kill the stale extension process (Logic holds onto it)
pkill -9 -f "VXFissionExtens"

# 4. Clear Logic's AU capability cache
rm -f ~/Library/Caches/AudioUnitCache/com.apple.audiounits.cache

# 5. Repopulate cache with correct data
auval -v aufx vxfs TyAu

# 6. Cold-restart Logic Pro (not just rescan — rescan re-writes the old in-memory cache)
```

---

## Validation

`auval` output confirming correct setup:

```
Default Format: AudioStreamBasicDescription:  1 ch,  44100 Hz, Float32        ← input
Default Format: AudioStreamBasicDescription:  2 ch,  44100 Hz, Float32, deinterleaved  ← output

Reported Channel Capabilities (explicit):
      [1, 1]  [1, 2]  [2, 2]

Input/Output Channel Handling:
1-1   1-2   ...   2-2
X     X           X

1 to 2 Channel Render Test at 256 frames
  PASS

AU VALIDATION SUCCEEDED.
```

---

## Why This Took So Long

| Attempt | Why It Failed |
|---|---|
| Changed `channelCapabilities` | Version not bumped → Logic used cache |
| Killed extension process | Still used cached capabilities |
| Deleted AU cache | Logic re-wrote it from in-memory (stale) data during rescan |
| Ran `auval` | Correct data written to cache, but Logic still used old in-memory copy |
| Changed input bus to mono | Version not bumped → Logic used cache |
| Bumped version + cold restart | **Fixed** |

The fix required **all three** together: correct code + version bump + cold Logic restart.

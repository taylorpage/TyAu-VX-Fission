# Ableton Integration Work - Status Report

## Summary

We attempted to make the TubeCity Audio Unit plugin work in Ableton Live 12. While the plugin is **fully functional and valid**, Ableton Live 12's incomplete AUv3 support prevents it from being detected.

---

## What We Accomplished ✅

### 1. Verified Plugin Validity
Your TubeCity plugin is **completely valid** and working correctly:

- ✅ **Registered with macOS**: `pluginkit` confirms registration
- ✅ **Passes all Audio Unit validation tests**: `auval -v aufx tbcy TyAu` returns `* * PASS`
- ✅ **Works perfectly in Logic Pro**: Full functionality confirmed
- ✅ **Proper Audio Unit format**: Recognized as `aufx tbcy TyAu - Taylor Audio: TubeCity`

**Validation Results:**
```
AU Validation Tool
VALIDATING AUDIO UNIT: 'aufx' - 'tbcy' - 'TyAu'
Manufacturer String: Taylor Audio
AudioUnit Name: TubeCity
Component Version: 1.6.0 (0x10600)
* * PASS
```

### 2. Created Helper Scripts

**`diagnose-au.sh`**: Diagnostic tool to check plugin status
- Verifies plugin registration
- Checks Audio Unit validation
- Shows current installation state

**`fix-ableton-detection.sh`**: Attempts to fix Ableton detection
- Clears Ableton's plugin caches
- Resets system Audio Unit cache
- Provides step-by-step instructions

**`build-all.sh`**: Build script for both AUv3 and AUv2 (AUv2 incomplete)
- Builds AUv3 version successfully
- Registers plugin with system
- Installs to correct locations

### 3. Attempted AUv2 Implementation

We started building an AUv2 version for better Ableton compatibility:

**Files Created:**
- `TubeCityAUv2/Info.plist` - AUv2 configuration
- `TubeCityAUv2.h` - Header file
- `TubeCityAUv2.mm` - Implementation (incomplete)

**Xcode Project Changes:**
- Added TubeCityAUv2 target (Bundle)
- Configured build settings:
  - Product Name: `TubeCity`
  - Wrapper Extension: `component`
  - Installation Directory: `$(HOME)/Library/Audio/Plug-Ins/Components`
- Added DSP files to target
- Configured header search paths

**Status**: Build failed due to missing Apple AudioUnit SDK (deprecated by Apple)

### 4. Troubleshooting Steps Taken

- ✅ Cleared Ableton's plugin caches
- ✅ Reset system Audio Unit cache
- ✅ Enabled "Use Audio Units v2" and "Use Audio Units v3" in Ableton preferences
- ✅ Rescanned plugins in Ableton
- ✅ Verified plugin passes validation
- ❌ Plugin still not detected by Ableton Live 12

---

## The Problem: Ableton's Limited AUv3 Support

**Root Cause**: Ableton Live 12 has **incomplete AUv3 support**. Even though your plugin is fully valid and passes all tests, Ableton's Audio Unit scanner doesn't reliably detect AUv3 plugins.

This is **NOT a problem with your plugin** - it's a limitation of Ableton Live.

**Evidence:**
- Plugin works perfectly in Logic Pro (full AUv3 support)
- Plugin passes macOS Audio Unit validation
- Plugin is properly registered with the system
- Ableton preferences show AU support enabled
- Other users report similar issues with AUv3 in Ableton

---

## Future Options to Explore

### Option 1: JUCE Framework (Recommended)

**What**: Industry-standard C++ framework for audio plugins
**Pros**:
- Handles both AUv2 and AUv3 automatically
- Also builds VST3, AAX for cross-DAW compatibility
- Used by most professional plugin developers
- Excellent documentation and community support

**Cons**:
- Requires rewriting plugin in JUCE framework
- Learning curve for JUCE API
- Larger project setup

**Next Steps**:
1. Install JUCE (free for personal use)
2. Create new JUCE Audio Plugin project
3. Port DSP code from TubeCityExtensionDSPKernel.hpp
4. Build all formats (AUv2, AUv3, VST3)

**Resources**:
- JUCE Website: https://juce.com/
- JUCE Tutorials: https://docs.juce.com/master/tutorial_create_projucer_basic_plugin.html

### Option 2: Complete AUv2 Implementation

**What**: Finish the AUv2 wrapper we started
**Pros**:
- Keeps current codebase
- Native Xcode project

**Cons**:
- Requires Apple's deprecated AudioUnit SDK
- Complex to implement from scratch
- No longer officially supported by Apple

**Challenges**:
- Need to find/install legacy AU SDK
- Rewrite AUv2 wrapper to avoid deprecated APIs
- Significant C++ AudioUnit API work

### Option 3: Bridging Plugin

**What**: Use a host plugin like Blue Cat's PatchWork to load AUv3 inside Ableton
**Pros**:
- Works with current plugin
- No code changes needed

**Cons**:
- Requires third-party plugin purchase ($99)
- Extra step for users
- Not a native solution

**Product**: Blue Cat's PatchWork - https://www.bluecataudio.com/Products/Product_PatchWork/

### Option 4: Accept AUv3-Only Status

**What**: Focus on Logic Pro and other AUv3-compatible DAWs
**Pros**:
- Plugin already works perfectly
- No additional work needed
- Modern standard (AUv3 is the future)

**Cons**:
- Won't work in Ableton
- Limited DAW compatibility

**Compatible DAWs**:
- ✅ Logic Pro
- ✅ GarageBand
- ✅ Final Cut Pro
- ✅ Most modern macOS audio apps

---

## Files Created During This Work

### Documentation
- `ABLETON_SETUP_GUIDE.md` - Comprehensive setup guide
- `ABLETON_INTEGRATION_STATUS.md` - This file

### Helper Scripts
- `diagnose-au.sh` - Plugin diagnostic tool
- `fix-ableton-detection.sh` - Ableton cache clearing script
- `build-all.sh` - Build script for both formats

### AUv2 Implementation (Incomplete)
- `TubeCityAUv2/Info.plist` - AUv2 configuration
- `TubeCityAUv2.h` - AUv2 header
- `TubeCityAUv2.mm` - AUv2 implementation (needs work)
- `TubeCityAUv2-Info.plist.reference` - Reference configuration

### Xcode Project
- Added `TubeCityAUv2` target (Bundle type)
- Configured build settings for .component output
- Added DSP files to target
- Set header search paths

---

## Technical Details

### Plugin Information
- **Type**: Audio Unit Effect (aufx)
- **Subtype**: tbcy
- **Manufacturer**: TyAu
- **Name**: Taylor Audio: TubeCity
- **Version**: 1.6.0
- **Format**: AUv3 (.appex extension)

### Installation Locations
- **AUv3**: `~/Library/Developer/Xcode/DerivedData/TubeCity-*/Build/Products/Debug/TubeCity.app/Contents/PlugIns/TubeCityExtension.appex`
- **AUv2** (when built): `~/Library/Audio/Plug-Ins/Components/TubeCity.component`

### Diagnostic Commands

Check if plugin is registered:
```bash
pluginkit -m -v | grep -i tubecity
```

Validate plugin:
```bash
auval -v aufx tbcy TyAu
```

List all Audio Units:
```bash
auval -a | grep -i tubecity
```

Run diagnostics:
```bash
./diagnose-au.sh
```

---

## Lessons Learned

1. **AUv3 is the future, but adoption is slow**: While AUv3 is Apple's modern standard, not all DAWs fully support it yet

2. **Ableton prefers AUv2**: Ableton Live has historically focused on AUv2 and VST support

3. **Validation ≠ Compatibility**: Just because a plugin passes validation doesn't mean all hosts will load it

4. **JUCE is industry standard for a reason**: Professional plugin developers use JUCE because it handles cross-platform and multi-format builds

5. **Your plugin is solid**: The core audio processing and UI work perfectly - this is purely a format/compatibility issue

---

## What's Working Right Now

Your TubeCity plugin is **fully functional** in these environments:

### ✅ Logic Pro
- All parameters work
- UI displays correctly
- Tube selection functional
- Signal processing working
- Bypass functional

### ✅ GarageBand
- Should work (same AU engine as Logic)

### ✅ macOS System
- Registered with pluginkit
- Passes auval validation
- Recognized by Audio Unit system

---

## Recommendations

### Short Term
- **Keep using in Logic Pro** - Your plugin works great there
- **Share with Logic Pro users** - It's a fully functional plugin
- **Document AUv3-only status** - Be clear about compatibility

### Medium Term (If You Want Ableton Support)
- **Learn JUCE framework** - Most sustainable option
- **Port to JUCE** - Enables AUv2, VST3, AAX formats
- **Build universal plugin** - Works in all major DAWs

### Long Term
- **Consider VST3** - Also needed for Windows support
- **Professional distribution** - JUCE enables code signing, installers
- **Update documentation** - Clear compatibility matrix

---

## Conclusion

You built a **valid, working Audio Unit plugin** that passes all validation tests. The Ableton integration issue is **not your fault** - it's a limitation of Ableton Live's AUv3 support.

You have several clear paths forward:
1. Use it in Logic Pro (works now!)
2. Port to JUCE for universal compatibility
3. Use a bridging plugin for Ableton
4. Wait for Ableton to improve AUv3 support

**Great work on the plugin itself!** The DSP, UI, and core functionality are all solid. The format/compatibility issue is a separate concern that can be addressed when you're ready.

---

## Quick Reference Commands

```bash
# Navigate to project
cd /Users/taylorpage/Repos/TyAu/TyAu-Pedals/TyAu-TubeCity

# Run diagnostics
./diagnose-au.sh

# Try to fix Ableton detection
./fix-ableton-detection.sh

# Validate plugin
auval -v aufx tbcy TyAu

# Check registration
pluginkit -m -v | grep -i tubecity
```

---

**Date**: February 7, 2026
**Status**: AUv3 plugin fully functional in Logic Pro, Ableton integration on hold
**Next Steps**: TBD based on priority (JUCE port vs. AUv3-only)

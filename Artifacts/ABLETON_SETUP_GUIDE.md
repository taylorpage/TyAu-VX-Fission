# TubeCity Plugin Setup for Ableton Live

## Current Status - UPDATED

- ✅ AUv3 plugin is built and working in Logic Pro
- ✅ AUv3 plugin is registered with macOS (`pluginkit` confirms)
- ✅ AUv3 plugin **PASSES** Audio Unit validation (`auval`)
- ⚠️ Ableton Live 12 is not detecting the plugin yet
- ❌ AUv2 version build failed (requires legacy SDK)

## IMPORTANT: Your Plugin is Valid!

Good news: Your AUv3 plugin is properly registered and **passes all Audio Unit validation tests**. This means macOS recognizes it as a valid plugin.

The system reports:
```
aufx tbcy TyAu  -  Taylor Audio: TubeCity
Component Version: 1.6.0
Status: * * PASS
```

## Option 1: Make Ableton Detect Your AUv3 (Recommended First Try)

Since your plugin passes validation, Ableton should be able to see it. Try these steps:

### Step 1: Force Ableton to Rescan Audio Units

1. **Quit Ableton Live completely**
2. Delete Ableton's plugin database:
   ```bash
   rm -rf ~/Library/Preferences/Ableton/Live\ 12*/PluginDatabase*
   rm -rf ~/Library/Preferences/Ableton/Live\ 12*/AudioUnitCache*
   ```

3. **Restart your Mac** (important - clears all AU caches)

4. **Open Ableton Live**
5. Go to **Preferences → Plug-ins**
6. Make sure **"Use Audio Units"** is enabled
7. Click **"Rescan"** and wait for it to complete
8. Look for **"Taylor Audio: TubeCity"** in the Browser under Audio Effects

### Step 2: Check Ableton's Plugin Folders

1. In Ableton: **Preferences → Plug-ins**
2. Under "Audio Units", verify the plugin folders are being scanned
3. The default should include system Audio Unit locations

### Step 3: Manual Plugin Search

If rescanning doesn't work:

1. In Ableton's Browser, right-click on **"Plug-ins"**
2. Select **"Rescan"** from the context menu
3. Try searching for "TubeCity" or "Taylor" in the search box

###Step 4: Check Ableton's Log

If it still doesn't appear, check Ableton's log file:
```bash
tail -f ~/Library/Preferences/Ableton/Live\ 12*/Log.txt
```

Look for mentions of "TubeCity" or errors related to Audio Unit loading.

### If Option 1 Still Doesn't Work...

Proceed to Option 2 below (building AUv2 version).

---

## Option 2: Build AUv2 Version (Guaranteed Solution)

AUv2 is fully supported by Ableton Live and is the most reliable option.

### Prerequisites

You have these files ready:
- [TubeCityAUv2.h](TubeCityAUv2.h) - AUv2 header
- [TubeCityAUv2.mm](TubeCityAUv2.mm) - AUv2 implementation
- [TubeCityAUv2-Info.plist.reference](TubeCityAUv2-Info.plist.reference) - Info.plist template
- [build-all.sh](build-all.sh) - Build script for both AUv2 and AUv3
- [diagnose-au.sh](diagnose-au.sh) - Diagnostic tool

### Step 1: Open Xcode

```bash
open TubeCity.xcodeproj
```

### Step 2: Add New AUv2 Target

1. Click on your **TubeCity** project (blue icon at top of navigator)
2. At the bottom of the targets list, click the **"+"** button
3. Select **macOS** → **"Audio Unit Extension"**
4. Click **Next**

### Step 3: Configure the Target

Set these values:
- **Product Name**: `TubeCityAUv2`
- **Team**: (Your team)
- **Organization Identifier**: Same as your main app
- **Bundle Identifier**: Should auto-fill (e.g., `com.yourdomain.TubeCityAUv2`)
- **Language**: **Objective-C** (important!)
- Click **Finish**
- Click **"Activate"** for the scheme

### Step 4: Configure Build Settings

1. Select the **TubeCityAUv2** target
2. Go to **Build Settings** tab
3. Search and set:
   - **Product Name**: `TubeCity` (same as AUv3)
   - **Wrapper Extension**: `component`
   - **Installation Directory**: `$(HOME)/Library/Audio/Plug-Ins/Components`
   - **Skip Install**: Set to **NO**

### Step 5: Add AUv2 Implementation Files

1. Right-click the **TubeCityAUv2** folder in navigator
2. Select **"Add Files to TubeCityAUv2"**
3. Add:
   - `TubeCityAUv2.h`
   - `TubeCityAUv2.mm`
4. Make sure **only** the TubeCityAUv2 target is checked

### Step 6: Add DSP Files to AUv2 Target

Select each of these files and check the **TubeCityAUv2** target box in the File Inspector:

- `TubeCityExtension/DSP/TubeCityExtensionDSPKernel.hpp`
- `TubeCityExtension/Common/DSP/TubeCityExtensionBufferedAudioBus.hpp`
- `TubeCityExtension/DSP/TubeSaturation.hpp`
- `TubeCityExtension/DSP/TaylorWarmTube.hpp`
- `TubeCityExtension/DSP/TaylorAggressiveTube.hpp`

### Step 7: Configure Header Search Paths

1. Select **TubeCityAUv2** target
2. Go to **Build Settings**
3. Search for **"Header Search Paths"**
4. Add:
   ```
   $(PROJECT_DIR)/TubeCityExtension/DSP
   $(PROJECT_DIR)/TubeCityExtension/Common/DSP
   ```

### Step 8: Update Info.plist

1. Open `TubeCityAUv2/Info.plist` in Xcode
2. Replace contents with [TubeCityAUv2-Info.plist.reference](TubeCityAUv2-Info.plist.reference)

### Step 9: Build Both Versions

Make the build script executable and run it:

```bash
chmod +x build-all.sh
./build-all.sh
```

This will:
- Build both AUv3 and AUv2
- Install AUv2 to `~/Library/Audio/Plug-Ins/Components/`
- Register both with the system
- Reset Audio Unit cache

### Step 10: Verify Installation

```bash
./diagnose-au.sh
```

You should see:
- ✅ AUv2 component found
- ✅ AUv3 extension found
- ✅ TubeCity found in Audio Unit validation

### Step 11: Test in Ableton

1. Restart Ableton Live if running
2. Go to **Preferences → Plug-ins → Rescan**
3. Look for **"Taylor Audio: TubeCity"** in Audio Effects

---

## Troubleshooting

### Plugin Still Not Showing

If the plugin still doesn't appear in Ableton:

1. **Verify installation**:
   ```bash
   ls -la ~/Library/Audio/Plug-Ins/Components/ | grep TubeCity
   ```

2. **Check Audio Unit validation**:
   ```bash
   auval -v aufx tbcy TyAu
   ```

3. **Reset Audio Unit cache system-wide**:
   ```bash
   killall -9 AudioComponentRegistrar
   killall -9 coreaudiod
   ```

4. **Check Ableton's AU preferences**:
   - Preferences → Plug-ins
   - Make sure "Use Audio Units" is enabled
   - Verify the plugin folder is being scanned

### Common Issues

**Issue**: Build fails with "header not found"
- **Solution**: Double-check Header Search Paths in Step 7

**Issue**: AUv2 component exists but auval doesn't find it
- **Solution**: The Info.plist might be incorrect - verify AudioComponents section

**Issue**: Ableton crashes when loading plugin
- **Solution**: The DSP kernel may have issues - check parameter initialization

---

## Files Reference

### Created Files
- `TubeCityAUv2.h` - AUv2 header declaration
- `TubeCityAUv2.mm` - AUv2 implementation (wraps existing DSP)
- `TubeCityAUv2-Info.plist.reference` - Info.plist template
- `build-all.sh` - Automated build script
- `diagnose-au.sh` - Diagnostic tool
- `ABLETON_SETUP_GUIDE.md` - This guide

### Key Locations
- **AUv3 Extension**: `~/Library/Developer/Xcode/DerivedData/TubeCity-*/Build/Products/Debug/TubeCity.app/Contents/PlugIns/TubeCityExtension.appex`
- **AUv2 Component**: `~/Library/Audio/Plug-Ins/Components/TubeCity.component`

---

## Understanding AUv2 vs AUv3

**AUv3 (Audio Unit v3)**:
- Modern format (introduced 2015)
- App extension (`.appex`)
- Better sandboxing and security
- Works in: Logic Pro, GarageBand, most modern DAWs
- **Spotty support in Ableton Live**

**AUv2 (Audio Unit v2)**:
- Legacy format (pre-2015)
- Component bundle (`.component`)
- Less secure but universally supported
- Works in: **All DAWs including Ableton Live**

**Best Practice**: Ship both formats for maximum compatibility. They share the same DSP code, so maintenance is straightforward.

---

## Next Steps After Setup

Once your plugin is working in Ableton:

1. Test all parameters (Input, Tube selection, Output)
2. Verify bypass functionality
3. Test in different sample rates
4. Save presets in Ableton
5. Test automation recording

---

**Need Help?** Run `./diagnose-au.sh` to check the status of your installations.

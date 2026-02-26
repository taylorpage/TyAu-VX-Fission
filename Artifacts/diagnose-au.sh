#!/bin/bash

# TubeCity Audio Unit Diagnostic Script
# Checks the status of AUv2 and AUv3 installations

echo "🔍 TubeCity Audio Unit Diagnostics"
echo "=================================="
echo ""

# Check AUv2 Component
echo "📦 Checking AUv2 Component..."
if [ -d "$HOME/Library/Audio/Plug-Ins/Components/TubeCity.component" ]; then
    echo "✅ AUv2 component found at:"
    echo "   ~/Library/Audio/Plug-Ins/Components/TubeCity.component"
    ls -lh "$HOME/Library/Audio/Plug-Ins/Components/TubeCity.component"
else
    echo "❌ AUv2 component NOT found"
    echo "   Expected location: ~/Library/Audio/Plug-Ins/Components/TubeCity.component"
fi
echo ""

# Check AUv3 Extension
echo "📦 Checking AUv3 Extension..."
DERIVED_DATA=$(find ~/Library/Developer/Xcode/DerivedData -name "TubeCity-*" -type d -maxdepth 1 | head -n 1)
if [ -n "$DERIVED_DATA" ]; then
    AUV3_APPEX="$DERIVED_DATA/Build/Products/Debug/TubeCity.app/Contents/PlugIns/TubeCityExtension.appex"
    if [ -d "$AUV3_APPEX" ]; then
        echo "✅ AUv3 extension found at:"
        echo "   $AUV3_APPEX"
    else
        echo "❌ AUv3 extension NOT found in expected location"
    fi
else
    echo "❌ Could not find DerivedData directory"
fi
echo ""

# Check AUv3 Registration
echo "📝 Checking AUv3 Registration..."
if pluginkit -m -v -i com.apple.AudioUnit-UI 2>/dev/null | grep -qi "tubecity"; then
    echo "✅ AUv3 is registered with pluginkit:"
    pluginkit -m -v -i com.apple.AudioUnit-UI 2>/dev/null | grep -i "tubecity"
else
    echo "❌ AUv3 is NOT registered with pluginkit"
    echo "   (This is needed for AUv3 to work in DAWs)"
fi
echo ""

# Check Audio Unit validation
echo "�� Checking Audio Unit validation..."
echo "   (This may take a moment...)"
AUVAL_OUTPUT=$(auval -a 2>&1 | grep -i "tubecity" || echo "")
if [ -n "$AUVAL_OUTPUT" ]; then
    echo "✅ TubeCity found in Audio Unit validation:"
    echo "$AUVAL_OUTPUT"
else
    echo "❌ TubeCity NOT found in Audio Unit validation"
fi
echo ""

# Check Ableton Live
echo "🎹 Checking Ableton Live..."
if [ -d "/Applications/Ableton Live 12 Trial.app" ]; then
    echo "✅ Ableton Live 12 Trial found"
    echo "   Version 12 has improved AUv3 support, but AUv2 is more reliable"
else
    echo "⚠️  Ableton Live not found at expected location"
fi
echo ""

# Summary
echo "📋 Summary & Recommendations:"
echo "=================================="
echo ""

HAS_AUV2=false
HAS_AUV3=false

if [ -d "$HOME/Library/Audio/Plug-Ins/Components/TubeCity.component" ]; then
    HAS_AUV2=true
fi

if pluginkit -m -v -i com.apple.AudioUnit-UI 2>/dev/null | grep -qi "tubecity"; then
    HAS_AUV3=true
fi

if [ "$HAS_AUV2" = true ]; then
    echo "✅ AUv2 is installed - Ableton should detect this"
    echo "   Next steps:"
    echo "   1. Restart Ableton Live"
    echo "   2. Go to Preferences → Plug-ins → Rescan"
    echo "   3. Look for 'Taylor Audio: TubeCity' in Audio Effects"
elif [ "$HAS_AUV3" = true ]; then
    echo "⚠️  Only AUv3 is registered - may not work reliably in Ableton"
    echo "   Recommendation: Build and install AUv2 version"
else
    echo "❌ No Audio Units found"
    echo "   You need to:"
    echo "   1. Complete the Xcode setup for AUv2 target"
    echo "   2. Run ./build-all.sh"
fi
echo ""

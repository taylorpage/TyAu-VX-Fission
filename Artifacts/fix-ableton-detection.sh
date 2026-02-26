#!/bin/bash

# Script to help Ableton Live detect TubeCity AU plugin

echo "🎸 TubeCity - Ableton Detection Fix Script"
echo "=========================================="
echo ""

# Check if Ableton is running
if pgrep -x "Live" > /dev/null; then
    echo "⚠️  Ableton Live is currently running!"
    echo "   Please quit Ableton Live before continuing."
    echo ""
    read -p "Press Enter after you've quit Ableton Live..."
fi

echo "🔍 Checking plugin status..."
echo ""

# Check if plugin is registered
if pluginkit -m -v | grep -qi "tubecity"; then
    echo "✅ TubeCity is registered with macOS"
    pluginkit -m -v | grep -i "tubecity"
else
    echo "❌ TubeCity is NOT registered with macOS"
    echo "   Run: open TubeCity.app from DerivedData to register it"
    exit 1
fi
echo ""

# Check if plugin passes validation
if auval -a 2>&1 | grep -qi "tubecity"; then
    echo "✅ TubeCity passes Audio Unit validation"
    auval -a 2>&1 | grep -i "tubecity"
else
    echo "❌ TubeCity not found in AU validation"
    exit 1
fi
echo ""

# Find and clear Ableton caches
echo "🗑️  Clearing Ableton Live plugin caches..."
ABLETON_PREFS="$HOME/Library/Preferences/Ableton"

if [ -d "$ABLETON_PREFS" ]; then
    # Find and remove plugin databases
    find "$ABLETON_PREFS" -name "*PluginDatabase*" -exec rm -rf {} \; 2>/dev/null
    find "$ABLETON_PREFS" -name "*AudioUnitCache*" -exec rm -rf {} \; 2>/dev/null
    echo "✅ Cleared Ableton plugin caches"
else
    echo "⚠️  Ableton preferences folder not found"
fi
echo ""

# Reset system AU cache
echo "🔄 Resetting system Audio Unit cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true
killall -9 coreaudiod 2>/dev/null || true
echo "✅ System AU cache reset"
echo ""

echo "✨ Done!"
echo ""
echo "📋 Next steps:"
echo "   1. Restart your Mac (recommended for best results)"
echo "   2. Open Ableton Live"
echo "   3. Go to Preferences → Plug-ins"
echo "   4. Make sure 'Use Audio Units' is enabled"
echo "   5. Click 'Rescan'"
echo "   6. Look for 'Taylor Audio: TubeCity' in Audio Effects"
echo ""
echo "💡 If it still doesn't appear:"
echo "   - Check Ableton's log: ~/Library/Preferences/Ableton/Live\ 12*/Log.txt"
echo "   - Try right-clicking 'Plug-ins' in Browser and select 'Rescan'"
echo "   - Ableton Live 12 has partial AUv3 support - an AUv2 version may be needed"
echo ""

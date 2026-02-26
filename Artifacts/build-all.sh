#!/bin/bash

# TyAu-TubeCity Build Script
# Builds both AUv3 and AUv2 versions of the plugin

set -e  # Exit on error

echo "🎸 Building TyAu-TubeCity plugins..."
echo ""

# Build AUv3 (original)
echo "📦 Building AUv3 version..."
xcodebuild -project TubeCity.xcodeproj \
    -scheme TubeCity \
    -configuration Debug \
    build \
    -allowProvisioningUpdates

echo "✅ AUv3 build succeeded!"
echo ""

# Build AUv2
echo "📦 Building AUv2 version..."
xcodebuild -project TubeCity.xcodeproj \
    -scheme TubeCityAUv2 \
    -configuration Debug \
    build \
    -allowProvisioningUpdates

echo "✅ AUv2 build succeeded!"
echo ""

# Find the DerivedData directory
DERIVED_DATA=$(find ~/Library/Developer/Xcode/DerivedData -name "TubeCity-*" -type d -maxdepth 1 | head -n 1)

if [ -z "$DERIVED_DATA" ]; then
    echo "❌ Error: Could not find DerivedData directory"
    exit 1
fi

echo "📂 Found build at: $DERIVED_DATA"
echo ""

# Register AUv3 extension
echo "📝 Registering AUv3 Audio Unit extension..."
AUV3_APP="$DERIVED_DATA/Build/Products/Debug/TubeCity.app"
if [ -d "$AUV3_APP" ]; then
    open "$AUV3_APP"
    echo "✅ AUv3 registered (opened app to register extension)"
else
    echo "⚠️  Warning: AUv3 app not found at expected location"
fi
echo ""

# Install AUv2 component
echo "📝 Installing AUv2 component..."
AUV2_COMPONENT="$DERIVED_DATA/Build/Products/Debug/TubeCity.component"
INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/Components"

if [ -d "$AUV2_COMPONENT" ]; then
    # Create the Components directory if it doesn't exist
    mkdir -p "$INSTALL_DIR"

    # Remove old version if it exists
    if [ -d "$INSTALL_DIR/TubeCity.component" ]; then
        echo "🗑️  Removing old AUv2 component..."
        rm -rf "$INSTALL_DIR/TubeCity.component"
    fi

    # Copy new version
    cp -R "$AUV2_COMPONENT" "$INSTALL_DIR/"
    echo "✅ AUv2 component installed to: $INSTALL_DIR/TubeCity.component"
else
    echo "⚠️  Warning: AUv2 component not found at expected location"
    echo "   Expected: $AUV2_COMPONENT"
fi
echo ""

# Reset Audio Unit cache
echo "🔄 Resetting Audio Unit cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo "✅ Cache reset"
echo ""

# Validate the AUv2 component
echo "🔍 Validating AUv2 component..."
if [ -d "$INSTALL_DIR/TubeCity.component" ]; then
    auval -a 2>&1 | grep -i "tubecity" || echo "⚠️  TubeCity not found in auval list (may need to restart)"
fi
echo ""

echo "🎉 Build complete!"
echo ""
echo "📋 Summary:"
echo "   - AUv3: Registered (for Logic Pro, GarageBand, etc.)"
echo "   - AUv2: Installed at ~/Library/Audio/Plug-Ins/Components/TubeCity.component"
echo ""
echo "🎸 Next steps:"
echo "   1. Restart Ableton Live if it's running"
echo "   2. In Ableton: Preferences → Plug-ins → Rescan"
echo "   3. Look for 'Taylor Audio: TubeCity' in Audio Effects"
echo ""

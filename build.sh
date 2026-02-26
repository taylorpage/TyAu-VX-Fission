#!/bin/bash

# TyAu-VX-Fission Build Script
# Builds the plugin in Debug configuration and registers it with the system

set -e  # Exit on error

echo "🎸 Building TyAu-VX-Fission plugin..."

# Build in Debug configuration
xcodebuild -project VXFission.xcodeproj \
    -scheme VXFission \
    -configuration Debug \
    build \
    -allowProvisioningUpdates

echo "✅ Build succeeded!"

# Register the Audio Unit extension
echo "📝 Registering Audio Unit extension..."
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/VXFission-*/Build/Products/Debug/VXFission.app

echo "🎸 VX Fission is ready! Load it in Logic Pro."

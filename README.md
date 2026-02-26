# TyAu-Template

This is a clean, **tested and validated** template for creating new Audio Unit effect plugins for the TyAu pedal series.

**Status:** ✅ Built, registered, and validated (`auval` passed)
**Registered as:** `aufx tmpl TyAu - Taylor Audio: Template`

## Features

- ✅ Clean light grey UI design
- ✅ Input/Output jacks with labels
- ✅ TaylorAudio logo
- ✅ LED bypass indicator (green when active)
- ✅ Simple black tick marks on knob
- ✅ Professional stomp button bypass
- ✅ Single gain parameter (0.0-2.0, default 1.0)
- ✅ Tube saturation DSP with EQ and oversampling
- ✅ Logic Pro compatible (mono 1-1 and stereo 2-2)

## How to Use This Template

### 1. Copy the Template Directory

```bash
cp -R TyAu-Template TyAu-YourPluginName
cd TyAu-YourPluginName
```

### 2. Rename All Files and References

Replace "Template" with your plugin name in:
- Directory names (Template → YourPluginName, TemplateExtension → YourPluginNameExtension)
- File names (all files with "Template" in the name)
- Code references (search and replace in all .swift, .hpp, .h files)
- Xcode project file (Template.xcodeproj → YourPluginName.xcodeproj)

### 3. Update Plugin Metadata

Edit `TemplateExtension/Info.plist`:
- Change `name` to "Taylor Audio: YourPluginName"
- Change `description` to "YourPluginName"
- Change `subtype` to a unique 4-character code (e.g., "ypln")

### 4. Update Parameters

Edit `TemplateExtension/Parameters/Parameters.swift`:
- Modify parameter specs (name, range, default, units)

Edit `TemplateExtension/Parameters/TemplateExtensionParameterAddresses.h`:
- Update parameter enum to match your parameters

### 5. Update DSP Processing

Edit `TemplateExtension/DSP/TemplateExtensionDSPKernel.hpp`:
- Implement your custom DSP algorithm in the `process()` method
- Update `setParameter()` and `getParameter()` to handle your parameters
- Modify member variables as needed

### 6. Update UI

Edit `TemplateExtension/UI/TemplateExtensionMainView.swift`:
- Add/remove knobs and controls as needed
- Bind controls to your parameters from `parameterTree.global.yourparam`

### 7. Build and Test

```bash
./build.sh
```

Then load in Logic Pro and test!

## Current Template Configuration

**Parameter:** Tube Gain
- Range: 0.0 - 2.0
- Default: 1.0
- Unit: Linear Gain

**Subtype:** `tmpl`
**Manufacturer:** `TyAu` (1954115685)

## Notes

- Always use unique 4-character subtype codes
- Avoid Apple's reserved codes: `gain`, `dist`, `dely`, `revb`, `comp`, `filt`
- Keep parameter identifiers lowercase and simple
- Test in both Logic Pro and GarageBand
- Run `auval -v aufx SUBTYPE TyAu` to validate your plugin

## File Structure

```
TyAu-Template/
├── Template/                     # Host app
│   ├── TemplateApp.swift
│   └── Template.entitlements
├── TemplateExtension/           # Audio Unit plugin
│   ├── Common/
│   │   └── Audio Unit/
│   │       └── TemplateExtensionAudioUnit.swift
│   ├── DSP/
│   │   └── TemplateExtensionDSPKernel.hpp
│   ├── Parameters/
│   │   ├── Parameters.swift
│   │   └── TemplateExtensionParameterAddresses.h
│   ├── UI/
│   │   ├── TemplateExtensionMainView.swift
│   │   ├── ParameterKnob.swift
│   │   ├── ParameterSlider.swift
│   │   └── BypassButton.swift
│   └── Info.plist
├── Template.xcodeproj
└── build.sh
```

Happy plugin building! 🎸

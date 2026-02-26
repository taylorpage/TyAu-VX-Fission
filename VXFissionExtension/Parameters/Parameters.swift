//
//  Parameters.swift
//  VXFissionExtension
//
//  Created by Taylor Page on 1/22/26.
//

import Foundation
import AudioToolbox

let VXFissionExtensionParameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
            address: .delayTime,
            identifier: "delayTime",
            name: "Delay",
            units: .milliseconds,
            valueRange: -50.0...50.0,
            defaultValue: 0.0
        )
        ParameterSpec(
            address: .bypass,
            identifier: "bypass",
            name: "Bypass",
            units: .boolean,
            valueRange: 0.0...1.0,
            defaultValue: 0.0
        )
    }
}

extension ParameterSpec {
    init(
        address: VXFissionExtensionParameterAddress,
        identifier: String,
        name: String,
        units: AudioUnitParameterUnit,
        valueRange: ClosedRange<AUValue>,
        defaultValue: AUValue,
        unitName: String? = nil,
        flags: AudioUnitParameterOptions = [AudioUnitParameterOptions.flag_IsWritable, AudioUnitParameterOptions.flag_IsReadable],
        valueStrings: [String]? = nil,
        dependentParameters: [NSNumber]? = nil
    ) {
        self.init(address: address.rawValue,
                  identifier: identifier,
                  name: name,
                  units: units,
                  valueRange: valueRange,
                  defaultValue: defaultValue,
                  unitName: unitName,
                  flags: flags,
                  valueStrings: valueStrings,
                  dependentParameters: dependentParameters)
    }
}

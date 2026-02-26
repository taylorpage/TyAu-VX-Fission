//
//  VXFissionExtensionParameterAddresses.h
//  VXFissionExtension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, VXFissionExtensionParameterAddress) {
    delayTime = 0,
    bypass    = 1
};

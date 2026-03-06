//
//  VXFissionExtensionMainView.swift
//  VXFissionExtension
//
//  Created by Taylor Page on 1/22/26.
//

import SwiftUI

private class BundleToken {}
private let extensionBundle = Bundle(for: BundleToken.self)

struct VXFissionExtensionMainView: View {
    var parameterTree: ObservableAUParameterGroup

    var body: some View {
        ZStack {
            // Background
            if let bgImage = NSImage(named: "backgroundSquare") {
                Image(nsImage: bgImage)
                    .resizable()
                    .aspectRatio(contentMode: .fill)
                    .clipShape(RoundedRectangle(cornerRadius: 8))
                    .shadow(color: .black.opacity(0.1), radius: 4, x: 0, y: 2)
            }

            // Delay knob centered (offset down)
            ParameterKnob(param: parameterTree.global.delayTime)
                .padding(.top, 60)

            // Title top-center
            VStack {
                Text("VX-FISSION")
                    .font(.custom("Jackwrite-Bold", size: 22))
                    .foregroundColor(Color(red: 0.878, green: 0.867, blue: 0.800))
                    .tracking(5)
                    .shadow(color: .black.opacity(0.55), radius: 1, x: 1, y: 1)
                    .padding(.top, 12)
                Spacer()
            }

            // LED meter top-center
            VStack {
                LEDMeter(
                    delayParam: parameterTree.global.delayTime,
                    bypassParam: parameterTree.global.bypass
                )
                .padding(.top, 50)
Spacer()
            }

            // Reset button bottom-right
            VStack {
                Spacer()
                HStack {
                    Spacer()
                    ResetButton(param: parameterTree.global.delayTime)
                        .padding(.trailing, 17)
                        .padding(.bottom, 47)
                }
            }

            // Logo bottom-center
            VStack {
                Spacer()
                Group {
                    if let path = extensionBundle.path(forResource: "TaylorAudio", ofType: "png"),
                       let logoImage = NSImage(contentsOfFile: path) {
                        Image(nsImage: logoImage)
                            .resizable()
                            .scaledToFit()
                            .frame(height: 34)
                            .opacity(0.80)
                    }
                }
                .padding(.bottom, 4)
            }
        }
        .frame(width: 360, height: 360)
    }
}

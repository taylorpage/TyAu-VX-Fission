//
//  VXFissionExtensionMainView.swift
//  VXFissionExtension
//
//  Created by Taylor Page on 1/22/26.
//

import SwiftUI

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
                    .foregroundColor(.white)
                    .shadow(color: .black.opacity(0.6), radius: 2, x: 0, y: 1)
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
                        .padding(.bottom, 37)
                }
            }

            // Logo bottom-left
            VStack {
                Spacer()
                HStack {
                    if let logoImage = NSImage(named: "TaylorAudio") {
                        Image(nsImage: logoImage)
                            .renderingMode(.template)
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 80)
                            .foregroundColor(.gray)
                            .opacity(0.7)
                    }
                    Spacer()
                }
                .padding(.leading, 12)
                .padding(.bottom, 12)
            }
        }
        .frame(width: 360, height: 360)
    }
}

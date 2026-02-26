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
            // Clean light grey background
            RoundedRectangle(cornerRadius: 8)
                .fill(Color(white: 0.9))
                .shadow(color: .black.opacity(0.1), radius: 4, x: 0, y: 2)

            // Output jack (left side) with vertical label
            HStack {
                HStack(spacing: 4) {
                    if let jackImage = NSImage(named: "jack") {
                        Image(nsImage: jackImage)
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 20)
                    }
                    Text("OUTPUT")
                        .font(.system(size: 9, weight: .bold))
                        .foregroundColor(.black)
                        .rotationEffect(.degrees(-90))
                        .fixedSize()
                }
                .offset(x: -21)
                Spacer()
            }
            .padding(.top, 20)

            // Input jack (right side) with vertical label
            HStack {
                Spacer()
                HStack(spacing: 4) {
                    Text("INPUT")
                        .font(.system(size: 9, weight: .bold))
                        .foregroundColor(.black)
                        .rotationEffect(.degrees(-90))
                        .fixedSize()
                        .offset(x: -3)
                    if let jackImage = NSImage(named: "jack") {
                        Image(nsImage: jackImage)
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 20)
                            .scaleEffect(x: -1, y: 1)
                            .offset(x: 3)
                    }
                }
                .offset(x: 18)
            }
            .padding(.top, 20)

            VStack(spacing: 20) {
                Spacer()
                    .frame(height: 40)

                // LED indicator
                ZStack {
                    // Main LED body
                    Circle()
                        .fill(param.boolValue ? Color(red: 0.3, green: 0.35, blue: 0.32) : Color.green)
                        .frame(width: 20, height: 20)

                    // Dark bezel/rim
                    Circle()
                        .stroke(Color.black.opacity(0.6), lineWidth: 2)
                        .frame(width: 20, height: 20)

                    // Center glow (when on)
                    Circle()
                        .fill(
                            RadialGradient(
                                gradient: Gradient(colors: [
                                    param.boolValue ? Color.clear : Color.white.opacity(0.6),
                                    Color.clear
                                ]),
                                center: .center,
                                startRadius: 0,
                                endRadius: 10
                            )
                        )
                        .frame(width: 20, height: 20)

                    // Glass reflection (top-left highlight)
                    Circle()
                        .fill(
                            LinearGradient(
                                gradient: Gradient(stops: [
                                    .init(color: Color.white.opacity(0.5), location: 0.0),
                                    .init(color: Color.white.opacity(0.2), location: 0.3),
                                    .init(color: Color.clear, location: 0.6)
                                ]),
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            )
                        )
                        .frame(width: 20, height: 20)
                        .mask(
                            Circle()
                                .frame(width: 8, height: 8)
                                .offset(x: -3, y: -3)
                        )
                }
                .shadow(
                    color: param.boolValue ? .clear : .green.opacity(0.8),
                    radius: param.boolValue ? 0 : 6,
                    x: 0,
                    y: 0
                )

                // Tube Gain knob
                ParameterKnob(param: parameterTree.global.tubegain)

                Spacer()

                // Centered stomp switch
                BypassButton(param: parameterTree.global.bypass)
                    .padding(.bottom, 40)
            }
            .padding(.horizontal, 32)

            // TaylorAudio logo (bottom left corner)
            VStack {
                Spacer()
                HStack {
                    if let logoImage = NSImage(named: "TaylorAudio") {
                        Image(nsImage: logoImage)
                            .renderingMode(.template)
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 100)
                            .foregroundColor(.gray)
                            .opacity(0.7)
                    }
                    Spacer()
                }
                .padding(.leading, 12)
                .padding(.bottom, 12)
            }
        }
        .frame(width: 280, height: 600)
    }

    var param: ObservableAUParameter {
        parameterTree.global.bypass
    }
}

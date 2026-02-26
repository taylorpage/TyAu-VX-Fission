//
//  LEDMeter.swift
//  VXFissionExtension
//
//  A horizontal LED meter that lights up based on delay amount.
//  Layout: [R] [Y] [G] [G] [G] [B] [G] [G] [G] [Y] [R]
//  Blue center = plugin active. LEDs fill outward as gain increases.
//

import SwiftUI

struct LEDMeter: View {
    let delayParam: ObservableAUParameter
    let bypassParam: ObservableAUParameter

    private struct LEDSpec {
        let distance: Int   // distance from center; 0 = blue, 1–3 = green, 4 = yellow, 5 = red
        let color: Color
    }

    private let specs: [LEDSpec] = [
        LEDSpec(distance: 5, color: .red),
        LEDSpec(distance: 4, color: Color(red: 1.0, green: 0.8, blue: 0.0)),
        LEDSpec(distance: 3, color: .green),
        LEDSpec(distance: 2, color: .green),
        LEDSpec(distance: 1, color: .green),
        LEDSpec(distance: 0, color: Color(red: 0.2, green: 0.5, blue: 1.0)),
        LEDSpec(distance: 1, color: .green),
        LEDSpec(distance: 2, color: .green),
        LEDSpec(distance: 3, color: .green),
        LEDSpec(distance: 4, color: Color(red: 1.0, green: 0.8, blue: 0.0)),
        LEDSpec(distance: 5, color: .red),
    ]

    // Plugin is active when not bypassed
    var isActive: Bool { !bypassParam.boolValue }

    // Maps abs(delay time) to a level 0–5; center (0 ms) = 0, full deflection = 5
    var level: Int {
        let absMs = Double(abs(delayParam.value))
        let maxMs = Double(delayParam.max) // 50.0
        return min(Int((absMs / maxMs) * 5.0), 5)
    }

    private let imageWidth: CGFloat = 300
    private let imageHeight: CGFloat = 44
    private let ledSize: CGFloat = 16
    private let ledSpacing: CGFloat = 8.25
    private let ledOffsetY: CGFloat = -3

    var body: some View {
        ZStack {
            // Border chassis behind the LEDs
            if let borderImage = NSImage(named: "ledBorder") {
                Image(nsImage: borderImage)
                    .resizable()
                    .frame(width: imageWidth, height: imageHeight)
            }

            // LEDs centered over the holes
            HStack(spacing: ledSpacing) {
                ForEach(0..<11, id: \.self) { i in
                    let spec = specs[i]
                    let isCenter = spec.distance == 0
                    let isLit = isActive && (isCenter || level >= spec.distance)
                    ledView(color: spec.color, isLit: isLit)
                }
            }
            .offset(y: ledOffsetY)
        }
    }

    private func ledView(color: Color, isLit: Bool) -> some View {
        let inner = ledSize - 2.0

        return ZStack {
            // Dark socket base
            Circle()
                .fill(Color(white: 0.06))
                .frame(width: ledSize, height: ledSize)

            if isLit {
                // Emitter core — concentrated bright glow from within, slightly below center
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [
                                Color.white,
                                color,
                                color.opacity(0.5),
                                Color.clear
                            ],
                            center: UnitPoint(x: 0.5, y: 0.56),
                            startRadius: 0,
                            endRadius: inner * 0.52
                        )
                    )
                    .frame(width: inner, height: inner)

                // Glass body — mostly transparent, just a whisper of color
                Circle()
                    .fill(color.opacity(0.07))
                    .frame(width: inner, height: inner)

                // Primary specular — wide soft arc dominating upper dome
                Ellipse()
                    .fill(
                        LinearGradient(
                            colors: [Color.white.opacity(0.88), Color.white.opacity(0.0)],
                            startPoint: UnitPoint(x: 0.5, y: 0.0),
                            endPoint: UnitPoint(x: 0.5, y: 1.0)
                        )
                    )
                    .frame(width: inner * 0.75, height: inner * 0.42)
                    .offset(x: 0, y: -inner * 0.17)
                    .blur(radius: 0.7)

                // Secondary catchlight — tiny bright dot for realism
                Circle()
                    .fill(Color.white.opacity(0.92))
                    .frame(width: inner * 0.17, height: inner * 0.17)
                    .offset(x: -inner * 0.16, y: -inner * 0.23)
                    .blur(radius: 0.25)

                // Subtle color refraction at edge
                Circle()
                    .stroke(color.opacity(0.35), lineWidth: 1.0)
                    .frame(width: inner - 0.5, height: inner - 0.5)

            } else {
                // Unlit glass — nearly transparent, dark emitter visible through dome
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [Color(white: 0.13), Color(white: 0.04)],
                            center: UnitPoint(x: 0.5, y: 0.56),
                            startRadius: 0,
                            endRadius: inner / 2
                        )
                    )
                    .frame(width: inner, height: inner)

                // Barely-there color tint
                Circle()
                    .fill(color.opacity(0.06))
                    .frame(width: inner, height: inner)

                // Primary specular still present — glass is always glassy
                Ellipse()
                    .fill(Color.white.opacity(0.2))
                    .frame(width: inner * 0.68, height: inner * 0.36)
                    .offset(x: 0, y: -inner * 0.16)
                    .blur(radius: 0.9)

                // Faint catchlight
                Circle()
                    .fill(Color.white.opacity(0.28))
                    .frame(width: inner * 0.15, height: inner * 0.15)
                    .offset(x: -inner * 0.15, y: -inner * 0.21)
                    .blur(radius: 0.25)
            }

            // Outer glass rim — gradient stroke: bright top-left, dark bottom-right
            Circle()
                .strokeBorder(
                    LinearGradient(
                        colors: [Color.white.opacity(0.3), Color.black.opacity(0.65)],
                        startPoint: UnitPoint(x: 0.25, y: 0.0),
                        endPoint: UnitPoint(x: 0.75, y: 1.0)
                    ),
                    lineWidth: 1.0
                )
                .frame(width: ledSize, height: ledSize)
        }
        // Tight bright core glow + wide soft colour halo
        .shadow(color: isLit ? color.opacity(0.95) : .clear, radius: 3)
        .shadow(color: isLit ? color.opacity(0.4) : .clear, radius: 9)
    }
}

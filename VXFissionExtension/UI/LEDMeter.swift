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

    // Maps delay time (0–500 ms) to a level 0–5
    var level: Int {
        let normalized = Double(delayParam.value - delayParam.min) / Double(delayParam.max - delayParam.min)
        return min(Int(normalized * 5.0), 5)
    }

    var body: some View {
        HStack(spacing: 5) {
            ForEach(0..<11, id: \.self) { i in
                let spec = specs[i]
                let isCenter = spec.distance == 0
                let isLit = isActive && (isCenter || level >= spec.distance)
                ledView(color: spec.color, isLit: isLit)
            }
        }
    }

    private func ledView(color: Color, isLit: Bool) -> some View {
        ZStack {
            if isLit {
                // Lit: full color fill
                Circle()
                    .fill(color)
                    .frame(width: 14, height: 14)
                // Specular highlight
                Circle()
                    .fill(
                        RadialGradient(
                            gradient: Gradient(colors: [Color.white.opacity(0.55), Color.clear]),
                            center: .center,
                            startRadius: 0,
                            endRadius: 7
                        )
                    )
                    .frame(width: 14, height: 14)
            } else {
                // Unlit: very dark base with a faint color tint so it reads as a dark colored bulb
                Circle()
                    .fill(Color(white: 0.08))
                    .frame(width: 14, height: 14)
                Circle()
                    .fill(color.opacity(0.25))
                    .frame(width: 14, height: 14)
            }
            Circle()
                .stroke(Color.black.opacity(0.6), lineWidth: 1)
                .frame(width: 14, height: 14)
        }
        .shadow(color: isLit ? color.opacity(0.9) : .clear, radius: isLit ? 5 : 0)
    }
}

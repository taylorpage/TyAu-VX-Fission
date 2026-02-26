//
//  ChannelToggle.swift
//  VXFissionExtension
//
//  Toggle that selects which stereo channel receives the delay.
//  param value: 0.0 = Left delayed, 1.0 = Right delayed
//

import SwiftUI

struct ChannelToggle: View {
    @State var param: ObservableAUParameter

    var isRightDelayed: Bool { param.boolValue }

    var body: some View {
        HStack(spacing: 0) {
            channelButton(label: "L", isSelected: !isRightDelayed)
            channelButton(label: "R", isSelected:  isRightDelayed)
        }
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color(white: 0.25))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .strokeBorder(Color(white: 0.15), lineWidth: 1)
        )
    }

    private func channelButton(label: String, isSelected: Bool) -> some View {
        Button {
            param.onEditingChanged(true)
            param.boolValue = (label == "R")
            param.onEditingChanged(false)
        } label: {
            Text(label)
                .font(.system(size: 13, weight: .bold, design: .monospaced))
                .foregroundColor(isSelected ? .white : Color(white: 0.5))
                .frame(width: 40, height: 30)
                .background(
                    RoundedRectangle(cornerRadius: 6)
                        .fill(isSelected ? Color.orange : Color.clear)
                        .padding(2)
                )
        }
        .buttonStyle(.plain)
    }
}

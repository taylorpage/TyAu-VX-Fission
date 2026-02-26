//
//  ResetButton.swift
//  VXFissionExtension
//
//  Tapping this resets the bound parameter to 0.
//

import SwiftUI

struct ResetButton: View {
    @State var param: ObservableAUParameter

    var body: some View {
        VStack(spacing: 4) {
            if let image = NSImage(named: "redButton") {
                Image(nsImage: image)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: 52, height: 52)
                    .shadow(color: .black.opacity(0.4), radius: 3, x: 0, y: 2)
                    .onTapGesture {
                        param.onEditingChanged(true)
                        param.value = 0.0
                        param.onEditingChanged(false)
                    }
            }
            Text("Reset")
                .font(.system(size: 9, weight: .medium))
                .foregroundColor(.gray)
        }
        .accessibility(label: Text("Reset"))
    }
}

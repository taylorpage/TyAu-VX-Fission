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
            Text("RESET")
                .font(.custom("Jackwrite-Bold", size: 12))
                .foregroundColor(Color(red: 0.878, green: 0.867, blue: 0.800))
                .tracking(1.5)
            if let image = NSImage(named: "rustedRedButton") {
                Image(nsImage: image)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: 36, height: 36)
                    .shadow(color: .black.opacity(0.4), radius: 3, x: 0, y: 2)
                    .onTapGesture {
                        param.onEditingChanged(true)
                        param.value = 0.0
                        param.onEditingChanged(false)
                    }
            }
        }
        .accessibility(label: Text("Reset"))
    }
}

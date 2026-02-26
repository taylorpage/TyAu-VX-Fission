//
//  VXFissionApp.swift
//  VXFission
//
//  Created by Taylor Page on 1/22/26.
//

import SwiftUI

@main
struct VXFissionApp: App {
    private let hostModel = AudioUnitHostModel()

    var body: some Scene {
        WindowGroup {
            ContentView(hostModel: hostModel)
        }
    }
}

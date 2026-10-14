import SwiftUI

@main
struct HamClockApp: App {
    @StateObject private var weather = SpaceWeatherStore()

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .environmentObject(weather)
                .task { await weather.run() }
        }
    }
}

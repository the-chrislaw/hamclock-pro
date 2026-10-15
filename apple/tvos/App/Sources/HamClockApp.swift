import SwiftUI

@main
struct HamClockApp: App {
    @StateObject private var weather = SpaceWeatherStore()
    @StateObject private var station = StationStore()

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .environmentObject(weather)
                .environmentObject(station)
                .task { await weather.run() }
        }
    }
}

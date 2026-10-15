import SwiftUI

@main
struct HamClockApp: App {
    @StateObject private var weather = SpaceWeatherStore()
    @StateObject private var quakes = QuakeStore()
    @StateObject private var station = StationStore()

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .environmentObject(weather)
                .environmentObject(station)
                .environmentObject(quakes)
                .task { await weather.run() }
                .task { await quakes.run() }
        }
    }
}

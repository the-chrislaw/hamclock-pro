import SwiftUI

@main
struct HamClockApp: App {
    @StateObject private var weather = SpaceWeatherStore()
    @StateObject private var quakes = QuakeStore()
    @StateObject private var sats = SatelliteStore()
    @StateObject private var station = StationStore()

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .environmentObject(weather)
                .environmentObject(station)
                .environmentObject(quakes)
                .environmentObject(sats)
                .task { await weather.run() }
                .task { await quakes.run() }
                .task { await sats.run() }
        }
    }
}

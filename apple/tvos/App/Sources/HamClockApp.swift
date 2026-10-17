import SwiftUI

@main
struct HamClockApp: App {
    @StateObject private var weather = SpaceWeatherStore()
    @StateObject private var quakes = QuakeStore()
    @StateObject private var localWx = LocalWeatherStore()
    @StateObject private var sats = SatelliteStore()
    @StateObject private var station = StationStore()
    @StateObject private var panes = PaneStore()

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .environmentObject(weather)
                .environmentObject(station)
                .environmentObject(quakes)
                .environmentObject(sats)
                .environmentObject(localWx)
                .environmentObject(panes)
                .task { await weather.run() }
                .task { await quakes.run() }
                .task { await sats.run() }
                .task(id: station.locator) { await localWx.run(station: station) }
        }
    }
}

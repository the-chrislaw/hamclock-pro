import SwiftUI

@main
struct HamClockApp: App {
    @StateObject private var settings = AppSettings()
    @StateObject private var feeds = Feeds()
    @StateObject private var sats = SatelliteStore()
    @StateObject private var localWx = LocalWeatherStore()

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .environmentObject(settings)
                .environmentObject(feeds)
                .environmentObject(sats)
                .environmentObject(localWx)
                .task { await feeds.run() }
                .task { await sats.run() }
                .task(id: settings.de.map { "\($0.lat),\($0.lon)" } ?? "") { await localWx.run(position: settings.de) }
        }
    }
}

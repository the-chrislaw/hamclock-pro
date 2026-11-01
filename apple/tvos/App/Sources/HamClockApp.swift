import SwiftUI

@main
struct HamClockApp: App {
    @StateObject private var settings = AppSettings()
    @StateObject private var feeds = Feeds()
    @StateObject private var sats = SatelliteStore()
    @StateObject private var localWx = LocalWeatherStore()
    @StateObject private var satAlerts = SatAlertStore()

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .environmentObject(settings)
                .environmentObject(feeds)
                .environmentObject(sats)
                .environmentObject(localWx)
                .environmentObject(satAlerts)
                .task { await feeds.run() }
                .task { await sats.run() }
                .task(id: settings.de.map { "\($0.lat),\($0.lon)" } ?? "") { await localWx.run(position: settings.de) }
                .task(id: (settings.de.map { "\($0.lat),\($0.lon)" } ?? "") + "/\(sats.fetched?.timeIntervalSince1970 ?? 0)") {
                    await sats.track(position: settings.de)
                }
                // Pass alerts: re-predicted when the elements, the station or the choices change.
                .task(id: "\(settings.de.map { "\($0.lat),\($0.lon)" } ?? "")/\(sats.fetched?.timeIntervalSince1970 ?? 0)/\(String(describing: settings.satAlerts))") {
                    await satAlerts.track(tle: sats.tle, position: settings.de, prefs: settings.satAlerts)
                }
        }
    }
}

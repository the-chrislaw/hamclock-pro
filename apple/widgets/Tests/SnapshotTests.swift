import SwiftUI
import WidgetKit
import XCTest

/// Renders every widget view at its family's point size to PNG with ImageRenderer.
/// This checks layout and legibility of our SwiftUI views; it is not the system's widget
/// host (no Home Screen, StandBy or CarPlay chrome, no tinting). PNGs go to $SNAPSHOT_DIR
/// (CI sets TEST_RUNNER_SNAPSHOT_DIR) or the temporary directory, and are attached to the result bundle.
@MainActor
final class SnapshotTests: XCTestCase {
    /// iPhone 16/17 Pro widget sizes in points.
    static let sizes: [(WidgetFamily, CGSize, String)] = [
        (.systemSmall, CGSize(width: 170, height: 170), "small"),
        (.systemMedium, CGSize(width: 364, height: 170), "medium"),
        (.accessoryRectangular, CGSize(width: 172, height: 76), "rectangular"),
        (.accessoryCircular, CGSize(width: 76, height: 76), "circular"),
    ]

    private var outDir: URL {
        let env = ProcessInfo.processInfo.environment["SNAPSHOT_DIR"]
        let dir = env.map { URL(fileURLWithPath: $0, isDirectory: true) }
            ?? FileManager.default.temporaryDirectory.appendingPathComponent("widget-snapshots", isDirectory: true)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir
    }

    private func render<V: View>(_ view: V, family: WidgetFamily, size: CGSize, name: String) throws {
        let accessory = [.accessoryRectangular, .accessoryCircular, .accessoryInline].contains(family)
        let card = view
            .environment(\.colorScheme, .dark)
            .foregroundStyle(.white)
            .padding(accessory ? 2 : 14)
            .frame(width: size.width, height: size.height)
            .background(accessory ? Color(white: 0.2) : Style.background)
            .clipShape(RoundedRectangle(cornerRadius: accessory ? 12 : 22, style: .continuous))
        let renderer = ImageRenderer(content: card)
        renderer.scale = 3
        let png = try XCTUnwrap(renderer.uiImage?.pngData(), "render \(name)")
        XCTAssertGreaterThan(png.count, 1000, name)
        try png.write(to: outDir.appendingPathComponent(name + ".png"))
        let attachment = XCTAttachment(data: png, uniformTypeIdentifier: "public.png")
        attachment.name = name
        attachment.lifetime = .keepAlways
        add(attachment)
    }

    func testSpaceWeather() throws {
        for (family, size, label) in Self.sizes {
            try render(SpaceWeatherView(entry: Samples.spaceWeatherEntry, family: family), family: family, size: size,
                       name: "spaceweather-\(label)")
        }
        var offline = Samples.weather
        offline.lastAttemptFailed = true
        let old = SpaceWeatherEntry(date: Samples.now.addingTimeInterval(3 * 3600), weather: offline)
        try render(SpaceWeatherView(entry: old, family: .systemSmall), family: .systemSmall,
                   size: Self.sizes[0].1, name: "spaceweather-small-offline")
        XCTAssertEqual(SpaceWeatherView(entry: Samples.spaceWeatherEntry, family: .accessoryInline).inlineText,
                       "Kp 2.3 · SFI 148 · C2.4")
    }

    func testBands() throws {
        for (family, size, label) in Self.sizes where family != .accessoryCircular {
            try render(BandsView(entry: Samples.bandsEntry, family: family), family: family, size: size,
                       name: "bands-\(label)")
        }
        let empty = BandsEntry(date: Samples.now, weather: SpaceWeatherSnapshot(), station: nil)
        try render(BandsView(entry: empty, family: .systemSmall), family: .systemSmall, size: Self.sizes[0].1,
                   name: "bands-small-nodata")
    }

    func testPass() throws {
        // The countdown is a system timer against the real clock, so anchor this entry at now.
        let now = Date()
        var soon = Samples.pass
        soon.aos = now + 23 * 60; soon.tca = now + 28 * 60; soon.los = now + 33 * 60
        let soonEntry = PassEntry(date: now, state: .pass(soon, elements: .live, elementsOld: false), callsign: "N0CALL")
        for (family, size, label) in Self.sizes where family != .accessoryCircular {
            try render(PassView(entry: soonEntry, family: family), family: family, size: size, name: "pass-\(label)")
        }
        var later = soon
        later.aos = now + 5 * 3600; later.tca = later.aos + 300; later.los = later.aos + 600
        let laterEntry = PassEntry(date: now, state: .pass(later, elements: .live, elementsOld: true), callsign: "N0CALL")
        try render(PassView(entry: laterEntry, family: .systemSmall), family: .systemSmall, size: Self.sizes[0].1,
                   name: "pass-small-later-oldelements")
        let up = PassEntry(date: Samples.pass.tca, state: Samples.passEntry.state, callsign: "N0CALL")
        try render(PassView(entry: up, family: .systemSmall), family: .systemSmall, size: Self.sizes[0].1,
                   name: "pass-small-upnow")
        let unset = PassEntry(date: Samples.now, state: .needsStation, callsign: "")
        try render(PassView(entry: unset, family: .systemSmall), family: .systemSmall, size: Self.sizes[0].1,
                   name: "pass-small-nostation")
        try render(PassView(entry: unset, family: .accessoryRectangular), family: .accessoryRectangular,
                   size: Self.sizes[2].1, name: "pass-rectangular-nostation")
    }
}

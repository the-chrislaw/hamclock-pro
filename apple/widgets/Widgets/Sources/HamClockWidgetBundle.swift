import SwiftUI
import WidgetKit

/// HamClock Pro widgets: Home Screen, Lock Screen and StandBy; the systemSmall size also
/// appears on the CarPlay dashboard (iOS 26+), which is why none of them disfavours CarPlay.
@main
struct HamClockWidgetBundle: WidgetBundle {
    var body: some Widget {
        BandsWidget()
        SpaceWeatherWidget()
        PassWidget()
    }
}

// "UDP sets DX": session-only DX override from UDP spots, offline.
#include "../dx_cluster_model.hpp"
#include "../udp_spot.hpp"
#include "../../../net/telnet_client.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
class FakeSocket : public hamclock::TelnetSocket {
public:
    using TelnetSocket::TelnetSocket;
    void connectToHost(const QString&, int) override {}
    void write(const QByteArray&) override {}
    void close() override {}
};
bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }
void wait_ms(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    using namespace hamclock::dxcluster;

    const auto c4 = locator_centre("FN31");
    check(c4 && near(c4->first, 41.5) && near(c4->second, -73.0), "4-char centre");
    const auto c6 = locator_centre("FN31pr");
    check(c6 && c6->first > 41.7 && c6->first < 41.8 && c6->second < -72.6 && c6->second > -72.8, "6-char centre");
    check(!locator_centre("") && !locator_centre("FN") && !locator_centre("ZZ99") && !locator_centre("FN31p"), "bad locators refused");

    QTemporaryDir tmp;
    const QString station = tmp.filePath("station.json");
    AppSettings settings(station, tmp.path());
    check(!settings.setDxOverride(10, 10), "unconfigured station refuses an override");
    check(settings.saveStation("W1AW", 41.7, -72.7, 51.5, -0.1, "").isEmpty(), "station saved");

    check(settings.setDxOverride(35.5, 139.0) && settings.dxOverrideActive() &&
              near(settings.dxLatitude(), 35.5) && near(settings.dxLongitude(), 139.0), "override shows");
    check(!settings.setDxOverride(95, 0) && near(settings.dxLatitude(), 35.5), "invalid override refused");
    // Saving DE with the displayed DX must not persist the override.
    check(settings.saveStation("W1AW", 40.0, -70.0, settings.dxLatitude(), settings.dxLongitude(), "").isEmpty(), "set DE while overridden");
    check(settings.dxOverrideActive() && near(settings.dxLatitude(), 35.5), "override survives a DE save");
    {
        AppSettings fresh(station, tmp.path());
        check(near(fresh.dxLatitude(), 51.5) && near(fresh.dxLongitude(), -0.1) && near(fresh.deLatitude(), 40.0), "file holds the real DX");
    }
    settings.clearDxOverride();
    check(!settings.dxOverrideActive() && near(settings.dxLatitude(), 51.5), "clear restores the saved DX");
    settings.setDxOverride(1, 2);
    check(settings.saveStation("W1AW", 40.0, -70.0, 48.0, 2.0, "").isEmpty() && !settings.dxOverrideActive() &&
              near(settings.dxLatitude(), 48.0), "an explicit DX save replaces the override");

    // Model: off by default, only local-program spots with a locator, expires.
    WidgetContext ctx;
    ctx.settings = &settings;
    DxClusterModel model(ctx, [&](QObject* p) -> TelnetSocket* { return new FakeSocket(p); });
    DxSpot spot;
    spot.dx = "JA1XYZ"; spot.spotter = "WSJT-X"; spot.grid = "PM95";
    model.applyDxOverride(spot);
    check(!settings.dxOverrideActive(), "off by default");
    check(settings.saveUdpSetsDx(true).isEmpty() && settings.udpSetsDx(), "option saved");
    DxSpot foreign = spot; foreign.spotter = "W3LPL";
    model.applyDxOverride(foreign);
    check(!settings.dxOverrideActive(), "foreign spotter ignored");
    DxSpot nogrid = spot; nogrid.grid.clear();
    model.applyDxOverride(nogrid);
    check(!settings.dxOverrideActive(), "spot without locator ignored");
    model.setDxOverrideMs(50);
    model.applyDxOverride(spot);
    check(settings.dxOverrideActive() && near(settings.dxLatitude(), 35.5) && near(settings.dxLongitude(), 139.0), "local spot sets DX");
    wait_ms(200);
    check(!settings.dxOverrideActive() && near(settings.dxLatitude(), 48.0), "override expires");
    model.applyDxOverride(spot);
    settings.saveUdpSetsDx(false);
    check(!settings.dxOverrideActive(), "turning the option off clears it");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}

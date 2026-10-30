// Satellite alert service end to end on real CelesTrak elements (fixture) and
// the shipped catalog: prediction over DE, selection, persistence, the banner,
// cancel on disable, and the Passes pane's "Alerts" filter. Offline; the system
// notification centre is unavailable here, so the in-app path is exercised.
#include "../sat_alert_service.hpp"
#include "../passes_model.hpp"
#include "../../../domain/astro_transmitters.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTimeZone>
#include <iostream>

namespace {
using namespace hamclock;
int checks = 0, failures = 0;
void check(bool ok, const std::string& label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
QByteArray read_file(const char* path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
// W1AW, FN31pr (Newington, CT) — the test station, never a real home address.
constexpr double kLat = 41.7146, kLon = -72.7272;
constexpr int kIss = 25544;

bool only_selected(const SatAlertService& s) {
    for (const AlertPass& p : s.passes())
        if (!s.isSelected(p.norad)) return false;
    return true;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 3) { std::cerr << "usage: test CATALOG.json AMATEUR.tle\n"; return 2; }
    const SatCatalog catalog = parse_sat_catalog(read_file(argv[1])).catalog;
    const std::vector<Tle> tles = parse_tle_set(read_file(argv[2]).toStdString());
    check(!catalog.entries.isEmpty() && tles.size() > 50, "fixtures");
    const Tle* iss_tle = nullptr;
    for (const Tle& t : tles) if (tle_norad_id(t) == kIss) iss_tle = &t;
    check(iss_tle != nullptr, "ISS in fixture");
    if (!iss_tle) return 1;
    const qint64 epoch = static_cast<qint64>(SatellitePropagator(*iss_tle).epoch_unix());
    QDateTime now = QDateTime::fromSecsSinceEpoch(epoch + 3600, QTimeZone::utc());

    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    SatAlertService service(&settings, catalog);
    service.setClock([&] { return now; });
    service.setTimeZone(QTimeZone::utc());
    int chimes = 0;
    service.setChime([&chimes] { ++chimes; });
    service.setElements(tles);
    check(!service.enabled() && service.passes().empty() && service.plan().empty(), "off by default: nothing predicted");
    check(service.permission() == "unsupported" && !service.systemNotifications(), "no system centre in a test binary");

    service.setEnabled(true);
    check(service.passes().empty() && service.statusText().contains("station"), "no station: says so");
    check(settings.saveStation("W1AW", kLat, kLon, 51.5, -0.12, "").isEmpty(), "station saved");
    check(!service.passes().empty(), "station change re-predicts");
    check(only_selected(service), "only selected satellites predicted");
    bool high = true, future = true, ordered = true;
    for (std::size_t i = 0; i < service.passes().size(); ++i) {
        const AlertPass& p = service.passes()[i];
        high = high && p.max_el_deg >= kDefaultAlertMinElevation;
        future = future && p.los > now.toSecsSinceEpoch() && p.aos < now.toSecsSinceEpoch() + 49 * 3600;
        if (i) ordered = ordered && service.passes()[i - 1].aos <= p.aos;
    }
    check(high && future && ordered, "passes above the minimum, within 48 h, sorted");
    check(!service.plan().empty() && static_cast<int>(service.plan().size()) <= kMaxScheduledAlerts, "a plan within budget");
    check(!service.upcoming().isEmpty() && service.upcoming().size() <= 5, "next alerts listed for Settings");
    check(service.groups().size() >= 3 && service.selectedCount() > 5, "catalog grouped by mode, workable selected");

    bool iss_passes = false;
    for (const AlertPass& p : service.passes()) iss_passes = iss_passes || p.norad == kIss;
    check(iss_passes, "ISS passes over W1AW in 48 h");
    service.toggleSatellite(kIss);
    bool no_iss = true;
    for (const AlertPass& p : service.passes()) no_iss = no_iss && p.norad != kIss;
    check(!service.isSelected(kIss) && no_iss, "deselected ISS leaves the predictions");
    for (const PlannedAlert& a : service.plan()) no_iss = no_iss && a.norad != kIss;
    check(no_iss, "and the notification plan");

    const QVariantList others = service.others();
    check(!others.isEmpty(), "non-catalog TLE birds offered as other");
    const int other_norad = others.front().toMap().value("norad").toInt();
    check(catalog.find(other_norad) == nullptr && !service.isSelected(other_norad), "other bird not selected by default");
    service.toggleSatellite(other_norad);
    check(service.isSelected(other_norad), "other bird selectable");

    service.setMinElevation(60);
    bool sixty = true;
    for (const AlertPass& p : service.passes()) sixty = sixty && p.max_el_deg >= 60;
    check(sixty, "min elevation applies");
    service.setMinElevation(15);
    service.setLeadMinutes(10);
    service.setQuietHours(22, 7);
    service.setDaylightOnly(false);
    service.setSound(false);

    // Persistence: a new service (next launch) reads what was chosen.
    {
        SatAlertService again(&settings, catalog);
        again.setClock([&] { return now; });
        check(again.enabled() && again.leadMinutes() == 10 && again.quietStart() == 22 && again.quietEnd() == 7 &&
              !again.sound() && !again.isSelected(kIss) && again.isSelected(other_norad), "choices persist");
    }

    // Banner: move the clock to just before a selected pass.
    service.toggleSatellite(kIss);
    service.setQuietHours(-1, -1);
    check(!service.passes().empty(), "passes again");
    const AlertPass first = service.passes().front();
    int raised = 0;
    QObject::connect(&service, &SatAlertService::alertRaised, [&raised] { ++raised; });
    now = QDateTime::fromSecsSinceEpoch(first.aos - 120, QTimeZone::utc());
    service.dismissBanner(); // nothing showing: harmless
    QMetaObject::invokeMethod(&service, "refreshSystemState");
    service.setLeadMinutes(5); // replans and updates the banner
    check(service.bannerId().endsWith(".soon") && service.bannerText().startsWith(first.name + " in 2 min"),
          "banner before AOS");
    check(raised == 1, "new banner raises an alert once");
    check(chimes == 0, "no chime with sound off");
    service.dismissBanner();
    check(service.bannerText().isEmpty(), "dismiss hides it");
    now = QDateTime::fromSecsSinceEpoch(first.aos + 30, QTimeZone::utc());
    service.setLeadMinutes(5);
    service.setSound(true); // any change re-evaluates the banner
    check(service.bannerLive() && service.bannerText().contains("up now"), "banner at AOS");
    const QString up_id = service.bannerId();
    now = now.addSecs(120);
    service.setMinElevation(16); // re-predicts with the pass already in progress
    service.setMinElevation(15);
    check(service.bannerId() == up_id, "a pass in progress keeps its id across predictions");

    // Passes pane "Alerts" filter shows only alert satellites.
    WidgetContext context;
    context.settings = &settings;
    context.sat_alerts = &service;
    PassesModel passes(context);
    passes.setClock([&] { return now; });
    std::set<int> amateur;
    passes.setElements(tles, amateur);
    passes.setFilter("alerts");
    bool filtered = !passes.rows().isEmpty();
    for (const QVariant& r : passes.rows()) filtered = filtered && r.toMap().value("alert").toBool();
    check(passes.filter() == "alerts" && filtered, "Passes pane Alerts filter");

    service.sendTestAlert();
    check(chimes >= 1, "chime with sound on");
    check(service.bannerText().startsWith("Test alert"), "test alert shows the banner without system notifications");
    service.setEnabled(false);
    check(service.plan().empty() && service.passes().empty() && service.bannerText().isEmpty(), "disable clears everything");
    passes.recompute();
    check(passes.rows().isEmpty(), "Alerts filter empty when alerts are off");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}

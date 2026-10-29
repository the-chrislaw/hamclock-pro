// Widget framework: PolledSource polling/backoff/cache, WidgetModel age and
// state mapping, WidgetRegistry ordering, AppSettings validation and the
// owner-only credentials file. Offline: all network access is faked.
#include "../src/net/polled_source.hpp"
#include "../src/ui/app_settings.hpp"
#include "../src/platform/credential_store.hpp"
#include "../src/widgets/widget_model.hpp"
#include "../src/widgets/widget_registry.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QTimer>
#include <iostream>
#include <stdexcept>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
template <class F> bool throws(F f) { try { f(); } catch (const std::invalid_argument&) { return true; } return false; }

struct FakeNet {
    hamclock::FetchResult next{200, "first", {}};
    int calls = 0;
    hamclock::Transport transport() {
        return [this](const QUrl&, hamclock::FetchDone done) {
            ++calls;
            auto result = next;
            QTimer::singleShot(0, [done, result] { done(result); });
        };
    }
};

class TestWidget : public hamclock::WidgetModel {
public:
    TestWidget(hamclock::PolledSource& source) : WidgetModel("test", "Test") { trackSource(source, "Test feed"); }
};

void spin(QCoreApplication& app) { for (int i = 0; i < 10; ++i) app.processEvents(); }
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("hamclock-pro-test");
    using namespace hamclock;
    using namespace std::chrono_literals;

    // Construction guards.
    FakeNet net;
    check(throws([&] { PolledSource s("Bad Name", QUrl("https://x.test/a"), {}, net.transport()); }), "unsafe cache name rejected");
    check(throws([&] { PolledSource s("ok", QUrl("http://x.test/a"), {}, net.transport()); }), "plain http rejected");
    check(throws([&] { PolledSource s("ok", QUrl("https://x.test/a"), {}, {}); }), "missing transport rejected");

    // Success, cache write, cache reload.
    const auto t0 = QDateTime(QDate(2026, 10, 13), QTime(12, 0), QTimeZone::utc());
    QFile::remove(PolledSource("core-a", QUrl("https://x.test/a"), {}, net.transport()).cachePath());
    {
        PolledSource source("core-a", QUrl("https://x.test/a"), {600s, 1800s, 3600s}, net.transport());
        source.setClock([&] { return t0; });
        int updates = 0; bool from_cache = true;
        QObject::connect(&source, &PolledSource::updated, [&](bool cached) { ++updates; from_cache = cached; });
        source.start();
        spin(app);
        check(updates == 1 && !from_cache && source.body() == "first", "fresh body delivered");
        check(source.fetchedAt() == t0 && !source.isStale(t0.addSecs(1800)) && source.isStale(t0.addSecs(1801)),
              "staleness follows policy");
        check(QFileInfo::exists(source.cachePath()), "body cached on disk");
        check(source.nextDelay() == 600s, "normal interval after success");

        // Failures back off 60, 120, 240 s ... capped by interval.
        net.next = {0, {}, "Host unreachable"};
        int failed = 0;
        QObject::connect(&source, &PolledSource::failed, [&](const QString&) { ++failed; });
        source.refreshNow(); spin(app);
        check(failed == 1 && source.nextDelay() == 60s && source.body() == "first", "first failure keeps body, retries in 60 s");
        source.refreshNow(); spin(app);
        check(source.nextDelay() == 120s, "second failure doubles");
        for (int i = 0; i < 6; ++i) { source.refreshNow(); spin(app); }
        check(source.nextDelay() == 600s, "backoff capped at interval");
        net.next = {503, "busy", {}};
        source.refreshNow(); spin(app);
        check(source.lastError() == "HTTP 503" && source.body() == "first", "HTTP errors never replace good data");
    }
    net.next = {0, {}, "offline"};
    {
        PolledSource source("core-a", QUrl("https://x.test/a"), {600s, 1800s, 3600s}, net.transport());
        bool cached = false;
        QObject::connect(&source, &PolledSource::updated, [&](bool from_cache) { cached = from_cache; });
        source.start(); spin(app);
        check(cached && source.body() == "first" && source.fetchedAt() == t0, "restart shows cached data with original time");
    }

    // Timer-driven refresh (issue #150): without refreshNow, the source must
    // keep re-fetching on its interval, and after failures too.
    net.next = {200, "tick", {}};
    {
        QFile::remove(PolledSource("core-t", QUrl("https://x.test/t"), {}, net.transport()).cachePath());
        PolledSource source("core-t", QUrl("https://x.test/t"), {1s, 1800s, 3600s}, net.transport());
        const int before = net.calls;
        int updates = 0;
        QObject::connect(&source, &PolledSource::updated, [&](bool) { ++updates; });
        source.start();
        QElapsedTimer wait; wait.start();
        while (wait.elapsed() < 3500) app.processEvents(QEventLoop::AllEvents, 50);
        check(net.calls - before >= 3 && updates >= 3, "timer re-fetches on its interval for several cycles");
    }

    // WidgetModel state mapping.
    net.next = {200, "x", {}};
    {
        PolledSource source("core-b", QUrl("https://x.test/b"), {600s, 1800s, 3600s}, net.transport());
        source.setClock([&] { return t0; });
        TestWidget widget(source);
        widget.setClock([&] { return t0.addSecs(120); });
        check(widget.state() == "loading", "initial state loading");
        source.start(); spin(app);
        check(widget.state() == "live" && widget.ageText() == "2 min ago", "fresh -> live with age");
        widget.setClock([&] { return t0.addSecs(4000); });
        net.next = {0, {}, "timeout"};
        source.refreshNow(); spin(app);
        check(widget.state() == "stale" && widget.statusText().contains("timeout"), "old + failing -> stale with reason");
    }
    check(WidgetModel::formatAge(t0, t0.addSecs(30)) == "just now" &&
          WidgetModel::formatAge(t0, t0.addSecs(3 * 3600)) == "3 h ago" &&
          WidgetModel::formatAge({}, t0).isEmpty(), "age formatting");

    // Panes tied to local station hardware or files are left out on iOS only.
    for (const char* id : {"radio", "rotator", "gps", "adsb", "sensors", "adif"}) {
        check(widget_available(id, false), "local-station pane offered on the Pi and Mac");
        check(!widget_available(id, true), "local-station pane left out on iOS");
    }
    check(widget_available("spaceweather", true) && widget_available("dxcluster", true), "network panes offered on iOS");
    check(!widget_available("radio", true) && widget_available("radio", false), "radio pane only off iOS");

    // Registry ordering and validation.
    {
        WidgetRegistry registry;
        PolledSource source("core-c", QUrl("https://x.test/c"), {}, net.transport());
        registry.add({"late", "Late", QUrl("qrc:/a.qml"), "strip", 90}, new TestWidget(source));
        registry.add({"early", "Early", QUrl("qrc:/b.qml"), "strip", 10}, new TestWidget(source));
        check(registry.rowCount() == 2 && registry.data(registry.index(0), WidgetRegistry::IdRole) == "early",
              "registry sorted by order");
        check(throws([&] { registry.add({"early", "Dup", QUrl("qrc:/c.qml")}, new TestWidget(source)); }), "duplicate id rejected");
        check(registry.model("late") != nullptr && registry.model("none") == nullptr, "lookup by id");
    }

    // Settings: validation and owner-only credentials.
    QTemporaryDir dir;
    {
        AppSettings settings({}, dir.path());
        check(!settings.configured(), "fresh install is unconfigured");
        check(!settings.saveStation("N0CALL", 95, 0, 0, 0, "").isEmpty(), "invalid latitude rejected");
        check(settings.saveStation("n0call", 37.77, -122.42, 51.5, -0.1, "America/Los_Angeles").isEmpty() &&
              settings.configured() && settings.callsign() == "N0CALL", "valid station saved, callsign uppercased");
        check(!settings.saveCluster("bad host;rm", 7300).isEmpty() && !settings.saveCluster("dx.example.org", 0).isEmpty(),
              "cluster host/port validated");
        check(settings.saveCluster("dx.example.org", 7373).isEmpty(), "cluster saved");
        check(!settings.saveAccount("qrz", "", "pw").isEmpty(), "account needs a user");
        check(settings.saveAccount("qrz", "N0CALL", "s3cret").isEmpty() && settings.hasLookupPassword(), "account saved");
        const auto perms = QFile::permissions(dir.filePath("credentials.ini"));
        check(perms == (QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadUser | QFileDevice::WriteUser) ||
              perms == (QFileDevice::ReadOwner | QFileDevice::WriteOwner), "credentials file is owner-only");
        check(settings.saveAccount("qrz", "N0CALL", "").isEmpty() && settings.lookupPassword() == "s3cret",
              "empty password keeps stored one");
    }
    {
        AppSettings reloaded({}, dir.path());
        check(reloaded.configured() && reloaded.deZone() == "America/Los_Angeles" && reloaded.clusterHost() == "dx.example.org" &&
              reloaded.clusterPort() == 7373 && reloaded.lookupService() == "qrz" && reloaded.lookupPassword() == "s3cret",
              "all settings survive restart");
        reloaded.clearAccount();
        check(!reloaded.hasLookupPassword() && reloaded.lookupService() == "none", "account cleared");
    }
    // With a secret store (the Keychain on Apple devices) passwords and keys never
    // reach credentials.ini, old ones are moved over, and each settings directory
    // keeps its own secrets.
    {
        struct MemoryStore final : CredentialStore {
            std::shared_ptr<QHash<QString, QString>> values = std::make_shared<QHash<QString, QString>>();
            bool refuse = false;
            bool set(const QString& k, const QString& v) override { if (refuse) return false; values->insert(k, v); return true; }
            QString get(const QString& k) const override { return values->value(k); }
            bool remove(const QString& k) override { if (refuse) return false; values->remove(k); return true; }
        };
        const auto stored = [](const QHash<QString, QString>& values, const QString& prefix) {
            for (auto it = values.begin(); it != values.end(); ++it)
                if (it.key().startsWith(prefix + "@")) return it.value();
            return QString();
        };
        const auto ini_text = [](const QTemporaryDir& d) {
            QFile f(d.filePath("credentials.ini"));
            return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
        };
        QTemporaryDir secret_dir;
        {
            AppSettings old_version({}, secret_dir.path());
            check(old_version.saveHamAlert("N0CALL", "alertpw").isEmpty(), "file-only HamAlert password saved");
        }
        auto store = std::make_unique<MemoryStore>();
        const auto values = store->values;
        AppSettings settings({}, secret_dir.path());
        settings.setSecretStore(std::move(store));
        check(stored(*values, "hamalert.password") == "alertpw" && settings.hasHamAlertPassword(), "old password moved to the secret store");
        check(!ini_text(secret_dir).contains("alertpw"), "moved password removed from credentials.ini");
        check(settings.saveAccount("hamqth", "N0CALL", "lookpw").isEmpty() && settings.saveHamsatKey("key123").isEmpty(), "accounts saved with a secret store");
        check(stored(*values, "lookup.password") == "lookpw" && stored(*values, "hamsat.key") == "key123", "secrets in the store");
        check(!ini_text(secret_dir).contains("lookpw") && !ini_text(secret_dir).contains("key123") && ini_text(secret_dir).contains("N0CALL"),
              "only user names in the file");
        check(settings.saveHamsatKey("").isEmpty() && stored(*values, "hamsat.key").isEmpty(), "empty key removes it from the store");

        // A second settings directory sharing the same store sees none of it.
        QTemporaryDir other_dir;
        auto shared = std::make_unique<MemoryStore>();
        shared->values = values;
        AppSettings other({}, other_dir.path());
        other.setSecretStore(std::move(shared));
        check(!other.hasHamAlertPassword() && !other.hasLookupPassword(), "another profile does not see these secrets");
        check(other.saveHamAlert("", "").isEmpty() && stored(*values, "hamalert.password") == "alertpw", "clearing another profile keeps these secrets");

        // A store that refuses leaves the file alone: nothing is lost.
        QTemporaryDir stuck_dir;
        {
            AppSettings old_version({}, stuck_dir.path());
            old_version.saveHamAlert("N0CALL", "keepme");
        }
        auto refusing = std::make_unique<MemoryStore>();
        refusing->refuse = true;
        AppSettings stuck({}, stuck_dir.path());
        stuck.setSecretStore(std::move(refusing));
        check(stuck.hamAlertPassword() == "keepme" && ini_text(stuck_dir).contains("keepme"), "unmigrated password still read from the file");
        check(!stuck.saveHamsatKey("k").isEmpty() && ini_text(stuck_dir).contains("keepme"), "failed store write keeps the file intact");
    }
    {
        QTemporaryDir ov_dir;
        AppSettings ov({}, ov_dir.path());
        check(ov.overscanPercent() == 0, "overscan off by default");
        check(!ov.saveOverscan(11).isEmpty() && !ov.saveOverscan(-1).isEmpty() && ov.overscanPercent() == 0, "bad overscan rejected");
        check(ov.saveOverscan(6).isEmpty() && ov.overscanPercent() == 6, "overscan applied");
        AppSettings ov2({}, ov_dir.path());
        check(ov2.overscanPercent() == 6, "overscan survives restart");
        check(ov2.textScalePercent() == 100, "text size normal by default");
        check(!ov2.saveTextScale(120).isEmpty() && ov2.textScalePercent() == 100, "bad text size rejected");
        check(ov2.saveTextScale(130).isEmpty() && ov2.textScalePercent() == 130, "text size applied");
        { AppSettings ov3({}, ov_dir.path()); check(ov3.textScalePercent() == 130, "text size survives restart"); }
        check(ov2.mapSize() == "large", "map large by default");
        check(!ov2.saveMapSize("huge").isEmpty() && ov2.mapSize() == "large", "bad map size rejected");
        const auto big_map = ov2.paneGrid(845, 590, 5, 250, 190);
        check(big_map["columns"].toInt() == 3 && big_map["rows"].toInt() == 1, "large map: one row of three on a 7-inch panel");
        check(ov2.saveMapSize("small").isEmpty() && ov2.mapSize() == "small", "map size applied");
        const auto small_map = ov2.paneGrid(845, 590, 5, 250, 190);
        check(small_map["columns"].toInt() == 3 && small_map["rows"].toInt() == 2, "small map: two rows on a 7-inch panel");
        { AppSettings ov3({}, ov_dir.path()); check(ov3.mapSize() == "small", "map size survives restart"); }
        check(ov2.orientationLock() == "auto", "rotation follows the device by default");
        check(!ov2.saveOrientationLock("sideways").isEmpty() && ov2.orientationLock() == "auto", "bad rotation lock rejected");
        check(ov2.saveOrientationLock("landscape").isEmpty() && ov2.orientationLock() == "landscape", "rotation lock applied");
        { AppSettings ov4({}, ov_dir.path()); check(ov4.orientationLock() == "landscape", "rotation lock survives restart"); }
        check(ov2.accentStyle() == 0, "accent blue by default");
        check(!ov2.saveAccentStyle(5).isEmpty() && ov2.accentStyle() == 0, "bad accent rejected");
        check(ov2.saveAccentStyle(3).isEmpty() && ov2.accentStyle() == 3, "accent applied");
        { AppSettings ov4({}, ov_dir.path()); check(ov4.accentStyle() == 3, "accent survives restart"); }
        check(ov2.bigClockFace() == 0, "big clock digital by default");
        check(!ov2.saveBigClockFace(2).isEmpty() && ov2.bigClockFace() == 0, "bad big clock face rejected");
        check(ov2.saveBigClockFace(1).isEmpty() && ov2.bigClockFace() == 1, "big clock analog applied");
        { AppSettings ov5({}, ov_dir.path()); check(ov5.bigClockFace() == 1, "big clock face survives restart"); }
        {
            AppSettings cs({}, ov_dir.path());
            int pane_signals = 0;
            QObject::connect(&cs, &AppSettings::panesChanged, [&] { ++pane_signals; });
            check(cs.setPane(1, "bands").isEmpty(), "pane set");
            check(cs.saveConfig("Panes").isEmpty(), "config saved");
            cs.setPane(1, "none");
            pane_signals = 0;
            check(cs.restoreConfig("Panes").isEmpty() && cs.panes()[1] == "bands" && pane_signals > 0,
                  "restoring a configuration re-applies panes and notifies");
        }
        check(ov2.blankIdleMinutes() == 0 && ov2.blankOffHour() == -1, "blanking off by default");
        check(!ov2.saveBlanking(5, 23, -1).isEmpty() && !ov2.saveBlanking(241, -1, -1).isEmpty() && ov2.blankIdleMinutes() == 0, "bad blanking rejected");
        check(ov2.saveBlanking(15, 23, 7).isEmpty() && ov2.blankIdleMinutes() == 15 && ov2.blankOnHour() == 7, "blanking applied");
        check(ov2.blankDays() == 0x7F && !ov2.saveBlankDays(128).isEmpty() && ov2.saveBlankDays(0x1F).isEmpty() && ov2.blankDays() == 0x1F, "blanking weekdays applied");
        AppSettings ov3({}, ov_dir.path());
        check(ov3.blankIdleMinutes() == 15 && ov3.blankOffHour() == 23, "blanking survives restart");
        check(ov3.blankDays() == 0x1F, "blanking weekdays survive restart");
    }
    {
        QTemporaryDir bt_dir;
        AppSettings bt({}, bt_dir.path());
        check(bt.bannerTitle().isEmpty(), "banner title off by default");
        check(bt.saveBannerTitle("  Club Station ").isEmpty() && bt.bannerTitle() == "Club Station", "banner title saved trimmed");
        AppSettings bt2({}, bt_dir.path());
        check(bt2.bannerTitle() == "Club Station", "banner title survives restart");
        check(bt.bannerBackground() == 0 && !bt.saveBannerBackground(9).isEmpty() && bt.bannerBackground() == 0, "bad banner background rejected");
        check(bt.saveBannerBackground(2).isEmpty() && AppSettings({}, bt_dir.path()).bannerBackground() == 2, "banner background persists");
        check(bt.dxZoneOverride().isEmpty() && !bt.saveDxZone("Not/AZone").isEmpty() && bt.dxZoneOverride().isEmpty(), "bad DX zone rejected");
        check(bt.saveDxZone("Asia/Tokyo").isEmpty() && AppSettings({}, bt_dir.path()).dxZoneOverride() == "Asia/Tokyo", "DX zone override persists");
        check(bt.saveDxZone("").isEmpty() && AppSettings({}, bt_dir.path()).dxZoneOverride().isEmpty(), "DX zone override cleared");
        check(bt2.saveBannerTitle("").isEmpty() && bt2.bannerTitle().isEmpty(), "banner title cleared");
    }
    {
        // Pane slots: defaults, validation, persistence.
        QTemporaryDir pane_dir;
        AppSettings panes({}, pane_dir.path());
        const auto defaults = panes.panes();
        check(defaults.mid(0, 3) == QStringList({"spaceweather", "solarwind", "dxcluster"}), "default pane arrangement");
        check(defaults.size() == AppSettings::kPaneSlots && defaults[3] == "bands", "every slot has a default widget");
        check(!panes.setPane(AppSettings::kPaneSlots, "weather").isEmpty() && !panes.setPane(-1, "weather").isEmpty(), "slot out of range rejected");
        check(!panes.setPane(0, "bad id;").isEmpty(), "malformed widget id rejected");
        check(panes.setPane(1, "weather").isEmpty() && panes.panes()[1] == "weather", "pane choice applied");
        check(panes.setPane(2, "none").isEmpty() && panes.panes()[2] == "none", "pane can be left empty");
        AppSettings again({}, pane_dir.path());
        check(again.panes().mid(0, 3) == QStringList({"spaceweather", "weather", "none"}), "pane choices survive restart");
        {
            // --pane (screenshots) shows a widget for this run only.
            AppSettings shown({}, pane_dir.path());
            check(shown.showPane(0, "quakes").isEmpty() && shown.panes()[0] == "quakes", "shown pane applied");
            check(!shown.showPane(AppSettings::kPaneSlots, "quakes").isEmpty(), "shown pane slot validated");
            check(AppSettings({}, pane_dir.path()).panes()[0] == "spaceweather", "shown pane not saved");
        }
        check(again.paneRotations() == QList<int>(AppSettings::kPaneSlots, 0), "rotation off by default");
        check(!again.setPaneRotation(AppSettings::kPaneSlots, 10).isEmpty() && !again.setPaneRotation(0, 7).isEmpty(), "bad rotation rejected");
        check(again.setPaneRotation(1, 30).isEmpty() && again.paneRotations().mid(0, 3) == QList<int>({0, 30, 0}), "rotation applied");
        check(!again.demoMode(), "demo mode off by default");
        again.setDemoMode(true);
        check(again.demoMode() && again.paneRotations().mid(0, 3) == QList<int>({0, 30, 0}), "demo mode leaves saved rotation alone");
        again.setDemoMode(false);
        AppSettings third({}, pane_dir.path());
        check(third.paneRotations().mid(0, 3) == QList<int>({0, 30, 0}), "rotation survives restart");
        check(again.paneRotationSets()[1].toStringList().isEmpty(), "rotation set empty by default (all widgets)");
        check(!again.togglePaneRotationMember(1, "none").isEmpty() && !again.togglePaneRotationMember(AppSettings::kPaneSlots, "weather").isEmpty() &&
                  !again.togglePaneRotationMember(1, "bad id;").isEmpty(), "bad rotation set member rejected");
        check(again.togglePaneRotationMember(1, "weather").isEmpty() && again.togglePaneRotationMember(1, "dxcluster").isEmpty() &&
                  again.paneRotationSets()[1].toStringList() == QStringList({"weather", "dxcluster"}), "rotation set built");
        AppSettings fourth({}, pane_dir.path());
        check(fourth.paneRotationSets()[1].toStringList() == QStringList({"weather", "dxcluster"}), "rotation set survives restart");
        check(again.togglePaneRotationMember(1, "weather").isEmpty() && again.paneRotationSets()[1].toStringList() == QStringList({"dxcluster"}), "toggle removes member");
        check(again.clearPaneRotationSet(1).isEmpty() && again.paneRotationSets()[1].toStringList().isEmpty(), "rotation set cleared");
        again.togglePaneRotationMember(0, "weather");
        again.resetPanes();
        check(again.paneRotationSets()[0].toStringList().isEmpty(), "reset clears rotation sets");
        check(again.panes() == defaults, "panes reset to default");
        check(again.paneRotations() == QList<int>(AppSettings::kPaneSlots, 0), "reset turns rotation off");
    }

    std::cout << "Widget core: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}

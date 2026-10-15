// Widget framework: PolledSource polling/backoff/cache, WidgetModel age and
// state mapping, WidgetRegistry ordering, AppSettings validation and the
// owner-only credentials file. Offline: all network access is faked.
#include "../src/net/polled_source.hpp"
#include "../src/ui/app_settings.hpp"
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
    {
        QTemporaryDir ov_dir;
        AppSettings ov({}, ov_dir.path());
        check(ov.overscanPercent() == 0, "overscan off by default");
        check(!ov.saveOverscan(11).isEmpty() && !ov.saveOverscan(-1).isEmpty() && ov.overscanPercent() == 0, "bad overscan rejected");
        check(ov.saveOverscan(6).isEmpty() && ov.overscanPercent() == 6, "overscan applied");
        AppSettings ov2({}, ov_dir.path());
        check(ov2.overscanPercent() == 6, "overscan survives restart");
    }
    {
        // Pane slots: defaults, validation, persistence.
        QTemporaryDir pane_dir;
        AppSettings panes({}, pane_dir.path());
        const auto defaults = panes.panes();
        check(defaults == QStringList({"spaceweather", "solarwind", "dxcluster"}), "default pane arrangement");
        check(!panes.setPane(3, "weather").isEmpty() && !panes.setPane(-1, "weather").isEmpty(), "slot out of range rejected");
        check(!panes.setPane(0, "bad id;").isEmpty(), "malformed widget id rejected");
        check(panes.setPane(1, "weather").isEmpty() && panes.panes()[1] == "weather", "pane choice applied");
        check(panes.setPane(2, "none").isEmpty() && panes.panes()[2] == "none", "pane can be left empty");
        AppSettings again({}, pane_dir.path());
        check(again.panes() == QStringList({"spaceweather", "weather", "none"}), "pane choices survive restart");
        check(again.paneRotations() == QList<int>({0, 0, 0}), "rotation off by default");
        check(!again.setPaneRotation(3, 10).isEmpty() && !again.setPaneRotation(0, 7).isEmpty(), "bad rotation rejected");
        check(again.setPaneRotation(1, 30).isEmpty() && again.paneRotations() == QList<int>({0, 30, 0}), "rotation applied");
        AppSettings third({}, pane_dir.path());
        check(third.paneRotations() == QList<int>({0, 30, 0}), "rotation survives restart");
        check(again.paneRotationSets()[1].toStringList().isEmpty(), "rotation set empty by default (all widgets)");
        check(!again.togglePaneRotationMember(1, "none").isEmpty() && !again.togglePaneRotationMember(5, "weather").isEmpty() &&
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
        check(again.paneRotations() == QList<int>({0, 0, 0}), "reset turns rotation off");
    }

    std::cout << "Widget core: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}

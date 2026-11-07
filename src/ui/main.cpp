// HamClock Pro main application (Qt Quick). Legacy Qt Widgets programs remain
// available while their screens are ported.
#include "app_settings.hpp"
#include "map_item.hpp"
#include "station_model.hpp"
#include "pane_fill.hpp"
#include "alarms_model.hpp"
#include "blanking_model.hpp"
#include "key_injector.hpp"
#include "timers_model.hpp"
#include "time_zone_search.hpp"
#include "location_tools.hpp"
#include "window_geometry.hpp"
#include "../net/polled_source.hpp"
#include "../net/telnet_client.hpp"
#include "../widgets/widget_registry.hpp"
#include "../widgets/satellites/sat_alert_service.hpp"
#include "../widgets/satellites/satellites_model.hpp"
#include "../platform/apple_lifecycle.hpp"
#include "../platform/apple_orientation.hpp"
#include "../platform/credential_store.hpp"
#include "../platform/apple_safearea.hpp"
#include "../platform/apple_textentry.hpp"
#include "../platform/apple_attribution.hpp"
#include "../platform/apple_devicelocation.hpp"
#include "../platform/apple_widgetshare.hpp"
#include "../platform/apple_passreminder.hpp"
#include "../platform/apple_skyfinder.hpp"
#include "../platform/apple_skysensor.hpp"
#include "../platform/apple_skytarget.hpp"
#include <QSettings>
#include <QEventLoop>
#include <QNetworkInformation>
#include <QGuiApplication>
#include <QThread>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWindow>
#include <QScreen>
#include <QTimer>
#include <QtQml>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>

namespace {
    constexpr qreal kSafeAreaToleranceDp = 0.5;

    std::optional<QMarginsF> parse_insets(const QString& text) {
        const auto parts = text.split(',');
        if (parts.size() != 4) return std::nullopt;
        std::array<qreal, 4> values{};
        for (int i = 0; i < 4; ++i) {
            bool ok = false;
            values[i] = parts[i].toDouble(&ok);
            if (!ok) return std::nullopt;
        }
        return QMarginsF(values[0], values[1], values[2], values[3]);
    }

    // Counts visible, non-empty Text-like items whose text is cut off (issue
    // #152). Items that elide on purpose set objectName "elideOk".
    int count_clipped(QQuickItem* item);
    int count_truncated(QQuickItem* item) {
        if (!item || !item->isVisible() || item->opacity() <= 0.0) return 0;
        int found = 0;
        // A visible pane slot with no size is a blank hole in the grid (#638).
        if (item->objectName() == "paneSlot" && (item->width() <= 0 || item->height() <= 0)) {
            std::fprintf(stderr, "EMPTY PANE SLOT: %.0fx%.0f\n", item->width(), item->height());
            ++found;
        }
        const QVariant truncated = item->property("truncated");
        if (truncated.isValid() && truncated.toBool() && item->objectName() != "elideOk" &&
            !item->property("text").toString().isEmpty() && item->width() > 0 && item->height() > 0) {
            std::fprintf(stderr, "TRUNCATED: \"%s\"\n", qPrintable(item->property("text").toString()));
            ++found;
        }
        found += count_clipped(item);
        for (auto* child : item->childItems()) found += count_truncated(child);
        return found;
    }

    // --safe-area: counts visible text and buttons that reach into the given insets of
    // the window (the status bar, notch and home indicator on iPhone and iPad). `shown` is
    // the part of the window that clipping ancestors leave visible, so scrolled-away
    // content does not count.
    int count_safe_area_overlaps(QQuickItem* item, const QMarginsF& insets, const QRectF& window, const QRectF& shown) {
        if (!item || !item->isVisible() || item->opacity() <= 0.0) return 0;
        int found = 0;
        const QRectF box = item->mapRectToScene(item->boundingRect());
        const QRectF part = box & shown;
        if (!item->property("text").toString().isEmpty() && !part.isEmpty()) {
            // Only the top and sides: content running off the bottom is free to scroll there.
            if (part.top() < insets.top() - kSafeAreaToleranceDp || part.left() < insets.left() - kSafeAreaToleranceDp ||
                part.right() > window.right() - insets.right() + kSafeAreaToleranceDp) {
                std::fprintf(stderr, "IN SAFE AREA: \"%s\" at %.1f,%.1f %.1fx%.1f\n",
                             qPrintable(item->property("text").toString()), part.x(), part.y(), part.width(), part.height());
                ++found;
            }
        }
        const QRectF child_shown = item->clip() ? part : shown;
        for (auto* child : item->childItems()) found += count_safe_area_overlaps(child, insets, window, child_shown);
        return found;
    }

    // The header and the page must share one inset: with the header showing, the page
    // starts right under it instead of adding the status bar height a second time.
    bool page_gap_below_header(const QQuickWindow& window) {
        const auto* header = window.property("header").value<QQuickItem*>();
        const auto* page = window.findChild<QQuickItem*>("pageGrid");
        if (!header || !page || !header->isVisible()) return false;
        const qreal header_bottom = header->mapToScene({0, header->height()}).y();
        const qreal page_top = page->mapToScene({0, 0}).y();
        if (std::abs(page_top - header_bottom) <= kSafeAreaToleranceDp) return false;
        std::fprintf(stderr, "PAGE GAP: header ends at %.1f, page starts at %.1f\n", header_bottom, page_top);
        return true;
    }

    // Text whose painted extent runs outside the nearest clipping ancestor or the
    // window is cut off without eliding (issue #532). Scrolling views and items
    // marked objectName "clipOk" are exempt.
    int count_clipped(QQuickItem* item) {
        const QVariant cw = item->property("contentWidth");
        const QVariant ch = item->property("contentHeight");
        const QString text = item->property("text").toString();
        if (!cw.isValid() || !ch.isValid() || text.isEmpty() || item->objectName() == "clipOk" || item->objectName() == "elideOk") return 0;
        if (item->property("truncated").toBool()) return 0; // already reported
        const double w = cw.toDouble(), h = ch.toDouble();
        if (w <= 0 || h <= 0) return 0;
        const int align = item->property("horizontalAlignment").toInt();
        double x = 0;
        if (align == Qt::AlignRight) x = item->width() - w;
        else if (align == Qt::AlignHCenter) x = (item->width() - w) / 2;
        // Mapped as a rectangle so a scaled ancestor (an adaptive pane's stage) scales the text too.
        const QRectF content = item->mapRectToScene(QRectF(x, 0, w, h));
        constexpr double tol = 1.5;
        QRectF bound = item->window() ? QRectF(0, 0, item->window()->width(), item->window()->height()) : QRectF();
        for (auto* a = item->parentItem(); a; a = a->parentItem()) {
            if (a->property("flickableDirection").isValid()) return 0; // scrolls on purpose
            if (a->clip()) bound = bound.isNull() ? a->mapRectToScene(a->boundingRect()) : bound.intersected(a->mapRectToScene(a->boundingRect()));
        }
        if (bound.isNull()) return 0;
        if (content.left() < bound.left() - tol || content.top() < bound.top() - tol || content.right() > bound.right() + tol ||
            content.bottom() > bound.bottom() + tol) {
            std::fprintf(stderr, "TRUNCATED: \"%s\" (clipped)\n", qPrintable(text));
            return 1;
        }
        return 0;
    }

// Prints the fill of every pane; returns true when pane `check` (1-based, 0 = none) has data but
// leaves too much blank, or is not on screen at all (nothing would be checked).
bool report_pane_fill(QQuickItem* root, const QImage& image, int check) {
    bool underfilled = false;
    bool found = check <= 0;
    for (const auto& fill : hamclock::measure_pane_fill(root, image)) {
        found = found || fill.slot == check;
        const bool has_data = fill.state == "live" || fill.state == "stale";
        const bool bad = fill.slot == check && has_data && fill.blankBand > hamclock::kMaxBlankBand;
        std::printf("PANE FILL: slot %d \"%s\" %s at %.0f,%.0f %.0fx%.0f: blank %.0f%%%s\n", fill.slot, qPrintable(fill.title),
                    qPrintable(fill.state), fill.paneRect.x(), fill.paneRect.y(), fill.paneRect.width(), fill.paneRect.height(),
                    fill.blankBand * 100, bad ? " UNDERFILLED" : "");
        underfilled = underfilled || bad;
    }
    if (!found) std::fprintf(stderr, "PANE FILL: pane %d is not on screen at this size\n", check);
    return underfilled || !found;
}

constexpr int kScreenshotDelayMs = 1500;
constexpr int kSettleStepMs = 250;
constexpr int kSettleMaxSteps = 16;

// Adaptive panes refine their scale over a few layout passes (slower on a busy machine):
// wait until every pane card reads the same twice in a row, at most about four seconds.
void wait_for_settled_panes(QQuickItem* root) {
    auto last = hamclock::pane_layout_state(root);
    for (int step = 0; step < kSettleMaxSteps; ++step) {
        QEventLoop loop;
        QTimer::singleShot(kSettleStepMs, &loop, &QEventLoop::quit);
        loop.exec();
        auto now = hamclock::pane_layout_state(root);
        if (now == last) return;
        last = std::move(now);
    }
    std::fprintf(stderr, "Panes still adjusting their scale; screenshot taken anyway\n");
}
constexpr int kSmallScreenWidth = 1280;
constexpr int kSmallScreenHeight = 800;

// Serves https://host/path/name.json from DIR/name.json; anything else fails.
// Used for deterministic screenshots and offline demos, never for live use.
hamclock::Transport fixture_transport(const QString& dir) {
    return [dir](const QUrl& url, hamclock::FetchDone done) {
        hamclock::FetchResult result;
        // A trailing slash (Launch Library) falls back to the last path segment.
        auto path = url.path();
        while (path.endsWith('/')) path.chop(1);
        QFile file(QDir(dir).filePath(QFileInfo(path).fileName()));
        if (file.open(QIODevice::ReadOnly)) {
            result.http_status = 200;
            result.body = file.readAll();
        } else {
            result.error = "No fixture for " + url.path();
        }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
}

hamclock::Transport offline_transport() {
    return [](const QUrl&, hamclock::FetchDone done) {
        QTimer::singleShot(0, [done] { done({0, {}, "Network disabled (--offline)"}); });
    };
}
// --help and --version only print text, so they must work with no display (SSH to a kiosk Pi).
bool text_only_request(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const QByteArray arg(argv[i]);
        if (arg == "--help" || arg == "-h" || arg == "--help-all" || arg == "--version" || arg == "-v") return true;
    }
    return false;
}

void set_identity() {
    QCoreApplication::setApplicationName("hamclock-pro");
    // No organization name: settings live in ~/.config/hamclock-pro (not .../hamclock-pro/hamclock-pro).
    QCoreApplication::setApplicationVersion(HAMCLOCK_VERSION);
}
} // namespace

int main(int argc, char** argv) {
    QCommandLineParser parser;
    parser.setApplicationDescription("HamClock Pro amateur radio dashboard");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption station({"s", "station"}, "Station file (default: config dir).", "path");
    const QCommandLineOption config({"c", "config-dir"}, "Settings directory.", "dir");
    const QCommandLineOption fullscreen("fullscreen", "Start fullscreen.");
    const QCommandLineOption windowed("windowed", "Start in a window even on small screens.");
    const QCommandLineOption first_run("first-run", "Open Settings at start when no station is configured (kiosk first boot).");
    const QCommandLineOption touch_keyboard("touch-keyboard", "Show an on-screen keyboard for text fields (touch-only kiosks).");
    const QCommandLineOption offline("offline", "Do not use the network (cached data only).");
    const QCommandLineOption fixtures("fixtures", "Serve data from files in DIR instead of the network.", "dir");
    const QCommandLineOption screenshot("screenshot", "Render once to PNG and exit.", "png");
    const QCommandLineOption size("size", "Window size for --screenshot, e.g. 1024x600.", "WxH", "1024x600");
    const QCommandLineOption theme("theme", "Force light or dark.", "light|dark");
    const QCommandLineOption pane("pane", "Show widget ID in pane N (1-based); repeatable. For screenshots.", "N=id");
    const QCommandLineOption open_settings("settings", "Open Settings at a section at start: station, accounts, display, screen, spots, hardware, advanced or about, or accounts:ID to open one account (screenshots).", "section");
    const QCommandLineOption allow_truncation("allow-truncation", "With --screenshot, report truncated text but do not fail.");
    const QCommandLineOption check_fill("check-fill", "With --screenshot, report every pane's blank space; fail (exit 5) when pane N has data but leaves too much blank (0 = report only).", "N");
    const QCommandLineOption fake_safe_area("safe-area", "With --screenshot, lay out as if the iOS safe-area insets were L,T,R,B and fail (exit 7) when text or a button reaches into them.", "L,T,R,B");
    for (const auto& option : {fake_safe_area, check_fill, open_settings, allow_truncation, first_run, touch_keyboard, station, config, fullscreen, windowed, offline, fixtures, screenshot, size, theme, pane})
        parser.addOption(option);
    if (text_only_request(argc, argv)) {
        QCoreApplication text_app(argc, argv);
        set_identity();
        parser.process(text_app); // prints and exits
        return 0;
    }
    QGuiApplication app(argc, argv); // QtGui only: the map is a pure QPainter renderer
    set_identity();
    hamclock::AppLifecycle::instance().attachToApplication(); // iPad: pause polling while suspended
    // Lets every PolledSource retry at once when the network comes back (Wi-Fi drop, sleep).
    if (!QNetworkInformation::loadDefaultBackend())
        qWarning("No network reachability backend; sources retry on their backoff only");
    parser.process(app);

    hamclock::AppSettings settings(parser.value(station), parser.value(config));
#ifdef Q_OS_DARWIN
    // Passwords and API keys live in the Keychain on Mac, iPhone and iPad. Screenshot
    // runs (CI) leave the Keychain alone; their config directories hold no accounts.
    if (!parser.isSet(screenshot)) {
        if (auto keychain = std::make_unique<hamclock::KeychainCredentialStore>("org.hamclockpro.app"); keychain->available())
            settings.setSecretStore(std::move(keychain));
    }
#endif

    hamclock::set_telnet_network_enabled(!parser.isSet(fixtures) && !parser.isSet(offline));
    hamclock::WidgetContext context;
    context.settings = &settings;
    context.owner = &app;
    context.transport = parser.isSet(fixtures) ? fixture_transport(parser.value(fixtures))
        : parser.isSet(offline) ? offline_transport()
        : hamclock::make_network_transport(&app);
    if (!parser.isSet(fixtures) && !parser.isSet(offline))
        context.auth_transport = hamclock::make_authorized_transport(&app);

    // Satellite pass alerts: fed by the Satellites pane's elements (downloaded once).
    hamclock::SatAlertService sat_alerts(&settings, hamclock::shipped_sat_catalog());
    context.sat_alerts = &sat_alerts;
    hamclock::WidgetRegistry registry;
    hamclock::register_all_widgets(registry, context);
    if (auto* sats = qobject_cast<hamclock::SatellitesModel*>(registry.model("satellites"))) {
        QObject::connect(sats, &hamclock::SatellitesModel::elementsChanged, &sat_alerts,
                         [&sat_alerts, sats] { sat_alerts.setElements(sats->elements()); });
        sat_alerts.setElements(sats->elements()); // a cached set may already be loaded
    }
    QTimer::singleShot(0, &sat_alerts, &hamclock::SatAlertService::start);
    for (const auto& assignment : parser.values(pane)) {
        const auto parts = assignment.split('=');
        bool number_ok = false;
        const int slot = parts.value(0).toInt(&number_ok) - 1;
        const auto problem = parts.size() == 2 && number_ok && registry.model(parts[1])
            ? settings.showPane(slot, parts[1]) : QString("Unknown widget or pane.");
        if (!problem.isEmpty()) {
            std::fprintf(stderr, "--pane %s: %s\n", qPrintable(assignment), qPrintable(problem));
            return 2;
        }
    }
    hamclock::StationModel station_model(settings);
    hamclock::TimeZoneSearch time_zone_search;
    hamclock::LocationTools location_tools;
    hamclock::DeviceLocation device_location;
    device_location.setAvailableOverride(hamclock::nativeLocationAvailable());
#if defined(Q_OS_IOS)
    // Starred passes get a local reminder before AOS (#573 item 4); other platforms have no backend.
    hamclock::PassReminderSync pass_reminders;
    if (auto* passes = registry.model("passes")) {
        QObject::connect(passes, SIGNAL(changed()), &pass_reminders, SLOT(syncNow()));
    }
#endif
#if defined(Q_OS_IOS)
    // Find-in-the-sky controllers (#573); without the HAMCLOCK_SKY_FINDER option the
    // sensor reports unavailable, so QML can hide the finder.
    hamclock::SkySensor sky_sensor;
    hamclock::SkyFinder sky_finder(&sky_sensor);
    // Selected satellite (satellites card detail) is the finder's target.
    hamclock::SkyTargetBridge sky_target(registry.model("satellites"), &sky_finder);
#endif
    hamclock::TimersModel timers;
    hamclock::AlarmsModel alarms(settings.optionsPath());
    hamclock::BlankingModel blanking;
    hamclock::KeyInjector key_injector;
    const auto apply_blanking = [&] {
        blanking.setConfig({settings.blankIdleMinutes(), settings.blankOffHour(), settings.blankOnHour(), settings.blankDays()});
    };
    apply_blanking();
    QObject::connect(&settings, &hamclock::AppSettings::optionsChanged, &blanking, apply_blanking);
    QObject::connect(&settings, &hamclock::AppSettings::optionsChanged, &alarms, [&alarms] { alarms.reload(); });
    // iPhone/iPad widgets read the station from the App Group (no-op off iOS). Shared at
    // start, on station/option changes, and when the app leaves the foreground (the
    // satellites card saves its tracked list straight to options.ini).
    const auto share_for_widgets = [&settings] {
        const QSettings ini(settings.optionsPath(), QSettings::IniFormat);
        hamclock::nativeShareForWidgets(hamclock::makeWidgetShare(
            settings.station(), settings.configured(), ini.value("satellites/tracked").toStringList()));
    };
    if (!parser.isSet(screenshot)) {
        share_for_widgets();
        QObject::connect(&settings, &hamclock::AppSettings::stationChanged, &app, share_for_widgets);
        QObject::connect(&settings, &hamclock::AppSettings::optionsChanged, &app, share_for_widgets);
        QObject::connect(&app, &QGuiApplication::applicationStateChanged, &app, [share_for_widgets](Qt::ApplicationState s) {
            if (s != Qt::ApplicationActive) share_for_widgets();
        });
    }
    location_tools.setTransport(context.transport);
    hamclock::MapItem::setSettings(&settings);

    QQuickStyle::setStyle("Basic"); // fully themed by our QML, identical on every platform
    qmlRegisterType<hamclock::MapItem>("HamClock", 1, 0, "WorldMap");
    qmlRegisterSingletonType(QUrl("qrc:/ui/Theme.qml"), "HamClock", 1, 0, "Theme");
    // Shared building blocks so widget cards (in other qrc folders) can `import HamClock`.
    for (const char* type : {"Card", "Metric", "StatusBadge", "AppButton", "FormField"})
        qmlRegisterType(QUrl(QString("qrc:/ui/%1.qml").arg(type)), "HamClock", 1, 0, type);

    const auto screen = QGuiApplication::primaryScreen();
    const bool small_screen = screen && screen->size().width() <= kSmallScreenWidth &&
        screen->size().height() <= kSmallScreenHeight;
    const bool start_fullscreen = parser.isSet(fullscreen) ||
        (!parser.isSet(windowed) && !parser.isSet(screenshot) && small_screen);

    // Declared before the engine so it outlives it: QML bindings on safeArea
    // re-evaluate during engine teardown and must not see a null object.
    hamclock::SafeArea safe_area;
    const auto insets = parse_insets(parser.value(fake_safe_area));
    if (parser.isSet(fake_safe_area) && !insets) {
        std::fprintf(stderr, "--safe-area %s: expected L,T,R,B.\n", qPrintable(parser.value(fake_safe_area)));
        return 2;
    }
    if (insets) safe_area.setInsets(*insets);
    hamclock::TextEntryHint text_entry_hint;
    text_entry_hint.refresh();
    QQmlApplicationEngine engine;
    // In screenshot mode any QML warning (binding error, missing type) fails
    // the run, so CI catches broken QML that would otherwise render blank.
    int qml_warnings = 0;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app, [&qml_warnings](const QList<QQmlError>& list) {
        for (const auto& warning : list) std::fprintf(stderr, "QML: %s\n", qPrintable(warning.toString()));
        qml_warnings += static_cast<int>(list.size());
    });
    engine.rootContext()->setContextProperty("appSettings", &settings);
    engine.rootContext()->setContextProperty("widgetRegistry", &registry);
    engine.rootContext()->setContextProperty("station", &station_model);
    engine.rootContext()->setContextProperty("timers", &timers);
    engine.rootContext()->setContextProperty("alarms", &alarms);
    engine.rootContext()->setContextProperty("satAlerts", &sat_alerts);
    engine.rootContext()->setContextProperty("blanking", &blanking);
    engine.rootContext()->setContextProperty("timeZones", &time_zone_search);
    engine.rootContext()->setContextProperty("locationTools", &location_tools);
    engine.rootContext()->setContextProperty("deviceLocation", &device_location);
#if defined(Q_OS_IOS)
    engine.rootContext()->setContextProperty("skySensor", &sky_sensor);
    engine.rootContext()->setContextProperty("skyFinder", &sky_finder);
#endif
    engine.rootContext()->setContextProperty("safeArea", &safe_area);
    engine.rootContext()->setContextProperty("textEntryHint", &text_entry_hint);
    engine.rootContext()->setContextProperty("dataSources", hamclock::attributionModel());
    engine.rootContext()->setContextProperty("startFullScreen", start_fullscreen);
    engine.rootContext()->setContextProperty("firstRun", parser.isSet(first_run));
    engine.rootContext()->setContextProperty("touchKeyboard", parser.isSet(touch_keyboard));
    engine.rootContext()->setContextProperty("keyInjector", &key_injector);
    engine.rootContext()->setContextProperty("openSettings", parser.value(open_settings));
#ifdef Q_OS_MACOS
    // Native menu bar (About, Settings..., Edit, View, Window, Help); not for screenshot runs.
    engine.rootContext()->setContextProperty("nativeMenuBar", !parser.isSet(screenshot));
#else
    engine.rootContext()->setContextProperty("nativeMenuBar", false); // iOS, Linux, Pi kiosk
#endif
    engine.rootContext()->setContextProperty("forcedTheme", parser.value(theme));
    engine.load(QUrl("qrc:/ui/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    safe_area.attach(qobject_cast<QWindow*>(engine.rootObjects().first()));
#if !defined(Q_OS_IOS)
    // Desktop: reopen at the last window size and position (not for kiosk full screen or screenshots).
    if (!parser.isSet(screenshot) && !start_fullscreen)
        hamclock::remember_window_geometry(qobject_cast<QWindow*>(engine.rootObjects().first()), settings.optionsPath());
#endif
    // Rotation lock (iPhone/iPad): apply the saved choice now and whenever it changes.
    QString orientation_lock = settings.orientationLock();
    hamclock::applyOrientationLock(orientation_lock);
    QObject::connect(&settings, &hamclock::AppSettings::optionsChanged, &app, [&settings, &orientation_lock] {
        if (settings.orientationLock() == orientation_lock) return;
        orientation_lock = settings.orientationLock();
        hamclock::applyOrientationLock(orientation_lock);
    });

    bool check_fill_ok = true;
    const int check_fill_slot = parser.value(check_fill).toInt(&check_fill_ok);
    if (parser.isSet(check_fill) && (!check_fill_ok || check_fill_slot < 0)) {
        std::fprintf(stderr, "--check-fill %s: expected a pane number (0 = report only).\n", qPrintable(parser.value(check_fill)));
        return 2;
    }
    if (parser.isSet(screenshot)) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        const auto parts = parser.value(size).split('x');
        if (!window || parts.size() != 2) return 2;
        // The window's resize event reaches the content item asynchronously on some
        // platforms (offscreen), so a grab straight away can show the old 1024x600
        // layout cropped to the new size (#587). Resize, then wait until the content
        // item has followed before the screenshot timer starts.
        const QSize wanted(parts[0].toInt(), parts[1].toInt());
        window->resize(wanted);
        for (int i = 0; i < 200 && window->contentItem()->size().toSize() != wanted; ++i) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            if (window->contentItem()->size().toSize() != wanted) QThread::msleep(5);
        }
        if (window->contentItem()->size().toSize() != wanted) window->contentItem()->setSize(wanted);
        const auto out = parser.value(screenshot);
        QTimer::singleShot(kScreenshotDelayMs, &app, [window, out, &qml_warnings, report_only = parser.isSet(allow_truncation),
                                                fill_slot = parser.isSet(check_fill) ? check_fill_slot : -1, insets] {
            if (window->contentItem()->size().toSize() != window->size()) {
                std::fprintf(stderr, "Content item is not window-sized: layout would be cropped\n");
                QCoreApplication::exit(6);
                return;
            }
            wait_for_settled_panes(window->contentItem());
            const int truncated = count_truncated(window->contentItem());
            const QRectF whole(QPointF(0, 0), QSizeF(window->size()));
            const bool unsafe = insets && (count_safe_area_overlaps(window->contentItem(), *insets, whole, whole) > 0 ||
                                           page_gap_below_header(*window));
            const QImage image = window->grabWindow();
            const bool underfilled = fill_slot >= 0 && report_pane_fill(window->contentItem(), image, fill_slot);
            const bool saved = image.save(out);
            std::fprintf(saved ? stdout : stderr, saved ? "Saved %s\n" : "Could not save %s\n",
                         qPrintable(out));
            QCoreApplication::exit(!saved ? 1 : qml_warnings ? 3 : truncated && !report_only ? 4 : underfilled ? 5 : unsafe ? 7 : 0);
        });
    }
    return app.exec();
}

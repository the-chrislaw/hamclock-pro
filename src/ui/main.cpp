// HamClock Pro main application (Qt Quick). Legacy Qt Widgets programs remain
// available while their screens are ported.
#include "app_settings.hpp"
#include "map_item.hpp"
#include "station_model.hpp"
#include "alarms_model.hpp"
#include "blanking_model.hpp"
#include "timers_model.hpp"
#include "time_zone_search.hpp"
#include "location_tools.hpp"
#include "../net/polled_source.hpp"
#include "../net/telnet_client.hpp"
#include "../widgets/widget_registry.hpp"
#include "../platform/apple_lifecycle.hpp"
#include "../platform/apple_orientation.hpp"
#include "../platform/apple_safearea.hpp"
#include "../platform/apple_textentry.hpp"
#include <QEventLoop>
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
#include <cstdio>

namespace {
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
        const QRectF content(item->mapToScene(QPointF(x, 0)), QSizeF(w, h));
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

constexpr int kScreenshotDelayMs = 1500;
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
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv); // QtGui only: the map is a pure QPainter renderer
    QGuiApplication::setApplicationName("hamclock-pro");
    // No organization name: settings live in ~/.config/hamclock-pro (not .../hamclock-pro/hamclock-pro).
    QGuiApplication::setApplicationVersion("1.0.0");
    hamclock::AppLifecycle::instance().attachToApplication(); // iPad: pause polling while suspended

    QCommandLineParser parser;
    parser.setApplicationDescription("HamClock Pro amateur radio dashboard");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption station({"s", "station"}, "Station file (default: config dir).", "path");
    const QCommandLineOption config({"c", "config-dir"}, "Settings directory.", "dir");
    const QCommandLineOption fullscreen("fullscreen", "Start fullscreen.");
    const QCommandLineOption windowed("windowed", "Start in a window even on small screens.");
    const QCommandLineOption first_run("first-run", "Open Settings at start when no station is configured (kiosk first boot).");
    const QCommandLineOption offline("offline", "Do not use the network (cached data only).");
    const QCommandLineOption fixtures("fixtures", "Serve data from files in DIR instead of the network.", "dir");
    const QCommandLineOption screenshot("screenshot", "Render once to PNG and exit.", "png");
    const QCommandLineOption size("size", "Window size for --screenshot, e.g. 1024x600.", "WxH", "1024x600");
    const QCommandLineOption theme("theme", "Force light or dark.", "light|dark");
    const QCommandLineOption pane("pane", "Show widget ID in pane N (1-based); repeatable. For screenshots.", "N=id");
    const QCommandLineOption allow_truncation("allow-truncation", "With --screenshot, report truncated text but do not fail.");
    for (const auto& option : {allow_truncation, first_run, station, config, fullscreen, windowed, offline, fixtures, screenshot, size, theme, pane})
        parser.addOption(option);
    parser.process(app);

    hamclock::AppSettings settings(parser.value(station), parser.value(config));
    if (parser.isSet(theme)) settings.setAppearance(parser.value(theme));

    hamclock::set_telnet_network_enabled(!parser.isSet(fixtures) && !parser.isSet(offline));
    hamclock::WidgetContext context;
    context.settings = &settings;
    context.owner = &app;
    context.transport = parser.isSet(fixtures) ? fixture_transport(parser.value(fixtures))
        : parser.isSet(offline) ? offline_transport()
        : hamclock::make_network_transport(&app);
    if (!parser.isSet(fixtures) && !parser.isSet(offline))
        context.auth_transport = hamclock::make_authorized_transport(&app);

    hamclock::WidgetRegistry registry;
    hamclock::register_all_widgets(registry, context);
    for (const auto& assignment : parser.values(pane)) {
        const auto parts = assignment.split('=');
        bool number_ok = false;
        const int slot = parts.value(0).toInt(&number_ok) - 1;
        const auto problem = parts.size() == 2 && number_ok && registry.model(parts[1])
            ? settings.setPane(slot, parts[1]) : QString("Unknown widget or pane.");
        if (!problem.isEmpty()) {
            std::fprintf(stderr, "--pane %s: %s\n", qPrintable(assignment), qPrintable(problem));
            return 2;
        }
    }
    hamclock::StationModel station_model(settings);
    hamclock::TimeZoneSearch time_zone_search;
    hamclock::LocationTools location_tools;
    hamclock::TimersModel timers;
    hamclock::AlarmsModel alarms(settings.optionsPath());
    hamclock::BlankingModel blanking;
    const auto apply_blanking = [&] {
        blanking.setConfig({settings.blankIdleMinutes(), settings.blankOffHour(), settings.blankOnHour(), settings.blankDays()});
    };
    apply_blanking();
    QObject::connect(&settings, &hamclock::AppSettings::optionsChanged, &blanking, apply_blanking);
    QObject::connect(&settings, &hamclock::AppSettings::optionsChanged, &alarms, [&alarms] { alarms.reload(); });
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
    engine.rootContext()->setContextProperty("blanking", &blanking);
    engine.rootContext()->setContextProperty("timeZones", &time_zone_search);
    engine.rootContext()->setContextProperty("locationTools", &location_tools);
    engine.rootContext()->setContextProperty("safeArea", &safe_area);
    engine.rootContext()->setContextProperty("textEntryHint", &text_entry_hint);
    engine.rootContext()->setContextProperty("startFullScreen", start_fullscreen);
    engine.rootContext()->setContextProperty("firstRun", parser.isSet(first_run));
    engine.rootContext()->setContextProperty("forcedTheme", parser.value(theme));
    engine.load(QUrl("qrc:/ui/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    safe_area.attach(qobject_cast<QWindow*>(engine.rootObjects().first()));
    // Rotation lock (iPhone/iPad): apply the saved choice now and whenever it changes.
    QString orientation_lock = settings.orientationLock();
    hamclock::applyOrientationLock(orientation_lock);
    QObject::connect(&settings, &hamclock::AppSettings::optionsChanged, &app, [&settings, &orientation_lock] {
        if (settings.orientationLock() == orientation_lock) return;
        orientation_lock = settings.orientationLock();
        hamclock::applyOrientationLock(orientation_lock);
    });

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
        QTimer::singleShot(kScreenshotDelayMs, &app, [window, out, &qml_warnings, report_only = parser.isSet(allow_truncation)] {
            if (window->contentItem()->size().toSize() != window->size()) {
                std::fprintf(stderr, "Content item is not window-sized: layout would be cropped\n");
                QCoreApplication::exit(6);
                return;
            }
            const int truncated = count_truncated(window->contentItem());
            const bool saved = window->grabWindow().save(out);
            std::fprintf(saved ? stdout : stderr, saved ? "Saved %s\n" : "Could not save %s\n",
                         qPrintable(out));
            QCoreApplication::exit(!saved ? 1 : qml_warnings ? 3 : truncated && !report_only ? 4 : 0);
        });
    }
    return app.exec();
}

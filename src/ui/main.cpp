// HamClock Pro main application (Qt Quick). Legacy Qt Widgets programs remain
// available while their screens are ported.
#include "app_settings.hpp"
#include "map_item.hpp"
#include "station_model.hpp"
#include "../net/polled_source.hpp"
#include "../widgets/widget_registry.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <QtQml>
#include <cstdio>

namespace {
constexpr int kScreenshotDelayMs = 1500;
constexpr int kSmallScreenWidth = 1280;
constexpr int kSmallScreenHeight = 800;

// Serves https://host/path/name.json from DIR/name.json; anything else fails.
// Used for deterministic screenshots and offline demos, never for live use.
hamclock::Transport fixture_transport(const QString& dir) {
    return [dir](const QUrl& url, hamclock::FetchDone done) {
        hamclock::FetchResult result;
        QFile file(QDir(dir).filePath(QFileInfo(url.path()).fileName()));
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
    QApplication app(argc, argv); // QApplication: the map renders via a QWidget
    QApplication::setApplicationName("hamclock-pro");
    QApplication::setOrganizationName("hamclock-pro");
    QApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("HamClock Pro amateur radio dashboard");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption station({"s", "station"}, "Station file (default: config dir).", "path");
    const QCommandLineOption config({"c", "config-dir"}, "Settings directory.", "dir");
    const QCommandLineOption fullscreen("fullscreen", "Start fullscreen.");
    const QCommandLineOption windowed("windowed", "Start in a window even on small screens.");
    const QCommandLineOption offline("offline", "Do not use the network (cached data only).");
    const QCommandLineOption fixtures("fixtures", "Serve data from files in DIR instead of the network.", "dir");
    const QCommandLineOption screenshot("screenshot", "Render once to PNG and exit.", "png");
    const QCommandLineOption size("size", "Window size for --screenshot, e.g. 1024x600.", "WxH", "1024x600");
    const QCommandLineOption theme("theme", "Force light or dark.", "light|dark");
    for (const auto& option : {station, config, fullscreen, windowed, offline, fixtures, screenshot, size, theme})
        parser.addOption(option);
    parser.process(app);

    hamclock::AppSettings settings(parser.value(station), parser.value(config));
    if (parser.isSet(theme)) settings.setAppearance(parser.value(theme));

    hamclock::WidgetContext context;
    context.settings = &settings;
    context.owner = &app;
    context.transport = parser.isSet(fixtures) ? fixture_transport(parser.value(fixtures))
        : parser.isSet(offline) ? offline_transport()
        : hamclock::make_network_transport(&app);

    hamclock::WidgetRegistry registry;
    hamclock::register_all_widgets(registry, context);
    hamclock::StationModel station_model(settings);
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
    engine.rootContext()->setContextProperty("startFullScreen", start_fullscreen);
    engine.rootContext()->setContextProperty("forcedTheme", parser.value(theme));
    engine.load(QUrl("qrc:/ui/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (parser.isSet(screenshot)) {
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        const auto parts = parser.value(size).split('x');
        if (!window || parts.size() != 2) return 2;
        window->resize(parts[0].toInt(), parts[1].toInt());
        const auto out = parser.value(screenshot);
        QTimer::singleShot(kScreenshotDelayMs, &app, [window, out, &qml_warnings] {
            const bool saved = window->grabWindow().save(out);
            std::fprintf(saved ? stdout : stderr, saved ? "Saved %s\n" : "Could not save %s\n",
                         qPrintable(out));
            QCoreApplication::exit(!saved ? 1 : qml_warnings ? 3 : 0);
        });
    }
    return app.exec();
}

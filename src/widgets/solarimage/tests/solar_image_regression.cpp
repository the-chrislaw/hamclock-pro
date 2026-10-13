// Solar image widget: image validation and model, offline.
// Fixture is a real NASA SDO AIA 193 latest_512 image captured 2026-10-13.
#include "../solar_image_model.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTimeZone>
#include <QTimer>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile file(QDir(argv[1]).filePath("latest_512_0193.jpg"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray jpeg = file.readAll();

    check(SolarImageModel::valid_image(jpeg), "real SDO jpeg accepted");
    for (const QByteArray& bad : {QByteArray(), QByteArray("<html>404</html>"), QByteArray("\xff\xd8\xff garbage garbage garbage"),
                                  jpeg.left(200), QByteArray(64, '\0')})
        check(!SolarImageModel::valid_image(bad), "non-image or truncated body rejected");

    WidgetContext context;
    QString mode = "ok";
    QStringList requested;
    context.transport = [&](const QUrl& url, FetchDone done) {
        requested << url.fileName();
        FetchResult r;
        if (mode == "ok") { r.http_status = 200; r.body = jpeg; }
        else if (mode == "html") { r.http_status = 200; r.body = "<html>maintenance</html>"; }
        else r.error = "network down";
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };

    SolarImageModel model(context);
    auto now = QDateTime(QDate(2026, 10, 13), QTime(22, 0), QTimeZone::utc());
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        for (int i = 0; i < model.channelCount(); ++i) model.source(i).setClock([&] { return now; });
    };
    set_clock();
    for (int i = 0; i < model.channelCount(); ++i) QFile::remove(model.source(i).cachePath());
    check(!model.hasImage() && model.state() == "loading", "loading before data");

    model.start();
    settle();
    check(requested == QStringList{"latest_512_0193.jpg"}, "only the shown channel is polled");
    check(model.state() == "live" && model.hasImage(), "fresh -> live");
    check(model.imageUrl().toString().startsWith("data:image/jpeg;base64,"), "image delivered as data URL");
    check(model.channelName() == "AIA 193 Å", "first channel");

    model.cycle();
    settle();
    check(model.channelName() == "AIA 304 Å" && requested.size() == 2 && requested.back() == "latest_512_0304.jpg",
          "tap cycles and polls the new channel");
    model.cycle();
    model.cycle();
    check(model.channelName() == "AIA 193 Å" && requested.size() == 3 && model.hasImage(),
          "wraps around; revisiting a channel reuses its image without refetching");

    now = now.addSecs(3 * 3600);
    set_clock();
    mode = "down";
    model.source(0).refreshNow();
    settle();
    check(model.state() == "stale" && model.hasImage(), "outage -> stale, last image kept");

    // A server returning an HTML error page is an error, not a picture.
    SolarImageModel bad(context);
    mode = "html";
    for (int i = 0; i < bad.channelCount(); ++i) QFile::remove(bad.source(i).cachePath());
    bad.start();
    settle();
    check(!bad.hasImage() && bad.state() == "error", "HTML body -> error, no image");

    SolarImageModel offline(context);
    mode = "down";
    for (int i = 0; i < offline.channelCount(); ++i) QFile::remove(offline.source(i).cachePath());
    offline.start();
    settle();
    check(!offline.hasImage() && offline.state() == "offline", "unreachable -> offline");

    std::cout << "Solar image: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}

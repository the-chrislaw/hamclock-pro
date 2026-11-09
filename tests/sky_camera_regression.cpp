// SkyCamera: preview size + injected camera format become marker geometry.
#include "../src/platform/apple_skycamera.hpp"
#include <QCoreApplication>
#include <cmath>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    SkySensor sensor;
    SkyFinder finder(&sensor);
    SkyMarker marker(&sensor, &finder);

    SkyCamera cam(&marker, [](QSizeF* px) { *px = QSizeF(4032, 3024); return 70.0; });
    check(cam.available(), "camera with a field of view is available");
    int changes = 0;
    QObject::connect(&cam, &SkyCamera::previewChanged, [&] { ++changes; });
    cam.setPreviewSize(0, 800);
    check(changes == 0, "empty size is ignored");
    cam.setPreviewSize(400, 800);
    const PreviewFov want = previewFov(70, QSizeF(4032, 3024), QSizeF(400, 800));
    check(std::fabs(cam.hfov() - want.hfov_deg) < 1e-9 && std::fabs(cam.vfov() - want.vfov_deg) < 1e-9,
          "fov matches previewFov");
    check(changes == 1, "one change signal");
    cam.setPreviewSize(400, 800);
    check(changes == 1, "same size does not signal again");

    // The marker received the geometry: a target on the view axis sits at the centre.
    finder.setTarget(90, 30);
    emit sensor.attitudeChanged(90, 30);
    check(marker.visible() && std::fabs(marker.x() - 200) < 1 && std::fabs(marker.y() - 400) < 1,
          "marker uses the preview geometry");

    // Permission is asked only when the camera view is turned on.
    int asked = 0;
    std::function<void(bool)> pending;
    check(cam.state() == "off" && !cam.enabled(), "camera starts off");
    cam.setAccessRequester([&](std::function<void(bool)> cb) { ++asked; pending = cb; });
    check(asked == 0, "no permission request at start");
    cam.setEnabled(true);
    check(asked == 1 && cam.state() == "requesting", "enabling asks and waits");
    pending(true);
    check(cam.state() == "on", "granted turns the camera on");
    cam.setEnabled(false);
    cam.setEnabled(true);
    check(asked == 1 && cam.state() == "on", "granted answer is remembered");
    cam.setEnabled(false);
    check(cam.state() == "off", "disabling turns it off");

    SkyCamera deny(&marker, [](QSizeF* px) { *px = QSizeF(4032, 3024); return 70.0; });
    deny.setAccessRequester([](std::function<void(bool)> cb) { cb(false); });
    deny.setEnabled(true);
    check(deny.state() == "denied", "refusal shows denied");

    SkyCamera race(&marker, [](QSizeF* px) { *px = QSizeF(4032, 3024); return 70.0; });
    std::function<void(bool)> late;
    race.setAccessRequester([&](std::function<void(bool)> cb) { late = cb; });
    race.setEnabled(true);
    race.setEnabled(false);
    late(true);
    check(race.state() == "off", "late answer after switching off is ignored");

    SkyCamera none(&marker, [](QSizeF*) { return 0.0; });
    check(!none.available() && none.state() == "unavailable", "no camera is unavailable");
    none.setEnabled(true);
    check(!none.enabled(), "cannot enable without a camera");
    none.setPreviewSize(400, 800);
    check(none.hfov() > 0 && none.vfov() > 0, "defaults still give usable angles");

    SkyCamera nosrc(nullptr, nullptr);
    nosrc.setPreviewSize(100, 100);
    check(!nosrc.available(), "null source is unavailable and does not crash");
    return failures ? 1 : 0;
}

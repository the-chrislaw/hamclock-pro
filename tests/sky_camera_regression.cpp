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

    SkyCamera none(&marker, [](QSizeF*) { return 0.0; });
    check(!none.available(), "no camera is unavailable");
    none.setPreviewSize(400, 800);
    check(none.hfov() > 0 && none.vfov() > 0, "defaults still give usable angles");

    SkyCamera nosrc(nullptr, nullptr);
    nosrc.setPreviewSize(100, 100);
    check(!nosrc.available(), "null source is unavailable and does not crash");
    return failures ? 1 : 0;
}

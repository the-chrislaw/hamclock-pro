#pragma once
// Camera AR marker for "Find it in the sky" (#573 item 3): joins the phone
// attitude, the finder's target and the preview geometry into a screen
// position for QML. Pure Qt-free-of-platform code (maths in apple_skyproject),
// so it is tested on Linux; the camera layer only supplies the field of view.
#include "apple_skyfinder.hpp"
#include "apple_skyproject.hpp"

#include <QObject>
#include <QSizeF>
#include <QVariantList>
#include <vector>

namespace hamclock {

class SkyMarker : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY changed)   // target is inside the preview
    Q_PROPERTY(bool inFront READ inFront NOTIFY changed)   // target is ahead of the camera
    Q_PROPERTY(double x READ x NOTIFY changed)             // pixels, valid when inFront
    Q_PROPERTY(double y READ y NOTIFY changed)
    Q_PROPERTY(QVariantList trackPoints READ trackPoints NOTIFY changed)
    Q_PROPERTY(double edgeAngle READ edgeAngle NOTIFY changed) // off-screen arrow, 0 = up, clockwise
public:
    SkyMarker(SkySensor* sensor, SkyFinder* finder, QObject* parent = nullptr);

    bool visible() const { return pt_.on_screen; }
    bool inFront() const { return pt_.in_front; }
    double x() const { return pt_.pos.x(); }
    double y() const { return pt_.pos.y(); }
    double edgeAngle() const { return pt_.edge_angle_deg; }
    // Pass track for the overlay: [{x, y}] in preview pixels, in track order,
    // only samples ahead of the camera and inside the preview (the polyline
    // breaks where the track leaves the view; "gap" is true on the first
    // point after a break).
    QVariantList trackPoints() const { return track_pts_; }

    // Preview size in pixels and field of view (see previewFov). Roll is not
    // measured, so the horizon is assumed level.
    Q_INVOKABLE void setPreview(double width, double height, double hfov_deg, double vfov_deg);

    // Pass track as az/el samples in degrees (from area D's pointing model).
    // An empty list clears it.
    void setTrack(const std::vector<std::pair<double, double>>& azel);

signals:
    void changed();

private:
    void onAttitude(double az, double el);
    void recompute();

    SkyFinder* finder_;
    CameraView view_;
    QSizeF size_;
    bool have_attitude_ = false;
    ScreenPoint pt_;
    std::vector<std::pair<double, double>> track_;
    QVariantList track_pts_;
};

} // namespace hamclock

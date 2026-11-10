#pragma once
// Camera preview geometry for "Find it in the sky" (#573 item 3). QML reports
// the preview rectangle; this works out the aspect-fill field of view from the
// rear camera's format (injected, so it is tested on Linux) and hands it to the
// SkyMarker. When no camera field of view is known, default angles are used and
// `available` is false so QML can keep the camera off and show arrow mode only.
#include "apple_camerafov.hpp"
#include "apple_skymarker.hpp"

#include <QObject>
#include <QSizeF>
#include <QString>
#include <functional>

namespace hamclock {

class SkyCamera : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(double hfov READ hfov NOTIFY previewChanged)
    // "off" (default), "requesting", "on", "denied" or "unavailable" (no camera). Permission is
    // requested only when QML sets `enabled`, never at start.
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY stateChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(double vfov READ vfov NOTIFY previewChanged)
public:
    // Returns the long-side field of view in degrees (0 = no camera) and fills the sensor size.
    using FovSource = std::function<double(QSizeF*)>;

    // Asks the OS for camera access; calls back with the result (possibly synchronously).
    using AccessRequester = std::function<void(std::function<void(bool)>)>;
    void setAccessRequester(AccessRequester r) { request_ = std::move(r); }
    bool enabled() const { return enabled_; }
    QString state() const;
    void setEnabled(bool on);

    SkyCamera(SkyMarker* marker, FovSource source, QObject* parent = nullptr);

    bool available() const { return long_fov_ > 0; }
    double hfov() const { return fov_.hfov_deg; }
    double vfov() const { return fov_.vfov_deg; }

    // Call when the preview item's size changes.
    Q_INVOKABLE void setPreviewSize(double width, double height);

signals:
    void previewChanged();
    void stateChanged();

private:
    SkyMarker* marker_;
    double long_fov_ = 0;
    QSizeF sensor_;
    PreviewFov fov_;
    AccessRequester request_;
    bool enabled_ = false;
    bool requesting_ = false;
    bool denied_ = false;
    bool granted_ = false;
    quint64 epoch_ = 0;
};

} // namespace hamclock

#pragma once
// QML map: paints land, coast, night shade, subsolar point and the DE/DX path
// with the pure QPainter MapRenderer (no QtWidgets) and fills its whole item.
#include <QQuickPaintedItem>
#include "../map/map_renderer.hpp"
#include <QTimer>
class QMouseEvent;

namespace hamclock {

class AppSettings;

class MapItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(double uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)
    Q_PROPERTY(bool showPath READ showPath WRITE setShowPath NOTIFY showPathChanged)
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
public:
    explicit MapItem(QQuickItem* parent = nullptr);
    ~MapItem() override;

    // Must be called once from C++ before the item is shown (see main.cpp).
    static void setSettings(AppSettings* settings);

    bool dark() const { return scene_.dark; }
    void setDark(bool dark);
    bool showPath() const { return scene_.route_visible; }
    void setShowPath(bool show);
    double uiScale() const { return scene_.ui_scale; }
    void setUiScale(double s);
    double zoom() const { return scene_.zoom; }
    void setZoom(double z);
    // Back to zoom 1 centred on DE/DX.
    Q_INVOKABLE void resetView();
    // Drag pan by pixels (only effective when zoomed in).
    Q_INVOKABLE void panBy(double dx, double dy);
    void paint(QPainter* painter) override;

signals:
    void darkChanged();
    void showPathChanged();
    void uiScaleChanged();
    void zoomChanged();
    // A tap (no drag) on the map: the location under the finger, for Set DE / Set DX.
    void tapped(double latitude, double longitude);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    QPointF press_, last_;
    bool dragging_ = false;
    void syncStation();
    MapRenderer renderer_;
    MapScene scene_;
    QTimer minute_timer_;
    int overlay_token_ = 0, style_token_ = 0;
};

} // namespace hamclock

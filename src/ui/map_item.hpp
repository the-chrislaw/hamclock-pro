#pragma once
// QML map: paints land, coast, night shade, subsolar point and the DE/DX path
// with the pure QPainter MapRenderer (no QtWidgets) and fills its whole item.
#include <QQuickPaintedItem>
#include "../map/map_renderer.hpp"
#include "../map/view_options.hpp"
#include <QTimer>
#include <QVariantList>
class QMouseEvent;

namespace hamclock {

class AppSettings;

class MapItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(double uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)
    Q_PROPERTY(bool showPath READ showPath WRITE setShowPath NOTIFY showPathChanged)
    Q_PROPERTY(bool nightShade READ nightShade WRITE setNightShade NOTIFY nightShadeChanged)
    // -1 = each layer's own labels, else 0 None, 1 Prefix, 2 Call, 3 Dot.
    Q_PROPERTY(int labelMode READ labelMode WRITE setLabelMode NOTIFY labelModeChanged)
    Q_PROPERTY(QStringList layerIds READ layerIds NOTIFY layersChanged)
    // Base style id ("" = plain map) and rotation interval in seconds (0 = off).
    Q_PROPERTY(QString style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(int rotation READ rotation WRITE setRotation NOTIFY styleChanged)
    Q_PROPERTY(QStringList styleIds READ styleIds NOTIFY styleChanged)
    Q_PROPERTY(QVariantList rotationChoices READ rotationChoices CONSTANT)
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
    bool nightShade() const { return options_.night_shade; }
    void setNightShade(bool on);
    int labelMode() const { return options_.label_mode; }
    void setLabelMode(int mode);
    QStringList layerIds() const;
    QString style() const { return options_.style; }
    void setStyle(const QString& id);
    int rotation() const { return options_.rotation_s; }
    void setRotation(int seconds);
    QStringList styleIds() const;
    QVariantList rotationChoices() const;
    // Map View menu: per-layer visibility, independent of which panes are shown.
    Q_INVOKABLE bool layerVisible(const QString& id) const { return options_.layer_visible(id); }
    Q_INVOKABLE void setLayerVisible(const QString& id, bool on);
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
    void nightShadeChanged();
    void labelModeChanged();
    void layersChanged();
    void styleChanged();
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
    void syncOverlays();
    void applyStyleOptions();
    void saveOptions();
    MapViewOptions options_;
    MapRenderer renderer_;
    MapScene scene_;
    QTimer minute_timer_;
    std::size_t rotation_count_ = 0;
    int overlay_token_ = 0, style_token_ = 0;
};

} // namespace hamclock

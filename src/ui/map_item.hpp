#pragma once
// QML map: paints land, coast, night shade, subsolar point and the DE/DX path
// with the pure QPainter MapRenderer (no QtWidgets) and fills its whole item.
#include <QQuickPaintedItem>
#include "../map/map_renderer.hpp"
#include <QTimer>

namespace hamclock {

class AppSettings;

class MapItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(bool showPath READ showPath WRITE setShowPath NOTIFY showPathChanged)
public:
    explicit MapItem(QQuickItem* parent = nullptr);
    ~MapItem() override;

    // Must be called once from C++ before the item is shown (see main.cpp).
    static void setSettings(AppSettings* settings);

    bool dark() const { return scene_.dark; }
    void setDark(bool dark);
    bool showPath() const { return scene_.route_visible; }
    void setShowPath(bool show);
    void paint(QPainter* painter) override;

signals:
    void darkChanged();
    void showPathChanged();

private:
    void syncStation();
    MapRenderer renderer_;
    MapScene scene_;
    QTimer minute_timer_;
    int overlay_token_ = 0, style_token_ = 0;
};

} // namespace hamclock

#pragma once
// QML map: renders the existing WorldMapWidget (land, coast, night shade,
// subsolar point, DE/DX path) into a QQuickPaintedItem, so the map code is
// shared with the legacy dashboard rather than duplicated.
#include <QQuickPaintedItem>
#include <QTimer>
#include <memory>

namespace hamclock {

class AppSettings;
class WorldMapWidget;

class MapItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(bool showPath READ showPath WRITE setShowPath NOTIFY showPathChanged)
public:
    explicit MapItem(QQuickItem* parent = nullptr);
    ~MapItem() override;

    // Must be called once from C++ before the item is shown (see main.cpp).
    static void setSettings(AppSettings* settings);

    bool dark() const { return dark_; }
    void setDark(bool dark);
    bool showPath() const { return show_path_; }
    void setShowPath(bool show);
    void paint(QPainter* painter) override;

signals:
    void darkChanged();
    void showPathChanged();

private:
    void syncStation();
    std::unique_ptr<WorldMapWidget> map_;
    QTimer minute_timer_;
    bool dark_ = false;
    bool show_path_ = true;
};

} // namespace hamclock

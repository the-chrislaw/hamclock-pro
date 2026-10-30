#pragma once
// List of dashboard widgets for QML. Widgets add themselves through a
// generated register function (see hamclock_add_widget in CMake); the
// dashboard shows each entry's card with its model.
#include "widget_model.hpp"
#include "../net/polled_source.hpp"
#include <QAbstractListModel>
#include <QStringList>
#include <QUrl>
#include <vector>

namespace hamclock {

class AppSettings;
class SatAlertService;

// Everything a widget may use. Widgets must not reach for globals.
struct WidgetContext {
    AppSettings* settings = nullptr;
    Transport transport;   // HTTP GET; fake in tests
    AuthTransport auth_transport; // optional GET with a bearer token; unset in tests/fixtures
    QObject* owner = nullptr;
    SatAlertService* sat_alerts = nullptr; // pass alert selection (Passes pane "Alerts" filter); may be null
};

// Placement hint for the layout: "strip" = instrument row (small cards),
// "side" = sidebar list, "wide" = full-width panel.
struct WidgetInfo {
    QString id;
    QString title;
    QUrl card;       // qrc:/widgets/<id>/<Card>.qml
    QString area = "strip";
    int order = 100; // lower first
};

class WidgetRegistry : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Role { IdRole = Qt::UserRole + 1, TitleRole, CardRole, AreaRole, ModelRole };
    explicit WidgetRegistry(QObject* parent = nullptr);

    // Takes ownership of model (reparented to the registry). Returns false when the
    // widget is not offered on this platform (see widget_available); the model is
    // kept alive but never listed, so callers should not start it.
    bool add(WidgetInfo info, WidgetModel* model);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE QObject* model(const QString& id) const;
    // Ids of the listed widgets, in list order.
    Q_INVOKABLE QStringList ids() const;

signals:
    void countChanged();

private:
    struct Entry { WidgetInfo info; WidgetModel* model; };
    std::vector<Entry> entries_;
};

// Panes that read the station's own hardware or files: rigctld/rotctld/flrig,
// gpsd or a serial GPS, a local ADS-B receiver, Linux sensors and a typed ADIF
// log path. iPhone and iPad have none of these and no way to point the panes at
// them, so the registry leaves them out there instead of offering dead panes.
bool widget_available(const QString& id, bool ios);

// Generated from every hamclock_add_widget() call (widgets_generated.cpp).
void register_all_widgets(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock

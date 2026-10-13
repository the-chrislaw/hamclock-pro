#pragma once
// List of dashboard widgets for QML. Widgets add themselves through a
// generated register function (see hamclock_add_widget in CMake); the
// dashboard shows each entry's card with its model.
#include "widget_model.hpp"
#include "../net/polled_source.hpp"
#include <QAbstractListModel>
#include <QUrl>
#include <vector>

namespace hamclock {

class AppSettings;

// Everything a widget may use. Widgets must not reach for globals.
struct WidgetContext {
    AppSettings* settings = nullptr;
    Transport transport;   // HTTP GET; fake in tests
    QObject* owner = nullptr;
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

    // Takes ownership of model (reparented to the registry).
    void add(WidgetInfo info, WidgetModel* model);
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE QObject* model(const QString& id) const;

signals:
    void countChanged();

private:
    struct Entry { WidgetInfo info; WidgetModel* model; };
    std::vector<Entry> entries_;
};

// Generated from every hamclock_add_widget() call (widgets_generated.cpp).
void register_all_widgets(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock

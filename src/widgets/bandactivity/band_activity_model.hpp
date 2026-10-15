#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QPointer>
#include <QVariantList>

namespace hamclock {

// Band x continent heatmap over the DX cluster's recent spots. It reads the `dxcluster` model through the
// registry (no link-time dependency) and mirrors its state: never live unless the cluster link is live.
class BandActivityModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY activityChanged) // {band, total, NA, SA, EU, AF, AS, OC}
    Q_PROPERTY(QStringList continents READ continents CONSTANT)
    Q_PROPERTY(int maxCell READ maxCell NOTIFY activityChanged)    // largest single cell, for shading
    Q_PROPERTY(QString busiestText READ busiestText NOTIFY activityChanged)
public:
    explicit BandActivityModel(const WidgetContext& context, QObject* parent = nullptr);
    // Source model: any QObject with `activity` (list of maps), `state`, `statusText`, `updatedAt` properties.
    void attach(QObject* source);

    QVariantList rows() const { return rows_; }
    QStringList continents() const;
    int maxCell() const { return max_cell_; }
    QString busiestText() const { return busiest_; }

signals:
    void activityChanged();

private slots:
    void refresh();

private:
    QPointer<QObject> source_;
    QVariantList rows_;
    int max_cell_ = 0;
    QString busiest_;
};

void register_widget_bandactivity(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock

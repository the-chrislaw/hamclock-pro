#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_dst_parse.hpp"
#include <QVariantList>
#include <vector>

namespace hamclock {

// Dst (disturbance storm time) index: latest hourly value, 24 h minimum and history.
class SolarDstModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY valuesChanged)
    Q_PROPERTY(int dst READ dst NOTIFY valuesChanged)
    Q_PROPERTY(QString word READ word NOTIFY valuesChanged)
    Q_PROPERTY(int min24 READ min24 NOTIFY valuesChanged)
    Q_PROPERTY(QString observed READ observed NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY valuesChanged)
public:
    explicit SolarDstModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    // False until a record at most 6 h old is known: an old index is never shown as current.
    bool hasData() const { return valid_; }
    int dst() const;
    QString word() const;
    int min24() const;                // lowest value in the 24 h up to the latest record
    QString observed() const;         // "02:00Z", empty without data
    QVariantList history() const;     // last 48 h, oldest first, nT
    PolledSource& source() { return *source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    std::vector<solardst::Sample> samples_;
    bool valid_ = false;
};

void register_widget_solardst(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock

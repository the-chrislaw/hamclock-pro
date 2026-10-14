#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_hamqsl_parse.hpp"
#include <QVariantList>

namespace hamclock {

// HamQSL (N0NBH) calculated HF day/night band conditions and VHF openings.
class SolarHamqslModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList bands READ bands NOTIFY valuesChanged)  // {band, day, night, dayRank, nightRank}
    Q_PROPERTY(QVariantList vhf READ vhf NOTIFY valuesChanged)      // {name, location, status, open}
    Q_PROPERTY(QString updated READ updated NOTIFY valuesChanged)
public:
    explicit SolarHamqslModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    // False until a report at most 6 h old is known: old conditions are never shown as current.
    bool hasData() const { return valid_; }
    QVariantList bands() const;
    QVariantList vhf() const;
    QString updated() const;  // "08:10Z", empty without data
    PolledSource& source() { return *source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    solarhamqsl::Report report_;
    bool valid_ = false;
};

void register_widget_solarhamqsl(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock

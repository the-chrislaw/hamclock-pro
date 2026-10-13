#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "band_outlook.hpp"
#include <QVariantList>

namespace hamclock {

class AppSettings;

// HF band outlook (day and night) computed locally from NOAA SWPC SFI, Kp and
// A index; see band_outlook.hpp for the model. "period" is day/night at DE.
class BandsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasOutlook READ hasOutlook NOTIFY outlookChanged)
    Q_PROPERTY(QVariantList bands READ bands NOTIFY outlookChanged)
    Q_PROPERTY(QString inputsText READ inputsText NOTIFY outlookChanged)
    Q_PROPERTY(QString period READ period NOTIFY outlookChanged)
public:
    explicit BandsModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasOutlook() const { return has_outlook_; }
    // Each entry: {band, day, night} with "Good" / "Fair" / "Poor".
    QVariantList bands() const;
    QString inputsText() const;
    QString period() const { return period_; }

    PolledSource& kpSource() { return *kp_source_; }
    PolledSource& fluxSource() { return *flux_source_; }

signals:
    void outlookChanged();

private:
    void parse();
    void refreshState();
    void updatePeriod();

    AppSettings* settings_;
    PolledSource* kp_source_;
    PolledSource* flux_source_;
    bool has_outlook_ = false;
    double sfi_ = 0, kp_ = 0, a_ = 0;
    std::vector<bands::BandOutlook> outlook_;
    QString period_ = "unknown";
};

void register_widget_bands(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock

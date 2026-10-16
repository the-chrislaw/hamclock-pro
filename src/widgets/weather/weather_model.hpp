#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "weather_parse.hpp"
#include <optional>

namespace hamclock {

class AppSettings;

// Current weather at DE and DX from Open-Meteo (no key; free for
// non-commercial use with attribution). Each end is its own PolledSource; a
// response is only accepted when it answers for the requested location, so a
// cached document from an old station position is never shown.
class WeatherModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasStation READ hasStation NOTIFY changed)
    Q_PROPERTY(QString deTemp READ deTemp NOTIFY changed)
    Q_PROPERTY(QString deSummary READ deSummary NOTIFY changed)
    Q_PROPERTY(QString deWind READ deWind NOTIFY changed)
    Q_PROPERTY(QString deAir READ deAir NOTIFY changed)
    Q_PROPERTY(QString dxAir READ dxAir NOTIFY changed)
    Q_PROPERTY(QString dxTemp READ dxTemp NOTIFY changed)
    Q_PROPERTY(QString dxSummary READ dxSummary NOTIFY changed)
    Q_PROPERTY(QString dxWind READ dxWind NOTIFY changed)
    Q_PROPERTY(QString deCity READ deCity NOTIFY changed)
    Q_PROPERTY(QString dxCity READ dxCity NOTIFY changed)
    Q_PROPERTY(QString deWindyUrl READ deWindyUrl NOTIFY changed)
    Q_PROPERTY(QString dxWindyUrl READ dxWindyUrl NOTIFY changed)
    Q_PROPERTY(QString wefaxUrl READ wefaxUrl CONSTANT)
    Q_PROPERTY(QString deLocal READ deLocal NOTIFY changed)
    Q_PROPERTY(QString dxLocal READ dxLocal NOTIFY changed)
public:
    explicit WeatherModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasStation() const;
    QString deTemp() const { return temp(de_); }
    QString deSummary() const { return summary(de_); }
    QString deWind() const { return wind(de_); }
    QString deAir() const { return air(de_); }
    QString dxAir() const { return air(dx_); }
    QString dxTemp() const { return temp(dx_); }
    QString dxSummary() const { return summary(dx_); }
    QString dxWind() const { return wind(dx_); }

    QString deCity() const { return de_.place; }
    QString dxCity() const { return dx_.place; }
    QString deWindyUrl() const { return windy(de_); }
    QString dxWindyUrl() const { return windy(dx_); }
    QString wefaxUrl() const { return weather::wefax_url(); }
    QString deLocal() const { return local(de_); }
    QString dxLocal() const { return local(dx_); }

    PolledSource* deSource() const { return de_.source; }
    PolledSource* dxSource() const { return dx_.source; }

signals:
    void changed();

private:
    struct End {
        PolledSource* source = nullptr;
        double lat = 0, lon = 0;
        std::optional<weather::Current> now;
        PolledSource* place_source = nullptr;  // OSM Nominatim reverse lookup, weekly
        QString place;                         // "San Francisco, US"; empty until known
    };
    void build();
    void rebuildEnd(End& end, const QString& cache_name, double lat, double lon);
    void parse(End& end);
    void parsePlace(End& end);
    void refreshState();
    static QString windy(const End& e);  // empty until the end has a position
    static QString temp(const End& e);
    static QString summary(const End& e);
    static QString wind(const End& e);
    QString local(const End& e) const;  // "14:32 (UTC-7)"
    static QString air(const End& e);  // "61% RH · dew 8 °C · 1015 hPa · 20% cloud"

    WidgetContext context_;
    AppSettings* settings_;
    End de_, dx_;
};

void register_widget_weather(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock

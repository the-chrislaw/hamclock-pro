#include "apple_attribution.hpp"
#include <QVariantMap>

namespace hamclock {

const std::vector<AttributionSource>& attributionSources() {
    static const std::vector<AttributionSource> sources = {
        {"NOAA Space Weather Prediction Center", "Kp, solar flux, X-ray, alerts, D-RAP, aurora",
         "US government data, public domain", "https://www.swpc.noaa.gov"},
        {"NASA Solar Dynamics Observatory", "Sun images (AIA, HMI)", "NASA imagery, credit NASA/SDO",
         "https://sdo.gsfc.nasa.gov"},
        {"HamQSL / N0NBH", "Solar and VHF conditions summary", "Free to use with credit to HamQSL.com and N0NBH",
         "https://www.hamqsl.com"},
        {"CelesTrak", "Satellite orbital elements", "Public GP data", "https://celestrak.org"},
        {"Open-Meteo", "Weather at DE and DX", "CC BY 4.0, weather data by Open-Meteo.com",
         "https://open-meteo.com/en/terms"},
        {"National Weather Service", "US forecasts and alerts", "US government data, public domain",
         "https://www.weather.gov"},
        {"National Hurricane Center", "Tropical storms", "US government data, public domain",
         "https://www.nhc.noaa.gov"},
        {"USGS", "Earthquakes", "US government data, public domain", "https://earthquake.usgs.gov"},
        {"NASA FIRMS", "Active fires", "NASA data, credit NASA FIRMS", "https://firms.modaps.eosdis.nasa.gov"},
        {"PSKReporter", "Who hears your signal", "Public reception reports, queried at the published rate limit",
         "https://pskreporter.info"},
        {"Parks on the Air", "POTA activator spots", "Public API", "https://pota.app"},
        {"Summits on the Air", "SOTA activator spots", "Public API", "https://www.sota.org.uk"},
        {"SatNOGS", "Satellite transmitter data", "Open data, CC BY-SA 4.0", "https://db.satnogs.org"},
        {"OpenStreetMap Nominatim", "Place names", "ODbL, (c) OpenStreetMap contributors",
         "https://www.openstreetmap.org/copyright"},
        {"Natural Earth", "Map coastlines, borders and cities", "Public domain", "https://www.naturalearthdata.com"},
        {"SGP4 (David Vallado)", "Satellite position model", "MIT licence, via the python-sgp4 distribution",
         "https://github.com/brandon-rhodes/python-sgp4"},
        {"AD1C cty.dat", "Callsign prefix to country", "MIT licence", "https://www.country-files.com"},
    };
    return sources;
}

QVariantList attributionModel() {
    QVariantList out;
    for (const auto& s : attributionSources()) {
        QVariantMap m;
        m["name"] = s.name;
        m["usedFor"] = s.used_for;
        m["terms"] = s.terms;
        m["url"] = s.url;
        out.append(m);
    }
    return out;
}

} // namespace hamclock

// Weather widget: Open-Meteo parser and model, offline.
// Fixture `forecast` is a real Open-Meteo response captured 2026-10-13 17:30 UTC
// for 37.77,-122.42.
#include "../weather_model.hpp"
#include "../weather_parse.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QTimer>
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    // Start from an empty cache: a place name cached by an earlier run is (rightly)
    // not re-requested, which would hide the request this test counts.
    QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).removeRecursively();
    // On the Mac PolledSource follows XDG_CACHE_HOME (set per test by CMake) instead.
    if (const QString xdg = qEnvironmentVariable("XDG_CACHE_HOME"); QDir::isAbsolutePath(xdg))
        QDir(xdg + "/" + QCoreApplication::applicationName() + "/sources").removeRecursively();
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile file(QDir(argv[1]).filePath("forecast"));
    check(file.open(QIODevice::ReadOnly), "fixture readable");
    const QByteArray body = file.readAll();

    check(weather::windy_url(37.7749, -122.4194) == "https://www.windy.com/?37.77,-122.42,6", "windy url");
    check(weather::wefax_url() == "https://www.weather.gov/marine/radiofax", "wefax url");
    check(weather::windy_url(-33.9, 151.2) == "https://www.windy.com/?-33.90,151.20,6", "windy url southern hemisphere");

    QFile rev(QDir(argv[1]).filePath("reverse"));
    check(rev.open(QIODevice::ReadOnly), "reverse fixture readable");
    const QByteArray rev_body = rev.readAll();
    check(weather::parse_place(rev_body) == "San Francisco, US", "place from real reverse reply");
    check(weather::parse_place(R"({"address":{"village":"Tiny","country_code":"nz"}})") == "Tiny, NZ", "village fallback");
    check(weather::parse_place(R"({"address":{"state":"Texas","country_code":"us"}})") == "Texas, US", "state fallback");
    check(weather::parse_place(R"({"address":{"city":"Oslo","country_code":"x1"}})") == "Oslo", "bad country code dropped");
    check(weather::parse_place(R"({"error":"Unable to geocode"})").isEmpty(), "open sea gives no place");
    check(weather::parse_place("garbage").isEmpty(), "garbage gives no place");
    check(weather::reverse_url(37.774, -122.416).contains("lat=37.77&lon=-122.42") &&
          weather::reverse_url(1, 2).contains("zoom=10"), "reverse url");

    const auto c = weather::parse_current(body);
    check(c && c->temperature_c == 15.7 && c->humidity_pct == 61, "temperature and humidity");
    check(c && c->wind_kmh == 7.9 && c->wind_dir_deg == 344 && c->weather_code == 0 && c->is_day, "wind, code, day");
    check(c && !c->pressure_hpa && !c->cloud_pct, "optional fields absent in older response");
    // Synthetic document (not a live capture): the optional pressure/cloud fields.
    const auto full = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3,"pressure_msl":1015.4,"cloud_cover":87}})");
    check(full && full->pressure_hpa && std::lround(*full->pressure_hpa) == 1015 && full->cloud_pct == 87, "pressure and cloud parsed");
    const auto dew = weather::dew_point_c(20, 50);  // reference: 9.3 °C
    check(dew && std::abs(*dew - 9.27) < 0.1, "dew point at 20 C / 50 %");
    check(weather::dew_point_c(20, 100) && std::abs(*weather::dew_point_c(20, 100) - 20) < 0.01, "dew point equals temperature at 100 % RH");
    check(!weather::dew_point_c(20, 0) && !weather::dew_point_c(80, 50), "dew point rejects 0 % RH and absurd temperature");
    check(c && !c->pressure_change_hpa, "no trend without hourly series");
    // Synthetic: hourly series oldest first (past_hours=3, forecast_hours=1).
    const auto trend = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3,"pressure_msl":1015.4},"hourly":{"time":["a","b","c","d"],"pressure_msl":[1018.0,1017.0,1016.0,1015.4]}})");
    check(trend && trend->pressure_change_hpa && std::abs(*trend->pressure_change_hpa + 2.6) < 1e-9, "3 h pressure change");
    const auto nulls = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3},"hourly":{"pressure_msl":[null,1017.0,1016.0,1015.4]}})");
    check(nulls && !nulls->pressure_change_hpa, "null hourly value gives no trend");
    check(weather::pressure_trend_word(2.6) == "Rising" && weather::pressure_trend_word(-2.6) == "Falling" &&
          weather::pressure_trend_word(1.0) == "Steady" && weather::pressure_trend_word(-0.4) == "Steady", "trend words");
    check(c && c->utc_offset_s == 0, "fixture (timezone=UTC) reports offset 0");
    const auto zoned = weather::parse_current(
        R"({"latitude":1,"longitude":1,"utc_offset_seconds":-25200,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3}})");
    check(zoned && zoned->utc_offset_s == -25200, "utc offset parsed");
    const auto noon = QDateTime(QDate(2026, 10, 13), QTime(12, 0), QTimeZone::utc());
    check(weather::local_time_text(noon, -25200) == "05:00 (UTC-7)" && weather::local_time_text(noon, 19800) == "17:30 (UTC+5:30)" &&
          weather::local_time_text(noon, 0) == "12:00 (UTC+0)", "local time text");
    check(c && !c->high_c && !c->low_c, "no high/low without daily block");
    // Synthetic: daily block (forecast_days=1).
    const auto day = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3},"daily":{"temperature_2m_max":[18.4],"temperature_2m_min":[8.6]}})");
    check(day && day->high_c && day->low_c && *day->high_c == 18.4 && *day->low_c == 8.6, "daily high/low parsed");
    const auto badday = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3},"daily":{"temperature_2m_max":[5.0],"temperature_2m_min":[8.6]}})");
    check(badday && !badday->high_c && !badday->low_c, "low above high is ignored");
    const auto nulday = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3},"daily":{"temperature_2m_max":[null],"temperature_2m_min":[8.6]}})");
    check(nulday && !nulday->high_c, "null daily value ignored");
    const auto two = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3},"daily":{"temperature_2m_max":[18.4,20.1],"temperature_2m_min":[8.6,9.2]}})");
    check(two && two->tomorrow_high_c && *two->tomorrow_high_c == 20.1 && *two->tomorrow_low_c == 9.2, "tomorrow high/low parsed");
    check(day && !day->tomorrow_high_c, "no tomorrow with one day");
    const auto badtom = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3},"daily":{"temperature_2m_max":[18.4,3],"temperature_2m_min":[8.6,9.2]}})");
    check(badtom && badtom->high_c && !badtom->tomorrow_high_c, "bad tomorrow ignored, today kept");
    check(weather::request_url(1, 2).contains("forecast_days=2"), "url asks for two days");
    check(weather::request_url(1, 2).contains("temperature_2m_max"), "url asks for daily extremes");
    check(weather::request_url(1, 2).contains("timezone=auto"), "url asks for local zone");
    check(weather::request_url(1, 2).contains("past_hours=3"), "url asks for hourly pressure");
    check(weather::compass_point(344) == "NNW" && weather::compass_point(0) == "N" &&
          weather::compass_point(360) == "N" && weather::compass_point(348) == "NNW" &&
          weather::compass_point(349) == "N" && weather::compass_point(90) == "E", "compass rose edges");
    check(weather::wmo_description(0) == "Clear sky" && weather::wmo_description(95) == "Thunderstorm" &&
          weather::wmo_description(42) == "Unknown", "WMO descriptions");
    check(weather::celsius_to_fahrenheit(100) == 212 && weather::celsius_to_fahrenheit(-40) == -40, "F conversion");
    check(weather::request_url(37.7749, -122.4194).contains("latitude=37.77&longitude=-122.42"), "url rounds");

    for (const char* bad : {"", "{}", "[", "{\"latitude\":1,\"longitude\":1,\"current\":{}}",
                            "{\"latitude\":1,\"longitude\":1,\"current\":{\"temperature_2m\":\"15\",\"relative_humidity_2m\":5,\"wind_speed_10m\":1,\"wind_direction_10m\":1,\"weather_code\":0}}",
                            "{\"latitude\":1,\"longitude\":1,\"current\":{\"temperature_2m\":999,\"relative_humidity_2m\":5,\"wind_speed_10m\":1,\"wind_direction_10m\":1,\"weather_code\":0}}"})
        check(!weather::parse_current(bad), "malformed weather rejected");

    // Model: DE = fixture location, DX far away (server answers for the fixture point, so rejected).
    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    WidgetContext context;
    context.settings = &settings;
    int requests = 0;
    bool fail_all = false;
    int place_requests = 0;
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult r;
        const bool place = url.host().contains("nominatim");
        (place ? place_requests : requests)++;
        if (fail_all) r.error = "Host unreachable";
        else { r.http_status = 200; r.body = place ? rev_body : body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    WeatherModel model(context);
    const auto now = QDateTime(QDate(2026, 10, 13), QTime(17, 40), QTimeZone::utc());
    model.setClock([&] { return now; });
    model.start();
    for (int i = 0; i < 10; ++i) app.processEvents();
    check(model.state() == "unconfigured" && requests == 0, "no station -> unconfigured, no requests");

    check(settings.saveStation("N0CALL", 37.77, -122.42, 51.5, -0.1, "").isEmpty(), "station saved");
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(requests == 2, "one request per end");
    check(model.deSource() && model.dxSource(), "sources built");
    for (auto* s : {model.deSource(), model.dxSource()}) s->setClock([&] { return now; });
    model.deSource()->refreshNow();
    model.dxSource()->refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.deTemp() == "15.7 °C" && model.deWind() == "NNW 8 km/h", "DE values");
    check(model.deAir() == "61% RH · dew 8 °C", "air line without optional fields");
    check(model.deSummary() == "Clear sky", "DE summary");
    check(model.dxTemp().isEmpty(), "response for another place never shown at DX");
    check(model.state() == "stale", "one end missing -> not live");

    // Moving DX to the fixture location makes it valid; both fresh -> live.
    check(settings.saveStation("N0CALL", 37.77, -122.42, 37.76, -122.41, "").isEmpty(), "DX moved");
    for (int i = 0; i < 20; ++i) app.processEvents();
    for (auto* s : {model.deSource(), model.dxSource()}) s->setClock([&] { return now; });
    check(model.dxTemp() == "15.7 °C" && model.state() == "live", "both ends fresh -> live");

    // Place names: delayed so a burst of DX changes collapses, then one request per end.
    check(place_requests == 0 && model.deCity().isEmpty(), "no place request before the delay");
    {
        QEventLoop loop;
        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        loop.exec();
    }
    check(place_requests == 2, "one place request per end after the delay");
    check(model.deCity() == "San Francisco, US" && model.dxCity() == "San Francisco, US", "city names stored");

    fail_all = true;
    model.setClock([&] { return now.addSecs(3 * 3600); });
    for (auto* s : {model.deSource(), model.dxSource()}) { s->setClock([&] { return now.addSecs(3 * 3600); }); s->refreshNow(); }
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.deTemp() == "15.7 °C", "outage keeps last value, marked stale");

    std::cout << "Weather: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}

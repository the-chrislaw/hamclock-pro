// Place search: port of src/ui/location_tools.cpp parseSearchReply and the Open-Meteo timezone field.
// Inline city-level samples (no street addresses); the weather fixture supplies a real Open-Meteo reply.
#include "../hcpcore.h"
#include "../places.hpp"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

int main() {
    using namespace hcp;
    const std::string reply =
        R"([{"place_id":1,"lat":"48.8534951","lon":"2.3483915","display_name":"Paris, Île-de-France, France"},)"
        R"({"lat":"45.5202471","lon":"-122.674194","display_name":"Portland, Multnomah County, Oregon, United States"},)"
        R"({"lat":48.85,"lon":2.35,"display_name":"numeric coordinates are skipped, as in Qt"},)"
        R"({"lat":"95","lon":"0","display_name":"out of range"},{"lat":"x","lon":"1","display_name":"junk"},7])";
    const auto places = places::parse_nominatim(reply);
    check(places && places->size() == 2, "two usable rows");
    if (places && places->size() == 2) {
        check((*places)[0].name == "Paris, Île-de-France, France" && near((*places)[0].lat, 48.8534951, 1e-9) &&
                  near((*places)[0].lon, 2.3483915, 1e-9), "Paris");
        check(near((*places)[1].lon, -122.674194, 1e-9), "Portland");
    }
    check(!places::parse_nominatim("{}") && !places::parse_nominatim(""), "non-array rejected");
    const auto none = places::parse_nominatim("[]");
    check(none && none->empty(), "empty array is valid");

    std::vector<HcpPlace> out(4);
    check(hcp_nominatim_parse(reply.data(), reply.size(), out.data(), out.size()) == 2 &&
              std::string(out[0].name) == "Paris, Île-de-France, France" && near(out[1].lat, 45.5202471, 1e-9),
          "C ABI nominatim");
    check(hcp_nominatim_parse(reply.data(), reply.size(), out.data(), 1) == 1 && hcp_nominatim_parse("{}", 2, out.data(), 4) == -1,
          "C ABI cap and rejection");
    // UTF-8 safe truncation: a name cut inside the two-byte "Î" must not keep half of it.
    std::string long_name(158, 'a');
    const std::string cut = R"([{"lat":"1","lon":"2","display_name":")" + long_name + "Îb\"}]";
    check(hcp_nominatim_parse(cut.data(), cut.size(), out.data(), 1) == 1 && std::string(out[0].name) == long_name,
          "C ABI truncates on a UTF-8 boundary");

    char tz[64];
    const auto weather = fixture("forecast");
    const std::string paris = R"({"latitude":48.86,"longitude":2.34,"timezone":"Europe/Paris","current":{}})";
    check(places::open_meteo_timezone(paris) == "Europe/Paris", "timezone field");
    check(hcp_open_meteo_timezone(paris.data(), paris.size(), tz, sizeof tz) == 1 && std::string(tz) == "Europe/Paris",
          "C ABI timezone");
    check(hcp_open_meteo_timezone("{\"current\":{}}", 14, tz, sizeof tz) == 0 && tz[0] == '\0' &&
              hcp_open_meteo_timezone("[]", 2, tz, sizeof tz) == 0, "C ABI timezone absent");
    check(hcp_open_meteo_timezone(weather.data(), weather.size(), tz, sizeof tz) == 1 && std::string(tz) == "GMT",
          "real Open-Meteo reply (weather fixture)");
    return finish("places");
}

#include "station_edit.hpp"
#include "station_view.hpp"
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace hamclock {
namespace {
int index(StationField field) {
    switch (field) {
    case StationField::Callsign: return 0;
    case StationField::DeLatitude: return 1;
    case StationField::DeLongitude: return 2;
    case StationField::DxLatitude: return 3;
    case StationField::DxLongitude: return 4;
    case StationField::Appearance: return 5;
    }
    return -1;
}
std::string text(double value) {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(17) << value;
    return output.str();
}
double decimal(const std::string& value) {
    if (value.empty() || value.size() > 64) throw std::invalid_argument("Invalid coordinate text");
    for (unsigned char c : value)
        if (!((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E'))
            throw std::invalid_argument("Invalid coordinate text");
    std::istringstream input(value);
    input.imbue(std::locale::classic());
    double result;
    input >> result;
    if (!input || input.peek() != std::char_traits<char>::eof())
        throw std::invalid_argument("Invalid coordinate text");
    return result;
}
}
StationEdit::StationEdit(const StationSettings& settings) : de_zone_(settings.de_zone) {
    (void)make_station_view(settings);
    original_ = {settings.callsign, text(settings.de.latitude), text(settings.de.longitude),
                 text(settings.dx.latitude), text(settings.dx.longitude), settings.appearance};
    draft_ = original_;
}
bool StationEdit::set(StationField field, const std::string& value) {
    const int position = index(field);
    if (position < 0) return false;
    const std::size_t maximum = position == 0 ? 16 : position == 5 ? 6 : 64;
    if (value.size() > maximum) return false;
    draft_[static_cast<std::size_t>(position)] = value;
    return true;
}
std::string StationEdit::get(StationField field) const {
    const int position = index(field);
    if (position < 0) throw std::invalid_argument("Invalid station field");
    return draft_[static_cast<std::size_t>(position)];
}
bool StationEdit::dirty() const { return draft_ != original_; }
void StationEdit::cancel() { draft_ = original_; }
StationSettings StationEdit::validated() const {
    StationSettings settings{{decimal(draft_[1]), decimal(draft_[2])},
                             {decimal(draft_[3]), decimal(draft_[4])}, draft_[0], draft_[5], de_zone_};
    (void)make_station_view(settings);
    return settings;
}
}

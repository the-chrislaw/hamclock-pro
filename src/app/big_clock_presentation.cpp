#include "big_clock_presentation.hpp"

#include <stdexcept>

namespace hamclock {
namespace {
bool leap_year(int year) noexcept {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
void validate_times(const BigClockCivilTimes& times) {
    if (times.utc) validate_big_clock_civil_time(*times.utc);
    if (times.de) validate_big_clock_civil_time(*times.de);
}
template<std::size_t Size>
void two_digits(std::array<char, Size>& text, std::size_t offset, int value) noexcept {
    text[offset] = static_cast<char>('0' + value / 10);
    text[offset + 1] = static_cast<char>('0' + value % 10);
}
} // namespace

void validate_big_clock_civil_time(const BigClockCivilTime& time) {
    if (time.year < 1 || time.year > 9999 || time.month < 1 || time.month > 12 ||
        time.day < 1 || time.hour < 0 || time.hour > 23 ||
        time.minute < 0 || time.minute > 59 || time.second < 0 || time.second > 59)
        throw std::invalid_argument("Invalid Big Clock civil fields");
    constexpr int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const int days = month_days[time.month - 1] +
                     ((time.month == 2 && leap_year(time.year)) ? 1 : 0);
    if (time.day > days) throw std::invalid_argument("Invalid Big Clock calendar date");
}

BigClockDigitalView present_big_clock_digital(
    const BigClockPreferences& preferences, const BigClockDigitalInput& input) {
    validate_big_clock_preferences(preferences);
    if (input.synchronization != SyncStatus::Unknown &&
        input.synchronization != SyncStatus::Synchronized &&
        input.synchronization != SyncStatus::Unsynchronized)
        throw std::invalid_argument("Unknown Big Clock synchronization status");
    validate_times(input.actual);
    if (input.preview) validate_times(*input.preview);

    const auto& display = input.preview ? *input.preview : input.actual;
    const bool utc = preferences.zone == BigClockZone::Utc;
    const auto& selected = utc ? display.utc : display.de;
    const auto& actual_selected = utc ? input.actual.utc : input.actual.de;
    BigClockDigitalView view{preferences,
        input.preview ? BigClockDisplayBasis::Preview : BigClockDisplayBasis::Actual,
        input.synchronization, BigClockDigitalStatus::Ready,
        selected.has_value(), actual_selected.has_value(), {}, {}, {}};
    view.zone_text = utc ? std::array<char, 4>{'U', 'T', 'C', '\0'} :
                          std::array<char, 4>{'D', 'E', '\0', '\0'};
    if (preferences.face != BigClockFace::Digital) {
        view.status = BigClockDigitalStatus::UnsupportedFace;
        return view;
    }
    if (!selected) {
        view.status = BigClockDigitalStatus::MissingSelectedTime;
        return view;
    }

    const auto& civil = *selected;
    const bool twelve = preferences.hour_format == BigClockHourFormat::Twelve;
    const int hour = twelve ? (civil.hour % 12 == 0 ? 12 : civil.hour % 12) : civil.hour;
    two_digits(view.time_text, 0, hour);
    view.time_text[2] = ':';
    two_digits(view.time_text, 3, civil.minute);
    std::size_t end = 5;
    if (preferences.show_seconds) {
        view.time_text[5] = ':';
        two_digits(view.time_text, 6, civil.second);
        end = 8;
    }
    if (twelve) {
        view.time_text[end] = ' ';
        view.time_text[end + 1] = civil.hour < 12 ? 'A' : 'P';
        view.time_text[end + 2] = 'M';
    }
    if (preferences.show_date) {
        view.date_text[0] = static_cast<char>('0' + civil.year / 1000);
        view.date_text[1] = static_cast<char>('0' + civil.year / 100 % 10);
        view.date_text[2] = static_cast<char>('0' + civil.year / 10 % 10);
        view.date_text[3] = static_cast<char>('0' + civil.year % 10);
        view.date_text[4] = '-';
        two_digits(view.date_text, 5, civil.month);
        view.date_text[7] = '-';
        two_digits(view.date_text, 8, civil.day);
    }
    return view;
}
} // namespace hamclock

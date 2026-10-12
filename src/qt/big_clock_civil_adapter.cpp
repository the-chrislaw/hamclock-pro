#include "big_clock_civil_adapter.hpp"
#include <QDateTime>
#include <QTimeZone>
#include <stdexcept>

namespace hamclock {
BigClockUtcCivilResult adapt_big_clock_utc_civil(std::optional<UtcSeconds> source) {
    if (!source) return {BigClockUtcCivilStatus::MissingSource, source, std::nullopt};
    // This precondition check must precede ALL Qt conversion, including validity
    // probes: the tagged baseline documents undefined out-of-range conversion.
    if (source->value < kBigClockUtcCivilMinimumSeconds ||
        source->value > kBigClockUtcCivilMaximumSeconds) {
        return {BigClockUtcCivilStatus::OutOfRange, source, std::nullopt};
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    const auto datetime = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(source->value), QTimeZone::UTC);
#else
    const auto datetime = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(source->value), Qt::UTC);
#endif
    const auto date = datetime.date();
    const auto time = datetime.time();
    if (!datetime.isValid() || !date.isValid() || !time.isValid() ||
        datetime.timeSpec() != Qt::UTC || datetime.toSecsSinceEpoch() != source->value) {
        return {BigClockUtcCivilStatus::InvalidConversion, source, std::nullopt};
    }
    const BigClockCivilTime civil{date.year(), date.month(), date.day(),
                                 time.hour(), time.minute(), time.second()};
    try {
        validate_big_clock_civil_time(civil);
    } catch (const std::invalid_argument&) {
        return {BigClockUtcCivilStatus::InvalidConversion, source, std::nullopt};
    }
    return {BigClockUtcCivilStatus::Ready, source, civil};
}
} // namespace hamclock

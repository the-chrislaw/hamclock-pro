#include "apple_calibration.hpp"

#include <QObject>
#include <cmath>

namespace hamclock {

HeadingQuality headingQuality(double a)
{
    if (std::isnan(a) || a < 0)
        return HeadingQuality::Unknown;
    if (a <= 15)
        return HeadingQuality::Good;
    if (a <= 30)
        return HeadingQuality::Fair;
    return HeadingQuality::Poor;
}

QString calibrationHint(double accuracy_deg, bool true_north)
{
    QString out;
    const HeadingQuality q = headingQuality(accuracy_deg);
    if (q == HeadingQuality::Poor || q == HeadingQuality::Unknown)
        out = QObject::tr("Compass needs calibration: move the phone in a figure 8.");
    else if (q == HeadingQuality::Fair)
        out = QObject::tr("Compass is approximate: wave the phone in a figure 8 for a better aim.");
    if (!true_north) {
        if (!out.isEmpty())
            out += QLatin1Char(' ');
        out += QObject::tr("Using magnetic north: aim may be off by the local declination.");
    }
    return out;
}

} // namespace hamclock

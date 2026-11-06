#pragma once
// Calibration hint for "Find it in the sky" (#573 item 1). CoreLocation reports
// a heading accuracy (CLHeading.headingAccuracy, degrees; negative = invalid).
// This turns it, and whether only magnetic north is available, into a short
// message for the finder screen. Pure logic, tested on Linux.
#include <QString>

namespace hamclock {

enum class HeadingQuality { Good, Fair, Poor, Unknown };

// accuracy_deg < 0 or NaN -> Unknown; <= 15 Good; <= 30 Fair; else Poor.
HeadingQuality headingQuality(double accuracy_deg);

// Empty when nothing needs saying. Poor/Unknown asks for a figure-8 wave;
// magnetic-only north notes that bearings are off by the local declination.
QString calibrationHint(double accuracy_deg, bool true_north);

} // namespace hamclock

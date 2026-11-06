#include "../src/platform/apple_calibration.hpp"

#include <cmath>
#include <cstdio>

using namespace hamclock;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)

int main()
{
    CHECK(headingQuality(5) == HeadingQuality::Good);
    CHECK(headingQuality(15) == HeadingQuality::Good);
    CHECK(headingQuality(20) == HeadingQuality::Fair);
    CHECK(headingQuality(30) == HeadingQuality::Fair);
    CHECK(headingQuality(31) == HeadingQuality::Poor);
    CHECK(headingQuality(-1) == HeadingQuality::Unknown);
    CHECK(headingQuality(std::nan("")) == HeadingQuality::Unknown);
    CHECK(calibrationHint(5, true).isEmpty());
    CHECK(calibrationHint(45, true).contains("figure 8"));
    CHECK(calibrationHint(-1, true).contains("calibration"));
    CHECK(calibrationHint(20, true).contains("approximate"));
    CHECK(calibrationHint(5, false).contains("magnetic"));
    const QString both = calibrationHint(45, false);
    CHECK(both.contains("figure 8") && both.contains("magnetic"));
    return fails ? 1 : 0;
}

#pragma once
namespace hamclock {
// Truncated ELP2000-82B lunar theory (Chapront-Touze and Chapront 1988),
// fitted to JPL DE200. About 1300 + 900 terms, kept to 3e-8 rad, so the
// position is within about 0.2 arcsec of the full theory (see
// docs/third-party-elp.md). Needed for eclipse contact times, where the
// Meeus chapter 47 series (about 0.1 degree) is far too coarse.
struct ElpPosition {
    double longitude_rad;  // geocentric, [0, 2 pi), J2000 inertial equinox
    double latitude_rad;   // mean dynamical ecliptic of J2000
    double distance_km;
};
// jde: Julian ephemeris day (TT; TDB differs by under 2 ms).
ElpPosition elp_moon(double jde);
// Same position with the longitude carried to the mean equinox of date by
// the general precession in longitude (5029.0966 arcsec per century), the
// frame of Meeus chapter 47. Nutation is not applied.
ElpPosition elp_moon_mean_of_date(double jde);
}

#pragma once
namespace hamclock {
// Truncated VSOP87D Earth (Bretagnon and Francou 1988): heliocentric
// ecliptic longitude, latitude and radius vector of the Earth in the dynamical
// ecliptic and equinox of date. Terms under 2e-7 rad / AU are dropped (156
// terms, about 0.05 arcsec for 1900-2100); see docs/third-party-vsop.md.
struct EarthHelio {
    double longitude_rad;  // [0, 2 pi)
    double latitude_rad;
    double radius_au;
};
// jde: Julian ephemeris day (TT).
EarthHelio vsop87d_earth(double jde);
}

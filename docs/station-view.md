# Read-only native station view

`make_station_view(const StationSettings&)` builds a presentation model from already-loaded settings. It performs no filesystem or network access and never changes the source settings. The application remains responsible for explicit loading, error reporting, startup selection, and rendering.

The model contains original DE/DX coordinates, an uppercase callsign, uppercase six-character Maidenhead locators, short-path great-circle distance rounded to the nearest kilometre (`111 KM`), and initial short-path true bearing rounded to the nearest degree (`90 DEG / TRUE`). A bearing rounding to 360 displays as zero. Coincident and antipodal endpoints display `BEARING UNDEFINED`; no invented direction is substituted.

Calculations use the original geographic domain module and its spherical Earth approximation. These values describe geometry, not propagation, magnetic bearings, or route forecasts. Uppercase formatting supports the current bitmap renderer and does not endorse that renderer as the accepted final visual design.

Invalid coordinates, subnormal settings coordinates, invalid callsigns, and appearances outside `system`/`light`/`dark` throw `std::invalid_argument`, matching persistence validation without creating a temporary file. Callsign validation remains input hygiene rather than regulatory validation. The appearance value is validated but rendering/theme selection belongs to the application.

`run_station_view_tests()` returns a failure count. Tests cover known east/north paths, north rounding, dateline travel, coincident/antipodal behavior, pole locators, unchanged input values, accepted appearances, and rejected invalid settings. No display, device, accessibility, or complete HamClock parity claim follows from these model tests.

# Independent station view and short-path audit

Status: **station presentation, geographic path and main integration locally verified; root computer QA pending**. Review performed 2026-10-03. Scope is the frozen `src/app/station_view.*`, `src/map/path.*`, their tests, main integration and associated documentation. No production source was changed by the independent reviewer. No additional blocker was found for this bounded draft increment; this is not product release approval.

## Main integration re-review

`make check integration-check` independently passed after integration. An additional executable harness saved and rendered station configurations for pole-origin, dateline, antipodal and coincident endpoints; each emitted a valid BMP. Missing/truncated/extra arguments, unknown optional flags, missing settings and malformed settings returned exit 2 and did not create the requested frame. Settings are loaded and converted before SDL initialization, so invalid input does not quietly fall back to the demo or require a display.

The renderer draws actual presentation labels, projects DE/DX with the same function used by the coastline, and splits every adjacent route sample before drawing. Startup intentionally omits the undefined antipodal line; coincidence produces no line segment. The route is computed once at startup and bounded to 65 samples with the default subdivision count. Sixteen-character callsigns occupy 96 logical pixels at the chosen scale, fitting the 183-pixel station panel; undefined-bearing text also fits.

Marker labels are positioned within the map's east/north bounds. The five-pixel marker itself can protrude two pixels beyond the geographic frame at an edge, remaining inside its enclosing panel; overlapping DE/DX labels at coincident or nearby locations are a visual limitation to check in computer QA. No native theme or accessibility claim follows from loading a saved appearance value.

## Independent evidence

A fresh standalone C++17 harness compiled these modules and tests with warnings enabled, AddressSanitizer and UndefinedBehaviorSanitizer. It reported zero failures, exited 0 and produced no sanitizer findings.

In addition to the supplied tests, independently derived fixtures verified:

- From geographic north pole (longitude 25°) to equator at 90°E, the midpoint is 45°N, 90°E. A pole's supplied longitude does not invent a different geographic route.
- Short arcs crossing the dateline, a southern-pole origin, a defined near-antipode and a tiny endpoint separation return finite valid samples.
- Uniform arc spacing was checked using adjacent unit-vector dot products, independently of the production `geometry()` distance helper. Reversing the endpoints gives the same Cartesian path in reverse order within floating-point tolerance.
- 128 subdivisions produce exactly 129 samples. Production tests additionally enforce 1–512 subdivisions and reject zero, 513 and maximum unsigned input before allocation.
- A maximum-length 16-character lowercase callsign becomes bounded uppercase ASCII; a bearing rounded from just below 360° displays zero degrees.

The presentation module performs no I/O, validates persisted settings, preserves source coordinates and does not mutate input. Supplied tests cover invalid bounds/nonfinite/subnormal coordinates, callsign length/characters and appearance values; analytic east/north fixtures, coincident and antipodal undefined bearings, endpoint locators and rounding. Distance is explicitly spherical, rounded to kilometres; bearing is initial short-path true bearing, not magnetic or propagation prediction.

The path module validates endpoints through the shared geometry layer and refuses routes classified antipodal or near-antipodal rather than inventing a heading. Coincident endpoints return one original DE coordinate. Interior sampling rotates a unit vector using the endpoint tangent; fixed subdivision bounds cap output at 513 coordinates. Renderer seam splitting is still necessary. The helper supplies no long path or azimuthal projection.

## Remaining integration gates

- Review main's explicit station loading and argument errors; an invalid/missing settings file must fail visibly rather than silently showing demo values. Load before opening a display when practical.
- Verify actual station labels and coordinates are used, bounded labels fit the native panels, and markers use the same projection as the coastline. Split every adjacent sampled path at the dateline.
- Undefined routes must remain visibly undefined and draw no fabricated line. Coincident DE/DX markers may overlap; labels must not imply a route exists.
- Run final `make check integration-check`, exercise headless station rendering and malformed CLI arguments, then inspect the result through root computer QA.
- Pi performance/input, accepted visual design integration, native theme support, accessibility, long path, additional projections and full feature parity remain separate release gates.

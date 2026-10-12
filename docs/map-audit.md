# Independent coastline and projection audit

Status: **data/import/projection increment independently verified locally after freeze; integrated renderer computer QA pending**. Review scope is the offline importer, generated coastline provenance, equirectangular projection and a narrow static renderer review. Other HamClock projections, overlays and Pi performance require separate evidence. This is not full product release approval.

## Frozen implementation re-review

The single bounded input read is implemented, and ZIP decoding and SHA-256 now operate on the same byte buffer. Forward projection explicitly clamps rounding at the eastern exclusive endpoint. Both initial issues below are addressed.

All eleven independent malformed-input fixtures were rerun and rejected. Regeneration from the supplied archive produced a header byte-for-byte identical to `src/map/coastline.hpp`. The exact ZIP hash remains `664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817`; archive VERSION.txt independently confirms 4.1.0 and its projection metadata declares WGS84 geographic degrees.

The independently opened [publisher's terms](https://www.naturalearthdata.com/about/terms-of-use/) state that its raster/vector datasets are public domain and may be modified and redistributed. This supports the bundled dataset's stated rights; it does not certify unrelated application name rights or other dependencies.

An independently compiled C++17 AddressSanitizer/UndefinedBehaviorSanitizer harness ran the frozen map tests, an additional global grid of projection/inverse samples, analytic 45°N/90°E, and both-direction 179° seam crossings with segment-width and seam-interpolation checks. It reported zero failures, exited 0 and produced no sanitizer findings. `make check integration-check` also passed with the integrated map test wiring.

Static renderer review confirms separate iteration within each bundled part and dateline splitting per adjacent vertex pair. The frozen array tests verify full contiguous part coverage, index limits and finite coordinate bounds. The update loop now redraws on clock-second changes, pane changes and window events, rather than every 25 ms. It still polls with a 25 ms delay; actual target idle CPU and input latency remain unmeasured. Computer QA of the integrated result is pending with the root agent.

No additional blocking defect was found for this bounded coastline/import/projection draft increment. The generated data are a generalized coastline outline, not land fill or a complete HamClock map replacement.

## Initial observations and required correction

The first importer revision used `stat`, an unbounded `read_bytes`, and a separate `ZipFile(path)` open. A growing or replaced path could bypass the input byte cap or make the recorded hash describe bytes different from the geometry source. Requested correction: read at most `MAX_ARCHIVE + 1` bytes once, validate that actual length, and decode the same bytes via an in-memory archive. Verify the corrected code after freeze.

Projection documentation promises normalized x in `[0,1)`. A longitude immediately below +180 can round its projected x to 1 because floating-point addition rounds to 360. Require an explicit tested endpoint convention before relying on inverse round trips or seam handling.

## Preliminary independent importer tests

An isolated Python harness, outside the repository, derived a minimal valid PolyLine shapefile independently and tested eleven malformed cases: truncated payload, NaN coordinate, longitude/latitude out of bounds, invalid part offsets, one-point part, excessive point count, traversal-only archive name, duplicate exact member, oversized decompressed shapefile and oversized archive. All were rejected by the inspected revision. A valid exact root member accompanied by an unrelated traversal member was accepted without extraction; the traversal member remained unused.

The supplied official archive decoded to 5,128 points in 134 parts. Independently calculated SHA-256: `664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817`. This result confirms local input identity only; publication source, license evidence and generated-output reproduction must also be checked.

## Follow-up evidence and production gates

- Root computer QA must inspect the integrated geographic result and interaction behavior; automated projection tests do not prove readable rendering.
- Target Pi CPU/memory/startup/input measurements and production UI verification remain separate gates.
- Azimuthal projection, land fill, live overlays and final original HamClock map parity are not implemented by this increment.
- Importer support is deliberately limited to bounded, sequential, non-null PolyLine shapefiles. Official coordinate overshoots within 1e-6 degree are clamped as documented; larger departures are rejected. This transform and float precision must remain documented when updating the asset.
# Integrator computer check

The integrator subsequently opened a fresh rebuilt development instance through computer-control tools and confirmed real coastline geography and the new map label. Pane selection and advancing UTC were visually exercised with the map retained. See `computer-validation.md` for the stale-process check and exact limits. Native accessibility and Pi performance remain unverified; this closes the narrow local visual smoke-test gate only.

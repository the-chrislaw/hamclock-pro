# Bundled coastline provenance

Checked/imported 2026-10-02. This is real geographical coastline data, not traced HamClock artwork or copied HamClock implementation.

- Dataset: Natural Earth, 1:110m ocean coastline, version **4.1.0** (confirmed by archive VERSION.txt).
- Official dataset page: https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-coastline/
- Download URL: https://naturalearth.s3.amazonaws.com/110m_physical/ne_110m_coastline.zip
- Hosting provenance: https://www.naturalearthdata.com/downloads/110m-physical-vectors/ states downloads have been hosted by AWS since 2021. The individual page's legacy link was malformed; the official AWS distribution URL above was used.
- Archive size: **85,352 bytes**.
- Archive SHA-256: **664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817**.
- Read member: `ne_110m_coastline.shp`, **89,652 uncompressed bytes**. No archive paths were extracted.
- Geographic reference: archive `.prj` declares GCS_WGS_1984, longitude/latitude degrees.
- Imported geometry: **5,128 vertices**, **134 polyline parts**. `CoastPoint` stores longitude, then latitude. `CoastPart` stores the first vertex index and vertex count.
- Output: `src/map/coastline.hpp`, original generated array format. Data source credit: Made with Natural Earth.

## Rights and transformations

Natural Earth's [terms](https://www.naturalearthdata.com/about/terms-of-use/) expressly describe its raster/vector data as public domain, allowing modification and redistribution without permission or compulsory attribution. The bundled data retain that status; application/converter code is separately covered by the repository's MIT license. This verified published statement does not settle unrelated application name rights or other dependencies.

The original Python standard-library converter verifies ZIP member size, shapefile magic/version/declared length, sequential record headers, exact PolyLine payload lengths, bounded point/part counts, increasing part offsets, minimum part length and finite coordinate bounds. It reads the input once, capped to 1 MiB plus a rejection byte, and decodes the same bytes used for its hash. It caps the selected uncompressed shapefile at 1 MiB, 100,000 total vertices and 10,000 total parts. Encrypted members, duplicate expected members, null/other shape types and truncated records are rejected. Other archive entries are ignored, not extracted.

The source contains four longitude overshoots beyond +180 degrees; the largest is **180.00000044181039**. The importer accepts only a 0.000001-degree boundary tolerance and clamps this tiny excess to the exact geographic boundary. Larger excursions remain errors. Coordinates are then formatted to seven decimal places and stored as float values. No line parts or vertices are removed and no invented points are introduced. This preserves source topology while limiting numeric precision to a display-appropriate format; coastline accuracy is still limited by the generalized 110m dataset.

Reproduce after obtaining the pinned official ZIP:

```sh
python3 tools/import_coastline.py /path/to/ne_110m_coastline.zip src/map/coastline.hpp
```

Compare the reported hash with the value above before accepting a changed output. A changed source release needs fresh provenance and visual review.

## Projection behavior and checks

`map_project_equirectangular` maps longitude/latitude to normalized x/y: north at y=0, south at y=1, Greenwich at x=0.5. +180 normalizes to -180 at x=0. Interior values close to +180 are clamped below x=1 if floating-point addition rounds to that exclusive endpoint. The inverse accepts x=1 as the equivalent western seam. Nonfinite/out-of-range input is rejected.

`map_split_equirectangular` splits a coastline segment at the dateline using the shorter longitudinal step and interpolated latitude, placing ends on opposite map edges. A jump of exactly 180 degrees is kept as supplied; that branch is intrinsically ambiguous without extra path information. This helper is for coastline polylines, not computed geodesic paths. A renderer must draw only within each part and use splitting; it must not join adjacent parts or draw seam-spanning chords.

Local validation: standalone C++ projection and asset checks compiled with warnings enabled and AddressSanitizer/UndefinedBehaviorSanitizer, then passed. Fixtures cover origin, poles, dateline normalization, both crossing directions, seam endpoint, inverse/forward round trips, near-east endpoint, invalid inputs, all bundled coordinate bounds and complete part coverage. Python synthetic checks verified accepted minimal geometry and rejection of truncation, invalid coordinate, invalid part offset and traversal-only ZIP members.

Unknown/unimplemented here: azimuthal projection, land fill, islands omitted by 110m generalization, live data overlays, full final HamClock 4.22 map inventory, and Pi performance. This layer establishes a licensed real coastline foundation and does not claim feature parity.

## Country borders (G5)

- Dataset: Natural Earth, 1:110m cultural vectors, **Admin 0 – Boundary Lines (land)**, `ne_110m_admin_0_boundary_lines_land.zip`, fetched from `https://naturalearth.s3.amazonaws.com/110m_cultural/` on October 14, 2026. Archive SHA-256: `bb2954981570498a4b4344f8e83fa54caa5858e15e800094112407d2bd2f940d` (also recorded in `src/map/borders.hpp`).
- Licence: public domain per the [Natural Earth terms](https://www.naturalearthdata.com/about/terms-of-use/), same as the coastline and land assets above. Voluntary credit: Made with Natural Earth.
- Converter: `tools/import_borders.py` (derived from `import_coastline.py`: bounded reads, only the root `.shp` member, no extraction). Output `src/map/borders.hpp` (3108 points, 333 parts).
- US state lines: same terms, `ne_110m_admin_1_states_provinces_lines.zip` (same S3 prefix, fetched October 14, 2026), archive SHA-256 `9ebde98fe3fa2d8ea69f1d63b865531717a666d111c11a3b38fdaf18b06a84e9`; `tools/import_borders.py --states` writes `src/map/states.hpp` (903 points, 109 parts). 110m covers the US only, so there are no Canadian provinces; cities are still not included.
- Cities: same terms, `ne_110m_populated_places_simple.zip` (same S3 prefix, fetched October 14, 2026), archive SHA-256 `3f3d99a9a5d84605bb3be07b94c9122b4d69d7545de478b314d75f5b0742afdf`; `tools/import_cities.py` reads only the root `.shp`/`.dbf` members and writes `src/map/cities.hpp` (243 cities: ASCII name, latitude, longitude, `pop_max`). Drawn as dots with names, largest first, skipping any that would cover a marker, label or map button.
- Caveat: Natural Earth draws de facto boundaries at 110m generalisation; lines are indicative only, not an authoritative statement of any border. Cities are the 110m selection of large places only.

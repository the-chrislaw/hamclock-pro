# Licensed filled geography: next implementation plan

Research date: October 3, 2026. Research only: no new dataset was downloaded to the checkout, no assets generated, no dependency installed and no code/build/Git state changed. Goal: real land fill following the approved native design, without inventing polygons by closing coastline fragments.

## Primary evidence and confidence

- The [official Natural Earth land page](https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-land/) describes 110m land polygons including major islands and labels its ZIP version **4.0.0**. The [official repository's land VERSION file](https://github.com/nvkelso/natural-earth-vector/blob/master/110m_physical/ne_110m_land.VERSION.txt) currently reads **4.1.0**. **High confidence** in this discrepancy; the actual ZIP member version/hash is unverified in this research-only batch. Do not silently equate the page label with repository data.
- The [official coastline page](https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-coastline/) lists **4.1.0**, says its coastline is matched to land/water polygons, and includes major islands. Existing `docs/map-provenance.md` records our pinned coastline archive version 4.1.0, WGS84 PRJ, 5,128 vertices/134 parts and SHA-256 `664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817`. Compatibility is plausible, but not a verified byte/topology comparison with either candidate land delivery.
- Natural Earth's [terms](https://www.naturalearthdata.com/about/terms-of-use/) place its raster/vector datasets in the public domain and permit modification/redistribution; attribution is optional. Maintain voluntary source credit and provenance anyway. This is a published dataset permission statement, not complete product/dependency legal clearance.
- The project-linked [official repository](https://github.com/nvkelso/natural-earth-vector) offers [land GeoJSON](https://github.com/nvkelso/natural-earth-vector/blob/master/geojson/ne_110m_land.geojson), shown as 135 KB. Its current raw rendering did not expose geometry contents in this browser run. **High confidence** in availability; **unknown** exact coordinate counts, holes, antimeridian cutting, per-layer version equivalence and conversion precision. Pin a commit and byte hash before import; never use mutable `master` as reproducibility evidence.

## Format requirements

The [Esri shapefile specification, printed pages 8–9 / PDF pages 12–13](https://www.esri.com/library/whitepapers/pdfs/shapefile.pdf#page=12) defines Polygon rings with at least four points and identical endpoints, allows multiple exterior rings, uses clockwise exteriors/counterclockwise holes and does not assign meaning to ring order. Polygon payload structure resembles PolyLine, but its semantics do not. Preserve record boundaries and rings; a polyline part list is insufficient for fill.

[RFC 7946 sections 3.1.6–3.1.9 and 4](https://www.rfc-editor.org/rfc/rfc7946) specifies longitude/latitude coordinates, Polygon first-ring exterior and later-ring holes, MultiPolygon grouping, opposite recommended winding to shapefile, and antimeridian cutting. It recommends backwards compatibility with opposite winding. A source filename alone does not prove RFC compliance. **High confidence** in format rules; topology of prospective source bytes remains uninspected.

## Compare two implementation paths

| Path | Advantage | Cost/risk | Recommendation |
|---|---|---|---|
| A: original bounded standard-library land SHP importer | Reuses audited ZIP/header/record-limit approach; direct official delivery; no new dependency | Must add genuine Polygon ring/record semantics, closure/degeneracy checks and fill seam handling | Preferred next batch, first with synthetic records |
| B: official GeoJSON into original bounded arrays, or reproducible SHP→GeoJSON conversion | Explicit Polygon/MultiPolygon and hole grouping; easier human inspection | Mutable delivery/version equivalence, JSON depth/type/number limits, possible legacy winding; conversion is not automatically simpler | Valid fallback after pinned-byte inspection; avoid installing GDAL/Mapshaper |

Both are offline build-time transforms; ship generated validated geometry, not an arbitrary user-data loader or network fetch in the operational app. Do not copy source implementation from other projects. Existing licensed geographic data can be transformed with original code.

## Path A: two steps ahead

**Step A1 — bounded importer and contract:** create an original `import_land.py` plus synthetic regression fixtures, independent of the existing coastline importer so no tested PolyLine behavior changes. Contract: ordered coordinates, ring offsets/counts, record offsets/counts and source digest. Accept only Polygon shape type 5; explicitly reject null/Z/M/mixed types for this first version. Require closed rings, at least three distinct vertices, finite bounds and bounded payload lengths/parts/points. Preserve source doubles until deliberate output precision; source provenance must state any transformation. No invented closure of an open ring.

Next acquisition can read the official page's ZIP link or a pinned repository delivery, but inspect VERSION/PRJ and verify exact archive SHA before accepting output. Resolve the land 4.0.0-versus-4.1.0 discrepancy; prefer a version-compatible 4.1.0 source if actually available and verified. Do not infer geometry compatibility from matching version strings alone.

**Step A2 — original fill geometry and native rendering:** consume record/ring groups into cached fill paths, preserve holes, then draw fine coastal outline, route and DE/DX markers. Independent image/point-containment tests and actual native light/dark inspection must precede publication. If a source record contains crossing or ambiguous seam geometry, checkpoint that case for explicit clipping tests rather than connecting edge fragments and closing them blindly. Use per-record fill groups to avoid accidental cancellation between independent overlapping features.

## Path B: two steps ahead

**Step B1 — pinned JSON ingestion:** use only Python standard-library JSON on a hard-capped byte buffer; validate FeatureCollection and Polygon/MultiPolygon groups with exact numeric position dimensions. Ignore descriptive properties rather than rendering them as trusted text; reject duplicate relevant JSON keys, booleans as numbers, NaN/Infinity, invalid types, unsupported CRS and excessive nesting/counts. Preserve explicit hole relationships independent of winding. Inspect source/version and compare representative coastline vertices before deciding whether this route is equivalent to A.

For reproducible conversion from SHP, first create the A parser, then emit deterministic GeoJSON with documented winding/grouping/precision. This adds an intermediate format, not a topology engine. Avoid pretending an uninstalled third-party conversion command is reproducible. Direct arrays are simpler if only the native renderer consumes the result.

**Step B2 — shared rendering acceptance:** feed the same validated polygon contract as A so renderer/seam tests do not depend on input serialization. Compare deterministic output from equivalent SHP and GeoJSON fixtures, check holes and multipolygons, and inspect actual geographical screenshots. If source conversions differ, record the differences instead of choosing whichever image looks nicer.

## Security limits and seam policy

Proposed first limits: archive/input cap 1 MiB plus rejection byte; selected uncompressed SHP/JSON cap 1 MiB; at most 100,000 vertices, 10,000 rings and 10,000 records/polygons, plus a 20,000-vertex per-ring cap. JSON structural depth cap 16; bound output size separately. These are engineering proposals, to confirm against source counts. Never expand limits automatically after rejection. Read once, hash and parse the same bytes. No archive extraction, encrypted members, duplicate expected members or arbitrary file paths. Output through a unique same-directory temporary file and replace only after complete validation; preserve existing output on any error.

Do not apply the existing **polyline** shorter-step seam splitter to a polygon and call `closeSubpath()`: that can fill the ocean, lose holes or create a world-spanning chord. Preserve ±180 boundary identity in polygon projection; per-point normalization of +180 to −180 can collapse a boundary. A fill needs explicit closed-ring clipping/unwrap policy with matched seam boundary segments. Antarctica/polar full-width edges must be distinguished from ordinary short dateline crossings. Reject or defer unsupported cases with useful provenance rather than silently repairing them. No topology-clean claim follows from closure/area checks alone; exhaustive self-intersection validation needs a separately bounded algorithm/review.

## Explicit fixtures and publication gates

- Binary/JSON malformed header/type/length/count/offset, truncated payload, bad encoding, duplicate keys/members, nonfinite/out-of-range positions, too-deep JSON, size/aggregate cap and existing-output preservation.
- Closed rectangle, triangle with repeated endpoint, repeated consecutive points, zero-area/open ring rejection, donut hole, two exterior islands in one record, nested island and independent record grouping. Winding variants exercise each format's policy.
- ±180 seam-touch ring, 170E→170W polygon, seam-crossing hole, both crossing directions, Antarctica/pole boundary, tiny viewport and all-map viewport. Assert ocean remains ocean outside known land and hole interiors; test actual fill pixels/containment separately from source counts.
- After real bytes are pinned: validate all rings/counts/bounds, record omissions/holes and representative land/ocean points, compare outline-to-existing-coastline alignment, then computer-inspect complete native card in light/dark/System. Benchmark actual Pi separately.

**Concrete next coding batch:** A1 only: original bounded Polygon parser, generated synthetic fixtures and atomic deterministic output contract, with independent audit. Queue actual pinned-source acquisition/provenance immediately afterward, then A2 renderer. This leaves a recoverable checkpoint if dateline topology needs another batch and moves toward the approved filled map without misleading closed-coastline shortcuts.

Unknowns remain actual dataset rings/holes, ZIP version, conversion fidelity, topology repair needs and Pi performance. None blocks the synthetic original parser work; none should be hidden by claiming the map is already complete.

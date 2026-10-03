# Real map implementation decision

Research checked 2026-10-02. No geographic assets were downloaded and no HamClock code was copied for this report.

**Recommendation:** start with bundled Natural Earth 110m coastline coordinates and an original, bounded SDL renderer. This replaces invented geometry with real geography while keeping the native application usable offline. Follow with land polygons and more detail where justified by the seven-inch display. Keep the projection and overlay interfaces independent of the background format.

Evidence labels: **Verified** means directly supported by a checked primary source or current repository file. **Inference** means a proposed engineering design or calculated estimate, not a measurement. **Unknown** means a release decision still needs evidence. These labels are not legal clearance or proof of performance.

## Choice comparison and two steps ahead

| Path | Step 1: first useful implementation | Step 2: consequence and next decision |
|---|---|---|
| Local vector coastline | Pin Natural Earth 110m, convert coordinates to a fixed local format, draw equirectangular coastline with markers. | Add matched land polygons, seam-safe paths and azimuthal projection. Compare 50m detail at actual screen size before increasing the asset size. |
| Local raster background | Prepare a bounded, downsampled geographic image from a permitted dataset, draw with original overlays. | Cache an inverse-projected azimuthal image for each DE center; evaluate resampling quality, storage and rebuild cost. Keep vector fallback for slow hardware. |
| Full tile engine | Prototype a pinned native engine with one explicitly permitted local or remote tile source. | Inventory every style/font/sprite/tile license and provider policy; measure Pi memory and drivers before accepting the added system complexity. |

### A. Local Natural Earth vectors: preferred foundation

**Verified:** Natural Earth says its raster and vector data are public domain, permits modification and redistribution, and requires neither permission nor credit. We should nevertheless include a provenance manifest and a plain-text credit in About. This status belongs to those data, not to arbitrary basemaps or software that uses them. [Natural Earth terms](https://www.naturalearthdata.com/about/terms-of-use/).

**Verified:** the 110m coastline page describes ocean coastline with major islands, matched to land/water polygons, and currently lists an 83.35 KB download, version 4.1.0. This is an archive size, not runtime memory or a complete geographical detail guarantee. [110m coastline](https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-coastline/).

**Inference:** convert the pinned archive during development/build, not at application startup. Ship coordinates and line-part offsets with validated counts, finite values, latitude/longitude bounds and a SHA-256 provenance record. Do not include an entire GIS parser in the runtime merely to render this small world layer. An original restricted converter can read the needed coordinate records; if using a conversion dependency instead, pin its version and record its separate license first.

**Verified:** this repository currently uses C++17 and SDL2. SDL's upstream license permits commercial use and redistribution subject to its origin, altered-source and source-notice conditions. Installed compatibility layers and transitive system components still need their own inventory. [SDL2 license](https://github.com/libsdl-org/SDL/blob/SDL2/LICENSE.txt).

**Inference:** precompute projected static coastlines only when size, center or projection changes. Redraw clock and changing overlays separately. Float coordinate pairs cost 8 bytes per vertex before container overhead; determine actual counts after conversion. No map server, credentials or location disclosure is needed for a bundled layer. Bound data size even for local files; reject unknown format versions and truncated coordinates.

**Unknown:** suitability of 110m detail for islands important to amateur radio, actual vertex counts, renderer timings, complete final HamClock 4.22 projection/layer inventory. A coastline-only patch establishes a real base layer, not full map parity. Shorelines do not supply political boundaries, callsign prefixes, cities or propagation data.

### B. Local raster: useful optional visual layer

**Verified:** Natural Earth I offers land-cover/shaded-relief variants and large/medium downloads; the plain land-cover page currently lists roughly 128/72 MB archives. Its data fall under the Natural Earth terms above. Exact raster dimensions and georeferencing must be inspected before ingest. [Natural Earth I](https://www.naturalearthdata.com/downloads/10m-raster-data/10m-natural-earth-1/).

**Inference:** use a development-time transformation into an explicitly documented plate-carree raster; retain source version, bounds, dimensions, hash and conversion settings. A 2048 × 1024 RGBA working image is exactly 8 MiB; an 800 × 480 RGBA image is about 1.46 MiB, excluding additional copies, decoder state and GPU allocation. These are arithmetic estimates, not Pi measurements. Ship only the derivative resolutions required; source archives need not remain in the runtime package.

**Inference:** BMP through existing SDL facilities can avoid adding a decoder initially, at a disk-size cost. A PNG/JPEG decoder adds its own license, vulnerability and decompression-size review. Validate dimensions before allocation; cap decoded bytes and failures. For azimuthal display, inverse-project each output pixel to source longitude/latitude and cache the result. Nearest/bilinear resampling and antimeridian handling require visual tests. Changing DE center invalidates the cache. Raster alone gives no selectable coastlines or labels; those remain separate overlays.

**Unknown:** whether this style matches the accepted quieter design, how much source resolution is useful, and Pi reprojection latency. Prefer a warm, subdued vector base until the user reviews an actual raster treatment.

### C. Full tiles/native engine: investigate only if detail needs justify it

**Verified:** MapLibre Native's top-level license is BSD-2-Clause, requiring retained source notices and reproduced binary notices. This does not license its tiles, fonts, styles, sprites or dependencies. [Engine license](https://github.com/maplibre/maplibre-native/blob/main/LICENSE.md).

**Verified:** its Linux guide specifies CMake/submodules and a dependency stack including curl, graphics/windowing, image libraries and ICU, with SQLite and EGL details. It supports own style/tile sets including a local MBTiles example. The guide does not establish Pi 3 or Pi 4 performance. [Linux documentation](https://maplibre.org/maplibre-native/docs/book/platforms/linux/index.html).

**Verified:** OSM data use requires ODbL attribution and applicable license compliance; providers can impose additional terms. The public OSM raster tile service requires identification, visible attribution and caching, and forbids bulk download/prefetch. Therefore it is unsuitable as a source for shipping an offline tile pack. [OSM copyright](https://www.openstreetmap.org/copyright), [OSMF tile policy](https://operations.osmfoundation.org/policies/tiles/).

**Inference:** compare a separately licensed offline tile pack with a permitted commercial/hosted endpoint, then evaluate its update cadence, recurring cost and privacy. A standard tile route should not be assumed to support a DE-centered azimuthal projection; verify the engine and perform a separate rendering spike. Geodesic paths still need geographic sampling. Networking adds HTTPS validation, timeouts, response/decoded-size caps, cache quotas, backoff and endpoint controls. Untrusted styles may reference additional URLs; explicitly disallow unexpected hosts, local files and unsupported schemes. A remote request can expose the requested map region and user IP.

**Unknown:** per-device CPU/memory, GPU compatibility, azimuthal support, complete transitive dependency notices, chosen tile provider rights. This is the largest decision branch; do not make it a shipping prerequisite for the first real world map.

## Projection and overlay contract

**Verified:** PROJ documents forward/inverse equidistant cylindrical and azimuthal equidistant projections, including spherical/ellipsoidal forms; PROJ itself is MIT licensed. Its documentation is a reference and its executable can be an independent test oracle without making it a runtime dependency. [Equirectangular](https://proj.org/en/stable/operations/projections/eqc.html), [azimuthal](https://proj.org/en/stable/operations/projections/aeqd.html), [PROJ license](https://proj.org/en/stable/about.html).

**Inference:** expose `project`, `unproject` and seam-aware polyline clipping independent of SDL. In plate-carree, longitude/latitude map linearly to x/y; split lines at the selected central-meridian seam. In spherical azimuthal equidistant, radius represents central angle from DE and direction represents initial bearing. Treat the antipode as a singular case; never draw an arbitrary connecting chord through it. Sample short/long great-circle paths in geographic space; a straight pixel line is generally incorrect. Use the same projection for coastline, DE/DX labels and overlays.

**Inference:** the terminator can be a geographic curve normal to the Earth-to-Sun direction, then projected like other geometry. Distinguish the geometric day/night boundary from atmospheric twilight and sunrise/refraction. Implement original solar equations only after selecting a documented accuracy target and independent fixtures. **Verified:** NOAA's calculation-details page states its calculator is no longer actively maintained; use it cautiously as one cross-check, not the sole authority. [NOAA details](https://gml.noaa.gov/grad/solcalc/calcdetails.html).

## First code patch and acceptance

1. Add a provenance manifest and pinned, bounded local coastline asset; record extraction details. Acquire from the official distribution only after checking its contents. No tracing existing HamClock images.
2. Add original projection/line-clipping domain code and coastline renderer. Replace the schematic geometry in native output and the design study with the same verified coordinates. Clearly identify offline base data.
3. Test inverse/forward coordinates, chosen center, poles, dateline and near-antipodes; test line-part boundaries and malformed asset rejection. Compare representative projection points against pinned PROJ configurations and record spherical versus ellipsoidal settings.
4. Review light/dark contrast, recognizability of major land/islands, seam artifacts, marker/path alignment and 800 × 480 touch use. Do not label this full parity.
5. Measure Pi 3 and Pi 4 startup, peak resident memory, cached and uncached redraw time, steady CPU, 30-minute stability and touchscreen response, both connected and offline. Suggested provisional gate: map redraw stays within a 33 ms interaction budget; negotiate limits after the first hardware baseline. No measured hardware claim yet.

A separate final-4.22 feature inventory must identify every required map mode, overlay, interaction and data feed. The evidence here supports the base-map decision; it does not settle that inventory or a full-feature release date.

# Natural Earth land asset provenance

Prepared October 11, 2026. Scope: LF2 immutable offline dataset, independently reviewed LF1 path experiment prerequisite; no native widget wiring or runtime file loader. `src/map/land.hpp` is exact output of the project's original audited importer, without manual edits.

## Source and permission

Dataset: Natural Earth **1:110 million land polygons, version 4.1.0**. The ZIP's `ne_110m_land.VERSION.txt` is `4.1.0` with CRLF. The legacy webpage's 4.0.0 label does not override the actual pinned archive. Official [dataset page](https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-land/) and [download URL](https://naturalearth.s3.amazonaws.com/110m_physical/ne_110m_land.zip). The source archive was first acquired on October 3, 2026. That temporary copy was absent when development resumed; root reacquired the exact official archive on October 11, 2026 through authorized network access into `/private/tmp/hamclock-resumed-land-r625nfyn/ne_110m_land.zip` and verified the same digest. The asset writer regenerated from this verified temporary copy without performing a download; the resumed batch as a whole did reacquire bytes. All reproducibility pins are unchanged.

The [Natural Earth terms](https://www.naturalearthdata.com/about/terms-of-use/), rechecked October 11, 2026, place the website's raster and vector datasets in the public domain and permit modification and redistribution without compulsory attribution. Voluntary credit: **Made with Natural Earth.** Credit Tom Patterson, Nathaniel Vaughn Kelso and contributors. This permission evidence applies to this dataset; it does not certify the product's name, dependencies or all legal obligations. Project code's MIT license does not change the dataset's public-domain status.

`ne_110m_land.prj` declares GCS_WGS_1984, WGS84 spheroid (semi-major axis 6378137.0 metres, inverse flattening 298.257223563), Greenwich prime meridian and degrees. In this source X/Y are longitude/latitude degrees. The intended fixed-world renderer is equirectangular; arbitrary shapefiles cannot be assumed to share this CRS.

## Reproducibility pins

| Item | Bytes | SHA-256 |
|---|---:|---|
| `ne_110m_land.zip` | 69,700 | `1926c621afd6ac67c3f36639bb1236134a48d82226dc675d3e3df53d02d2a3de` |
| ZIP member `ne_110m_land.shp` | 89,504 | `8689e6932b8e370e2ca4587cf3ba21e460b1235db37b6ed3c172c35b4a6088de` |
| Generated `src/map/land.hpp` | 242,871 | `cc2d3904939d2cfe258660b014cdfd19c106a8f65c18dbd034cd13f0f57f72dc` |
| Original `tools/import_land.py` at generation | — | `280cddfa690805f04a5b5fada264321d719f9d598f32036110e058cffd441366` |

Regeneration: `PYTHONDONTWRITEBYTECODE=1 python3 tools/import_land.py PATH_TO_PINNED_ZIP src/map/land.hpp`. Verify the archive digest before substituting another input. No archive members are extracted. Exact output matches the previously measured temporary header byte for byte; there are **5,143 points, 128 rings, 127 records**, maximum ring 1,299 points. Contiguous offsets preserve complete coverage. Point coordinates are C++ doubles; offsets/counts are uint32_t.

## Explicit transformations

The importer snaps only finite values **outside** ±180 longitude or ±90 latitude whose binary-double distance beyond the endpoint is at most **1e-12 degrees**, to the same signed endpoint. Every in-range coordinate remains untouched. Reversed declared boxes are rejected before snapping, and normalized box containment remains strict. No wrapping, simplification, ring reordering, winding reversal or topology repair occurs.

Ten point entries change (closure duplicates count independently). Global point indices are zero-based; record numbers are source one-based. `180.00000000000014` becomes `180.0`; `-90.00000000000003` becomes `-90.0`. All other coordinate components are unchanged.

| Source record | Global point indices | Components snapped |
|---|---|---|
| 8 | 484, 485, 486 | longitude at 484/485; latitude at 485/486 |
| 17 | 817, 818 | longitude |
| 94 | 2206, 2209, 2210 | longitude |
| 113 | 3516, 3517 | longitude |

The global SHP box's south/east bounds normalize; records **8, 17, 94, 113** have normalized boxes (record8 south/east; others east). Original global box is `(-180.0,-90.00000000000003,180.00000000000014,83.64513000000002)`. These boxes are importer validation metadata and are not emitted in the C++ arrays.

The original measured planar ring winding is 127 clockwise and one counterclockwise; the only two-ring record is source113. LF1 interprets each record separately with WindingFill, preserving exterior/hole opposition and treating ring order as irrelevant to hole classification. This is a source interpretation, not a general clean-geometry guarantee.

## Geometry and validation limits

Record8 contains the preserved full-width polar edge `(180,-90)` to `(-180,-90)`. The fixed-world contract maps the two longitude endpoints to opposite sides of the viewport, never collapsing this closure to a shortest wrapped edge. Future ordinary longitude edges exceeding 180 degrees require explicit seam treatment and renewed review.

Record79/ring78 retains the repeated-start microtriangle at point3 and all fourteen vertices. No source contact is silently removed. LF1 permanent tests cover its exact point preservation, main-outline containment and local raster land/ocean samples; independent audits additionally exercised the temporary pinned full layer. Those results do not certify subpixel microtriangle visibility, exhaustive inter-record topology, general polygon validity or navigation accuracy.

Author verification for this batch: original importer regeneration, expected digest/byte size, exact temporary-header comparison, and standalone C++17 compilation with strict warnings, ASan/UBSan. Compile-time array sizes/types and executable contiguous offsets/coverage/count caps passed. Importer limits remain 1 MiB input/archive/SHP, 100,000 points, 10,000 rings/records, 20,000 points per ring and 12 MiB generated output; they were not raised.

Independent LF2 reproduction/provenance review is a required publication gate, recorded separately in `docs/workstreams/land-asset-and-provider-review.md`. Native filled-map integration, approved palette/overlay inspection, coastline alignment, target-screen computer control, Qt6.4 compatibility and actual Pi3/Pi4 performance remain separate unfinished gates. This batch establishes no full HamClock parity, accessibility compliance, hardware certification or legal certification.

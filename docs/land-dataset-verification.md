# Natural Earth 110m land dataset verification

Verified October 3, 2026. Scope: public-domain source acquisition into temporary storage and frozen-importer evaluation. No repository geography asset, source, test, build or Git state was modified. This record does not establish rendering correctness or full map parity.

## Exact acquisition and rights

- Official dataset description: [Natural Earth 110m land](https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-land/).
- Retrieved URL and final URL: `https://naturalearth.s3.amazonaws.com/110m_physical/ne_110m_land.zip` (no redirect).
- Successful completion time: **2026-10-03T20:09:42.844149Z**.
- Temporary archive: `/private/tmp/hamclock-land-verification-ycvls7y2/ne_110m_land.zip`.
- Bytes: **69,700**.
- Archive SHA-256: **1926c621afd6ac67c3f36639bb1236134a48d82226dc675d3e3df53d02d2a3de**.
- SHP member SHA-256: **8689e6932b8e370e2ca4587cf3ba21e460b1235db37b6ed3c172c35b4a6088de**.

Used a 15-second network timeout and a 1 MiB-plus-one bounded archive read, HTTPS certificate verification, no dependency installation, and a unique temporary directory. No archive members were extracted. The initial sandboxed network call failed DNS resolution; the authorized network escalation retrieved the public dataset successfully. No automatic approval review rejection occurred.

The official page's legacy download link is malformed (`https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/110m/physical/ne_110m_land.zip`) and failed in browsing. The [official terms page](https://www.naturalearthdata.com/about/terms-of-use/) identifies AWS hosting since 2021 and places all Natural Earth vector/raster datasets in the public domain, permitting modification and redistribution without compulsory attribution. Preserve voluntary credit and recorded hashes. This evidence addresses this dataset, not all project legal obligations.

## Version discrepancy resolved for these bytes

The website page still labels land **4.0.0**, while the retrieved ZIP's `ne_110m_land.VERSION.txt` contains **4.1.0** followed by CRLF. The [official repository version file](https://github.com/nvkelso/natural-earth-vector/blob/master/110m_physical/ne_110m_land.VERSION.txt) also displays 4.1.0. Thus the archive's actual version agrees with repository evidence and the existing coastline 4.1.0 provenance; the website label is inconsistent with delivered bytes. **Confidence: high** for the measured archive and published version. Repository history retrieval failed; no immutable Git commit equivalence is claimed. The archive hash above is the actual reproducibility pin.

Matching layer version numbers are not proof of exact boundary alignment. Existing coastline `docs/map-provenance.md` pins its separate archive hash `664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817`, 5,128 vertices and 134 polyline parts. Land and coastline have different geometry contracts and must remain separate.

## Archive inventory

| Root member | Uncompressed bytes |
|---|---:|
| `ne_110m_land.README.html` | 21,649 |
| `ne_110m_land.VERSION.txt` | 7 |
| `ne_110m_land.cpg` | 5 |
| `ne_110m_land.dbf` | 3,431 |
| `ne_110m_land.prj` | 147 |
| `ne_110m_land.shp` | 89,504 |
| `ne_110m_land.shx` | 1,116 |

PRJ declares `GCS_WGS_1984`, WGS84 spheroid (6378137.0, inverse flattening 298.257223563), Greenwich prime meridian and angular Degree units. SHP X/Y can consequently be treated as longitude/latitude degrees for this specific pinned source; do not generalize that assumption to arbitrary shapefiles. Application rendering remains equirectangular, not a coordinate-system claim of Mercator.

## Frozen importer result: rejected, correctly

Ran the existing unchanged `tools/import_land.py` via `read_zip` on the actual archive. It rejected with:

`ValueError: invalid geographic bounding box`

The first failure is the global SHP box:

`(-180.0, -90.00000000000003, 180.00000000000014, 83.64513000000002)`

South is below −90 by approximately **2.84×10⁻¹⁴ degrees**; east is above +180 by approximately **1.42×10⁻¹³ degrees**. The frozen importer deliberately requires strict geographic limits. No validation was weakened, no corrected archive was created, and no generated asset was accepted.

A separate read-only structural diagnostic of the bounded SHP bytes identified **127 records**, **128 rings**, **5,143 vertices**; all 128 rings explicitly closed, minimum 5 / maximum 1,299 points, no zero shoelace-area rings. Planar winding count was 127 clockwise / 1 counterclockwise, consistent with one hole candidate under the source format's convention. This is a diagnostic count, not a full topology/hole-containment certification or a successful importer result.

There are four record boxes with the same tiny boundary excursions: records 8, 17, 94 and 113. Ten vertices exceed strict longitude/latitude bounds by these tiny amounts, including repeated closure points. All inspected coordinates are finite. Record 8 (Antarctica) contains a full-width edge from approximately +180 to −180 at approximately −90. That edge must not be reinterpreted as a short dateline segment and collapsed. Frozen source bytes and their original ±180-side identity remain intact.

## Compatibility diagnostic and limits

A read-only comparison with current generated coastline coordinates found different exact vertex sets, as expected for distinct layers and numeric transformations. A diagnostic nearest-vertex comparison in degrees, allowing longitude wrapping, found 2,564 unique land vertices within 10⁻⁵ degrees of a coastline vertex, another 2,419 within 10⁻³, and 31 farther away. It is not a geodesic metric or a boundary-alignment test. Inland hole boundaries, pole closure, layer generalization and coastline source precision can affect this result; do not infer an erroneous source or seamless matching from these counts alone. Native overlay inspection and segment-aware geometry tests remain required.

The structural diagnostic did not implement topology repair, self-intersection/containment proof, fill clipping or a land renderer. No approved land-map appearance has been demonstrated yet.

## Two safe next paths

**A — reviewed explicit boundary normalization:** keep current rejection behavior until a new bounded code increment proposes a documented tiny tolerance, for example a fixed 10⁻¹²-degree envelope justified by these measured bytes. Apply normalization consistently to file/record boxes and vertices, snapping only such tiny excursions to the same signed ±180/±90 boundary; never wrap +180 into −180. Require independent review and fixtures just inside/at/outside the envelope, closure preservation, box containment, NaN/infinity rejection and Antarctica full-width edge retention. Report every changed coordinate and pin source/output hashes. This is a proposed policy, not approval to relax validation now. Next, run the revised importer against the exact archive and audit output before starting polygon fill.

**B — independently verified alternative delivery:** keep strict importer unchanged; obtain a commit-pinned official land GeoJSON or another official source version in a separate authorized batch. Verify its license/version/hash/CRS, inspect numeric boundary values and compare geometry against these pinned SHP bytes. GeoJSON serialization may eliminate tiny excursions, but that is unverified here. If it passes a separately audited bounded JSON parser, preserve explicit exterior/holes and record any quantization or topology differences. Next, feed the same polygon renderer contract and test seams/poles/holes; do not choose a delivery merely because it sidesteps an error.

Recommendation: A is the smaller documented next increment because the source/version and exact rejection cause are established. B remains an evidence-backed fallback path, with byte/conversion verification outstanding. Neither path should fabricate land by closing existing coastline lines.


## Subsequent independently reviewed endpoint increment

The earlier rejection above records the strict baseline faithfully. The new importer explicitly snaps only finite excursions whose binary-double distance beyond ±180/±90 is no greater than 1e-12 degrees. All in-range values, signed boundary identity, ring order and full-width polar edge remain unchanged. Reversed boxes are rejected before snapping; point containment in normalized declared boxes remains strict. No generic epsilon is added to interior boxes.

Frozen importer SHA-256 `280cddfa690805f04a5b5fada264321d719f9d598f32036110e058cffd441366` and permanent test SHA-256 `99580619b0fb948e1ae124a228f51757408ca26ff33eae2a61a51a033c01e33e`. Author checks passed 31 methods under Python3.9 and3.14.7; independent increment reviewer passed those plus 429 separate probes. The reviewer authored the baseline importer, while baseline importer approval came from another agent; this distinction is recorded in workstreams/map-and-catalog-audit.md.

The unchanged pinned archive now parses to 5,143 vertices, 128 rings and 127 records. Ten vertices, four record boxes and the global box normalize; no interior coordinate is changed. The in-memory emitted header is 242,871 bytes, SHA-256 `cc2d3904939d2cfe258660b014cdfd19c106a8f65c18dbd034cd13f0f57f72dc`. No land header is bundled in this increment. Parsing success does not establish clean topology or native rendering: subsequent source diagnostics found a repeated-start microloop in record79/ring78, requiring a source-preserving painter experiment before filled-map publication. See workstreams/land-fill-contract.md when that separately reviewed research is available.

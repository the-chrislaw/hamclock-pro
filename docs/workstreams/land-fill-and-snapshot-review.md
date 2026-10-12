# Land-fill contract and I02 snapshot review

October 3, 2026. Status: **LF1 research and header support a bounded experiment; I02 actual source/regressions pass bounded offline gate**. Reviewer owns only this report. No writer source, shared research/build files, Git, provider, native UI, dependency or hardware operations changed by this review. This is not full parity, legal, accessibility or hardware certification.

## Land primary evidence and measurements

Read [land-fill contract](land-fill-contract.md), [pinned dataset verification](../land-dataset-verification.md), prior endpoint audit, and primary sources independently. [Esri printed pages 8–9](https://www.esri.com/library/whitepapers/pdfs/shapefile.pdf#page=12) support directional exteriors/holes, arbitrary ring order and clean polygon requirements; preserving an array order is not topology proof. [Qt 6.11 QPainterPath](https://doc.qt.io/qt-6.11/qpainterpath.html#setFillRule), reporting 6.11.2, supports explicit fill-rule selection; default odd-even and `simplified()` behavior justify avoiding undocumented repair. [QPainter clipping](https://doc.qt.io/qt-6.11/qpainter.html#setClipRect) supports logical-coordinate clipping. These documents do not prove actual raster behavior on the source's contact case or the project's Qt 6.4/Pi target.

[RFC 7946 §3.1.9](https://www.rfc-editor.org/rfc/rfc7946#section-3.1.9) supports seam cutting as GeoJSON guidance; it does not redefine supplied planar SHP edges as shortest spherical arcs. [Natural Earth terms](https://www.naturalearthdata.com/about/terms-of-use/) support public-domain vector/raster reuse, with the documented source/hash/version chain remaining necessary. An MIT application license or dataset permission does not clear unrelated dependencies, original screenshots/source or product naming.

Independent read-only probes of the previously pinned ZIP passed **11 checks**: 127 clockwise/one counterclockwise ring; record 113's two-ring counts 1,299/52; record 79's repeated first vertex at index 3; only >180° longitude edge is ring 7 segment 380 `(180,-90) → (-180,-90)`. Independent winding evaluation reproduced all seven proposed safe land/ocean fixtures (London, Tokyo, Los Angeles, Atlantic, Pacific, Caspian candidate hole, Antarctica). No asset was generated or downloaded.

The researcher's 1,482,817-pair exact binary-rational topology diagnostic was **not independently rerun** here. Its reported nonadjacent contacts and unchecked cross-record topology remain explicit caveats. The repeated vertex and full-width edge are independently confirmed; there is no complete clean/simple/spherical-topology claim. Do not remove the microloop or wrap the full-width closure to make a test pass.

## LF1 coding readiness and gates

Per-record WindingFill, original ring directions/subpaths, direct fixed-world projection with +180 at x=1 and −180 at x=0, and no simplification form a coherent bounded experiment. This is an engineering choice supported by SHP semantics, not a proven raster result. Keeping different records independent avoids odd-even cancellation across overlapping records. The existing wrapped marker projection and coastline fragments must not be reused as polygon closure.

LF1 may proceed only after a **concrete independently reviewed header** specifies bounded point/ring/record counts, safe offset arithmetic, invalid/nonfinite input rejection, complete coverage, fixed-source seam acceptance and owned immutable results. Tests need holes/hole-first/islands/overlaps, opposite winding, Antarctica, explicitly split seam rectangles, contact microloop, containment and QImage pixels away from antialiased boundaries. Passing generic fixtures alone is insufficient for the actual contact ring. Future recentering/projections/clipping need separate semantics and output-expansion caps.

LF2 real asset remains gated on accepted importer/provenance plus LF1 source/raster evidence; LF3 shared widget integration remains root-owned and requires native computer-control acceptance and distinct Pi performance checks. No production land renderer or real asset is approved solely by this research. Confidence high for primary rules and measured source facts; medium for proposed WindingFill interpretation; actual raster/contact case pending.

## I02 header review before implementation

Read frozen `src/app/instrument_snapshot.hpp`. Supports explicitly scoped latest observation plus one forecast rather than a full series/history model. I01 known IDs and static Unavailable catalog are separate from caller-injected runtime phase and derived freshness. Typed UTC/monotonic/policy fields, bounded plain ASCII provenance, finite explicit values, quality states and per-product age policy avoid invented cadence and absent-as-zero values. Exact refresh deadline is stale; observed/fetched/issued future evidence cannot look current, while future forecast validity is legitimate. Original product units/ranges/provider authenticity remain parser/data review gates.

Before cpp, reviewer requested future forecast issuance handling, future timestamps even on missing/invalid observations, consistent value absence for Missing/Invalid, validation despite absent/hidden snapshots, full signed-span ages and nonthrowing state commit. Author revised the header; corrected contract approved before cpp. The proposed `accept` must validate matching ID/schema, create an independent temporary copy, then nonthrowing swap before setting Ready. Public evaluator must not bypass invalid phase/policy/schema because a widget is disabled or data absent. Caller-supplied provenance/quality remains unverified evidence.

Frozen actual implementation reviewed independently: validates known IDs, enums, bounded text, finite explicit values, deadline order and forecast issue/valid order; checks phase/policy/schema before state selection; refuses future observation/fetch/issue evidence in enabled phases; ordered unsigned subtraction covers full timestamp span. Optional candidate copy precedes statically checked nonthrowing swap and Ready assignment. No I/O/provider/UI behavior appears.

Fresh strict-warning C++17 ASan/UBSan executable passed **104 permanent checks**. Separate temporary sanitizer harness passed **524 probes**: every invalid uint8 phase/unit value, 25 independently derived wide-arithmetic extreme timestamp cases, and allocation failure at successive candidate string copies. Injected failures preserved prior cache provenance/value and Offline phase; permanent rejected-schema cases compare every stored field. No implementation blocker found. Final corrected header explicitly qualifies future/monotonic errors as enabled-phase behavior and states hidden-phase/schema validation precedence. This was verified against the frozen header; no further source or comment change is needed. Missing async generation guards, family-specific unit validation, real scheduling/transport/permissions, series gaps, images and native presentation remain separately tracked rather than implicitly supplied by this preparatory model.

## Exact next action

Return bounded I02 source approval to root/author. Final header qualifiers and hashes were rechecked. LF1 header was independently approved before cpp: explicit internal caller-valid read-only views, cap/error/all-or-nothing contracts, safe direct coordinates and seam restrictions. Reviewer requested safe offset/count checks before dereference and nonquadratic distinct-vertex detection; implementation, synthetic/QImage/contact-source results remain pending a separate audit. No native/provider integration or realasset approval follows.

Root-only documentation updates to land-importer.md and the appended endpoint verification were read independently and match the prior audit. Recomputed importer/test hashes, 242,871-byte in-memory generated output hash cc2d3904939d2cfe258660b014cdfd19c106a8f65c18dbd034cd13f0f57f72dc, ten changed vertices and four changed record boxes against raw pinned SHP. Nothing was written as an asset. Corrected 180W…170W wording in the land contract was rechecked.


## Frozen review hashes

- `src/app/instrument_snapshot.hpp`: `a82175f81d7c6503ed8fc520dcd82b2d85a6a6b0097d68b8812ace65fa255a45`
- `src/app/instrument_snapshot.cpp`: `a89320649e08295454283f64ff3c92a957be8c58679f3d682a60d032eb56cf51`
- `tests/instrument_snapshot_regression.cpp`: `da0ffd89fdacc2ad1c6d0dacad843d238fbb9df2f5aca37715814d19ec0ae52f`
- `src/qt/land_fill_path.hpp`: `59d4f344c668018ee90feaa286f92eef29230153325d01d5dc9221412fee2ea1`
- `docs/workstreams/land-fill-contract.md`: `901cf51edd848802d534272e80708d8af96e8f543cf06d702b2a7ef672a40268`

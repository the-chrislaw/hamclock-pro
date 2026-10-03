# Independent synthetic land importer audit

Status: **bounded synthetic importer and final regression review pass; no remaining blocker found in scope**. Reviewed 2026-10-03. Scope: frozen `tools/import_land.py` and `tests/land_import_regression.py`. Reviewer changed no production code, tests, build files or Git state. No real land dataset was downloaded or certified.

## Independent evidence

All **27 permanent regression methods passed** with bytecode writing disabled. An independent temporary harness added **nine substantive checks**: a separately built synthetic Polygon with longitude -180/+180 and polar coordinates; raw-byte digest; ring/record offsets; generated C++17 header compilation/execution with static assertions; actual CLI CRC corruption rejection; compressed member larger than the decompressed limit; an unwritten FIFO rejected within a bounded subprocess timeout; publication onto one hard-link name preserving the other name's prior bytes; and an injected write error preserving the destination while removing temporary files. Actual CLI rejections returned 2 and preserved the prior generated output.

The permanent suite covers malformed headers/record types/lengths/counts/offsets, finite coordinates and declared geographic bounds, closed rings with distinct vertices/nonzero area, individual/aggregate caps, encrypted/duplicate/traversal-only members, same-byte digest and decode, source symlink/FIFO/directory rejection, deterministic emission, output digest/cap validation, successful 0600 publication and failures before replacement. Its ring fixtures preserve a clockwise exterior and opposite-winding hole, including reordered ring input; these verify preservation rather than hole classification.

## Source review

Input is opened once with `O_NOFOLLOW | O_NONBLOCK`, verified as a bounded regular file on that same descriptor, and read at most archive-cap-plus-one bytes. Hashing and ZIP decoding use the same captured bytes. Final source symlinks are rejected; nonblocking open prevents waiting indefinitely on a FIFO writer. The documented stable/trusted parent assumption remains relevant to ancestor path changes.

Archive bytes are capped at 1 MiB. ZIP metadata parsing is consequently bounded by that captured input; the importer then enforces 256 members and exactly one root `ne_110m_land.shp`. It never extracts members. The selected member must be unencrypted and within the 1 MiB decompressed bound, and its stream read is capped; declared/actual lengths and the ZIP CRC are checked. The independent compressed-bomb and CRC cases verify actual CLI behavior rather than only patched limits.

SHP parsing validates magic, version/type, total length, sequential records, exact payload length, count caps, strictly increasing starts and bounded ring spans before reading coordinates. Coordinates and boxes must be finite and geographically bounded, and points must lie in both record and overall boxes. Integer arithmetic is Python bounded-by-validation arithmetic with no native overflow. Per-ring and total point/ring/record limits prevent unbounded geometry work or output growth.

Each ring must be closed, have at least three distinct vertices and nonzero signed planar area. The importer preserves point order, ring/record grouping, original winding, full double precision and distinct -180/+180 identity. It **does not** infer exterior/hole roles from order, repair topology, certify self-intersection absence or clip dateline/polar seams. A seam-crossing ring's planar area is not a proof of correct globe topology. These explicit limitations are appropriate for this synthetic stage.

Emission has a bounded digest format, deterministic 17-digit double literals and an output-size cap. In the actual CLI, geometry originates from the bounded decoder; `emit` is not a standalone validator for arbitrary caller-supplied geometry. No external package or network operation is required.

Publication rejects an already present nonregular final destination, writes a same-directory private temporary regular file, flushes/fsyncs it, then replaces the final directory entry atomically. Earlier failures preserve prior destination bytes and clean temporary names, as tested. It is last-writer-wins replacement under the documented trusted stable parent assumption, not exclusive creation, adversarial path locking or directory-fsync crash durability. A concurrent final symlink introduced after the initial check would be replaced as an entry rather than followed; this does not establish stronger path-security guarantees.

## Finding and disposition

**Regression isolation issue corrected:** the original `test_zip_exact_root_and_hash` used fixed member `../ignored` and asserted absence of that fixed path in a shared temporary parent. An unrelated existing file could cause a false failure. After reviewer notification, the writer derived the name from the fixture's temporary-directory basename plus `-land-unextracted` and used it consistently in member and assertion. Independent inspection confirmed the correction and all **27 methods passed again**. The importer itself was unchanged.

The final source/regression gate passes for bounded synthetic import and output preservation; review is frozen. Reviewed SHA-256: importer `c7ab2c104c63bb9a7295b487f31ee5086aa63714b714e20ad9099dc3172128a9`, regression `7f2cd787c2ec069bb359f1f22e77238977b844b43dd3154c9c941302a1425b41`. Real official-asset provenance/checksum/reproducibility, actual feature/hole topology, dateline clipping, renderer behavior, license review and target performance remain future gates. This report grants no real-map fill, legal clearance or full product security claim.

## Build integration verification

Root added `land-check` to the phony Make targets and full `verify` dependency list, invoking the existing Python regression with bytecode disabled. CMake discovers a Python interpreter only inside `BUILD_TESTING`, registers the same regression with an absolute source path, inherits bytecode-disabled environment and bounds execution at 15 seconds. The native application/runtime targets gain no Python dependency and no UI source changed in this increment. Tests and their CLI subprocesses use the same selected interpreter through `sys.executable`.

Independent `make verify` passed all prior checks plus the 27 land methods using system Python 3.9. A separately configured/built temporary CMake directory found installed Python 3.14.7; all **eight CTest targets passed**, including the same land suite under that interpreter. No integration blocker found. This is synthetic importer coverage rather than actual land asset/rendering verification.

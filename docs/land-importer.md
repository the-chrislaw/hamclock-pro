# Original bounded polygon importer

Development tool only; no real land geometry is bundled by this increment.

Usage: `python3 tools/import_land.py /private/offline/ne_110m_land.zip /private/output/land.hpp`. Supply an offline regular file in a stable private directory. The importer never extracts archive members or performs network operations. It rejects final source symlinks/FIFOs/nonregular files, malformed ZIP/SHP payloads and coordinates outside strict geographic bounds. Existing regular output is atomically replaced after complete parsing/emission, with documented trusted-parent/last-writer-wins assumptions.

Bounds: 1 MiB archive and decompressed SHP, 256 ZIP members, 100,000 vertices, 10,000 rings/records, 20,000 vertices per ring, 12 MiB emitted output. Only Polygon type5, closed rings, at least three distinct vertices and nonzero planar area are accepted. Explicit rings, source winding, records and distinct -180/+180 are preserved. This is not topology validation, exterior/hole classification, spherical seam clipping or a renderer.

`make land-check`, `make verify` and the eighth CTest execute27 offline synthetic methods. Independent review also exercised9 malicious/publication/generated-C++ probes. All passed; the shared temporary-name isolation finding was fixed. Python3.9 and3.14 tests pass; application runtime adds no dependency. See land-import-audit.md for exact scope.

Real official asset verification is tracked separately. Exact source bytes, version, projection, public-domain statement, boundary transformations, hole/seam rendering and Pi performance must be reviewed before a filled map ships. The strict initial importer may reject tiny source-coordinate overshoots; that is a visible validation failure, not authorization to relax bounds silently.

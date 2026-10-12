# Independent permanent importer regression audit

Status: **locally verified for this bounded test increment**. Reviewed 2026-10-03 after the writer froze `tests/map_import_regression.py`. No production or test source was changed by the independent reviewer.

The frozen suite ran independently with `python3 -B tests/map_import_regression.py`: 31 test methods passed, including multiple parameterized subcases, in 0.038 seconds. Additional independent checks reran eleven malformed shapefile/ZIP fixtures and verified a single capped archive read with a matching raw-byte hash. A separately corrupted stored ZIP member failed CRC validation through the converter CLI, returned exit 2 and created no new output file.

## Why these checks are meaningful

- The synthetic valid base is independently constructed from the documented SHP binary layout; exact expected coordinates and parts are asserted before malformed fixtures are used.
- Targeted corruptions assert specific rejection messages. Payload truncation and extra-payload cases repair the outer file length, preventing a trivial unrelated header-length rejection from satisfying the intended test.
- Per-record limits and temporarily lowered aggregate limits exercise distinct branches. Mocked limits restore automatically. Nonfinite and out-of-bounds coordinate checks include both coordinate axes and signs.
- ZIP tests exercise duplicate/missing/exact-root members, traversal-only names, ignored unrelated traversal entries, encrypted flags, archive byte cap and a highly compressed decoded-size overflow. No archive entry is extracted.
- A stream spy requires exactly one input open and the `MAX_ARCHIVE + 1` read request; independently expected SHA-256 comes from the exact supplied raw ZIP buffer.
- The subprocess test requires exit 2, the intended malformed-magic error and byte-for-byte preservation of a preexisting output sentinel. Generated-coordinate/part fragments have independent literal expectations rather than a self-comparison.

## Scope and limitations

The suite and its converter subprocess use no network. Synthetic archive and output writes stay in `TemporaryDirectory`; bytecode writing is disabled. Individual fixture allocations are bounded to approximately one MiB, and the CLI subprocess has a ten-second timeout. These local results do not establish Pi performance or full map parity.

The nonblocking isolation finding is resolved. The unrelated-traversal fixture now derives its parent-relative filename from the unique temporary directory basename and checks that same path. This removes the fixed `outside.txt` collision assumption while retaining the no-extraction assertion. After this one-line behavior change was frozen, independent re-review reran all 31 importer methods successfully (0.038 seconds) and `make verify` passed the native self-tests, executable regressions and importer suite. No remaining blocker was found for this test batch.

Malformed-input rejection happens before output writing, so these tests support preservation on validation failure. The converter's explicit developer-selected output uses ordinary `write_text`; atomic replacement or preservation on disk-full/write failure is not established or claimed by this test increment. Official asset reproduction, source license and map numerical review remain recorded separately in `docs/map-audit.md`.

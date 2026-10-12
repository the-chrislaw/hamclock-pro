# Resumed instrument model audit

Reviewed October 11, 2026 by an independent agent. **Bounded offline source and build gates pass.** Reviewer owns only this report; no source, build configuration, Git, provider, hardware or UI edits. No applicable AGENTS.md was found in checkout or the checked ancestor directories.

## Scope and source findings

Read actual catalog/snapshot headers, implementations and regression tests; compared the prior map/catalog and land-fill/snapshot audit reports and instrument workstream requirements. All six source/test hashes below match the prior reviewed freeze. The catalog remains 24 known manual families with explicitly unavailable module descriptors and a provisional three-slot single-choice model. It does not claim exhaustive original enums, multi-choice rotation, utility integration, provider support or native presentation. Its bounded fixed storage, explicit unknown-ID/slot rejection and atomic duplicate rejection have no allocation or I/O path.

Snapshot input rejects unknown IDs, units, qualities and phases; rejects nonfinite values, missing values with Valid quality, numbers with Missing/Invalid quality, malformed bounded ASCII provenance, reversed refresh/forecast ordering and negative age policy. Public evaluation validates schema even when hidden; enabled phases reject future fetch/observation/forecast-issue evidence and reversed monotonic evidence. Forecast presence never becomes a current observation; missing readings are never fabricated zeros. Ordered unsigned UTC subtraction safely represents the full signed64 span. Cache acceptance validates and copies first, then commits by statically verified nonthrowing swap. Failed validation/allocation preserves cache and phase.

No copied third-party implementation or production data appears in this scoped change; tests identify original synthetic fixtures. Descriptive provenance is explicitly caller-supplied, not rights/authenticity proof. This inspection does not certify every dependency, product name, provider permission or full project legal compliance. Provenance labels must remain plain text when a later UI renders them; validation permits printable ASCII including markup characters, so downstream HTML must escape them. Family-specific unit/range checks, histories, async generation guards, provider scheduling and protocol rights remain separate contracts.

## Fresh executable evidence

AppleClang 21, C++17, `-Wall -Wextra -Wpedantic -Werror`, AddressSanitizer and UndefinedBehaviorSanitizer:

- Catalog permanent regression: **1,566 checks, zero failures**.
- Snapshot permanent regression: **104 checks, zero failures**.
- Independently written temporary probe: **33,554,832 assertions, zero failures**. Most assertions enumerate all 256 underlying slot values against all 65,536 underlying instrument IDs and verify invalid-operation state preservation. Additional probes cover every invalid uint8 phase/unit, 25 signed64 timestamp pairs against an independent wider-integer oracle, and injected allocation failure at each of three successive long provenance-string copies. Allocation probes retain all prior provenance fields, observed value and Offline phase. These are finite-domain/model probes, not distinct real-world feature tests or hardware evidence.

Temporary binaries and probe source are in `/private/tmp/hamclock-resumed-model-audit`. No new dependency installed and no network used.

Root's frozen Makefile/CMake diff adds separate catalog and snapshot targets with the required catalog source dependency, includes the clock type header in Make prerequisites, includes `instrument-check` in `verify`, and registers both CTests through the existing target loop. Independent fresh scratch configure/build at `/private/tmp/hamclock-resumed-model-audit/build` succeeds with installed Qt base 6.11.2 and CMake; **all 12 CTests pass**. Independent `make verify` passes CLI/application regression, 31 coastline and 31 land importer methods, locator suites, 152 clock, 85 spot, and the 1,566/104 instrument checks. CMake uses Python 3.14.7; direct Make uses system Python 3.9. Both passed their respective importer paths. The harmless synthetic duplicate ZIP-member warning is a deliberate rejection fixture.

No native UI changed or was integrated in this increment. Existing offscreen/layout tests passing does not constitute new computer-control validation. Physical Pi3/Pi4 support, touchscreen usability, browser control, accessibility compliance, original HamClock feature parity and live source permissions remain untested/unclaimed.

## Frozen hashes

| File | SHA256 |
| --- | --- |
| src/app/instrument_catalog.hpp | fb094f58f2a7ffcabfd1a31d7cb7926fd5ed9425f82548359371528e11134e00 |
| src/app/instrument_catalog.cpp | 31672af4c910fe39d02ef7e272cb1951a8618792ff28eefdc2cfe5745ea24393 |
| src/app/instrument_snapshot.hpp | a82175f81d7c6503ed8fc520dcd82b2d85a6a6b0097d68b8812ace65fa255a45 |
| src/app/instrument_snapshot.cpp | a89320649e08295454283f64ff3c92a957be8c58679f3d682a60d032eb56cf51 |
| tests/instrument_catalog_regression.cpp | 0e36d942d7ac1165a0dd8a7318c29b9b754f3d2db8068845dbf1319def1b4ee9 |
| tests/instrument_snapshot_regression.cpp | da0ffd89fdacc2ad1c6d0dacad843d238fbb9df2f5aca37715814d19ec0ae52f |
| Makefile | 8806bb5c45127d70d297727d5e07889f2d0a69b1b6b7a2ea642bdc2f68ee10ef |
| CMakeLists.txt | 09be79ad41291121f9b6280cc37e7e96a31d4b6de91c5d89a1c126bb3eb6efdf |

Exact next action: root may prepare a bounded private draft increment containing only these reviewed model/test/build files and evidence after verifying the selected publication tree. Map LF1 files remain separately owned and outside this approval. No blocker found for this frozen offline model increment.

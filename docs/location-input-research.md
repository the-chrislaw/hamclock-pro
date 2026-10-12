# Location input: original behavior, standards and implementation paths

Research date: October 3, 2026. Scope: final-original HamClock 4.22 location entry. Research only; no original source implementation or assets were copied, and no live HamClock configuration was changed.

## Verified author evidence

The mirrored author-written [HamClock 4.22 User Guide, printed page 1 / PDF page 3](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf#page=3) permits Maidenhead entry and signed degrees or latitude N/S and longitude E/W suffixes. [Printed page 10 / PDF page 12](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf#page=12) specifies automatic linked-value computation, full internal precision and southwest-corner grid coordinates. Its example distinguishes a six-character mixed-case grid from its four-character main-display representation. Invalid confirmation stays flagged. **Confidence: high** for those explicit behaviors.

The document does **not** enumerate accepted grid-input lengths or input-case rules. Six-character output and four-character display do not prove either input length is accepted. It does not settle whitespace, exponent syntax, sign-plus-hemisphere conflicts, lowercase suffixes, degree symbols, comma decimal separators or pole/date-line normalization. **Confidence: unknown** for these exact parser details. PDF text was browsed directly; screenshots failed in this run, and the mirror is not byte-compared with an immutable author archive.

## Primary standards evidence and convention conflict

The official [IARU-R1 VHF Handbook 10.02, locator section / PDF pages 110–112](https://www.iaru-r1.org/wp-content/uploads/2024/11/VHF_Handbook_V10_02.pdf#page=110) defines longitude before latitude: A–R fields, numeric squares, A–X subsquares, then numeric subdivisions; alternating subdivisions extend to even-length higher precision. It recommends accepting mixed case while presenting uppercase. It specifies the **centroid** as the reference location. These standards establish a conversion specification, not HamClock's accepted grammar. **Confidence: high** for the cited handbook; no claim that 10.02 is the newest available edition.

The [ARRL grid-square explanation](https://www.arrl.org/about-grid-squares) independently describes four-character squares and six-character subsquares, including a mixed-case example. This supports common formats, not original HamClock validation rules.

**Important design consequence:** HamClock's explicitly documented southwest convention differs from IARU's centroid convention. Represent `SouthwestCorner` and `Centroid` as explicit conversion choices. Original-parity entry must select southwest; future contest/interchange features can select centroid deliberately. Never conflate the two.

## What can and cannot be concluded

| Question | Evidence | Conclusion / confidence |
|---|---|---|
| Southwest corner in original? | Author page 10 | Yes / high |
| Six-character and mixed-case representation? | Author page 10 example | Yes for representation / high |
| Original accepts four/six-character entry? | No explicit length rule in inspected manual | Unknown; plausible, unverified |
| Original accepts two/eight/ten characters? | Standards allow extended precision; original unspecified | Unknown; standards support cannot establish parity |
| Original accepts lowercase/mixed input? | Standard recommends it; author shows mixed-case output | Unknown original input rule |
| Hemisphere suffix forms? | Author page 1 | N/S latitude, E/W longitude supported / high; exact grammar unknown |

Do not borrow claims from OpenHamClock, HamClock-Next or other replacements to fill these gaps. They are separate products with different reference-point and grammar choices.

## Path A: original bounded Maidenhead parser

**Proposed engineering policy, not an original-behavior claim:** first implement a pure parser for explicitly bounded 4/6-character ASCII locators, with case-insensitive letters and canonical uppercase. Reject malformed pairs and unsupported lengths instead of truncating. Keep supported-length policy separate from the conversion arithmetic so confirmed additional lengths can be added without changing the corner convention. Return a validated geographic cell (west/south bounds plus width/height), then derive southwest or centroid explicitly. No network lookup or third-party parser is necessary.

**First acceptance step:** hand-derived fixtures and independent standard checks: equatorial `JJ00` has southwest (latitude 0, longitude 0), dimensions 1° latitude × 2° longitude; `JJ00AA` shares that corner with dimensions 1/24° × 1/12°. `JN18DU` has southwest latitude 48 + 20/24°, longitude 2 + 3/12°; centroid adds half each cell dimension. Verify mixed case, invalid A–R/A–X letters, odd lengths, embedded NUL/non-ASCII, boundaries and no mutation on failure. These are mathematical derivations and proposed tests, not a record of original runtime acceptance.

**Second acceptance step:** connect a single authoritative grid edit to the draft, recompute coordinates without a recursive update cycle, preserve intermediate typing, and save only validated data. Test Cancel/error/restart and derived route updates. Compare harmless original-runtime drafts at 2/4/6/8/10 lengths and varied case only in a controlled test instance; do not change the user's live station. Record accept/reject separately from output truncation. Until that evidence exists, call this implemented functionality with an open parity question.

## Path B: bounded hemisphere-coordinate parser

**Proposed engineering policy:** an axis-aware pure parser layered ahead of existing numeric validation. Latitude admits N/S, longitude E/W; use locale-independent ASCII digits and decimal point, explicit byte limits and full consumption. Accept ordinary signed decimal or unsigned magnitude plus a permitted suffix. Provisionally reject simultaneous sign and suffix rather than silently resolve contradictory direction, and reject exponent/symbol/locale extensions unless deliberately supported and tested. Existing decimal-coordinate entry must remain available.

**First acceptance step:** table-driven fixtures for north/south/east/west, zero, limits ±90/±180, wrong-axis suffix, malformed/partial tokens, overlong input, nonfinite/subnormal values and precision retention. Document the provisional grammar and separately investigate original acceptance of spaces, lowercase and combined signs. No existing persisted coordinate should be reformatted from a rounded visible string.

**Second acceptance step:** connect valid authoritative coordinate edits to locator display while retaining the exact double coordinate in the draft. Reopen/save/cancel tests must show that grid display does not snap a precise coordinate to a corner. Only explicit locator entry should invoke southwest conversion. Computer-test physical-keyboard and touch flows, meaningful field-specific errors and accessibility values; Pi/onscreen-keyboard validation remains separate.

## Recommendation and next evidence gate

Proceed with the independently specified pure conversion/domain layer and explicit reference-point policy. Hemisphere parsing is a smaller compatibility addition; locator parsing provides the missing grid entry and needs the extra authority/rounding safeguards. Both can be tested before UI integration, with a shared immutable validated station snapshot afterward. Exact original input limits remain an evidence gate, not a reason to invent parity or block all original implementation work.

No full feature parity, standards certification, legal clearance for redistributing author material, hardware support or accessibility compliance is established here. Source links document behavior research only.

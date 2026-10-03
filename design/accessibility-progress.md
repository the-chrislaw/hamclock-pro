# HamClock Pro prototype accessibility progress

This is a technical accessibility improvement record for the HTML design study, not an ADA compliance certification. The native Raspberry Pi application requires separate accessibility and hardware testing. All readings and diagrams remain illustrative.

## Implemented

- Native HTML buttons, labeled appearance select, h1/h2 headings, station landmark and keyboard skip link.
- Appearance offers System (default), Light and Dark; System follows changes to the computer preference without overriding an explicit selection. No persistence or network calls added.
- Propagation is now a semantic table with row/column headers and a caption. Every colored cell also carries a visible + / ~ / − symbol and a screen-reader condition name; visible legend identifies each symbol.
- Solar-flux, sunspot and wind pane selection updates the chart title and description with the sample value, unit and change. Description clearly states that the curve is schematic, not measured history. Solar disc and map include substantive alternatives and their limitations.
- Pane chooser is a nonmodal disclosure of ordinary buttons. Trigger reports expanded state; selecting or Escape returns focus. Tab is not trapped. Clicking outside closes it without stealing focus from another control.
- Compact controls have at least 24 CSS-pixel dimensions. Optional Touch controls sets buttons/select to at least 44 CSS pixels and lets panels expand vertically instead of clipping. Actual physical size and usability need verification on the seven-inch touch screen.
- Optional Larger text increases labels/details to 14 pixels and rearranges instruments to two columns. Under 700 CSS pixels the dashboard stacks and retains data; under 380 pixels instruments stack singly. Narrow layout restores solar details and offline status rather than hiding them.
- Three-pixel visible focus outlines for buttons, links and select. Updated dark-theme labels and propagation cell text; map labels and low-contrast toolbar/path details strengthened.

## Measured contrast samples

Calculated from CSS sRGB colors using WCAG relative luminance; these are specified solid-color pairs, not a whole-page rendered contrast audit.

| Pair | Ratio |
| --- | ---: |
| Light primary text / panel | 11.10:1 |
| Light secondary text / panel | 4.90:1 |
| Dark primary text / panel | 10.42:1 |
| Dark secondary text / panel | 5.08:1 |
| Dark chart labels / panel | 6.78:1 |
| Dark pane labels / panel | 7.43:1 |
| Dark path details / summary background | 7.34:1 |
| Propagation text / good fill | 6.07:1 |
| Propagation text / fair fill | 7.43:1 |
| Propagation text / poor fill | 9.35:1 |
| Light map labels / ocean | 5.45:1 |
| Dark map labels / land | 4.60:1 |

The original light toolbar text measured 4.34:1 against its light endpoint, so it was darkened. Gradients, transparency, night shading, focus outlines and thin graph/path strokes still need rendered measurements at their worst background location. Small chart/geography labels are backed by text alternatives but should be assessed visually with real hardware.

## Verification and remaining work

HTML parsing succeeded, IDs are unique, the semantic table is present, and JavaScript syntax checking passed. Root agent should now review the frozen HTML in the browser at 800×480, narrow 320px, larger-text/touch and both explicit appearance settings. This agent did not run browser visual or screen-reader tests.

Still required: VoiceOver/NVDA walkthrough; manual 200% text enlargement and 400% zoom; confirm no horizontal document overflow or inaccessible controls; physical touch and keyboard tests on Pi 4 seven-inch display; native accessible control mapping; live-data reading cadence; meaningful scientific chart descriptions once real observations exist. Some compact labels remain small by design, so enlargement must be easy to reach. No claim of completed WCAG 2.2 AA conformance is made.

Skill used: accessibility/SKILL.md from the ECC plugin. Target: WCAG 2.2 AA.

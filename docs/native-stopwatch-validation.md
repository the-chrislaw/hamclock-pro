# Native stopwatch validation — October 11, 2026

Scope: original native stopwatch dialog and dashboard entry over the independently audited monotonic model and held-display presentation. This is a bounded subset, not full original HamClock parity. Run starts/resumes and explicitly releases a held display; Stop freezes counting; Lap holds displayed elapsed while underlying time advances; Reset clears state; Exit only closes the view. Timers survive view closure within this process, not application restart. No countdown, alarm, new dependencies, network activation, radio operation or production station changes.

## Automated and independent evidence

Final corrected local Qt 6.11.2 build: all 18 CTests pass (3.66 seconds), including 167 dashboard checks and 117 dialog checks; executable `make verify` passes. Independent reviewer reran 117 permanent checks plus 12 separate strict Address/Undefined sanitizer probes and a fresh isolated whole build: all 18 CTests pass. Final code/test hashes and source findings are in workstreams/native-stopwatch-dialog-audit.md.

Two regressions were found, fixed and rerun before publication. Initial dashboard integration failed eight focus checks; activating the dashboard before restoring origin focus resolved both local and independent suites. First actual native accessibility inspection exposed static names that omitted current timer/status values. The corrected implementation prepares dynamic accessible names with its candidate view before committing shared state; permanent actual QAccessible regressions cover initial, tick, Hold, Stop, error and reset. Independent source/sanitizer/full-build review was repeated after that correction.

## Actual native computer observations

Controlled the rebuilt isolated FilledMapQA.app with synthetic station N1TEST, DE 0/0, DX 0/90. Live HamClock and user station files were untouched. The approved local QA wrapper was reused with the final compiled dashboard executable. Quit the old modal through Exit and then application Quit; relaunched the corrected process. A background window may initially produce a stale screenshot; exposed accessibility Raise and keyboard focus were used before evaluating visible controls. This was not treated as a timer performance benchmark.

Observed final corrected UI via computer controls at 21:08–21:09 UTC:

- Ready showed 00:00:00.00 in both visible text and the native accessibility tree, with Stop/Lap disabled.
- Run advanced visible and accessible elapsed (00:00:00.19), status Running and enabled Stop/Lap. Light view had clear text and all five buttons inside the compact dialog.
- Lap held displayed 00:00:03.12 while actual elapsed was 00:00:03.29. Native accessibility text included both elapsed values and “Display held — counting continues”.
- Exit returned keyboard focus to Open stopwatch. Space reopened the view with the same held 00:00:03.12 and actual 00:00:08.99, confirming continued counting while closed.
- Space on Run released Hold; Tab/Space stopped at 00:00:12.86. A subsequent reverse Tab observation still showed exactly 00:00:12.86 and Stopped. Reverse Tab skipped disabled Stop/Lap to Run; Space resumed at 00:00:13.17. Reset restored Ready and 00:00:00.00.
- Keyboard dashboard appearance selected Dark and reopened by Tab/Space. Dark dialog showed all five unclipped buttons, clear Ready/elapsed text and a visible focus outline on Run. System selection reopened with the current host dark appearance. Light, Dark and System were app previews, not host preference changes.
- Station/grid/route summary remained N1TEST, JJ00AA/NJ50AA, 10008 km/90 degrees across view closure. The existing real map remained available beneath the modal.

## Limits and next work

Actual native exception/reversed-clock branches were not forced through the real QElapsedTimer; injectable-clock automated and sanitizer tests cover them. Actual VoiceOver announcements, full accessibility compliance, physical touch, seven-inch hardware, Pi 3/Pi 4, minimum Qt 6.4, suspend behavior and long-running resource measurements are untested. Current native tree values are verified, not a screen-reader certification. Timer state intentionally does not persist across process exit. Countdown model is the next separately owned and independently audited deliverable.

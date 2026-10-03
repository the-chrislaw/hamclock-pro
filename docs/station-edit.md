# Station editing draft

Original C++17 presentation model for future native station-edit controls. `StationEdit` validates its baseline through the station-view model, then snapshots all six fields as text. Geographic numbers use the classic locale and 17 significant digits for exact supported-double round trips.

`set()` accepts bounded intermediate text, including temporarily invalid strings such as `-` or an empty field: callsign up to 16 bytes, coordinates up to 64, appearance up to 6. Oversized values and unknown fields return false without changes. `get()` throws `std::invalid_argument` for an unknown field. `dirty()` compares draft text with the initial snapshot, so different spellings of an equivalent number count as changes. `cancel()` restores the original snapshot.

`validated()` returns independent settings without changing the draft or original snapshot. Coordinate parsing accepts only classic-locale decimal syntax, including decimal exponents, with complete consumption. Empty/incomplete input, whitespace, hexadecimal numbers, NaN/infinity, overflow, and underflow are rejected. Final station-view validation applies geographic bounds, the persistence subnormal guard, callsign rules, and System/Light/Dark choices. Errors throw `std::invalid_argument`; the UI must keep the draft open and surface the error.

There are no writes, network calls, or configuration execution. Producing validated settings does not save them or establish a new baseline. Application integration must obtain an explicit save action and handle persistence errors; closing/cancelling must not overwrite the last saved configuration. Native widgets, display interactions, and accessibility remain separate work.

`run_station_edit_tests()` covers minimum-normal and full-precision round trips, intermediate drafts, malformed and unsafe numeric formats, bounds, cancellation, all field lengths, invalid enumeration values, invalid baseline rejection, and unchanged baseline/draft on validation failures.

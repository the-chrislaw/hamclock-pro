# Local station settings

Original C++17 persistence code, without third-party dependencies, for macOS and Raspberry Pi POSIX systems. The caller supplies the path explicitly. This module does not choose a home directory, open network ports, or execute configuration content.

`StationSettings` contains DE and DX latitude/longitude, callsign, and appearance. Coordinates must be finite and within the domain module's geographic bounds. This persistence format additionally rejects nonzero subnormal doubles (absolute values below `std::numeric_limits<double>::min()`) because the standard-library decimal parser cannot reliably reload them on supported platforms. Save validation rejects these values before filesystem replacement; it never silently rounds them to zero. Zero and the smallest normal double are supported. Callsigns accept 1–16 ASCII letters, digits, `/`, or `-`; this is input hygiene, not a claim of regulatory callsign validation. Appearance is exactly `system`, `light`, or `dark`.

The UTF-8-compatible ASCII file format is versioned, one key/value per line with a required final newline:

```
version=1
de_lat=37.7
de_lon=-122.4
dx_lat=51.5
dx_lon=-0.1
callsign=N0CALL/P
appearance=system
```

All seven fields are required. Field order is unrestricted. Duplicate or unknown fields, unsupported versions, empty lines, malformed numbers, whitespace, and invalid values are rejected. Files are limited to 4096 bytes, including reads from a file that grows while being read. Numeric serialization uses the classic locale and 17 significant digits for double round trips.

Loading accepts regular files only and rejects final-path symlinks. It never substitutes defaults for missing or corrupted settings. Invalid content throws `std::invalid_argument`; filesystem failures throw `std::runtime_error`. The UI must surface a failed load and offer an explicit recovery decision before overwriting it.

Saving validates before touching disk and uses `lstat` to reject existing final destinations that are symlinks or nonregular files (including directories). It creates an exclusive random temporary file beside the target, sets mode `0600`, writes and synchronizes its contents, closes it, then atomically renames it over the explicit target. Failures before replacement preserve the prior target and attempt temporary-file cleanup. The destination check is not a claim of eliminating time-of-check/time-of-use races. This API assumes the caller selects a trusted, stable private parent directory under their control; other processes with write access could change the path between inspection and replacement. Parent-directory symlinks are not prohibited. The operation does not synchronize the parent directory, so durable replacement after sudden power failure is not guaranteed. No interprocess locking is provided; concurrent writers use last-successful-rename wins.

`run_settings_tests()` returns a failure count. Tests cover exact round trips including the smallest normal double, rejection and prior-file preservation for positive/negative subnormal coordinates in all four fields, all appearance choices, permissions, rejected fields and values, size bounds, missing paths, preservation after invalid saves, explicit recovery, preservation of rejected symlink and FIFO save destinations, symlinks on load, nonblocking FIFO rejection, directory-destination rejection, and temporary-file cleanup. Integrate the three `src/platform` files into the application build and invoke this test function from its self-test entry point.

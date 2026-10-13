# Vendored SGP4 (satellite propagation)

- Code: `src/domain/astro_sgp4/SGP4.cpp`, `SGP4.h`: David Vallado's reference SGP4/SDP4 C++ implementation (version 2020-07-13, from "Revisiting Spacetrack Report #3" and the 2006 AIAA paper by Vallado, Crawford, Hujsak and Kelso), with one local patch: `sgp4init` skips the `strcpy` of `satnum` onto itself (flagged by ASan as overlapping, undefined behaviour). Line marked `hamclock:`.
- Source: the C++ extension shipped in the `sgp4` 2.27 PyPI source distribution (Brandon Rhodes), which redistributes it under the MIT licence. Licence text: `src/domain/astro_sgp4/LICENSE-python-sgp4-MIT`.
- Use: compiled into `hamclock-widgets` with warnings suppressed for these two files only. Our wrapper is `src/domain/astro_satellite.*`.
- Validation: `widget-satellites-satellites_regression` checks the Spacetrack Report #3 verification case (object 88888) against the published state vectors.

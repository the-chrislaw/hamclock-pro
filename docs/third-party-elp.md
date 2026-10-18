# Vendored lunar theory (ELP2000-82B)

- Data: `src/domain/astro_elp/elp82b_terms.inc`, generated from the 36 series files of "Lunar Solution ELP 2000-82B", M. Chapront-Touze and J. Chapront, Astron. Astrophys. 190, 342 (1988), as published by the CDS as catalogue VI/79 (https://cdsarc.cds.unistra.fr/ftp/VI/79/, fetched 2026-10-18). Constants are the DE200-fitted ones from the CDS `elp82b.f`.
- Licence: the CDS distributes the catalogue as open scientific data with citation requested (ReadMe cites the two papers above); no restrictive licence is stated. The tables are numeric coefficients, not code.
- Truncation: terms below 3e-8 rad (0.006 arcsec, 11 m in distance) are dropped, leaving 1,252 main-problem and 671 perturbation terms (about 48 kB). Compared with the full theory the position differs by under 0.2 arcsec and 0.2 km over 1992-2050.
- Evaluator: `src/domain/astro_elp.{hpp,cpp}`, our own code following `elp82b.f` (every perturbation argument is reduced to multipliers of 13 fundamental angles).
- Validation: `widget-eclipse-elp_regression` (full-theory values from a direct port of the Fortran, Meeus example 47.a, agreement with the coarse series). Eclipse contact times validated against NASA tables follow in a later change.

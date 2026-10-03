# Original short-path map sampling

`map_short_path(DE, DX, subdivisions=64)` produces geographic samples of the short great-circle route on the same spherical model used by station geometry. It adds no dependencies, network access or external assets.

The algorithm constructs a unit vector at DE and an orthogonal tangent using local north/east components toward DX. Each interior point rotates through the central angle with sine/cosine and converts back using atan2. This avoids dividing by the sine of the complete angle in conventional endpoint slerp and avoids subtracting nearly identical endpoint vectors. The shared `geometry()` function validates both inputs and decides whether a route is defined.

- Subdivisions must be between 1 and 512 inclusive, limiting the result to at most 513 coordinates.
- A defined route returns subdivisions + 1 points, preserving the exact supplied first/last coordinates.
- Coincident locations, including different longitude representations of a pole, return the single supplied DE location.
- Antipodal and near-antipodal cases classified undefined by station geometry throw `std::invalid_argument`; no arbitrary route is drawn.
- Coordinates use degrees north/east and remain in geographic bounds. Preserve each geographic sample until projection; do not connect raw screen endpoints across the dateline.

The renderer should apply `map_split_equirectangular()` to each adjacent sample, then draw each returned segment. This route helper supports the short path only. It does not provide a propagation prediction, ellipsoidal survey route, long path or azimuthal map implementation. At a pole, longitude is mathematically non-unique; latitude and Cartesian direction determine correctness, and tests do not demand a particular pole longitude.

Validation: an independent standalone harness compiled the domain/path sources with C++17, warnings, AddressSanitizer and UndefinedBehaviorSanitizer and passed. Fixtures cover known equatorial midpoint, dateline short route, north/south pole crossing, coincident pole, exact endpoint preservation, 1/64/512 subdivision sizes, equal arc spacing, valid near-antipodal sampling, undefined antipodal tolerance, invalid/nonfinite inputs and subdivision limits. These local checks do not establish Pi rendering performance.

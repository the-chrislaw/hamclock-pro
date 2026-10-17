# Vendored QR encoder

- Code: `src/domain/astro_qr/qrcodegen.cpp`, `qrcodegen.hpp`: Project Nayuki "QR Code generator library" (C++), unmodified, from https://github.com/nayuki/QR-Code-generator (`cpp/`), fetched 2026-10-17.
- Licence: MIT, Copyright (c) Project Nayuki. The full licence text is in the header of both files.
- Use: compiled into `hamclock-widgets` as its own warning-free static library (`hamclock-qrcodegen`); our wrapper is `src/domain/astro_qr.*`. Intended for showing the EME tool link as a QR code on the kiosk (issue #440).
- Validation: `widget-eme-qr_regression` checks size, finder patterns, timing patterns and the version-1 capacity limits.

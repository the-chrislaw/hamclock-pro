#pragma once
#include <string>
#include <vector>
namespace hamclock {
// Small QR-code wrapper over the vendored Nayuki encoder (src/domain/astro_qr/,
// MIT, docs/third-party-qr.md). Used to show a link as a QR code on kiosks
// that have no browser.
struct QrMatrix {
    int size = 0;                  // modules per side; 0 when the text does not fit
    std::vector<bool> dark;        // size*size, row-major, true = dark module
    bool at(int x, int y) const { return x >= 0 && y >= 0 && x < size && y < size && dark[size_t(y) * size + x]; }
};
// Medium error correction; empty matrix (size 0) when the text is too long.
QrMatrix qr_encode(const std::string& text);
} // namespace hamclock

// Pane fill measure (adaptive panes, "no wasted space"): the tallest run of blank
// rows in a card body, as a fraction of its height.
#include "../src/ui/pane_fill.hpp"
#include <QImage>
#include <cmath>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

constexpr QRgb kSurface = 0xff2b3236;
constexpr QRgb kInk = 0xffe2e7e6;

QImage blank_card() {
    QImage image(100, 100, QImage::Format_RGB32);
    image.fill(kSurface);
    return image;
}
void draw_rows(QImage& image, int from, int to) {
    for (int y = from; y < to; ++y) image.setPixel(50, y, kInk);
}
} // namespace

int main() {
    using hamclock::blank_band;
    const QRect body(0, 0, 100, 100);

    // An empty body is all blank.
    QImage image = blank_card();
    check(near(blank_band(image, body, kSurface), 1.0), "empty body is blank");

    // A two-line summary in the top fifth leaves the rest blank: the old compact Pi pane.
    draw_rows(image, 0, 20);
    check(near(blank_band(image, body, kSurface), 0.8), "summary at the top leaves 80 % blank");
    check(blank_band(image, body, kSurface) > hamclock::kMaxBlankBand, "top summary is underfilled");

    // The same content grown to fill, centred: only small bands above and below.
    image = blank_card();
    draw_rows(image, 20, 80);
    check(near(blank_band(image, body, kSurface), 0.2), "centred content leaves its larger band");
    check(blank_band(image, body, kSurface) <= hamclock::kMaxBlankBand, "centred content passes");

    // A list with gaps between rows: the largest gap counts, not their sum.
    image = blank_card();
    for (int y = 0; y < 100; y += 20) draw_rows(image, y, y + 10);
    check(near(blank_band(image, body, kSurface), 0.1), "row gaps count one at a time");

    // Anti-aliasing noise close to the surface colour is still blank.
    image = blank_card();
    image.setPixel(10, 50, qRgb(qRed(kSurface) + 3, qGreen(kSurface) - 2, qBlue(kSurface) + 1));
    check(near(blank_band(image, body, kSurface), 1.0), "near-surface noise is blank");

    // The rectangle is clipped to the image; an empty rectangle measures nothing.
    check(near(blank_band(image, QRect(50, 50, 100, 100), kSurface), 1.0), "rect clipped to image");
    check(near(blank_band(image, QRect(), kSurface), 0.0), "empty rect");

    if (failures) return 1;
    std::cout << "pane fill regression passed\n";
    return 0;
}

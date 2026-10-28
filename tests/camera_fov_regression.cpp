#include "../src/platform/apple_camerafov.hpp"

#include <cmath>
#include <cstdio>

using namespace hamclock;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)
static bool near(double a, double b, double e = 0.05) { return std::fabs(a - b) < e; }

int main()
{
    // Landscape preview with the same aspect as the sensor: no crop.
    PreviewFov a = previewFov(65, {4000, 3000}, {800, 600});
    CHECK(near(a.hfov_deg, 65));
    CHECK(a.vfov_deg < a.hfov_deg && a.vfov_deg > 40);
    // Portrait full-screen preview crops the width: both below the sensor's long-side fov.
    PreviewFov p = previewFov(65, {4000, 3000}, {390, 844});
    CHECK(p.hfov_deg < 65 && p.vfov_deg < 65.05);
    CHECK(p.vfov_deg > p.hfov_deg);
    CHECK(near(p.vfov_deg, 65)); // height of a portrait preview spans the sensor's long side
    // Wider preview than sensor crops the height instead.
    PreviewFov w = previewFov(65, {4000, 3000}, {1000, 400});
    CHECK(near(w.hfov_deg, 65) && w.vfov_deg < 30);
    // Invalid input falls back to defaults.
    PreviewFov d = previewFov(0, {4000, 3000}, {1, 1});
    CHECK(near(d.hfov_deg, 60) && near(d.vfov_deg, 45));
    CHECK(near(previewFov(65, {0, 0}, {1, 1}).hfov_deg, 60));
    return fails ? 1 : 0;
}

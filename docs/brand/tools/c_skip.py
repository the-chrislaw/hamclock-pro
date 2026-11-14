"""Concept C "Skip": HF sky-wave propagation. A signal leaves a station on
Earth's limb, refracts off the glowing ionosphere and skips around the curve
of the planet - the physics every HamClock band panel is predicting."""
import math

import geo
from svg import Concept, blur, lin, rad

EX, EY, RE = 512, 1720, 1060      # Earth: a huge circle whose top sits at y = 660
RI = RE + 330                     # ionosphere layer radius
LAND = geo.load_land_rings()
VIEW = geo.Ortho(-35, -12, EX, EY, RE)
HOPS = (-17, 17)                 # station and far-end ground points (one sky-wave hop)

PALETTE = {
    "light": dict(sky0="#0B0A2E", sky1="#1A2466", sky2="#2D5FA8", earth0="#1D5FA0", earth1="#071B3C",
                  iono0="#5CFFC8", iono1="#9A7CFF"),
    "dark": dict(sky0="#020210", sky1="#0B1238", sky2="#1B3D73", earth0="#174E86", earth1="#030D22",
                 iono0="#4FE8B6", iono1="#8A6CF0"),
}


def _pal(variant):
    return PALETTE.get(variant, PALETTE["light"])


def _pt(r, deg):
    a = math.radians(deg)
    return EX + r * math.sin(a), EY - r * math.cos(a)


def defs(variant):
    p = _pal(variant)
    return "".join([
        lin("gC_sky", [(0, p["sky0"], 1), (0.55, p["sky1"], 1), (1, p["sky2"], 1)], 0, 0, 0, 1),
        rad("gC_earth", [(0, p["earth0"], 1), (1, p["earth1"], 1)], cy=0.0, r=0.6),
        lin("gC_iono", [(0, p["iono1"], 1), (0.5, p["iono0"], 1), (1, p["iono1"], 1)], 0, 0, 1, 0),
        lin("gC_sig", [(0, "#E6FBFF", 1), (0.5, "#9BEBFF", 1), (1, "#FFC86B", 1)], 0, 0, 1, 0),
        rad("gC_horizon", [(0, "#7FD8FF", 0), (0.97, "#7FD8FF", 0), (0.985, "#A8E6FF", 0.9), (1, "#A8E6FF", 0)]),
        lin("gC_fade", [(0, "#FFC86B", 0.85), (1, "#FFC86B", 0)], 0, 1, 1, 0),
        blur("fC_iono", 22), blur("fC_glow", 13), blur("fC_haze", 30), blur("fC_tight", 4),
    ])


STARS = ((0.12, 0.10, 2.4), (0.27, 0.22, 1.6), (0.45, 0.08, 2.0), (0.63, 0.17, 1.4), (0.8, 0.07, 2.4),
         (0.9, 0.24, 1.6), (0.07, 0.3, 1.4), (0.36, 0.33, 1.2), (0.7, 0.3, 1.2))


def background(w, h, variant):
    k = min(w, h) / 1024
    stars = "".join('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#DCEBFF" opacity="0.6"/>' % (x * w, y * h, r * k)
                    for x, y, r in STARS)
    return '<rect width="%d" height="%d" fill="url(#gC_sky)"/>%s' % (w, h, stars)


def _arc_d(r, a0, a1):
    x0, y0 = _pt(r, a0)
    x1, y1 = _pt(r, a1)
    return "M%.1f %.1f A%.1f %.1f 0 0 1 %.1f %.1f" % (x0, y0, r, r, x1, y1)


def _planet(variant):
    return "".join([
        '<path d="%s" fill="none" stroke="url(#gC_iono)" stroke-width="70" opacity="0.55" filter="url(#fC_iono)"/>'
        % _arc_d(RI, -60, 60),
        '<path d="%s" fill="none" stroke="url(#gC_iono)" stroke-width="10" opacity="0.85"/>' % _arc_d(RI, -60, 60),
        '<circle cx="%d" cy="%d" r="%d" fill="#6FCBFF" opacity="0.45" filter="url(#fC_haze)"/>' % (EX, EY, RE + 30),
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gC_earth)"/>' % (EX, EY, RE),
        '<path d="%s" fill="#5FB8F0" fill-opacity="0.38"/>' % geo.ortho_land_d(VIEW, LAND),
        '<circle cx="%d" cy="%d" r="%d" fill="none" stroke="#BDEBFF" stroke-width="5" opacity="0.8"/>'
        % (EX, EY, RE - 2),
    ])


def _hop_d():
    """Straight ray up, a short refraction curve inside the layer, straight ray down."""
    x0, y0 = _pt(RE, HOPS[0])
    x1, y1 = _pt(RE, HOPS[1])
    ax, ay = _pt(RI - 22, 0)
    k = 0.16
    lx, ly = ax + (x0 - ax) * k, ay + (y0 - ay) * k
    rx, ry = ax + (x1 - ax) * k, ay + (y1 - ay) * k
    return "M%.1f %.1f L%.1f %.1f Q%.1f %.1f %.1f %.1f L%.1f %.1f" % (
        x0, y0, lx, ly, ax, ay - 24, rx, ry, x1, y1)


def _next_hop_d():
    """The start of the next hop, fading out: the signal keeps skipping."""
    x0, y0 = _pt(RE, HOPS[0])
    ax, ay = _pt(RI - 22, 0)
    x1, y1 = _pt(RE, HOPS[1])
    turn = math.radians(0.3 * (HOPS[1] - HOPS[0]))  # steepened for legibility
    dx, dy = ax - x0, ay - y0
    rx = dx * math.cos(turn) - dy * math.sin(turn)
    ry = dx * math.sin(turn) + dy * math.cos(turn)
    return "M%.1f %.1f L%.1f %.1f" % (x1, y1, x1 + 0.42 * rx, y1 + 0.42 * ry)


def _signal(variant):
    d = _hop_d()
    sx, sy = _pt(RE, HOPS[0])
    mx, my = _pt(RI - 30, 0)
    ex, ey = _pt(RE, HOPS[-1])
    waves = "".join(
        '<path d="M%.1f %.1f A%d %d 0 0 1 %.1f %.1f" fill="none" stroke="#FFFFFF" stroke-width="13" '
        'stroke-linecap="round" opacity="%.2f" transform="rotate(%d %.1f %.1f)"/>'
        % (sx - r, sy, r, r, sx + r, sy, o, HOPS[0], sx, sy)
        for r, o in ((62, 0.8), (104, 0.45)))
    nd = _next_hop_d()
    return "".join([
        '<path d="%s" fill="none" stroke="url(#gC_fade)" stroke-width="20" stroke-linecap="round"/>' % nd,
        '<path d="%s" fill="none" stroke="#7FE3FF" stroke-width="50" stroke-linecap="round" stroke-linejoin="round" '
        'opacity="0.55" filter="url(#fC_glow)"/>' % d,
        '<path d="%s" fill="none" stroke="url(#gC_sig)" stroke-width="24" stroke-linecap="round"/>' % d,
        '<path d="%s" fill="none" stroke="#FFFFFF" stroke-width="8" stroke-linecap="round" opacity="0.9"/>' % d,
        waves,
        '<circle cx="%.1f" cy="%.1f" r="34" fill="#E9FFF6" opacity="0.75" filter="url(#fC_glow)"/>' % (mx, my),
        '<circle cx="%.1f" cy="%.1f" r="32" fill="#FFFFFF"/>' % (sx, sy),
        '<circle cx="%.1f" cy="%.1f" r="46" fill="#FFB547" opacity="0.6" filter="url(#fC_tight)"/>' % (ex, ey),
        '<circle cx="%.1f" cy="%.1f" r="30" fill="#FFB547"/>' % (ex, ey),
    ])


def layers(variant):
    return [("planet", _planet(variant)), ("signal", _signal(variant))]


CONCEPT = Concept(
    slug="skip",
    title="C - Skip",
    rationale="HF sky-wave in one stroke: a signal leaves a station on Earth's limb, bounces off the "
              "glowing ionosphere and skips over the horizon. The physics behind every band forecast.",
    defs=defs, background=background, layers=layers,
)

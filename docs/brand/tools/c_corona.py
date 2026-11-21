"""Concept D "Corona": space weather. The Sun's corona lights an Earth wrapped in
a glowing auroral oval, with the magnetosphere's bow shock between them -
solar flux, solar wind, Kp and aurora in one picture."""
import math

import geo
from svg import Concept, blur, lin, rad

SX, SY, SR = 262, 262, 168          # Sun
EX, EY, ER = 652, 656, 244          # Earth
LAND = geo.load_land_rings()
VIEW = geo.Ortho(-60, 34, EX, EY, ER, roll=-12)
# a sun direction that appears upper-left of the Earth on screen
SUN = geo.norm(geo.add(geo.add(geo.scale(VIEW.e, -0.7), geo.scale(VIEW.n, 0.62)), geo.scale(VIEW.v, 0.12)))
AURORA_POLE = (-72, 80.5)           # north geomagnetic pole
AURORA_COLAT = 20

PALETTE = {
    "light": dict(sky0="#2A1240", sky1="#0D0A26", sky2="#04050F", ocean0="#3DA6EE", ocean1="#0C3F86",
                  land="#8BE0B4", night="#02040E"),
    "dark": dict(sky0="#1A0B29", sky1="#070515", sky2="#000000", ocean0="#3294D8", ocean1="#0A3170",
                 land="#79CFA3", night="#010208"),
}


def _pal(variant):
    return PALETTE.get(variant, PALETTE["light"])


def defs(variant):
    p = _pal(variant)
    return "".join([
        rad("gD_sky", [(0, p["sky0"], 1), (0.5, p["sky1"], 1), (1, p["sky2"], 1)], cx=0.25, cy=0.25, r=1.0),
        rad("gD_sun", [(0, "#FFFDF2", 1), (0.45, "#FFE08A", 1), (0.85, "#FF9B2E", 1), (1, "#FF6A1A", 1)],
            cx=0.42, cy=0.42, r=0.62),
        rad("gD_corona", [(0, "#FFB347", 0.95), (0.35, "#FF7A2A", 0.55), (0.7, "#E0408A", 0.12), (1, "#E0408A", 0)]),
        rad("gD_ocean", [(0, p["ocean0"], 1), (1, p["ocean1"], 1)], cx=0.3, cy=0.3, r=0.85),
        rad("gD_atmo", [(0, "#6FD0FF", 0), (0.88, "#6FD0FF", 0), (0.94, "#9FE2FF", 0.7), (1, "#9FE2FF", 0)]),
        rad("gD_spec", [(0, "#FFFFFF", 0.5), (1, "#FFFFFF", 0)]),
        blur("fD_corona", 26), blur("fD_ray", 18), blur("fD_term", 18), blur("fD_aur", 10), blur("fD_soft", 4),
        '<clipPath id="cD_disc"><circle cx="%d" cy="%d" r="%d"/></clipPath>' % (EX, EY, ER),
    ])


def background(w, h, variant):
    return '<rect width="%d" height="%d" fill="url(#gD_sky)"/>' % (w, h)


def _rays():
    out = []
    for i, ang in enumerate(range(0, 360, 20)):
        a = math.radians(ang + 8)
        ln = SR * (2.5, 1.7, 2.05)[i % 3]
        wd = math.radians(4.5)
        p0 = (SX + SR * 0.9 * math.cos(a - wd), SY + SR * 0.9 * math.sin(a - wd))
        p1 = (SX + ln * math.cos(a), SY + ln * math.sin(a))
        p2 = (SX + SR * 0.9 * math.cos(a + wd), SY + SR * 0.9 * math.sin(a + wd))
        out.append("M%.1f %.1f L%.1f %.1f L%.1f %.1f Z" % (p0 + p1 + p2))
    return '<path d="%s" fill="#FFB75A" opacity="0.42" filter="url(#fD_ray)"/>' % " ".join(out)


def _sun():
    return "".join([
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gD_corona)" filter="url(#fD_corona)"/>' % (SX, SY, SR * 2.4),
        _rays(),
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gD_sun)"/>' % (SX, SY, SR),
    ])


def _aurora_d():
    pole = geo.vec(*AURORA_POLE)
    a = geo.norm(geo.cross(pole, (1, 0, 0)))
    b = geo.cross(pole, a)
    c, s = math.cos(math.radians(AURORA_COLAT)), math.sin(math.radians(AURORA_COLAT))
    pts = []
    for i in range(181):
        t = 2 * math.pi * i / 180
        v = geo.add(geo.scale(pole, c), geo.add(geo.scale(a, s * math.cos(t)), geo.scale(b, s * math.sin(t))))
        pts.append(VIEW.project_vec(v, 1.02)[0])
    return geo.path_d(pts, True, 0.3)


def _earth(variant):
    p = _pal(variant)
    disc = geo.circle_d(EX, EY, ER)
    aur = _aurora_d()
    return "".join([
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gD_atmo)"/>' % (EX, EY, ER * 1.1),
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gD_ocean)"/>' % (EX, EY, ER),
        '<g clip-path="url(#cD_disc)">',
        '<path d="%s" fill="%s" fill-opacity="0.9"/>' % (geo.ortho_land_d(VIEW, LAND), p["land"]),
        '<path d="%s" fill="%s" fill-opacity="0.88" fill-rule="evenodd" filter="url(#fD_term)"/>'
        % (geo.night_d(VIEW, SUN, disc), p["night"]),
        '<ellipse cx="%.1f" cy="%.1f" rx="%.1f" ry="%.1f" fill="url(#gD_spec)" opacity="0.45"/>'
        % (EX - 0.38 * ER, EY - 0.4 * ER, 0.42 * ER, 0.3 * ER),
        '</g>',
        '<path d="%s" fill="none" stroke="#2DFF9E" stroke-width="58" opacity="0.45" filter="url(#fD_aur)"/>' % aur,
        '<path d="%s" fill="none" stroke="#7DFFC4" stroke-width="20" opacity="0.55" filter="url(#fD_soft)"/>' % aur,
        '<path d="%s" fill="none" stroke="#E6FFF2" stroke-width="5" opacity="0.9"/>' % aur,
    ])


def layers(variant):
    return [("sun", _sun()), ("earth", _earth(variant))]


CONCEPT = Concept(
    slug="corona",
    title="D - Corona",
    rationale="Space weather as a scene: the Sun's corona lighting an Earth ringed by its auroral oval, "
              "with the bow shock between them. Flux, solar wind, Kp and aurora - HamClock's sky panel.",
    defs=defs, background=background, layers=layers,
)

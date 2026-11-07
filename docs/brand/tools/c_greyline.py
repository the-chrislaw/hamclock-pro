"""Concept A "Greyline": a lit globe split by the soft day/night terminator with a
glowing great-circle DE->DX arc lifted off the surface."""
import math

import geo
from svg import Concept, blur, lin, rad

CX, CY, R = 512, 530, 352
VIEW = geo.Ortho(-38, 24, CX, CY, R, roll=-8)
SUN = geo.subsolar(50, -23)
DE, DX = (-84, 38), (28, 46)  # eastern US -> eastern Europe
LAND = geo.load_land_rings()

PALETTE = {
    "light": dict(sky0="#173B74", sky1="#071431", sky2="#030817", ocean0="#45B4F5", ocean1="#1567B8",
                  ocean2="#0A3576", land="#9BE8C4", night="#040B22", arc0="#FFE29A", arc1="#FF9F3A"),
    "dark": dict(sky0="#0C1E3D", sky1="#040A1A", sky2="#000000", ocean0="#3A9EDF", ocean1="#0F539B",
                 ocean2="#082A5E", land="#86D9B2", night="#02061A", arc0="#FFE29A", arc1="#FF9F3A"),
}


def _pal(variant):
    return PALETTE.get(variant, PALETTE["light"])


def defs(variant):
    p = _pal(variant)
    return "".join([
        rad("gA_sky", [(0, p["sky0"], 1), (0.55, p["sky1"], 1), (1, p["sky2"], 1)], cy=0.35, r=0.8),
        rad("gA_ocean", [(0, p["ocean0"], 1), (0.6, p["ocean1"], 1), (1, p["ocean2"], 1)],
            cx=0.68, cy=0.38, r=0.75, fx=0.72, fy=0.32),
        rad("gA_atmo", [(0, "#5FC8FF", 0), (0.86, "#5FC8FF", 0), (0.93, "#7FD8FF", 0.55), (1, "#7FD8FF", 0)]),
        rad("gA_spec", [(0, "#FFFFFF", 0.55), (1, "#FFFFFF", 0)]),
        rad("gA_shade", [(0, "#000000", 0), (0.75, "#000000", 0), (1, "#000814", 0.45)]),
        lin("gA_arc", [(0, p["arc0"], 1), (1, p["arc1"], 1)], 0, 0, 1, 0),
        lin("gA_glass", [(0, "#FFFFFF", 0.22), (0.5, "#FFFFFF", 0.0)], 0, 0, 0, 1),
        blur("fA_term", 16), blur("fA_glow", 14), blur("fA_glow2", 5), blur("fA_atmo", 6),
        '<clipPath id="cA_disc"><circle cx="%d" cy="%d" r="%d"/></clipPath>' % (CX, CY, R),
    ])


def background(w, h, variant):
    stars = ""
    if variant != "tinted":
        pts = [(0.16, 0.14, 2.2), (0.84, 0.12, 1.6), (0.9, 0.3, 2.4), (0.1, 0.42, 1.4),
               (0.72, 0.06, 1.2), (0.3, 0.07, 1.4), (0.94, 0.86, 1.6), (0.06, 0.82, 1.8)]
        stars = "".join('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#CFE6FF" opacity="0.55"/>'
                        % (x * w, y * h, s * min(w, h) / 1024) for x, y, s in pts)
    return '<rect width="%d" height="%d" fill="url(#gA_sky)"/>%s' % (w, h, stars)


CITIES = ((-74, 40.7), (-87.6, 41.9), (-79.4, 43.7), (-84.4, 33.7), (-95.4, 29.8), (-99.1, 19.4),
          (-80.2, 25.8), (-74.1, 4.7), (-77, -12), (-46.6, -23.5), (-43.2, -22.9), (-58.4, -34.6),
          (-66.9, 10.5), (-70.7, -33.4), (-73.6, 45.5), (-96.8, 32.8), (-105, 39.7), (-82.4, 23.1),
          (-34.9, -8), (-43.9, -19.9), (-47.9, -15.8), (-90.5, 14.6), (-79.5, 9), (-71, 42.4),
          (-75.2, 40), (-83, 42.3), (-81.7, 41.5), (-90.2, 38.6), (-38.5, -3.7), (-49.3, -25.4))


def _city_lights(variant):
    if variant == "tinted":
        return ""
    dots = []
    for lon, lat in CITIES:
        v = geo.vec(lon, lat)
        (x, y), vis = VIEW.project_vec(v)
        if vis and geo.dot(v, SUN) < -0.08:
            dots.append('<circle cx="%.1f" cy="%.1f" r="4.5"/>' % (x, y))
    return ('<g fill="#FFD27A" opacity="0.55" filter="url(#fA_glow2)">%s</g>'
            '<g fill="#FFE9B8" opacity="0.9">%s</g>') % ("".join(dots), "".join(dots).replace('r="4.5"', 'r="2.2"'))


def _globe(variant):
    p = _pal(variant)
    disc = geo.circle_d(CX, CY, R)
    land = geo.ortho_land_d(VIEW, LAND)
    night = geo.night_d(VIEW, SUN, disc)
    return "".join([
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gA_atmo)" filter="url(#fA_atmo)"/>' % (CX, CY, R * 1.12),
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gA_ocean)"/>' % (CX, CY, R),
        '<g clip-path="url(#cA_disc)">',
        '<path d="%s" fill="%s" fill-opacity="0.92"/>' % (land, p["land"]),
        '<path d="%s" fill="%s" fill-opacity="0.9" fill-rule="evenodd" filter="url(#fA_term)"/>' % (night, p["night"]),
        '<path d="%s" fill="none" stroke="#FFA85C" stroke-width="20" stroke-opacity="0.45" filter="url(#fA_glow)"/>'
        % geo.visible_terminator_d(VIEW, SUN),
        _city_lights(variant),
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gA_shade)"/>' % (CX, CY, R),
        '<ellipse cx="%d" cy="%d" rx="%d" ry="%d" fill="url(#gA_spec)" opacity="0.5"/>'
        % (CX + 0.32 * R, CY - 0.42 * R, 0.42 * R, 0.30 * R),
        '</g>',
        '<circle cx="%d" cy="%d" r="%.1f" fill="none" stroke="#BFE8FF" stroke-opacity="0.35" stroke-width="3"/>'
        % (CX, CY, R - 1.5),
    ])


def _arc():
    pts = []
    path = geo.great_circle(geo.vec(*DE), geo.vec(*DX), 80)
    n = len(path) - 1
    for i, v in enumerate(path):
        lift = 1.0 + 0.2 * math.sin(math.pi * i / n)
        (x, y), _ = VIEW.project_vec(v, lift)
        pts.append((x, y))
    d = geo.path_d(pts, closed=False, min_step=0.2)
    (dex, dey), _ = VIEW.project(*DE)
    (dxx, dxy), _ = VIEW.project(*DX)
    return "".join([
        '<path d="%s" fill="none" stroke="#FFB14A" stroke-width="48" stroke-linecap="round" opacity="0.6" filter="url(#fA_glow)"/>' % d,
        '<path d="%s" fill="none" stroke="url(#gA_arc)" stroke-width="22" stroke-linecap="round"/>' % d,
        '<path d="%s" fill="none" stroke="#FFF6DD" stroke-width="7" stroke-linecap="round" opacity="0.9"/>' % d,
        '<circle cx="%.1f" cy="%.1f" r="30" fill="#FFE6A8" opacity="0.6" filter="url(#fA_glow2)"/>' % (dex, dey),
        '<circle cx="%.1f" cy="%.1f" r="25" fill="#FFFFFF"/>' % (dex, dey),
        '<circle cx="%.1f" cy="%.1f" r="11" fill="#FFB14A"/>' % (dex, dey),
        '<circle cx="%.1f" cy="%.1f" r="32" fill="#FF9F3A" opacity="0.6" filter="url(#fA_glow2)"/>' % (dxx, dxy),
        '<circle cx="%.1f" cy="%.1f" r="25" fill="#FF9F3A"/>' % (dxx, dxy),
        '<circle cx="%.1f" cy="%.1f" r="10" fill="#FFF3D6"/>' % (dxx, dxy),
    ])


def layers(variant):
    return [("globe", _globe(variant)), ("path", _arc())]


CONCEPT = Concept(
    slug="greyline",
    title="A - Greyline",
    rationale="The globe exactly as HamClock draws it: the day/night grey line plus a glowing "
              "great-circle DE to DX path. Says 'world + radio propagation' at a glance.",
    defs=defs, background=background, layers=layers,
)

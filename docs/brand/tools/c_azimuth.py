"""Concept B "Azimuth": a precision 24-hour dial whose face is the ham's
azimuthal-equidistant great-circle map; the single hand is both the clock hand
and the beam heading to DX (great circles from the centre are straight lines)."""
import math

import geo
from svg import Concept, blur, lin, rad

CX, CY = 512, 512
R_BEZEL, R_FACE = 420, 336
QTH = (-96, 39)
SUN = geo.subsolar(25, 18)
HEADING = 38  # degrees clockwise from up; roughly the US -> Europe short path
LAND = geo.load_land_rings()
MAP = geo.AzEq(QTH[0], QTH[1], CX, CY, R_FACE)

PALETTE = {
    "light": dict(bg0="#2A2F38", bg1="#121419", bezel0="#3A404B", bezel1="#16191F", face0="#1C7390",
                  face1="#0B3447", land="#7FD9C0", night="#020A10", tick="#E9EEF5", major="#FFB547",
                  hand0="#FFD27A", hand1="#FF8A2B"),
    "dark": dict(bg0="#181B21", bg1="#050607", bezel0="#2C313A", bezel1="#0E1014", face0="#166380",
                 face1="#082A3A", land="#6CC8AE", night="#010508", tick="#DDE3EA", major="#FFB547",
                 hand0="#FFD27A", hand1="#FF8A2B"),
}


def _pal(variant):
    return PALETTE.get(variant, PALETTE["light"])


def defs(variant):
    p = _pal(variant)
    return "".join([
        rad("gB_bg", [(0, p["bg0"], 1), (1, p["bg1"], 1)], cy=0.3, r=0.85),
        lin("gB_bezel", [(0, p["bezel0"], 1), (1, p["bezel1"], 1)], 0, 0, 0, 1),
        lin("gB_rim", [(0, "#FFFFFF", 0.45), (0.5, "#FFFFFF", 0.05), (1, "#FFFFFF", 0.2)], 0, 0, 0, 1),
        rad("gB_face", [(0, p["face0"], 1), (1, p["face1"], 1)], cy=0.42, r=0.62),
        rad("gB_vignette", [(0, "#000000", 0), (0.78, "#000000", 0), (1, "#000000", 0.55)]),
        lin("gB_hand", [(0, p["hand1"], 1), (1, p["hand0"], 1)], 0, 1, 0, 0),
        lin("gB_glass", [(0, "#FFFFFF", 0.16), (0.45, "#FFFFFF", 0.0)], 0, 0, 0.4, 1),
        blur("fB_soft", 10), blur("fB_glow", 12), blur("fB_shadow", 18),
        '<clipPath id="cB_face"><circle cx="%d" cy="%d" r="%d"/></clipPath>' % (CX, CY, R_FACE),
    ])


def background(w, h, variant):
    return '<rect width="%d" height="%d" fill="url(#gB_bg)"/>' % (w, h)


def _ticks(p):
    out = []
    for i in range(24):
        a = math.radians(i * 15)
        major = i % 6 == 0
        r0, r1 = (R_FACE + 18, R_BEZEL - 16) if major else (R_FACE + 26, R_BEZEL - 30)
        sw = 20 if major else 9
        col = p["major"] if major else p["tick"]
        x0, y0 = CX + r0 * math.sin(a), CY - r0 * math.cos(a)
        x1, y1 = CX + r1 * math.sin(a), CY - r1 * math.cos(a)
        out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%d" '
                   'stroke-linecap="round"/>' % (x0, y0, x1, y1, col, sw))
    return "".join(out)


def _dial(variant):
    p = _pal(variant)
    disc = geo.circle_d(CX, CY, R_FACE)
    land = geo.azeq_land_d(MAP, LAND)
    night = geo.night_d(MAP, SUN, disc)
    rings = "".join('<circle cx="%d" cy="%d" r="%.1f" fill="none" stroke="#9FE3FF" stroke-opacity="0.13" '
                    'stroke-width="2"/>' % (CX, CY, R_FACE * k) for k in (1 / 3, 2 / 3))
    return "".join([
        '<circle cx="%d" cy="%d" r="%d" fill="#000" opacity="0.55" filter="url(#fB_shadow)"/>'
        % (CX, CY + 14, R_BEZEL),
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gB_bezel)"/>' % (CX, CY, R_BEZEL),
        '<circle cx="%d" cy="%d" r="%.1f" fill="none" stroke="url(#gB_rim)" stroke-width="4"/>'
        % (CX, CY, R_BEZEL - 2),
        _ticks(p),
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gB_face)"/>' % (CX, CY, R_FACE),
        '<g clip-path="url(#cB_face)">',
        '<path d="%s" fill="%s" fill-opacity="0.85"/>' % (land, p["land"]),
        '<path d="%s" fill="%s" fill-opacity="0.8" fill-rule="evenodd" filter="url(#fB_soft)"/>'
        % (night, p["night"]),
        rings,
        '<circle cx="%d" cy="%d" r="%d" fill="url(#gB_vignette)"/>' % (CX, CY, R_FACE),
        '</g>',
        '<circle cx="%d" cy="%d" r="%.1f" fill="none" stroke="#000" stroke-opacity="0.6" stroke-width="5"/>'
        % (CX, CY, R_FACE + 1),
    ])


def _hand(variant):
    a = math.radians(HEADING)
    tip = R_FACE - 34
    tx, ty = CX + tip * math.sin(a), CY - tip * math.cos(a)
    bx, by = CX - 60 * math.sin(a), CY + 60 * math.cos(a)
    return "".join([
        '<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#FF9A3A" stroke-width="40" opacity="0.5" '
        'stroke-linecap="round" filter="url(#fB_glow)"/>' % (CX, CY, tx, ty),
        '<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#FFC46B" stroke-width="22" '
        'stroke-linecap="round"/>' % (bx, by, tx, ty),
        '<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#FFF4DC" stroke-width="7" '
        'stroke-linecap="round"/>' % (CX, CY, tx, ty),
        '<circle cx="%.1f" cy="%.1f" r="44" fill="#FF9A3A" opacity="0.7" filter="url(#fB_glow)"/>' % (tx, ty),
        '<circle cx="%.1f" cy="%.1f" r="26" fill="#FF8A2B"/>' % (tx, ty),
        '<circle cx="%.1f" cy="%.1f" r="11" fill="#FFF4DC"/>' % (tx, ty),
        '<circle cx="%d" cy="%d" r="34" fill="#F4F6F9"/>' % (CX, CY),
        '<circle cx="%d" cy="%d" r="13" fill="#FF8A2B"/>' % (CX, CY),
    ])


def _glass(variant):
    return '<ellipse cx="%d" cy="%d" rx="%d" ry="%d" fill="url(#gB_glass)"/>' % (
        CX, CY - 150, R_FACE - 30, 200)


def layers(variant):
    return [("dial", _dial(variant)), ("hand", _hand(variant) + _glass(variant))]


CONCEPT = Concept(
    slug="azimuth",
    title="B - Azimuth",
    rationale="A precision 24-hour dial whose face is the ham's azimuthal great-circle map; the one "
              "hand is both the clock hand and the beam heading to DX. Clock + radio in one object.",
    defs=defs, background=background, layers=layers,
)

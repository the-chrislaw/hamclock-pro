"""Small SVG-writing helpers shared by the concept builders.

Every concept draws in a 1024x1024 art frame. A concept supplies a background
(drawn to any canvas size, so tvOS can use 1280x768) and one or more foreground
layers (art frame), which become the Liquid Glass / tvOS parallax layers."""
from dataclasses import dataclass
from typing import Callable

ART = 1024


@dataclass(frozen=True)
class Concept:
    slug: str
    title: str
    rationale: str
    defs: Callable[[str], str]
    background: Callable[[int, int, str], str]
    layers: Callable[[str], list]  # -> [(layer_name, svg_fragment)], back to front
    tv_scale: float = 0.92


def lin(gid, stops, x1=0, y1=0, x2=0, y2=1, units="objectBoundingBox"):
    s = "".join('<stop offset="%s" stop-color="%s" stop-opacity="%s"/>' % (o, c, a) for o, c, a in stops)
    return ('<linearGradient id="%s" x1="%s" y1="%s" x2="%s" y2="%s" gradientUnits="%s">%s</linearGradient>'
            % (gid, x1, y1, x2, y2, units, s))


def rad(gid, stops, cx=0.5, cy=0.5, r=0.5, fx=None, fy=None, units="objectBoundingBox"):
    s = "".join('<stop offset="%s" stop-color="%s" stop-opacity="%s"/>' % (o, c, a) for o, c, a in stops)
    f = ' fx="%s" fy="%s"' % (fx, fy) if fx is not None else ""
    return ('<radialGradient id="%s" cx="%s" cy="%s" r="%s"%s gradientUnits="%s">%s</radialGradient>'
            % (gid, cx, cy, r, f, units, s))


def blur(fid, sd):
    return ('<filter id="%s" x="-50%%" y="-50%%" width="200%%" height="200%%">'
            '<feGaussianBlur stdDeviation="%s"/></filter>' % (fid, sd))


def document(w, h, defs, body):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">'
            '<defs>%s</defs>%s</svg>' % (w, h, w, h, defs, body))


def place(fragment, w, h, k=1.0):
    """Centre the 1024 art frame on a w x h canvas, scaled by k relative to min(w, h)."""
    s = k * min(w, h) / ART
    tx, ty = (w - ART * s) / 2, (h - ART * s) / 2
    return '<g transform="translate(%.2f %.2f) scale(%.5f)">%s</g>' % (tx, ty, s, fragment)


def compose(concept, variant, w=ART, h=ART, which=None, k=1.0, with_background=True):
    """Full SVG document. which = None (all layers) or a set of layer names."""
    body = concept.background(w, h, variant) if with_background else ""
    for name, frag in concept.layers(variant):
        if which is None or name in which:
            body += place(frag, w, h, k)
    return document(w, h, concept.defs(variant), body)

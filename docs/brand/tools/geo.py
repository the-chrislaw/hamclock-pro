"""Geometry helpers for the brand art: Natural Earth land rings (read from the
public-domain src/map/land.hpp already in the repo), orthographic and azimuthal
equidistant projections, terminator and great-circle paths, as SVG path data."""
import math
import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
LAND_HPP = os.path.join(ROOT, "src", "map", "land.hpp")


def load_land_rings():
    """Return the land rings as lists of (lon, lat) tuples."""
    with open(LAND_HPP, encoding="utf-8") as handle:
        text = handle.read()
    pts_block = text.split("land_points[] = {", 1)[1].split("};", 1)[0]
    ring_block = text.split("land_rings[] = {", 1)[1].split("};", 1)[0]
    pts = [(float(a), float(b)) for a, b in re.findall(r"\{(-?[\d.e+-]+), (-?[\d.e+-]+)\}", pts_block)]
    rings = [(int(a), int(b)) for a, b in re.findall(r"\{(\d+)u, (\d+)u\}", ring_block)]
    if not pts or not rings:
        raise RuntimeError("could not parse " + LAND_HPP)
    return [pts[s:s + n] for s, n in rings]


def vec(lon, lat):
    lo, la = math.radians(lon), math.radians(lat)
    return (math.cos(la) * math.cos(lo), math.cos(la) * math.sin(lo), math.sin(la))


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    m = math.sqrt(dot(a, a)) or 1.0
    return (a[0] / m, a[1] / m, a[2] / m)


def scale(a, k):
    return (a[0] * k, a[1] * k, a[2] * k)


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


class Ortho:
    """Orthographic view of the globe looking at (lon0, lat0), optionally rolled."""

    def __init__(self, lon0, lat0, cx, cy, r, roll=0.0):
        self.v = vec(lon0, lat0)
        east = norm(cross((0, 0, 1), self.v))
        north = cross(self.v, east)
        c, s = math.cos(math.radians(roll)), math.sin(math.radians(roll))
        self.e = add(scale(east, c), scale(north, s))
        self.n = add(scale(north, c), scale(east, -s))
        self.cx, self.cy, self.r = cx, cy, r

    def project_vec(self, p, lift=1.0):
        """Screen point and visibility; hidden points are pushed onto the limb."""
        x, y, z = dot(p, self.e), dot(p, self.n), dot(p, self.v)
        if z < 0 and lift <= 1.0:
            m = math.hypot(x, y) or 1.0
            x, y = x / m, y / m
        return (self.cx + self.r * lift * x, self.cy - self.r * lift * y), z >= 0

    def project(self, lon, lat, lift=1.0):
        return self.project_vec(vec(lon, lat), lift)


class AzEq:
    """Azimuthal equidistant map centred on (lon0, lat0); the antipode sits on radius r."""

    def __init__(self, lon0, lat0, cx, cy, r):
        self.view = Ortho(lon0, lat0, 0, 0, 1)
        self.cx, self.cy, self.r = cx, cy, r

    def project_vec(self, p):
        x, y, z = dot(p, self.view.e), dot(p, self.view.n), dot(p, self.view.v)
        c = math.acos(max(-1.0, min(1.0, z)))
        m = math.hypot(x, y)
        if m < 1e-12:
            return (self.cx, self.cy)
        k = self.r * c / math.pi / m
        return (self.cx + k * x, self.cy - k * y)

    def project(self, lon, lat):
        return self.project_vec(vec(lon, lat))


def densify(ring, max_step=2.0):
    """Insert points so no edge spans more than max_step degrees (avoids chords)."""
    out = []
    for (a, b), (c, d) in zip(ring, ring[1:] + ring[:1]):
        out.append((a, b))
        if abs(c - a) > 180:
            continue
        n = int(max(abs(c - a), abs(d - b)) / max_step)
        for i in range(1, n + 1):
            t = i / (n + 1)
            out.append((a + (c - a) * t, b + (d - b) * t))
    return out


def path_d(points, closed=True, min_step=0.8):
    """SVG path data with near-duplicate points dropped."""
    kept = []
    for x, y in points:
        if kept and abs(x - kept[-1][0]) + abs(y - kept[-1][1]) < min_step:
            continue
        kept.append((x, y))
    if len(kept) < 2:
        return ""
    body = " L".join("%.1f %.1f" % p for p in kept)
    return "M" + body + (" Z" if closed else "")


def ortho_land_d(view, rings, min_step=0.8):
    parts = []
    for ring in rings:
        pts = [view.project(lon, lat) for lon, lat in densify(ring)]
        if not any(vis for _, vis in pts):
            continue
        parts.append(path_d([p for p, _ in pts], True, min_step))
    return " ".join(p for p in parts if p)


def azeq_land_d(view, rings, min_step=0.8):
    parts = []
    for ring in rings:
        pts = [view.project(lon, lat) for lon, lat in densify(ring, 1.0)]
        parts.append(path_d(pts, True, min_step))
    return " ".join(p for p in parts if p)


def subsolar(lon, lat):
    return vec(lon, lat)


def terminator_points(sun, samples=720):
    a = norm(cross(sun, (0, 0, 1)) if abs(sun[2]) < 0.99 else cross(sun, (1, 0, 0)))
    b = cross(sun, a)
    return [add(scale(a, math.cos(t)), scale(b, math.sin(t)))
            for t in (2 * math.pi * i / samples for i in range(samples))]


def point_in_poly(pt, poly):
    x, y = pt
    inside = False
    for (x1, y1), (x2, y2) in zip(poly, poly[1:] + poly[:1]):
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1 + 1e-12) + x1:
            inside = not inside
    return inside


def night_d(view, sun, disc_d):
    """Night area as path data (evenodd). disc_d is the outline of the whole map disc."""
    curve = [view.project_vec(p) for p in terminator_points(sun)]
    curve = [c[0] if isinstance(c[0], tuple) else c for c in curve]
    probe_vec = scale(sun, -1)
    probe = view.project_vec(probe_vec)
    probe = probe[0] if isinstance(probe[0], tuple) else probe
    d = path_d(curve, True, 0.3)
    if isinstance(view, Ortho) and dot(probe_vec, view.v) < 0:
        # anti-solar point hidden: probe a visible night point on the limb instead
        side = norm(add(scale(sun, -1), scale(view.v, dot(sun, view.v))))
        probe = view.project_vec(add(scale(side, 0.98), scale(view.v, 0.2)))[0]
    return d if point_in_poly(probe, curve) else disc_d + " " + d


def great_circle(a, b, steps=96):
    """Points along the minor great-circle arc from vector a to vector b."""
    omega = math.acos(max(-1.0, min(1.0, dot(a, b))))
    out = []
    for i in range(steps + 1):
        t = i / steps
        if omega < 1e-9:
            out.append(a)
            continue
        k1 = math.sin((1 - t) * omega) / math.sin(omega)
        k2 = math.sin(t * omega) / math.sin(omega)
        out.append(add(scale(a, k1), scale(b, k2)))
    return out


def visible_terminator_d(view, sun):
    """Open path along the part of the terminator that faces the viewer."""
    pts = terminator_points(sun)
    vis = [view.project_vec(p) for p in pts]
    start = next((i for i, (_, v) in enumerate(vis) if not v), 0)
    ordered = vis[start:] + vis[:start]
    runs, run = [], []
    for xy, v in ordered:
        if v:
            run.append(xy)
        elif run:
            runs.append(run)
            run = []
    if run:
        runs.append(run)
    return " ".join(path_d(r, False, 0.3) for r in runs if len(r) > 1)


def circle_d(cx, cy, r):
    return "M%.1f %.1f a%.1f %.1f 0 1 0 %.1f 0 a%.1f %.1f 0 1 0 %.1f 0 Z" % (
        cx - r, cy, r, r, 2 * r, r, r, -2 * r)

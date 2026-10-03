#!/usr/bin/env python3
"""Original bounded SHP Polygon importer; synthetic-stage, no bundled data.

Read only ne_110m_land.shp from an offline ZIP; never extract members.
decode_shp returns (points, rings, records). Points are (longitude, latitude)
double values, rings are (first_point, count), records are (first_ring, count).
Closed rings and original winding/order are preserved: SHP exteriors are
clockwise, holes counterclockwise; ring order does not identify holes. Multiple
exteriors are allowed. This importer does NOT classify/repair topology, clip
seams, or convert winding to GeoJSON. +/-180 retain distinct boundary identity.
Closure/area validation alone does not certify absence of self-intersections.
Only out-of-range endpoint excursions of at most 1e-12 degrees are snapped
to the same signed geographic endpoint; every in-range coordinate is retained.
Output publication assumes a trusted stable parent directory, last writer wins;
it does not provide directory-fsync crash durability or adversarial path locking.
"""
import argparse
import hashlib
import io
import math
import os
from pathlib import Path
import re
import stat
import struct
import tempfile
import zipfile

MAX_ARCHIVE = MAX_SHP = 1024 * 1024
MAX_POINTS = 100000
MAX_RINGS = MAX_RECORDS = 10000
MAX_RING_POINTS = 20000
MAX_MEMBERS = 256
MAX_OUTPUT = 12 * 1024 * 1024
MEMBER = "ne_110m_land.shp"
ENDPOINT_OVERSHOOT = 1e-12  # Degrees; does not relax declared-box containment.


def _geographic(value, limit):
    if not math.isfinite(value):
        raise ValueError("invalid geographic coordinate")
    if -limit <= value <= limit:
        return value
    if abs(value) - limit <= ENDPOINT_OVERSHOOT:
        return math.copysign(limit, value)
    raise ValueError("invalid geographic coordinate")


def _box(data, offset):
    west, south, east, north = struct.unpack_from("<4d", data, offset)
    # Reject reversed source bounds before normalization can collapse them.
    if not (west <= east and south <= north):
        raise ValueError("invalid geographic bounding box")
    try:
        west, east = (_geographic(v, 180.) for v in (west, east))
        south, north = (_geographic(v, 90.) for v in (south, north))
    except ValueError as error:
        raise ValueError("invalid geographic bounding box") from error
    return west, south, east, north


def decode_shp(data):
    if not 100 <= len(data) <= MAX_SHP:
        raise ValueError("shapefile size outside bounds")
    if struct.unpack_from(">I", data)[0] != 9994:
        raise ValueError("invalid shapefile magic")
    if struct.unpack_from(">I", data, 24)[0] * 2 != len(data):
        raise ValueError("invalid declared file length")
    if struct.unpack_from("<II", data, 28) != (1000, 5):
        raise ValueError("expected version 1000 Polygon shapefile")
    overall = _box(data, 36)
    points, rings, records = [], [], []
    pos = 100
    while pos < len(data):
        if len(records) >= MAX_RECORDS or pos + 8 > len(data):
            raise ValueError("record cap or truncated header")
        sequence, words = struct.unpack_from(">II", data, pos)
        pos += 8
        size = words * 2
        if sequence != len(records) + 1 or size < 44 or pos + size > len(data):
            raise ValueError("invalid record sequence or size")
        if struct.unpack_from("<I", data, pos)[0] != 5:
            raise ValueError("only non-null Polygon records supported")
        box = _box(data, pos + 4)
        count_rings, count_points = struct.unpack_from("<II", data, pos + 36)
        if not (1 <= count_rings <= MAX_RINGS and 4 <= count_points <= MAX_POINTS):
            raise ValueError("record counts outside bounds")
        if len(rings) + count_rings > MAX_RINGS or len(points) + count_points > MAX_POINTS:
            raise ValueError("aggregate counts outside bounds")
        if size != 44 + 4 * count_rings + 16 * count_points:
            raise ValueError("unexpected record payload length")
        starts = struct.unpack_from("<" + "I" * count_rings, data, pos + 44)
        if starts[0] != 0 or any(a >= b for a, b in zip(starts, starts[1:])):
            raise ValueError("ring offsets must start at zero and increase")
        ends = starts[1:] + (count_points,)
        if any(b > count_points or not 4 <= b-a <= MAX_RING_POINTS for a, b in zip(starts, ends)):
            raise ValueError("ring bounds outside limits")
        record_points = []
        point_offset = pos + 44 + 4 * count_rings
        for index in range(count_points):
            lon, lat = struct.unpack_from("<dd", data, point_offset + 16 * index)
            lon, lat = _geographic(lon, 180.), _geographic(lat, 90.)
            if not (box[0] <= lon <= box[2] and box[1] <= lat <= box[3]
                    and overall[0] <= lon <= overall[2] and overall[1] <= lat <= overall[3]):
                raise ValueError("coordinate outside declared bounds")
            record_points.append((lon, lat))
        for start, end in zip(starts, ends):
            ring = record_points[start:end]
            if ring[0] != ring[-1] or len(set(ring)) < 3:
                raise ValueError("ring must be closed with three distinct vertices")
            twice_area = math.fsum(a[0]*b[1]-b[0]*a[1] for a, b in zip(ring, ring[1:]))
            if twice_area == 0:
                raise ValueError("zero-area ring")
        base = len(points)
        records.append((len(rings), count_rings))
        rings.extend((base + a, b-a) for a, b in zip(starts, ends))
        points.extend(record_points)
        pos += size
    if not records:
        raise ValueError("empty land geometry")
    return points, rings, records


def read_zip(path):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        metadata = os.fstat(fd)
        if not stat.S_ISREG(metadata.st_mode) or not 0 < metadata.st_size <= MAX_ARCHIVE:
            raise ValueError("input must be a bounded regular file")
        stream = os.fdopen(fd, "rb")
        fd = -1
        with stream:
            raw = stream.read(MAX_ARCHIVE + 1)
    finally:
        if fd >= 0:
            os.close(fd)
    if not 0 < len(raw) <= MAX_ARCHIVE:
        raise ValueError("archive size outside bounds")
    with zipfile.ZipFile(io.BytesIO(raw)) as archive:
        members = archive.infolist()
        if len(members) > MAX_MEMBERS:
            raise ValueError("archive member cap exceeded")
        matches = [member for member in members if member.filename == MEMBER]
        if len(matches) != 1:
            raise ValueError("expected exactly one root land shapefile")
        member = matches[0]
        if member.flag_bits & 1 or not 100 <= member.file_size <= MAX_SHP:
            raise ValueError("encrypted or oversized shapefile")
        with archive.open(member) as stream:
            data = stream.read(MAX_SHP + 1)
        if len(data) != member.file_size or len(data) > MAX_SHP:
            raise ValueError("uncompressed length mismatch")
    return decode_shp(data), hashlib.sha256(raw).hexdigest()


def emit(geometry, digest):
    if not re.fullmatch(r"[0-9a-f]{64}", digest):
        raise ValueError("invalid source digest")
    points, rings, records = geometry
    lines = ["#pragma once", "#include <cstdint>",
             "// Generated by original tools/import_land.py; source SHA-256: " + digest,
             "// Source rings/winding preserved. No seam clipping or topology repair.",
             "namespace hamclock {",
             "struct LandPoint { double longitude; double latitude; };",
             "struct LandRing { std::uint32_t start; std::uint32_t count; };",
             "struct LandRecord { std::uint32_t first_ring; std::uint32_t count; };",
             "inline constexpr LandPoint land_points[] = {"]
    def literal(value):
        text = format(value, ".17g")
        return text if "." in text or "e" in text else text + ".0"
    lines.extend("    {%s, %s}," % (literal(lon), literal(lat)) for lon, lat in points)
    lines += ["};", "inline constexpr LandRing land_rings[] = {"]
    lines.extend("    {%du, %du}," % ring for ring in rings)
    lines += ["};", "inline constexpr LandRecord land_records[] = {"]
    lines.extend("    {%du, %du}," % record for record in records)
    lines += ["};", "} // namespace hamclock", ""]
    output = "\n".join(lines)
    if len(output.encode("utf-8")) > MAX_OUTPUT:
        raise ValueError("generated output cap exceeded")
    return output


def publish(path, text):
    path = Path(path)
    try:
        existing = path.lstat()
    except FileNotFoundError:
        existing = None
    if existing is not None and not stat.S_ISREG(existing.st_mode):
        raise ValueError("output must be a regular file")
    fd, name = tempfile.mkstemp(prefix=path.name + ".tmp.", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(text)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
    finally:
        try:
            os.unlink(name)
        except FileNotFoundError:
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        geometry, digest = read_zip(args.archive)
        publish(args.output, emit(geometry, digest))
        print("SHA-256:", digest)
        print("Points:", len(geometry[0]), "Rings:", len(geometry[1]), "Records:", len(geometry[2]))
    except (ValueError, OSError, zipfile.BadZipFile, RuntimeError, struct.error) as error:
        parser.exit(2, "Land import rejected: " + str(error) + "\n")


if __name__ == "__main__":
    main()

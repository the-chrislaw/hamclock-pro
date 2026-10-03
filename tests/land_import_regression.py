#!/usr/bin/env python3
"""Offline synthetic tests; no real dataset or third-party package needed."""
import hashlib
import importlib.util
import io
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

TOOL = Path(__file__).resolve().parents[1] / "tools/import_land.py"
spec = importlib.util.spec_from_file_location("land_import", TOOL)
land = importlib.util.module_from_spec(spec)
spec.loader.exec_module(land)
OUTER = [(0., 0.), (0., 4.), (4., 4.), (4., 0.), (0., 0.)]
HOLE = [(1., 1.), (3., 1.), (3., 3.), (1., 3.), (1., 1.)]


def shape(records):
    payloads = []
    all_points = []
    for rings in records:
        points = [point for ring in rings for point in ring]
        all_points.extend(points)
        box = (min(p[0] for p in points), min(p[1] for p in points),
               max(p[0] for p in points), max(p[1] for p in points))
        starts, count = [], 0
        for ring in rings:
            starts.append(count)
            count += len(ring)
        payload = struct.pack("<I4dII", 5, *box, len(rings), len(points))
        payload += struct.pack("<" + "I" * len(starts), *starts)
        payload += b"".join(struct.pack("<2d", *point) for point in points)
        payloads.append(payload)
    data = bytearray(100)
    struct.pack_into(">I", data, 0, 9994)
    struct.pack_into("<II", data, 28, 1000, 5)
    box = (min(p[0] for p in all_points), min(p[1] for p in all_points),
           max(p[0] for p in all_points), max(p[1] for p in all_points)) if all_points else (0., 0., 0., 0.)
    struct.pack_into("<4d", data, 36, *box)
    for index, payload in enumerate(payloads, 1):
        data += struct.pack(">II", index, len(payload)//2) + payload
    struct.pack_into(">I", data, 24, len(data)//2)
    return data


def archive(data, entries=None):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w", compression=zipfile.ZIP_DEFLATED) as file:
        for name, contents in (entries if entries is not None else [(land.MEMBER, data)]):
            file.writestr(name, contents)
    return stream.getvalue()


class LandRegression(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)

    def reject(self, data):
        with self.assertRaises(ValueError):
            land.decode_shp(data)

    def test_hole_and_record_contract(self):
        points, rings, records = land.decode_shp(shape([[OUTER, HOLE], [OUTER]]))
        self.assertEqual(points, OUTER + HOLE + OUTER)
        self.assertEqual(rings, [(0, 5), (5, 5), (10, 5)])
        self.assertEqual(records, [(0, 2), (2, 1)])
        # Deliberately hole first: ring order does not classify a SHP hole.
        self.assertEqual(land.decode_shp(shape([[HOLE, OUTER]]))[0], HOLE + OUTER)

    def test_seam_and_polar_identity(self):
        polar = [(-180., -90.), (-180., -80.), (180., -80.), (180., -90.), (-180., -90.)]
        self.assertEqual(land.decode_shp(shape([[polar]]))[0], polar)
        crossing = [(170., 40.), (170., 50.), (-170., 50.), (-170., 40.), (170., 40.)]
        self.assertEqual(land.decode_shp(shape([[crossing]]))[0], crossing)

    def test_preserves_precision_and_winding(self):
        ring = [(0., 0.), (0., 1.123456789012345), (2., 1.), (0., 0.)]
        self.assertEqual(land.decode_shp(shape([[ring]]))[0], ring)
        self.assertEqual(land.decode_shp(shape([[list(reversed(ring))]]))[0], list(reversed(ring)))

    def test_endpoint_envelope_adjacent_floats(self):
        # 1e-12 is not exactly representable relative to 90/180. Exercise the
        # last accepted binary float and its immediate rejected successor.
        for limit in (90., 180.):
            accepted = limit + land.ENDPOINT_OVERSHOOT
            while accepted - limit > land.ENDPOINT_OVERSHOOT:
                accepted = math.nextafter(accepted, limit)
            rejected = math.nextafter(accepted, math.inf)
            self.assertLessEqual(accepted - limit, land.ENDPOINT_OVERSHOOT)
            self.assertGreater(rejected - limit, land.ENDPOINT_OVERSHOOT)
            for sign in (-1., 1.):
                self.assertEqual(land._geographic(sign * accepted, limit), sign * limit)
                with self.assertRaises(ValueError):
                    land._geographic(sign * rejected, limit)
                for value in (sign * limit, math.nextafter(sign * limit, 0.)):
                    self.assertEqual(land._geographic(value, limit), value)
            for value in (float('nan'), float('inf'), -float('inf')):
                with self.assertRaises(ValueError):
                    land._geographic(value, limit)
        self.assertEqual(math.copysign(1., land._geographic(-0., 180.)), -1.)

    def test_endpoint_normalization_points_boxes_closure_and_full_width(self):
        for latitude in (-90., 90.):
            outside_lat = math.nextafter(latitude, math.copysign(math.inf, latitude))
            inward_lat = latitude - math.copysign(10., latitude)
            outside_lon = math.nextafter(180., math.inf)
            ring = [(-outside_lon, outside_lat), (-outside_lon, inward_lat),
                    (outside_lon, inward_lat), (outside_lon, outside_lat),
                    (-outside_lon, outside_lat)]
            normalized = [(-180., latitude), (-180., inward_lat),
                          (180., inward_lat), (180., latitude), (-180., latitude)]
            self.assertEqual(land.decode_shp(shape([[ring]]))[0], normalized)
            self.assertEqual(normalized[3][0] - normalized[4][0], 360.)

    def test_endpoint_tolerance_does_not_expand_interior_boxes(self):
        data = shape([[OUTER]])
        struct.pack_into('<d', data, 36 + 16, math.nextafter(4., 0.))
        self.reject(data)
        data = shape([[OUTER]])
        struct.pack_into('<d', data, 112 + 16, math.nextafter(4., 0.))
        self.reject(data)

    def test_reversed_outside_boxes_not_repaired(self):
        for offset in (36, 112):
            for values in ((math.nextafter(180., math.inf), 0., 180., 4.),
                           (0., math.nextafter(90., math.inf), 4., 90.)):
                data = shape([[OUTER]])
                struct.pack_into('<4d', data, offset, *values)
                self.reject(data)

    def test_repeated_consecutive_points(self):
        ring = [OUTER[0]] + OUTER
        self.assertEqual(land.decode_shp(shape([[ring]]))[0], ring)

    def test_file_headers(self):
        self.reject(b"short")
        for offset, fmt, value in [(0, ">I", 7), (24, ">I", 50), (28, "<I", 999), (32, "<I", 3)]:
            with self.subTest(offset=offset):
                data = shape([[OUTER]])
                struct.pack_into(fmt, data, offset, value)
                self.reject(data)

    def test_empty_file(self):
        self.reject(shape([]))

    def test_truncated_record_header_and_payload(self):
        for data in [shape([]) + b"xx", shape([[OUTER]])[:-2]]:
            struct.pack_into(">I", data, 24, len(data)//2)
            self.reject(data)

    def test_record_number_and_length(self):
        for offset, value in [(100, 2), (104, 2), (104, 1000000)]:
            data = shape([[OUTER]])
            struct.pack_into(">I", data, offset, value)
            self.reject(data)

    def test_exact_payload_length(self):
        data = shape([[OUTER]]) + b"xx"
        struct.pack_into(">I", data, 24, len(data)//2)
        struct.pack_into(">I", data, 104, (len(data)-108)//2)
        self.reject(data)

    def test_null_and_mixed_record_type(self):
        for value in [0, 3, 15, 25]:
            data = shape([[OUTER]])
            struct.pack_into("<I", data, 108, value)
            self.reject(data)

    def test_counts(self):
        for offset, value in [(144, 0), (144, land.MAX_RINGS+1), (148, 3), (148, land.MAX_POINTS+1)]:
            data = shape([[OUTER]])
            struct.pack_into("<I", data, offset, value)
            self.reject(data)

    def test_invalid_ring_offsets(self):
        for first, second in [(1, 5), (0, 0), (0, 2), (0, 11)]:
            data = shape([[OUTER, HOLE]])
            struct.pack_into("<II", data, 152, first, second)
            self.reject(data)

    def test_open_zero_area_and_insufficient_distinct_ring(self):
        for ring in [OUTER[:-1], [(0., 0.), (1., 1.), (2., 2.), (0., 0.)], [(0., 0.)]*4]:
            self.reject(shape([[ring]]))

    def test_nonfinite_and_outside_coordinates(self):
        for bad in [float("nan"), float("inf"), 180.0000001, -180.0000001]:
            data = shape([[OUTER]])
            struct.pack_into("<d", data, 156, bad)
            self.reject(data)
        for bad in [90.0000001, -90.0000001]:
            data = shape([[OUTER]])
            struct.pack_into("<d", data, 164, bad)
            self.reject(data)

    def test_bad_header_and_record_boxes(self):
        for offset in [36, 112]:
            for values in [(float("nan"), 0., 4., 4.), (2., 0., 1., 4.), (0., 0., 181., 4.), (0., 0., 2., 4.)]:
                data = shape([[OUTER]])
                struct.pack_into("<4d", data, offset, *values)
                self.reject(data)

    def test_individual_and_aggregate_caps(self):
        data = shape([[OUTER], [OUTER]])
        for name, maximum in [("MAX_POINTS", 7), ("MAX_RINGS", 1), ("MAX_RECORDS", 1), ("MAX_RING_POINTS", 4), ("MAX_SHP", 100)]:
            with self.subTest(name=name), patch.object(land, name, maximum):
                self.reject(data)

    def test_zip_exact_root_and_hash(self):
        ignored_name = self.directory.name + "-land-unextracted"
        raw = archive(shape([[OUTER]]), [("../" + ignored_name, b"nothing"), (land.MEMBER, shape([[OUTER]]))])
        path = self.directory / "input.zip"
        path.write_bytes(raw)
        geometry, digest = land.read_zip(path)
        self.assertEqual(geometry[0], OUTER)
        self.assertEqual(digest, hashlib.sha256(raw).hexdigest())
        self.assertFalse((self.directory.parent / ignored_name).exists())

    def test_zip_missing_traversal_and_duplicate(self):
        data = shape([[OUTER]])
        for entries in [[], [("../"+land.MEMBER, data)], [(land.MEMBER, data), (land.MEMBER, data)]]:
            path = self.directory / "input.zip"
            path.write_bytes(archive(data, entries))
            with self.assertRaises(ValueError):
                land.read_zip(path)

    def test_zip_caps_and_corruption(self):
        path = self.directory / "input.zip"
        path.write_bytes(archive(shape([[OUTER]])))
        for name, cap in [("MAX_ARCHIVE", 10), ("MAX_SHP", 100), ("MAX_MEMBERS", 0)]:
            with patch.object(land, name, cap), self.assertRaises(ValueError):
                land.read_zip(path)
        path.write_bytes(b"badzip")
        with self.assertRaises(zipfile.BadZipFile):
            land.read_zip(path)

    def test_encrypted_zip_rejected(self):
        raw = bytearray(archive(shape([[OUTER]])))
        struct.pack_into("<H", raw, 6, 1)
        central = raw.index(b"PK\x01\x02")
        struct.pack_into("<H", raw, central+8, 1)
        path = self.directory / "input.zip"
        path.write_bytes(raw)
        with self.assertRaises(ValueError):
            land.read_zip(path)

    def test_one_bounded_archive_read(self):
        raw = archive(shape([[OUTER]]))
        path = self.directory / "once.zip"
        path.write_bytes(raw)
        real_fdopen = os.fdopen
        class Once:
            calls = 0
            def __init__(self, stream):
                self.stream = stream
            def __enter__(self):
                return self
            def __exit__(self, *args):
                self.stream.close()
            def read(self, size=-1):
                self.calls += 1
                self.assertions(size)
                return self.stream.read(size)
            def assertions(self, size):
                if self.calls != 1 or size != land.MAX_ARCHIVE+1:
                    raise AssertionError("input must be read exactly once with cap")
        with patch.object(land.os, "fdopen", side_effect=lambda fd, mode: Once(real_fdopen(fd, mode))):
            _, digest = land.read_zip(path)
        self.assertEqual(digest, hashlib.sha256(raw).hexdigest())

    def test_source_nonregular_and_symlink_rejected_without_blocking(self):
        target = self.directory / "valid.zip"
        target.write_bytes(archive(shape([[OUTER]])))
        link = self.directory / "link.zip"
        link.symlink_to(target)
        fifo = self.directory / "fifo.zip"
        os.mkfifo(fifo)
        for path in [link, fifo, self.directory]:
            with self.assertRaises((ValueError, OSError)):
                land.read_zip(path)

    def test_emit_contract_precision_and_caps(self):
        geometry = land.decode_shp(shape([[OUTER, HOLE]]))
        output = land.emit(geometry, "a"*64)
        self.assertIn("double longitude; double latitude;", output)
        self.assertIn("{0u, 2u}", output)
        self.assertIn("{5u, 5u}", output)
        self.assertEqual(output, land.emit(geometry, "a"*64))
        with self.assertRaises(ValueError):
            land.emit(geometry, "comment injection\n")
        with patch.object(land, "MAX_OUTPUT", 10), self.assertRaises(ValueError):
            land.emit(geometry, "a"*64)

    def test_publish_success_and_error_cleanup(self):
        path = self.directory / "land.hpp"
        path.write_text("old")
        land.publish(path, "new\n")
        self.assertEqual(path.read_text(), "new\n")
        self.assertEqual(path.stat().st_mode & 0o777, 0o600)
        with patch.object(land.os, "replace", side_effect=OSError("fixture")), self.assertRaises(OSError):
            land.publish(path, "bad\n")
        self.assertEqual(path.read_text(), "new\n")
        self.assertEqual(list(self.directory.glob("*.tmp.*")), [])

    def test_flush_failure_preserves_output(self):
        path = self.directory / "land.hpp"
        path.write_text("original")
        with patch.object(land.os, "fsync", side_effect=OSError("fixture")), self.assertRaises(OSError):
            land.publish(path, "not published")
        self.assertEqual(path.read_text(), "original")
        self.assertEqual(list(self.directory.glob("*.tmp.*")), [])

    def test_publish_symlink_fifo_directory_rejected(self):
        target = self.directory / "target"
        target.write_text("preserve")
        link = self.directory / "link"
        link.symlink_to(target)
        fifo = self.directory / "fifo"
        os.mkfifo(fifo)
        for path in [link, fifo, self.directory]:
            with self.assertRaises(ValueError):
                land.publish(path, "bad")
        self.assertEqual(target.read_text(), "preserve")

    def test_actual_cli_valid_and_rejection_preserve_output(self):
        source, destination = self.directory / "input.zip", self.directory / "land.hpp"
        source.write_bytes(archive(shape([[OUTER, HOLE]])))
        result = subprocess.run([sys.executable, str(TOOL), str(source), str(destination)], capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 0, result.stderr)
        original = destination.read_bytes()
        source.write_bytes(archive(shape([[OUTER[:-1]]])))
        result = subprocess.run([sys.executable, str(TOOL), str(source), str(destination)], capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 2)
        self.assertEqual(destination.read_bytes(), original)
        self.assertEqual(list(self.directory.glob("*.tmp.*")), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)

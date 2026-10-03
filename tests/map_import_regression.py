#!/usr/bin/env python3
"""Offline importer security regressions using independent synthetic fixtures."""
import sys
sys.dont_write_bytecode = True
import hashlib
import importlib.util
import io
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
from unittest import mock
import warnings
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('coastline_import', ROOT / 'tools/import_coastline.py')
IMPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORTER)


def polyline(points, starts=(0,), shape_type=3):
    """Independent SHP record fixture: bounding box, counts, offsets, XY doubles."""
    return (struct.pack('<I4dII', shape_type, -180, -90, 180, 90, len(starts), len(points))
            + b''.join(struct.pack('<I', value) for value in starts)
            + b''.join(struct.pack('<dd', *point) for point in points))


def shapefile(*records):
    data = bytearray(100)
    struct.pack_into('>I', data, 0, 9994)
    struct.pack_into('<II', data, 28, 1000, 3)
    struct.pack_into('<4d', data, 36, -180, -90, 180, 90)
    for index, record in enumerate(records, 1):
        data += struct.pack('>II', index, len(record) // 2) + record
    struct.pack_into('>I', data, 24, len(data) // 2)
    return bytes(data)


def changed(data, offset, fmt, value, fix_length=False):
    result = bytearray(data)
    struct.pack_into(fmt, result, offset, value)
    if fix_length:
        struct.pack_into('>I', result, 24, len(result) // 2)
    return bytes(result)


def zip_bytes(entries, compression=zipfile.ZIP_STORED):
    stream = io.BytesIO()
    with warnings.catch_warnings():
        warnings.simplefilter('ignore', UserWarning)
        with zipfile.ZipFile(stream, 'w', compression=compression) as archive:
            for name, payload in entries:
                info = zipfile.ZipInfo(name, date_time=(2020, 1, 1, 0, 0, 0))
                info.compress_type = compression
                archive.writestr(info, payload)
    return stream.getvalue()


class MapImportRegression(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='hamclock-import-test-')
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        self.valid = shapefile(polyline([(10, 20), (30, 40)]))

    def reject(self, data, reason):
        with self.assertRaisesRegex(ValueError, reason):
            IMPORTER.decode_shp(data)

    def read(self, raw):
        path = self.directory / 'fixture.zip'
        path.write_bytes(raw)
        return IMPORTER.read_zip(path)

    def reject_zip(self, raw, reason):
        with self.assertRaisesRegex(ValueError, reason):
            self.read(raw)

    def test_valid_two_point_coordinates_and_part(self):
        self.assertEqual(IMPORTER.decode_shp(self.valid), ([(10.0,20.0),(30.0,40.0)], [(0,2)]))

    def test_valid_multiple_parts_and_records(self):
        data = shapefile(polyline([(0,0),(1,2),(3,4),(5,6)], (0,2)), polyline([(-10,-20),(-30,-40)]))
        self.assertEqual(IMPORTER.decode_shp(data), ([(0,0),(1,2),(3,4),(5,6),(-10,-20),(-30,-40)], [(0,2),(2,2),(4,2)]))

    def test_boundary_tolerance_snaps_only_tiny_overshoot(self):
        data = shapefile(polyline([(180.0000004,90.0000004),(-180.0000004,-90.0000004)]))
        self.assertEqual(IMPORTER.decode_shp(data)[0], [(180,90),(-180,-90)])

    def test_header_too_short(self):
        self.reject(b'0'*99, 'size outside bounds')

    def test_magic(self):
        self.reject(changed(self.valid,0,'>I',42), 'magic')

    def test_file_length(self):
        self.reject(changed(self.valid,24,'>I',50), 'shapefile length')

    def test_version(self):
        self.reject(changed(self.valid,28,'<I',999), 'version 1000')

    def test_header_type(self):
        self.reject(changed(self.valid,32,'<I',5), 'PolyLine shapefile')

    def test_empty_coastline(self):
        self.reject(shapefile(), 'empty coastline')

    def test_truncated_record_header_with_valid_file_length(self):
        data = bytearray(shapefile()+b'\0\0\0\0')
        struct.pack_into('>I',data,24,len(data)//2)
        self.reject(bytes(data), 'truncated record header')

    def test_truncated_record_payload_with_valid_file_length(self):
        data = bytearray(self.valid[:-2]);struct.pack_into('>I',data,24,len(data)//2)
        self.reject(bytes(data), 'record sequence or size')

    def test_nonsequential_record(self):
        self.reject(changed(self.valid,100,'>I',2), 'record sequence or size')

    def test_record_length_extra_payload(self):
        data = bytearray(self.valid+b'\0\0')
        struct.pack_into('>I',data,24,len(data)//2)
        struct.pack_into('>I',data,104,(len(data)-108)//2)
        self.reject(bytes(data), 'payload length')

    def test_null_and_mixed_record_types(self):
        for kind in (0,5):
            with self.subTest(kind=kind):
                self.reject(shapefile(polyline([(0,0),(1,1)]),polyline([(2,2),(3,3)],shape_type=kind)), 'non-null PolyLine')

    def test_record_count_limits(self):
        for offset,value in ((144,0),(144,10001),(148,1),(148,100001)):
            with self.subTest(offset=offset,value=value):
                self.reject(changed(self.valid,offset,'<I',value), 'record counts')

    def test_aggregate_point_cap(self):
        data=shapefile(polyline([(0,0),(1,1)]),polyline([(2,2),(3,3)]))
        with mock.patch.object(IMPORTER,'MAX_POINTS',3):
            self.reject(data,'aggregate counts')

    def test_aggregate_part_cap(self):
        data=shapefile(polyline([(0,0),(1,1)]),polyline([(2,2),(3,3)]))
        with mock.patch.object(IMPORTER,'MAX_PARTS',1):
            self.reject(data,'aggregate counts')

    def test_part_offsets(self):
        for starts,reason in (((1,),'start at zero'),((0,0),'increase'),((0,4),'part bounds'),((0,1),'part bounds')):
            with self.subTest(starts=starts):
                self.reject(shapefile(polyline([(0,0),(1,1),(2,2),(3,3)],starts)),reason)

    def test_nonfinite_and_outside_coordinates(self):
        for point in ((float('nan'),0),(0,float('nan')),(float('inf'),0),(0,float('-inf')),(180.01,0),(-180.01,0),(0,90.01),(0,-90.01)):
            with self.subTest(point=point):
                self.reject(shapefile(polyline([(0,0),point])),'geographic coordinate')

    def test_valid_zip_and_raw_hash(self):
        raw=zip_bytes([(IMPORTER.MEMBER,self.valid),('readme.txt',b'harmless')])
        decoded,sha=self.read(raw)
        self.assertEqual(decoded, ([(10,20),(30,40)],[(0,2)]))
        self.assertEqual(sha,hashlib.sha256(raw).hexdigest())

    def test_hash_and_decode_use_single_bounded_read(self):
        raw=zip_bytes([(IMPORTER.MEMBER,self.valid)])
        requests=[]
        class ObservedStream(io.BytesIO):
            def read(self, size=-1):
                requests.append(size)
                return super().read(size)
        class ObservedPath:
            reads=[]
            def open(self,mode):
                self.reads.append(mode)
                return ObservedStream(raw)
        path=ObservedPath()
        decoded,sha=IMPORTER.read_zip(path)
        self.assertEqual(path.reads,['rb'])
        self.assertEqual(requests,[IMPORTER.MAX_ARCHIVE+1])
        self.assertEqual(decoded[0],[(10,20),(30,40)])
        self.assertEqual(sha,hashlib.sha256(raw).hexdigest())

    def test_duplicate_required_member(self):
        self.reject_zip(zip_bytes([(IMPORTER.MEMBER,self.valid),(IMPORTER.MEMBER,self.valid)]),'exactly one')

    def test_missing_and_traversal_only_member(self):
        for name in ('README.txt','../'+IMPORTER.MEMBER,'nested/'+IMPORTER.MEMBER):
            with self.subTest(name=name):
                self.reject_zip(zip_bytes([(name,self.valid)]),'exactly one')

    def test_unrelated_traversal_is_not_extracted(self):
        outside_name=self.directory.name+'-outside.txt'
        raw=zip_bytes([(IMPORTER.MEMBER,self.valid),('../'+outside_name,b'BAD'),('/absolute.txt',b'BAD')])
        self.assertEqual(self.read(raw)[0][1],[(0,2)])
        self.assertEqual(sorted(p.name for p in self.directory.iterdir()),['fixture.zip'])
        self.assertFalse((self.directory.parent/outside_name).exists())

    def test_decompressed_shapefile_cap(self):
        # Highly compressible payload exceeds decoded cap while ZIP stays tiny.
        raw=zip_bytes([(IMPORTER.MEMBER,b'\0'*(IMPORTER.MAX_SHP+1))],zipfile.ZIP_DEFLATED)
        self.assertLess(len(raw),4096)
        self.reject_zip(raw,'oversized shapefile')

    def test_encrypted_member_flag_rejected_before_decode(self):
        raw=bytearray(zip_bytes([(IMPORTER.MEMBER,self.valid)]))
        # Set the encryption flag in local and central directory headers.
        struct.pack_into('<H',raw,6,1)
        central=raw.index(b'PK\x01\x02')
        struct.pack_into('<H',raw,central+8,1)
        self.reject_zip(bytes(raw),'encrypted')

    def test_too_small_shapefile_member(self):
        self.reject_zip(zip_bytes([(IMPORTER.MEMBER,b'x'*99)]),'oversized shapefile')

    def test_archive_cap(self):
        self.reject_zip(b'0'*(IMPORTER.MAX_ARCHIVE+1),'archive size')

    def test_invalid_zip_format(self):
        with self.assertRaises(zipfile.BadZipFile):
            self.read(b'not a ZIP')

    def test_cli_rejection_preserves_existing_output(self):
        source=self.directory/'bad.zip';source.write_bytes(zip_bytes([(IMPORTER.MEMBER,changed(self.valid,0,'>I',42))]))
        output=self.directory/'coastline.hpp';output.write_bytes(b'existing output sentinel\n')
        result=subprocess.run([sys.executable,'-B',str(ROOT/'tools/import_coastline.py'),str(source),str(output)],capture_output=True,text=True,timeout=10)
        self.assertEqual(result.returncode,2)
        self.assertIn('invalid shapefile magic',result.stderr)
        self.assertEqual(output.read_bytes(),b'existing output sentinel\n')

    def test_tiny_generated_header_golden_coordinates(self):
        rendered=IMPORTER.emit([(10,20),(30,40)],[(0,2)],'test-digest')
        self.assertIn('// Source archive SHA-256: test-digest\n',rendered)
        self.assertIn('inline constexpr CoastPoint coastline_points[] = {\n    {10.0000000f, 20.0000000f},\n    {30.0000000f, 40.0000000f},\n};',rendered)
        self.assertIn('inline constexpr CoastPart coastline_parts[] = {\n    {0u, 2u},\n};',rendered)


if __name__ == '__main__':
    unittest.main(verbosity=2)

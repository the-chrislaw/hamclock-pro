#!/usr/bin/env python3
"""Retag a Qt iOS kit's arm64 objects for the iOS Simulator, in place.

Qt's iOS kit ships fat objects and archives whose arm64 slice is tagged for the
device (LC_BUILD_VERSION platform iOS), and the Simulator link rejects them. The
code is plain arm64, so only the platform field has to change. Xcode 16's vtool
refuses these objects ("not enough space to hold load commands", they have no
header padding) and ar cannot unpack fat archives, so this edits the 4-byte field
directly: no sizes or offsets move, and archive symbol tables stay valid.

Run it on a copy of the kit. The result cannot link for a device, so it must
never reach the Qt cache or a device build.

Usage: retag_ios_kit_for_simulator.py <kit dir>
Exits non-zero if a slice cannot be retagged or nothing was retagged at all.
"""
import os
import struct
import sys
from collections.abc import Iterator

CPU_TYPE_ARM64 = 0x0100000C
MH_MAGIC_64 = 0xFEEDFACF
MH_HEADER_64_SIZE = 32
FAT_MAGIC = 0xCAFEBABE
FAT_MAGIC_64 = 0xCAFEBABF
AR_MAGIC = b"!<arch>\n"
AR_HEADER_SIZE = 60
AR_LONG_NAME_PREFIX = b"#1/"
LC_VERSION_MIN_IPHONEOS = 0x25
LC_BUILD_VERSION = 0x32
PLATFORM_IOS = 2
PLATFORM_IOSSIMULATOR = 7
BUILD_VERSION_PLATFORM_OFFSET = 8


class KitFormatError(Exception):
    """A slice this tool cannot retag; the Simulator link would reject it."""


def macho_platform_offsets(data: bytes, start: int) -> tuple[list[int], int]:
    """Return (offsets of device platform fields, simulator slices) for one thin Mach-O."""
    if struct.unpack_from("<I", data, start)[0] != MH_MAGIC_64:
        return [], 0
    cputype = struct.unpack_from("<I", data, start + 4)[0]
    ncmds = struct.unpack_from("<I", data, start + 16)[0]
    if cputype != CPU_TYPE_ARM64:
        return [], 0

    offsets, already_simulator = [], 0
    cursor = start + MH_HEADER_64_SIZE
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", data, cursor)
        if cmdsize < 8:
            raise KitFormatError(f"bad load command size {cmdsize}")
        if cmd == LC_VERSION_MIN_IPHONEOS:
            raise KitFormatError("arm64 slice uses LC_VERSION_MIN_IPHONEOS, which has no simulator form")
        if cmd == LC_BUILD_VERSION:
            platform = struct.unpack_from("<I", data, cursor + BUILD_VERSION_PLATFORM_OFFSET)[0]
            if platform == PLATFORM_IOS:
                offsets.append(cursor + BUILD_VERSION_PLATFORM_OFFSET)
            elif platform == PLATFORM_IOSSIMULATOR:
                already_simulator += 1
            else:
                raise KitFormatError(f"arm64 slice has unexpected platform {platform}")
        cursor += cmdsize
    return offsets, already_simulator


def fat_slices(data: bytes, start: int) -> list[tuple[int, int]]:
    """Return the absolute (start, end) of every slice in a fat header."""
    magic, count = struct.unpack_from(">II", data, start)
    if magic == FAT_MAGIC:
        entries = [struct.unpack_from(">iiIII", data, start + 8 + i * 20) for i in range(count)]
    else:
        entries = [struct.unpack_from(">iiQQII", data, start + 8 + i * 32) for i in range(count)]
    return [(start + entry[2], start + entry[2] + entry[3]) for entry in entries]


def archive_members(data: bytes, start: int, end: int) -> list[tuple[int, int]]:
    """Return the absolute (start, end) of every member's payload in an ar archive."""
    members = []
    cursor = start + len(AR_MAGIC)
    while cursor < end:
        if cursor + AR_HEADER_SIZE > end or data[cursor + 58:cursor + 60] != b"`\n":
            raise KitFormatError(f"bad archive member header at {cursor}")
        name = data[cursor:cursor + 16].rstrip()
        size = int(data[cursor + 48:cursor + 58].strip())
        payload = cursor + AR_HEADER_SIZE
        name_len = int(name[len(AR_LONG_NAME_PREFIX):]) if name.startswith(AR_LONG_NAME_PREFIX) else 0
        members.append((payload + name_len, payload + size))
        cursor = payload + size + (size & 1)
    return members


def platform_offsets(data: bytes, start: int, end: int) -> tuple[list[int], int]:
    """Walk a thin Mach-O, fat file or archive; same result as macho_platform_offsets."""
    if end - start < 8:
        return [], 0
    if data[start:start + len(AR_MAGIC)] == AR_MAGIC:
        children = archive_members(data, start, end)
    elif struct.unpack_from(">I", data, start)[0] in (FAT_MAGIC, FAT_MAGIC_64):
        children = fat_slices(data, start)
    else:
        return macho_platform_offsets(data, start)

    offsets, already_simulator = [], 0
    for child_start, child_end in children:
        child_offsets, child_simulator = platform_offsets(data, child_start, child_end)
        offsets += child_offsets
        already_simulator += child_simulator
    return offsets, already_simulator


def retagged(data: bytes, offsets: list[int]) -> bytes:
    """Return a copy of data with each platform field set to the iOS Simulator."""
    copy = bytearray(data)
    for offset in offsets:
        struct.pack_into("<I", copy, offset, PLATFORM_IOSSIMULATOR)
    return bytes(copy)


def kit_files(root: str) -> Iterator[str]:
    """Yield the kit's object files and static libraries (symlinks skipped)."""
    for directory, _, names in os.walk(root):
        for name in names:
            path = os.path.join(directory, name)
            if name.endswith((".o", ".a")) and not os.path.islink(path):
                yield path


def main(argv: list[str]) -> int:
    if len(argv) != 2 or not os.path.isdir(argv[1]):
        print(f"usage: {argv[0]} <kit dir>", file=sys.stderr)
        return 2

    files = slices = already_simulator = failures = 0
    for path in kit_files(argv[1]):
        files += 1
        try:
            with open(path, "rb") as handle:
                data = handle.read()
            offsets, simulator = platform_offsets(data, 0, len(data))
            if offsets:
                with open(path, "wb") as handle:
                    handle.write(retagged(data, offsets))
        except (KitFormatError, OSError, ValueError, struct.error) as error:
            print(f"cannot retag {path}: {error}", file=sys.stderr)
            failures += 1
            continue
        slices += len(offsets)
        already_simulator += simulator

    print(f"retagged {slices} arm64 slices for the iOS Simulator in {files} files "
          f"({already_simulator} already tagged, {failures} failed)")
    if failures or slices == 0:
        print("kit was not retagged cleanly", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

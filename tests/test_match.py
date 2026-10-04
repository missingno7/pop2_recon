"""Fail-closed acceptance fixtures for complete OMF function contributions."""
import struct
import sys
import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from match import check_object
import common
import match
import validate
from common import sha


def omf_record(kind, body=b""):
    """Emit one OMF record with a valid length and checksum."""
    length = len(body) + 1
    prefix = bytes((kind,)) + struct.pack("<H", length) + body
    checksum = (-sum(prefix)) & 0xFF
    return prefix + bytes((checksum,))


def omf_index(value):
    if value < 0x80:
        return bytes((value,))
    return bytes(((value >> 8) | 0x80, value & 0xFF))


def make_omf(code=b"\xCB", *, code_length=None, data_chunks=None,
             extra_segments=(), extra_data_chunks=(), publics=("_target",),
             externals=(), unused_thread=False, actual_fixup=False):
    """Make a tiny OMF module; segment tuple is (name, class, length)."""
    segs = [("_TEXT", "CODE", len(code) if code_length is None else code_length)]
    segs.extend(extra_segments)
    names = []
    for name, cls, _ in segs:
        for value in (name, cls):
            if value not in names:
                names.append(value)
    name_index = {name: i + 1 for i, name in enumerate(names)}
    lnames = b"".join(bytes((len(name),)) + name.encode("ascii") for name in names)
    records = [omf_record(0x80, b"\x04test"), omf_record(0x96, lnames)]

    for name, cls, length in segs:
        # Alignment code 1 (byte), combine public, 16-bit non-BIG SEGDEF.
        body = (bytes((0x28,)) + struct.pack("<H", length) +
                omf_index(name_index[name]) + omf_index(name_index[cls]) + b"\x00")
        records.append(omf_record(0x98, body))

    if externals:
        body = b"".join(bytes((len(name),)) + name.encode("ascii") + b"\x00"
                         for name in externals)
        records.append(omf_record(0x8C, body))

    for public in publics:
        # PUBDEF: group 0, segment 1, then name/offset/type index.
        entry = bytes((len(public),)) + public.encode("ascii") + struct.pack("<H", 0) + b"\x00"
        records.append(omf_record(0x90, b"\x00\x01" + entry))

    chunks = [(0, code)] if data_chunks is None else data_chunks
    for segment_index, offset, payload in [(1, start, body) for start, body in chunks]:
        records.append(omf_record(0xA0, omf_index(segment_index) +
                                  struct.pack("<H", offset) + payload))
    for segment_index, offset, payload in extra_data_chunks:
        records.append(omf_record(0xA0, omf_index(segment_index) +
                                  struct.pack("<H", offset) + payload))
    if unused_thread:
        # Define frame thread zero (method=0, datum index 1) without using it.
        records.append(omf_record(0x9C, b"\x00\x01"))
    if actual_fixup:
        # Offset16 FIXUPP against external index one, anchored to the CODE data.
        locat = 0x8000 | (1 << 10)
        body = bytes((locat >> 8, locat & 0xFF, 0x84, 0x01))
        records.append(omf_record(0x9C, body))
    records.append(omf_record(0x8A))
    return b"".join(records)


class SpaceFixture:
    def __init__(self, relocations=()):
        self.name = "root"
        self.relocations = tuple(relocations)

    def position(self, segment, offset):
        if segment != 1:
            raise ValueError("unexpected test segment")
        return offset

    def extent(self, segment, offset, size):
        if segment != 1 or offset + size > 1:
            raise ValueError("unexpected test extent")
        return b"\xCB" * size


class MatchAcceptanceTests(unittest.TestCase):
    def check(self, obj, expected=b"\xCB", *, space=None, target_size=None):
        target = {"id": "fixture", "segment": 1, "offset": 0,
                  "size": len(expected) if target_size is None else target_size}
        return check_object(obj, target, space or SpaceFixture(), expected, "_target")

    def test_complete_retf_contribution_is_accepted(self):
        report = self.check(make_omf())
        self.assertTrue(report["exact"])
        self.assertEqual(report["state"], "CODE_EXACT")
        self.assertEqual(report["emitted_size"], 1)
        self.assertEqual(report["fixups"], 0)
        self.assertEqual(report["relocations"], 0)

    def test_emitted_size_mismatch_is_reported_without_trimming(self):
        report = self.check(make_omf(code=b"\xCB\x90"))
        self.assertFalse(report["exact"])
        self.assertEqual(report["state"], "CANDIDATE_C")
        self.assertEqual(report["expected_size"], 1)
        self.assertEqual(report["emitted_size"], 2)
        self.assertEqual(report["first_difference"], 1)

    def test_unused_external_and_thread_declaration_do_not_hide_fixup(self):
        # A relocation-free component can contain compiler scaffolding; an
        # actual FIXUPP subrecord remains an independent acceptance obligation.
        with self.assertRaisesRegex(ValueError, "Fixup-bearing components"):
            self.check(make_omf(externals=("_helper",), unused_thread=True,
                                actual_fixup=True))

    def test_unused_thread_and_external_without_fixup_can_be_inspected(self):
        report = self.check(make_omf(externals=("_helper",), unused_thread=True))
        self.assertTrue(report["exact"])
        self.assertEqual(report["unreferenced_external_declarations"], ["_helper"])
        self.assertEqual(report["fixup_thread_declarations"][0]["kind"], "thread")

    def test_original_target_relocation_requires_candidate_fixup(self):
        space = SpaceFixture(relocations=({"image_offset": 0},))
        with self.assertRaisesRegex(ValueError, "Original relocation obligations"):
            self.check(make_omf(), space=space)

    def test_ledat_hole_is_not_accepted_as_zero_filled_target_data(self):
        # The reader's segment buffer is zero-initialized, so compare against
        # the apparent matching bytes and require the independent coverage gate.
        obj = make_omf(code=b"\xCB\x00", code_length=2,
                       data_chunks=[(0, b"\xCB")])
        with self.assertRaisesRegex(ValueError, "CODE segment contains uninitialized holes"):
            self.check(obj, expected=b"\xCB\x00")

    def test_initialized_secondary_storage_is_unowned(self):
        obj = make_omf(code=b"\xCB", extra_segments=(("_DATA", "DATA", 1),),
                       extra_data_chunks=((2, 0, b"\x44"),))
        with self.assertRaisesRegex(ValueError, "Unowned DATA/BSS/secondary declarations"):
            self.check(obj)

    def test_additional_public_is_rejected(self):
        obj = make_omf(publics=("_target", "_alias"))
        with self.assertRaisesRegex(ValueError, "public must own the complete CODE segment"):
            self.check(obj)


class ManifestOverlapTests(unittest.TestCase):
    def test_overlapping_canonical_owner_ranges_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for folder in ("evidence", "recipes", "src"):
                (root / folder).mkdir()
            original = b"locked original"
            target_bytes = b"\xCB"
            target = {"boundary_status": "REVIEWED", "boundary_evidence": ["independent"],
                      "space": "root", "segment": 1, "offset": 0, "size": 1,
                      "sha256": sha(target_bytes)}
            inventory = {"target_sha256": sha(original), "functions": [
                {"id": "A", **target}, {"id": "B", **target}]}
            (root / "evidence/targets.json").write_text(json.dumps(inventory), encoding="utf-8")
            source = root / "src/a.c"
            source.write_text("int candidate;\n", encoding="utf-8")
            source_hash = sha(source.read_bytes())
            for ident in ("A", "B"):
                recipe = {"target_id": ident, "source": "src/a.c",
                          "source_sha256": source_hash}
                (root / f"recipes/{ident}.json").write_text(json.dumps(recipe), encoding="utf-8")
            owners = [{"target_id": ident, "kind": "MATCHING_C", "state": "CODE_EXACT",
                       "space": "root", "segment": 1, "offset": offset, "size": 1,
                       "recipe": f"recipes/{ident}.json"}
                      for ident, offset in (("A", 0), ("B", 0))]
            manifest = {"owners": owners}
            fake_oracle = SimpleNamespace(data=original, spaces={"root": SpaceFixture()})
            with patch.object(match, "ROOT", root), \
                 patch.object(validate, "ROOT", root), \
                 patch.object(common, "ROOT", root):
                with self.assertRaisesRegex(ValueError, "Overlapping canonical owners"):
                    validate.check_manifest(fake_oracle, manifest)


if __name__ == "__main__":
    unittest.main()

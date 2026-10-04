"""Behavioral checks for MZ parsing and the immutable segmented oracle."""
import contextlib
import hashlib
import io
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import inventory
import oracle
from common import sha
from mz import MZ


def mz_file(resident_size=96, appended=b"", relocations=()):
    """Build a small well-formed MZ with a 32-byte header."""
    pages = (resident_size + 511) // 512
    last = resident_size % 512
    if last == 0:
        last = 0
    fields = [0x5A4D, last, pages, len(relocations), 2, 0, 0xFFFF,
              0, 0, 0, 0, 0, 28, 0]
    data = bytearray(resident_size)
    struct.pack_into("<14H", data, 0, *fields)
    for i, (offset, segment) in enumerate(relocations):
        struct.pack_into("<HH", data, 28 + 4 * i, offset, segment)
    return bytes(data) + appended


DESCRIPTOR = "<HHHBBHHHHH"


def overlay_fixture(*, bad_sequence=False, first_reloc_offset=0,
                    second_position_paragraph=8, table_offset=32):
    """Two tiny overlays, with a three-byte resident-to-payload gap."""
    resident_size = 93
    # Overlay 2 has one relocation word and one paragraph of payload. Overlay
    # 3 starts at EOF of that payload and is one byte short of two paragraphs.
    rows = [
        [0x100, 0, 6, 0, 0, 0, 1, 0, 2, 1],  # rpos 96, code 112, size 16
        [0x100, 0, second_position_paragraph, 0, 0, 0, 0, 0,
         4 if bad_sequence else 3, 2],
    ]
    resident = bytearray(mz_file(resident_size))
    for i, row in enumerate(rows):
        struct.pack_into(DESCRIPTOR, resident, 32 + i * 18, *row)
    gap = b"gap"
    first_reloc = struct.pack("<HH", first_reloc_offset, 0x100)
    first = first_reloc + bytes(12) + b"A" * 16
    second = b"B" * 31
    data = bytes(resident) + gap + first + second
    profile = {
        "descriptor_table_file_offset": table_offset,
        "descriptor_count": 2,
        "record_format": DESCRIPTOR,
        "first_overlay_id": 2,
        "payload_shortfall_by_overlay": {"3": 1},
    }
    return data, profile


class MZParserTests(unittest.TestCase):
    def test_rejects_truncated_or_malformed_header(self):
        with self.assertRaisesRegex(ValueError, "Truncated MZ header"):
            MZ.parse(b"MZ" + bytes(25))

        wrong_magic = bytearray(mz_file())
        wrong_magic[:2] = b"NE"
        with self.assertRaisesRegex(ValueError, "Not an MZ"):
            MZ.parse(wrong_magic)

        zero_pages = bytearray(mz_file())
        struct.pack_into("<H", zero_pages, 4, 0)
        with self.assertRaisesRegex(ValueError, "Invalid MZ page count"):
            MZ.parse(zero_pages)

        outside_file = bytearray(mz_file())
        struct.pack_into("<H", outside_file, 4, 0xFFFF)
        with self.assertRaisesRegex(ValueError, "outside file"):
            MZ.parse(outside_file)

    def test_rejects_relocation_table_and_site_outside_declared_bounds(self):
        table_past_header = bytearray(mz_file(relocations=((0, 0),)))
        struct.pack_into("<H", table_past_header, 24, 30)
        with self.assertRaisesRegex(ValueError, "relocation table outside header"):
            MZ.parse(table_past_header)

        relocation_outside_image = mz_file(resident_size=64, relocations=((32, 0),))
        with self.assertRaisesRegex(ValueError, "relocation outside load image"):
            MZ.parse(relocation_outside_image)


class OverlayOracleTests(unittest.TestCase):
    def test_rejects_descriptor_sequence_and_table_or_payload_bounds(self):
        data, profile = overlay_fixture(bad_sequence=True)
        with self.assertRaisesRegex(ValueError, "Descriptor overlay sequence"):
            oracle.Oracle(data, profile)

        data, profile = overlay_fixture(table_offset=80)
        with self.assertRaisesRegex(ValueError, "Descriptor table outside resident image"):
            oracle.Oracle(data, profile)

        data, profile = overlay_fixture(second_position_paragraph=0xFFFF)
        with self.assertRaisesRegex(ValueError, "Overlay payload overlaps/outside file"):
            oracle.Oracle(data, profile)

    def test_rejects_relocation_site_past_overlay_payload(self):
        data, profile = overlay_fixture(first_reloc_offset=15)
        with self.assertRaisesRegex(ValueError, "Relocation outside overlay 2"):
            oracle.Oracle(data, profile)

    def test_records_exact_three_byte_gap_and_final_one_byte_shortfall(self):
        data, profile = overlay_fixture()
        parsed = oracle.Oracle(data, profile)
        self.assertEqual(parsed.gaps, [{
            "file_offset": 93,
            "size": 3,
            "sha256": sha(b"gap"),
            "kind": "unowned_file_gap",
        }])
        final = parsed.spaces["overlay-3"].descriptor
        self.assertEqual(final["allocated_size"], 32)
        self.assertEqual(final["file_payload_size"], 31)
        self.assertEqual(final["absent_final_paragraph_bytes"], 1)
        self.assertEqual(parsed.spaces["overlay-3"].file_offset + 31, len(data))

    def test_locked_target_preserves_known_gap_and_eof_shortfall(self):
        parsed = oracle.Oracle.load()
        self.assertEqual(parsed.gaps, [{
            "file_offset": 152637,
            "size": 3,
            "sha256": "709e80c88487a2411e1ee4dfb9f22a861492d20c4765150c0c794abd70f8147c",
            "kind": "unowned_file_gap",
        }])
        final = parsed.spaces["overlay-17"]
        self.assertEqual(final.descriptor["allocated_size"], 21104)
        self.assertEqual(final.descriptor["file_payload_size"], 21103)
        self.assertEqual(final.descriptor["absent_final_paragraph_bytes"], 1)
        self.assertEqual(final.file_offset + final.size, len(parsed.data))

    def test_address_translation_and_overlapping_spaces_keep_identity(self):
        parsed = oracle.Oracle.load()
        root = parsed.spaces["root"]
        entry = parsed.mz.describe(parsed.data)["entry"]
        entry_pos = root.position(entry["segment"], entry["offset"])
        self.assertEqual(entry_pos, entry["unwrapped_image_offset"])
        self.assertEqual(root.file_offset, parsed.mz.header_size)
        self.assertEqual(root.extent(entry["segment"], entry["offset"], 4),
                         parsed.data[root.file_offset + entry_pos:
                                     root.file_offset + entry_pos + 4])
        runtime = root.runtime_address(entry["segment"], entry["offset"], 0x1000)
        self.assertEqual(runtime["segment"], (entry["segment"] + 0x1000) & 0xFFFF)
        self.assertEqual(runtime["offset"], entry["offset"])
        self.assertEqual(runtime["space"], "root")

        first, second = parsed.spaces["overlay-2"], parsed.spaces["overlay-3"]
        self.assertEqual(first.link_segment, second.link_segment)
        self.assertEqual(first.position(first.link_segment, 0), 0)
        self.assertEqual(second.position(second.link_segment, 0), 0)
        self.assertNotEqual(first.file_offset, second.file_offset)
        self.assertNotEqual(first.extent(first.link_segment, 0, 4),
                            second.extent(second.link_segment, 0, 4))
        self.assertNotEqual(first.address(0)["space"], second.address(0)["space"])


class ImmutableLockTests(unittest.TestCase):
    def test_verification_never_refreshes_lock_or_reads_candidate_as_oracle(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            assets = root / "assets"
            assets.mkdir()
            target = mz_file(resident_size=32)
            (assets / "PRINCE.EXE").write_bytes(target)
            (assets / "DATA.DAT").write_bytes(b"original-support")
            inv = inventory.inventory(assets)
            lock_path = root / "layout" / "oracle.lock.json"
            lock_path.parent.mkdir()
            lock = {"schema": 1, "authority": "fixture", "target": {
                "path": "assets/PRINCE.EXE", "size": len(target), "sha256": sha(target)},
                "assets": inv}
            lock_path.write_text(json.dumps(lock), encoding="utf-8")
            profile_path = root / "layout" / "oracle-format.json"
            profile_path.write_text(json.dumps({
                "target_sha256": sha(target), "descriptor_table_file_offset": 32,
                "descriptor_count": 0, "record_format": DESCRIPTOR,
                "first_overlay_id": 2, "payload_shortfall_by_overlay": {},
            }), encoding="utf-8")
            candidate = root / "candidate.exe"
            candidate.write_bytes(b"MZ candidate bytes must have no authority")
            before = lock_path.read_bytes()

            with patch.object(inventory, "ROOT", root), \
                 patch.object(inventory, "LOCK", lock_path), \
                 patch.object(oracle, "ROOT", root):
                self.assertEqual(inventory.verify_assets()["target"]["sha256"], sha(target))
                with patch.object(sys, "argv", ["inventory.py"]), \
                     contextlib.redirect_stdout(io.StringIO()):
                    inventory.main()
                loaded = oracle.Oracle.load()
                self.assertEqual(loaded.data, target)
                self.assertEqual(loaded.describe()["target_sha256"], sha(target))
                self.assertEqual(lock_path.read_bytes(), before)

                with patch.object(sys, "argv", ["inventory.py", "--freeze"]):
                    with self.assertRaisesRegex(ValueError, "Refusing to overwrite frozen oracle lock"):
                        inventory.main()
                self.assertEqual(lock_path.read_bytes(), before)


if __name__ == "__main__":
    unittest.main()

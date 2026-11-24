"""Diagnostic archive bounds and whole-member refusal cases."""
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from omf import OmfReader
from runtime_scan import archive_members, scan_archive, whole_code
from tests.test_runtime import runtime_omf, omf_record


def archive(member, dictionary_payload=b"dictionary"):
    body = bytearray(16)
    body[0] = 0xf0
    struct.pack_into("<H", body, 1, 13)
    body.extend(member)
    body.extend(b"\0"*((-len(body)) % 16))
    body.extend(b"\xf1"+b"\0"*15)
    struct.pack_into("<I", body, 3, len(body))
    body.extend(dictionary_payload)
    return bytes(body)


class RuntimeScanTests(unittest.TestCase):
    def test_dictionary_bytes_cannot_be_treated_as_members(self):
        member = runtime_omf(b"\x55\x8b\xec\x5d\xcb")
        data = archive(member, dictionary_payload=member)
        self.assertEqual(list(archive_members(data)), [(16, member)])

    def test_invalid_dictionary_bounds_are_refused(self):
        data = bytearray(archive(runtime_omf()))
        struct.pack_into("<I", data, 3, len(data)+1)
        with self.assertRaisesRegex(ValueError, "LIB page/dictionary bounds"):
            list(archive_members(bytes(data)))

    def test_secondary_storage_matching_prefix_does_not_become_candidate(self):
        obj = runtime_omf(b"\xcb", extra_segments=(("_DATA", "DATA", 2),))
        self.assertIsNone(whole_code(OmfReader().read(obj), 1))

    def test_holes_and_actual_fixups_do_not_become_literal_candidates(self):
        hole = runtime_omf(b"\xcb", code_length=2)
        fixup = runtime_omf(b"\xcb", actual_fixup=True)
        self.assertIsNone(whole_code(OmfReader().read(hole), 1))
        self.assertIsNone(whole_code(OmfReader().read(fixup), 1))

    def test_complete_shared_entry_member_retains_every_public(self):
        code = b"\x55\x8b\xec\x5d\xcb"
        records = []
        for kind, body, _ in OmfReader.records(runtime_omf(code)):
            if kind == 0x8a:
                records.append(omf_record(0x90, b"\x00\x01\x06_entry\x03\x00\x00"))
            records.append(omf_record(kind, body))
        member = b"".join(records)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"shared.lib"
            path.write_bytes(archive(member))
            oracle = SimpleNamespace(spaces={"root": SimpleNamespace(name="root",
                data=code, file_offset=0, relocations=())})
            candidate = scan_archive(path, oracle, minimum_size=1)["review_candidates"][0]
            self.assertEqual(candidate["code_size"], 5)
            self.assertEqual([(p["name"], p["offset"]) for p in candidate["declarations"]["publics"]],
                             [("_target", 0), ("_entry", 3)])

    def test_public_outside_complete_code_does_not_become_candidate(self):
        records = []
        for kind, body, _ in OmfReader.records(runtime_omf(b"\xcb")):
            if kind == 0x8a:
                records.append(omf_record(0x90, b"\x00\x01\x06_entry\x01\x00\x00"))
            records.append(omf_record(kind, body))
        self.assertIsNone(whole_code(OmfReader().read(b"".join(records)), 1))

    def test_whole_code_required_and_original_obligations_remain_visible(self):
        code = b"\x55\x8b\xec\x5d\xcb"
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"test.lib"
            path.write_bytes(archive(runtime_omf(code)))
            space = SimpleNamespace(name="root", data=code[:-1]+b"\x90"+code,
                file_offset=100, relocations=({"image_offset": 7},))
            oracle = SimpleNamespace(spaces={"root": space})
            report = scan_archive(path, oracle, minimum_size=1)
            hits = report["review_candidates"][0]["hits"]
            self.assertEqual([h["image_offset"] for h in hits], [5])
            self.assertEqual(hits[0]["original_relocations"], [{"image_offset": 7}])


if __name__ == "__main__":
    unittest.main()

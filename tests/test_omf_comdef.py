"""COMDEF communals (MSC uninitialized globals) parse in compiler output, near and far."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from omf import OmfError, OmfReader


def record(kind, body):
    data = bytes([kind]) + (len(body) + 1).to_bytes(2, "little") + body
    return data + bytes([(-sum(data)) & 0xFF])


def module(comdef_body):
    return (record(0x80, b"\x04UNIT") + record(0x8C, b"\x04PREV\x00") + record(0xB0, comdef_body) +
            record(0x8A, b"\x00"))


class ComdefTests(unittest.TestCase):
    def test_near_communal_shares_the_external_index_space(self):
        obj = OmfReader().read(module(b"\x0cRANDOM_STATE\x00\x62\x04"))
        self.assertEqual(obj.externals, ["PREV", "RANDOM_STATE"])
        self.assertEqual(obj.communals, [{"name": "RANDOM_STATE", "kind": "near", "length": 4}])

    def test_far_communal_is_an_external_with_its_element_count(self):
        obj = OmfReader().read(module(b"\x03BIG\x00\x61\x64\x12"))
        self.assertEqual(obj.externals, ["PREV", "BIG"])
        self.assertEqual(obj.communals, [{"name": "BIG", "kind": "far", "length": 1800}])

    def test_other_communal_types_are_refused(self):
        with self.assertRaisesRegex(OmfError, "unsupported COMDEF data type"):
            OmfReader(library_records=True).read(module(b"\x03BIG\x00\x60\x04"))


if __name__ == "__main__":
    unittest.main()

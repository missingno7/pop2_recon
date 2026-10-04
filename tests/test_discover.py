"""Checks that candidate discovery preserves uncertainty and uses Oracle mapping."""
import sys
import hashlib
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from context import _fallback_discovery, _owner_recipe, build_context
from discover import discover


class FakeSpace:
    def __init__(self, name, data, *, link_segment=0, file_offset=0, relocations=()):
        self.name = name
        self.data = data
        self.link_segment = link_segment
        self.file_offset = file_offset
        self.relocations = tuple(relocations)
        self.descriptor = None

    @property
    def size(self):
        return len(self.data)

    def position(self, segment, offset):
        pos = (segment - self.link_segment) * 16 + offset
        if not 0 <= pos < self.size:
            raise ValueError("outside fake space")
        return pos

    def address(self, pos):
        return {"space": self.name, "segment": self.link_segment + (pos // 65536) * 4096,
                "offset": pos % 65536}

    def extent(self, segment, offset, size):
        pos = self.position(segment, offset)
        if size <= 0 or pos + size > self.size:
            raise ValueError("outside fake extent")
        return self.data[pos:pos + size]


class FakeMZ:
    def __init__(self, segment=0, offset=0):
        self.header = {"initial_cs": segment, "initial_ip": offset}


class FakeOracle:
    def __init__(self, spaces, *, entry_segment=0, entry_offset=0):
        self.spaces = {space.name: space for space in spaces}
        max_end = max(space.file_offset + space.size for space in spaces)
        data = bytearray(max_end)
        for space in spaces:
            data[space.file_offset:space.file_offset + space.size] = space.data
        self.data = bytes(data)
        self.mz = FakeMZ(entry_segment, entry_offset)


class DiscoveryTests(unittest.TestCase):
    def test_near_call_stays_in_source_when_spaces_overlap(self):
        # call +0 targets offset 3; both spaces expose that same address.
        code = bytes.fromhex("e8 00 00 c3 c3")
        root = FakeSpace("root", code, file_offset=0)
        overlay = FakeSpace("overlay-2", code, file_offset=len(code))
        oracle = FakeOracle([root, overlay])
        inventory = discover(oracle, reference=ROOT / "missing-reference", prologue_scan=False)
        ids = {row["id"] for row in inventory["functions"]}
        self.assertIn("root:0000:0003", ids)
        self.assertNotIn("overlay-2:0000:0003", ids)

    def test_candidates_remain_unknown_without_code_exact_claims(self):
        oracle = FakeOracle([FakeSpace("root", bytes.fromhex("c3"))])
        inventory = discover(oracle, reference=ROOT / "missing-reference", prologue_scan=False)
        self.assertTrue(inventory["functions"])
        self.assertTrue(all(row["state"] == "UNKNOWN" for row in inventory["functions"]))
        self.assertFalse(any("CODE_EXACT" in row for row in inventory["functions"]))

    def test_context_rechecks_oracle_hash_and_can_disassemble_bytes(self):
        oracle = FakeOracle([FakeSpace("root", bytes.fromhex("c3"))])
        inventory = discover(oracle, reference=ROOT / "missing-reference", prologue_scan=False)
        packet = build_context(inventory, "root:0000:0000", oracle, asm=True)
        self.assertEqual(packet["target_sha256"], inventory["target_sha256"])
        self.assertEqual(packet["asm"][0]["mnemonic"], "ret")
        stale = dict(inventory, target_sha256="0" * 64)
        with self.assertRaisesRegex(ValueError, "does not match"):
            build_context(stale, "root:0000:0000", oracle)

    def test_nonzero_entry_cs_ip_and_wrapping_near_call_keep_input_alias(self):
        code = bytearray(b"\x90" * 0x10010)
        code[32:35] = bytes.fromhex("e8 ec ff")  # 0101:0010 -> 0101:ffff
        code[0x1000f] = 0xc3
        root = FakeSpace("root", bytes(code), link_segment=0x100)
        oracle = FakeOracle([root], entry_segment=0x101, entry_offset=0x10)
        inventory = discover(oracle, reference=ROOT / "missing-reference", prologue_scan=False)
        ids = {row["id"] for row in inventory["functions"]}
        self.assertIn("root:0101:0010", ids)
        self.assertIn("root:0101:ffff", ids)
        entry_context = build_context(inventory, "root:0101:0010", oracle, asm=True)
        call_insn = next(ins for ins in entry_context["asm"] if ins["mnemonic"] == "call")
        self.assertEqual(call_insn["near_target"], 0xffff)
        packet = build_context(inventory, "root:0101:ffff", oracle, asm=True)
        self.assertEqual(packet["asm"][0]["address"],
                         {"space": "root", "segment": 0x101, "offset": 0xffff})

    def test_far_call_keeps_far_segment_in_candidate_id(self):
        root_data = bytearray(b"\x90" * 128)
        root_data[32:37] = bytes.fromhex("9a 20 00 44 23")  # lcall 2344:0020
        overlay_data = bytearray(b"\x90" * 64)
        overlay_data[0x20] = 0xc3
        root = FakeSpace("root", bytes(root_data), link_segment=0x100, file_offset=0)
        overlay = FakeSpace("overlay-2", bytes(overlay_data), link_segment=0x2344, file_offset=128)
        oracle = FakeOracle([root, overlay], entry_segment=0x101, entry_offset=0x10)
        inventory = discover(oracle, reference=ROOT / "missing-reference", prologue_scan=False)
        ids = {row["id"] for row in inventory["functions"]}
        self.assertIn("overlay-2:2344:0020", ids)

    def test_reviewed_fallback_target_uses_oracle_without_overriding_state(self):
        oracle = FakeOracle([FakeSpace("root", bytes.fromhex("c3"))])
        index = {"target_sha256": hashlib.sha256(oracle.data).hexdigest(),
                 "functions": [{"id": "root:0000:0000", "space": "root", "segment": 0,
                                "offset": 0, "size": 1,
                                "sha256": hashlib.sha256(bytes.fromhex("c3")).hexdigest(),
                                "state": "READY_FOR_REVIEW", "boundary_status": "REVIEWED"}]}
        discovery = _fallback_discovery(index, "root:0000:0000", oracle)
        packet = build_context(discovery, "root:0000:0000", oracle, asm=True)
        self.assertEqual(packet["function"]["state"], "READY_FOR_REVIEW")
        self.assertEqual(packet["asm"][0]["address"]["offset"], 0)

    def test_owner_and_recipe_are_loaded_as_separate_context_metadata(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "recipes").mkdir()
            recipe = {"target_id": "root:0000:0000", "name": "example"}
            (root / "recipes/example.json").write_text(json.dumps(recipe), encoding="utf-8")
            owner = {"target_id": "root:0000:0000", "kind": "MATCHING_C",
                     "recipe": "recipes/example.json", "state": "CODE_EXACT"}
            attached = _owner_recipe({"owners": [owner]}, "root:0000:0000", root=root)
            self.assertEqual(attached["owner"], owner)
            self.assertEqual(attached["recipe"], recipe)


if __name__ == "__main__":
    unittest.main()

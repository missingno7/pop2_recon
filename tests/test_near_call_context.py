"""Near caller witnesses are original-byte boundaries, not outbound fixup proof."""
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import match
from common import sha, write_json
from oracle import Space


class NearCallerContextTests(unittest.TestCase):
    def check(self, *, code=None, caller_space="overlay-x", relocation=()):
        raw = code or bytes.fromhex("900ee80300909090cb90")
        original = b"independent locked fixture"
        space = Space("overlay-x", 0x29cf, 128, raw, tuple(relocation))
        other = Space("overlay-y", 0x29cf, 256, raw, tuple(relocation))
        oracle = SimpleNamespace(data=original, spaces={"overlay-x": space, "overlay-y": other})
        target = {"id": "leaf", "space": "overlay-x", "segment": 0x29cf, "offset": 8,
                  "size": 1, "sha256": sha(raw[8:9]), "boundary_status": "REVIEWED",
                  "boundary_evidence": ["Separate entry and RETF"],
                  "call_sites": [{"kind": "push_cs_near_call", "space": caller_space,
                                  "image_offset": 2, "file_offset": oracle.spaces[caller_space].file_offset+2,
                                  "pushed_cs_site": 1}]}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_json(root / "evidence/targets.json", {"target_sha256": sha(original), "functions": [target]})
            with patch.object(match, "ROOT", root):
                return match.get_target("leaf", oracle)

    def test_complete_original_push_cs_near_reference_is_checked(self):
        target, space, raw = self.check()
        self.assertEqual(raw, b"\xcb")
        self.assertEqual(target["call_sites"][0]["image_offset"], 2)

    def test_equal_addresses_in_another_overlay_do_not_prove_a_call(self):
        with self.assertRaisesRegex(ValueError, "cross overlay spaces"):
            self.check(caller_space="overlay-y")

    def test_changed_original_opcode_or_displacement_is_refused(self):
        with self.assertRaisesRegex(ValueError, "does not exist"):
            self.check(code=bytes.fromhex("9090e80300909090cb90"))
        with self.assertRaisesRegex(ValueError, "does not target"):
            self.check(code=bytes.fromhex("900ee80200909090cb90"))

    def test_relocated_word_cannot_be_hidden_in_a_relative_call(self):
        with self.assertRaisesRegex(ValueError, "provenance mismatch"):
            self.check(relocation=({"image_offset": 3},))

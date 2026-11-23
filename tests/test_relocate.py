"""Relocation renames identifiers and binding keys only; independent witnesses never change."""
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import relocate


class RelocateTests(unittest.TestCase):
    def test_binding_keys_follow_externals_and_witnesses_are_kept(self):
        witness = {"image_offset": 3757, "bytes_hex": "a0135c", "operand_offset": 1, "space": "root"}
        binding = {"target_id": "group:old", "symbols": {"STATE_BYTE": {"offset": 23571, "witnesses": [witness]}},
                   "declarations": "old", "fixups": "old"}
        old = SimpleNamespace(externals=["STATE_BYTE", "CALLEE"])
        new = SimpleNamespace(externals=["STATEBYTE", "CALLEE"])
        with patch.object(relocate, "declarations", return_value="new-decl"), \
             patch.object(relocate, "fixes", return_value="new-fixups"):
            out = relocate.renamed_binding(binding, old, new, "group:misc/new", [("state_byte", "StateByte")])
        self.assertEqual(out["symbols"], {"STATEBYTE": {"offset": 23571, "witnesses": [witness]}})
        self.assertEqual((out["declarations"], out["fixups"], out["target_id"]),
                         ("new-decl", "new-fixups", "group:misc/new"))
        self.assertEqual(binding["symbols"], {"STATE_BYTE": {"offset": 23571, "witnesses": [witness]}})

    def test_a_rename_must_hit_a_whole_identifier(self):
        self.assertEqual(relocate.rename_source("int AB; int ABC;", [("AB", "Ab")]), "int Ab; int ABC;")
        with self.assertRaisesRegex(ValueError, "does not occur"):
            relocate.rename_source("int ABC;", [("AB", "Ab")])


if __name__ == "__main__":
    unittest.main()

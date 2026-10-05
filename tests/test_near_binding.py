"""Adversarial whole-member near-code proofs using independent synthetic OMF."""
import copy
import struct
import sys
import tempfile
import unittest
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import common
import pin_runtime
from binding import declarations, fixes
from common import sha, write_json
from omf import OmfReader
from oracle import Space
from runtime import verify_member
from test_runtime import runtime_omf, omf_record


def object_with_fixup(code, fix_body, *, provider=False):
    raw = runtime_omf(code=code, externals=() if provider else ("_helper",),
                      extra_segments=(("_DATA", "DATA", 1),) if provider else (),
                      extra_data_chunks=((2, 0, b"\x77"),) if provider else ())
    records = []
    # The fixup must immediately follow its CODE LEDATA, not provider DATA.
    for kind, body, _ in OmfReader.records(raw):
        if kind == 0x90 and provider:
            body = b"\x00\x01\x07_helper\x01\x00\x00"
        records.append(omf_record(kind, body))
        if kind == 0xa0 and body[0] == 1:
            records.append(omf_record(0x9c, fix_body))
    return b"".join(records)


class NearRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.raw_code = b"\x90\xe9\0\0"
        self.owner = object_with_fixup(self.raw_code, b"\x84\x02\x06\x01\x01")
        self.provider = object_with_fixup(b"\xcb\x50\xe8\0\0\x58\xcb\xc3",
                                         b"\x84\x03\x00\x01\x01\x07\x00", provider=True)
        library = self.owner+self.provider
        self.library = self.root/"synthetic.lib"
        self.library.write_bytes(library)
        data = bytearray(1024)
        data[16:21] = struct.pack("<BHH", 0x9a, 0x140, 0x10)
        data[32:37] = struct.pack("<BHH", 0x9a, 0x80, 0x10)
        data[320:328] = b"\xcb\x50\xe8\x02\0\x58\xcb\xc3"
        data[384:388] = b"\x90\xe9"+struct.pack("<h", 0x41-0x84)
        self.expected = b"\x90\xe9"+struct.pack("<h", 0x41-0x144)
        data[576:580] = self.expected
        self.space = Space("root", 0, 0, bytes(data), ({"image_offset": 19}, {"image_offset": 35}))
        self.oracle = SimpleNamespace(data=b"independent frozen fixture", spaces={"root": self.space})
        owner, provider = OmfReader().read(self.owner), OmfReader().read(self.provider)
        anchor = lambda at: {"image_offset": at, "bytes_hex": self.space.data[at:at+5].hex()}
        self.proof = {"schema": 1, "mode": "external-near-offset16-v1", "space": "root",
            "target_sha256": sha(self.oracle.data), "entry_call": anchor(16),
            "declarations": declarations(owner), "fixups": fixes(owner),
            "symbols": {"_helper": {"name": "_helper", "segment": 16, "offset": 0x41,
                "incoming_wrapper": {"entry_call": anchor(32), "bytes_hex": self.space.data[384:388].hex(), "path": [0, 1]},
                "provider": {"library_path": str(self.library), "library_sha256": sha(library),
                    "archive_offset": len(self.owner), "member_extent": len(self.provider),
                    "member_sha256": sha(self.provider), "object_segment": "_TEXT", "module_offset": 0x40,
                    "prefix_size": 8, "prefix_sha256": sha(self.space.data[320:328]),
                    "declarations": declarations(provider), "fixups": fixes(provider)}}}}
        self.recipe = {"name": "test_near", "library_path": str(self.library), "library_sha256": sha(library),
            "archive_offset": 0, "member_extent": len(self.owner), "member_sha256": sha(self.owner),
            "object_segment": "_TEXT", "publics": ["_target"], "space": "root", "segment": 16,
            "offset": 0x140, "size": 4, "target_sha256": sha(self.expected),
            "binding": "evidence/bindings/test.json"}

    def tearDown(self):
        self.temp.cleanup()

    def check(self, proof=None, *, update_hash=True):
        path = self.root/self.recipe["binding"]
        write_json(path, self.proof if proof is None else proof)
        if update_hash:
            self.recipe["binding_sha256"] = sha(path.read_bytes())
        with patch.object(common, "ROOT", self.root):
            return verify_member(self.recipe, self.oracle)

    def mutate_original(self, at, raw):
        data = bytearray(self.space.data)
        data[at:at+len(raw)] = raw
        self.space = replace(self.space, data=bytes(data))
        self.oracle.spaces["root"] = self.space

    def test_whole_member_with_one_near_equation_and_zero_provider_ownership(self):
        before = self.library.read_bytes()
        result = self.check()
        self.assertTrue(result["exact"])
        self.assertEqual(result["fixups"], 1)
        self.assertEqual(result["ordinary_bytes_compared"], 2)
        self.assertEqual(result["symbol_grounding"][0]["provider_owned_bytes"], 0)
        self.assertNotEqual(result["raw_code_sha256"], result["sha256"])
        self.assertEqual(before, self.library.read_bytes())

    def test_binding_file_hash_is_independent(self):
        self.check()
        proof = copy.deepcopy(self.proof)
        proof["space"] = "overlay-2"
        with self.assertRaisesRegex(ValueError, "evidence changed"):
            self.check(proof, update_hash=False)

    def test_foreign_oracle_and_overlay_binding_rejected(self):
        for key, value in (("target_sha256", "other"), ("space", "overlay-2")):
            proof = copy.deepcopy(self.proof)
            proof[key] = value
            with self.assertRaises(ValueError):
                self.check(proof)

    def test_linear_alias_cannot_replace_live_cs_frame(self):
        proof = copy.deepcopy(self.proof)
        proof["symbols"]["_helper"].update(segment=0, offset=321)
        with self.assertRaisesRegex(ValueError, "caller-established CS frame"):
            self.check(proof)

    def test_target_cannot_be_selected_from_candidate_displacement(self):
        proof = copy.deepcopy(self.proof)
        proof["symbols"]["_helper"]["offset"] += 1
        with self.assertRaisesRegex(ValueError, "grounded near entry"):
            self.check(proof)

    def test_original_candidate_field_is_not_masked(self):
        self.mutate_original(578, struct.pack("<h", -258))
        self.recipe["target_sha256"] = sha(self.space.data[576:580])
        with self.assertRaisesRegex(ValueError, "near equations differ"):
            self.check()

    def test_original_ordinary_byte_is_not_normalized(self):
        self.mutate_original(576, b"\x91")
        self.recipe["target_sha256"] = sha(self.space.data[576:580])
        with self.assertRaisesRegex(ValueError, "near equations differ"):
            self.check()

    def test_far_entry_anchor_needs_exact_relocation_membership(self):
        self.space = replace(self.space, relocations=self.space.relocations+({"image_offset": 19},))
        self.oracle.spaces["root"] = self.space
        with self.assertRaisesRegex(ValueError, "relocation correspondence"):
            self.check()

    def test_original_mz_relocation_at_operand_or_owned_boundary_is_rejected(self):
        for at in (578, 575):
            space = replace(self.space, relocations=self.space.relocations+({"image_offset": at},))
            self.oracle.spaces["root"] = space
            with self.assertRaisesRegex(ValueError, "ungenerated relocation"):
                self.check()

    def test_provider_signature_and_internal_equation_are_both_verified(self):
        for at, raw in ((321, b"\x51"), (323, b"\x03\x00")):
            original_space = self.space
            self.mutate_original(at, raw)
            proof = copy.deepcopy(self.proof)
            proof["symbols"]["_helper"]["provider"]["prefix_sha256"] = sha(self.space.data[320:328])
            with self.assertRaisesRegex(ValueError, "ordinary context|internal near equation"):
                self.check(proof)
            self.space = original_space
            self.oracle.spaces["root"] = original_space

    def test_provider_member_and_public_identity_are_pinned(self):
        proof = copy.deepcopy(self.proof)
        proof["symbols"]["_helper"]["provider"]["member_sha256"] = "other"
        with self.assertRaisesRegex(ValueError, "member identity changed"):
            self.check(proof)

    def test_duplicate_original_provider_signature_is_rejected(self):
        self.mutate_original(800, self.space.data[320:328])
        with self.assertRaisesRegex(ValueError, "signature is ambiguous"):
            self.check()

    def test_conditional_wrapper_path_is_static_evidence(self):
        raw = b"\x72\x01\x90\xe9"+struct.pack("<h", 0x41-0x86)
        self.mutate_original(384, raw)
        proof = copy.deepcopy(self.proof)
        proof["symbols"]["_helper"]["incoming_wrapper"].update(bytes_hex=raw.hex(), path=[0, 3])
        self.assertTrue(self.check(proof)["exact"])
        # Change branch edge while keeping the tail jump destination identical.
        raw = b"\x72\x00\x90\xe9"+struct.pack("<h", 0x41-0x86)
        self.mutate_original(384, raw)
        proof["symbols"]["_helper"]["incoming_wrapper"]["bytes_hex"] = raw.hex()
        with self.assertRaisesRegex(ValueError, "invalid control-flow edge"):
            self.check(proof)

    def test_wrapper_witness_cannot_supply_a_noninstruction_path(self):
        proof = copy.deepcopy(self.proof)
        proof["symbols"]["_helper"]["incoming_wrapper"]["path"] = [0, 2, 1]
        with self.assertRaisesRegex(ValueError, "bounded entry-to-tail path"):
            self.check(proof)

    def test_raw_addend_and_explicit_zero_displacement_are_not_normalized(self):
        for code, body, message in ((b"\x90\xe9\x01\x00", b"\x84\x02\x06\x01\x01", "zero-addend"),
                                    (self.raw_code, b"\x84\x02\x02\x01\x01\x00\x00", "Unsupported/overlapping")):
            owner = object_with_fixup(code, body)
            library = owner+self.provider
            self.library.write_bytes(library)
            self.recipe.update(library_sha256=sha(library), member_extent=len(owner), member_sha256=sha(owner))
            proof = copy.deepcopy(self.proof)
            proof["fixups"] = fixes(OmfReader().read(owner))
            proof["symbols"]["_helper"]["provider"].update(library_sha256=sha(library), archive_offset=len(owner))
            with self.assertRaisesRegex(ValueError, message):
                self.check(proof)

    def test_fixup_kind_frame_displacement_and_order_are_frozen(self):
        for key, value in (("loc_type", 5), ("self_relative", False), ("displacement", 0)):
            proof = copy.deepcopy(self.proof)
            proof["fixups"][0][key] = value
            with self.assertRaisesRegex(ValueError, "ordered fixups changed"):
                self.check(proof)

    def test_full_provider_with_secondary_storage_cannot_gain_trimmed_ownership(self):
        self.recipe.update(archive_offset=len(self.owner), member_extent=len(self.provider),
                           member_sha256=sha(self.provider), size=8, publics=["_helper"])
        with self.assertRaisesRegex(ValueError, "unowned secondary storage"):
            self.check()

    def test_verify_only_runtime_publication_leaves_manifest_and_recipe_untouched(self):
        self.check()
        submitted = self.root/"build/workers/test/submitted.json"
        manifest = self.root/"layout/manifest.json"
        write_json(submitted, self.recipe)
        write_json(manifest, {"owners": []})
        before = manifest.read_bytes()
        with patch.object(common, "ROOT", self.root), patch.object(pin_runtime, "ROOT", self.root), \
             patch.object(pin_runtime.Oracle, "load", return_value=self.oracle):
            report = pin_runtime.pin(submitted, verify_only=True)
        self.assertTrue(report["exact"])
        self.assertEqual(before, manifest.read_bytes())
        self.assertFalse((self.root/"recipes/runtime/test_near.json").exists())

    def test_runtime_publication_rolls_back_a_physical_alias_overlap(self):
        self.check()
        old = copy.deepcopy(self.recipe)
        old.update(segment=0, offset=576)
        write_json(self.root/"recipes/runtime/old.json", old)
        manifest = self.root/"layout/manifest.json"
        owner = {"space": "root", "segment": 0, "offset": 576, "size": 4,
                 "target_id": "root:0000:0240", "kind": "PINNED_RUNTIME", "state": "PINNED_RUNTIME",
                 "recipe": "recipes/runtime/old.json", "fixup_count": 1, "relocation_count": 0}
        write_json(manifest, {"owners": [owner]})
        submitted = self.root/"build/workers/test/submitted.json"
        write_json(submitted, self.recipe)
        before = manifest.read_bytes()
        with patch.object(common, "ROOT", self.root), patch.object(pin_runtime, "ROOT", self.root), \
             patch.object(pin_runtime.Oracle, "load", return_value=self.oracle):
            with self.assertRaisesRegex(ValueError, "Overlapping canonical owners"):
                pin_runtime.pin(submitted)
        self.assertEqual(before, manifest.read_bytes())
        self.assertFalse((self.root/"recipes/runtime/test_near.json").exists())
        self.assertTrue((self.root/"recipes/runtime/old.json").exists())


if __name__ == "__main__":
    unittest.main()

"""Independent original witnesses and adversarial real OMF obligations."""
import copy
from dataclasses import replace
import struct
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from binding import declarations, fixes
from common import sha
from match import check_object
from omf import OmfReader
from oracle import Space
from test_match import make_omf, omf_record


def bound_object(code=b"\xa1\x00\x00\xcb", *, fix_body=b"\xc4\x01\x56\x01"):
    raw = make_omf(code=code, extra_segments=(("_DATA", "DATA", 0),), externals=("_data",))
    records = []
    for kind, body, _ in OmfReader.records(raw):
        if kind == 0x8a:
            continue
        if kind == 0xa0:
            records += [omf_record(0x96, b"\x06DGROUP"), omf_record(0x9a, b"\x05\xff\x02")]
        records.append(omf_record(kind, body))
    if fix_body:
        records.append(omf_record(0x9c, fix_body))
    records.append(omf_record(0x8a))
    return b"".join(records)


class BindingTests(unittest.TestCase):
    def setUp(self):
        self.raw = bound_object()
        self.expected = b"\xa1\x10\x00\xcb"
        data = bytearray(512)
        data[10:15] = b"\xbb\x10\x00\x8e\xdb"
        data[40:43] = b"\xa1\x10\x00"
        data[64:68] = self.expected
        self.space = Space("root", 0, 0, bytes(data), ({"image_offset": 11},))
        self.oracle = SimpleNamespace(data=b"independent oracle identity", spaces={"root": self.space},
                                      mz=SimpleNamespace(header={"initial_cs": 0, "initial_ip": 10}))
        self.target = {"id": "test", "space": "root", "segment": 0, "offset": 64, "size": 4}
        obj = OmfReader().read(self.raw)
        frame = {"space": "root", "image_offset": 10, "bytes_hex": "bb10008edb"}
        self.proof = {"schema": 1, "mode": "external-dgroup-offset16-v1", "target_id": "test",
                      "target_sha256": sha(self.oracle.data), "declarations": declarations(obj),
                      "fixups": fixes(obj), "frame": {"segment": 16, "witness": frame, "entry_path": frame},
                      "symbols": {"_data": {"kind": "near-data-alias", "group": "DGROUP", "offset": 16,
                          "width": 2, "witnesses": [{"space": "root", "image_offset": 40,
                          "bytes_hex": "a11000", "operand_offset": 1, "ds_context_evidence": "reviewed"}]}}}

    def check(self, raw=None, proof=None, expected=None):
        return check_object(raw or self.raw, self.target, self.space,
                            self.expected if expected is None else expected, "_target",
                            binding=self.proof if proof is None else proof, oracle=self.oracle)

    def test_whole_extent_and_linker_equation_pass_without_editing_raw(self):
        before = bytes(self.raw)
        result = self.check()
        self.assertTrue(result["exact"])
        self.assertEqual(result["fixups"], 1)
        self.assertEqual(result["ordinary_bytes_compared"], 2)
        self.assertEqual(result["equations"][0]["linked_value"], 16)
        self.assertNotEqual(result["emitted_sha256"], result["expected_sha256"])
        self.assertEqual(before, self.raw)

    def test_wrong_field_is_not_masked(self):
        result = self.check(expected=b"\xa1\x11\x00\xcb")
        self.assertFalse(result["exact"])
        self.assertEqual(result["first_difference"], 1)

    def test_context_annotation_cannot_supply_runtime_frame_authority(self):
        proof = copy.deepcopy(self.proof)
        del proof["symbols"]["_data"]["witnesses"][0]["ds_context_evidence"]
        result = self.check(proof=proof)
        self.assertTrue(result["exact"])
        self.assertIn("runtime DS association", result["proof_scope"])
        self.assertNotIn("symbol_image_offset", result["equations"][0])

    def test_wrong_ordinary_byte_is_not_normalized(self):
        result = self.check(expected=b"\xa1\x10\x00\xc3")
        self.assertFalse(result["exact"])
        self.assertEqual(result["first_difference"], 3)

    def test_binding_oracle_identity_is_independent(self):
        proof = copy.deepcopy(self.proof)
        proof["target_sha256"] = "different"
        with self.assertRaisesRegex(ValueError, "another target/oracle"):
            self.check(proof=proof)

    def test_symbol_cannot_be_fitted_from_target_field(self):
        proof = copy.deepcopy(self.proof)
        proof["symbols"]["_data"]["offset"] = 17
        with self.assertRaisesRegex(ValueError, "does not ground"):
            self.check(proof=proof)

    def test_candidate_cannot_serve_as_its_own_witness(self):
        proof = copy.deepcopy(self.proof)
        proof["symbols"]["_data"]["witnesses"][0]["image_offset"] = 64
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            self.check(proof=proof)

    def test_frame_requires_original_relocation(self):
        self.space = replace(self.space, relocations=())
        self.oracle.spaces["root"] = self.space
        with self.assertRaisesRegex(ValueError, "relocated startup DS load"):
            self.check()

    def test_frame_requires_mz_entry_reachability(self):
        self.oracle.mz.header["initial_ip"] = 0
        with self.assertRaisesRegex(ValueError, "straight-line MZ entry"):
            self.check()

    def test_original_relocation_cannot_be_filled_by_offset_fixup(self):
        self.space = replace(self.space, relocations=self.space.relocations+({"image_offset": 65},))
        self.oracle.spaces["root"] = self.space
        with self.assertRaisesRegex(ValueError, "not generated by offset16"):
            self.check()

    def test_ordered_fixups_and_declarations_are_frozen(self):
        for key in ("fixups", "declarations"):
            proof = copy.deepcopy(self.proof)
            proof[key] = []
            with self.assertRaisesRegex(ValueError, "changed"):
                self.check(proof=proof)

    def test_nonzero_raw_addend_is_not_accepted_by_fitting_value(self):
        raw = bound_object(code=b"\xa1\x01\x00\xcb")
        with self.assertRaisesRegex(ValueError, "zero encoded data addends"):
            self.check(raw=raw)

    def test_segment_change_and_unknown_call_are_rejected(self):
        for tail in (b"\x1f", b"\xcd\x21", b"\xe8\x00\x00"):
            raw = bound_object(code=b"\xa1\x00\x00"+tail)
            obj = OmfReader().read(raw)
            proof = copy.deepcopy(self.proof)
            proof["declarations"] = declarations(obj)
            with self.assertRaisesRegex(ValueError, "segment context|unknown code"):
                self.check(raw=raw, proof=proof, expected=b"\xa1\x10\x00"+tail)

    def test_hardcoded_address_without_symbolic_fixup_is_rejected(self):
        raw = bound_object(code=b"\xa1\x10\x00\xcb", fix_body=b"")
        with self.assertRaisesRegex(ValueError, "Unused binding proof"):
            self.check(raw=raw)

    def test_loader_resolved_offset16_is_not_ordinary_offset16(self):
        raw = bound_object(fix_body=b"\xd4\x01\x56\x01")
        proof = copy.deepcopy(self.proof)
        proof["fixups"] = fixes(OmfReader().read(raw))
        with self.assertRaisesRegex(ValueError, "Unsupported DGROUP fixup"):
            self.check(raw=raw, proof=proof)

    def test_explicit_zero_displacement_remains_distinct_from_omission(self):
        raw = bound_object(fix_body=b"\xc4\x01\x52\x01\x00\x00")
        proof = copy.deepcopy(self.proof)
        proof["fixups"] = fixes(OmfReader().read(raw))
        with self.assertRaisesRegex(ValueError, "Unsupported DGROUP fixup"):
            self.check(raw=raw, proof=proof)

    def test_identical_fixup_fields_cannot_overlap(self):
        raw = bound_object(fix_body=b"\xc4\x01\x56\x01"*2)
        proof = copy.deepcopy(self.proof)
        proof["fixups"] = fixes(OmfReader().read(raw))
        with self.assertRaisesRegex(ValueError, "Overlapping/out-of-range"):
            self.check(raw=raw, proof=proof)

    def test_relocation_word_straddling_owned_start_is_rejected(self):
        self.space = replace(self.space, relocations=self.space.relocations+({"image_offset": 63},))
        self.oracle.spaces["root"] = self.space
        with self.assertRaisesRegex(ValueError, "not generated by offset16"):
            self.check()


if __name__ == "__main__":
    unittest.main()

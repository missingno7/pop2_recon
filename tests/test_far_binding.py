"""Root CALL FAR proof fixtures, including adversarial full OMF obligations."""
import copy
from dataclasses import replace
from pathlib import Path
import struct
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from binding import declarations, fixes
from common import sha
from match import check_object
from omf import OmfReader
from oracle import Space
from test_match import make_omf, omf_record
import test_binding


def far_object(code=bytes.fromhex('9a00000000cb'), fix_body=bytes.fromhex('cc015601'), mixed=False):
    raw = make_omf(code=code, externals=('_callee', '_data') if mixed else ('_callee',),
                   extra_segments=(('_DATA', 'DATA', 0),) if mixed else ())
    records = []
    for kind, body, _ in OmfReader.records(raw):
        if kind == 0x8a:
            continue
        if mixed and kind == 0xa0:
            records += [omf_record(0x96, b'\x06DGROUP'), omf_record(0x9a, b'\x05\xff\x02')]
        records.append(omf_record(kind, body))
    records += [omf_record(0x9c, fix_body), omf_record(0x8a)]
    return b''.join(records)


class FarBindingTests(unittest.TestCase):
    def setUp(self):
        self.raw = far_object()
        self.expected = bytes.fromhex('9a60000000cb')
        image = bytearray(512)
        image[32:37] = self.expected[:5]
        image[64:70] = self.expected
        image[96] = 0xcb
        self.space = Space('root', 0, 1000, bytes(image),
                           ({'index': 0, 'image_offset': 35}, {'index': 1, 'image_offset': 67}))
        self.oracle = SimpleNamespace(data=b'independent far oracle', spaces={'root': self.space})
        self.target = {'id': 'owner', 'space': 'root', 'segment': 0, 'offset': 64, 'size': 6}
        self.entry = {'id': 'callee', 'space': 'root', 'segment': 0, 'offset': 96, 'size': 1,
                      'sha256': sha(b'\xcb'), 'entry_kind': 'direct-code'}
        obj = OmfReader().read(self.raw)
        self.proof = {'schema': 1, 'mode': 'external-far-call-v1', 'target_id': 'owner',
                      'target_sha256': sha(self.oracle.data), 'declarations': declarations(obj),
                      'fixups': fixes(obj), 'relocation_indices': [1],
                      'symbols': {'_callee': {'kind': 'far-code-entry', 'space': 'root',
                          'segment': 0, 'offset': 96, 'entry_target_id': 'callee',
                          'entry_sha256': sha(b'\xcb'), 'witnesses': [{'space': 'root',
                          'image_offset': 32, 'file_offset': 1032, 'bytes_hex': '9a60000000',
                          'relocation_index': 0}]}}}

    def check(self, raw=None, proof=None, expected=None):
        with patch('match.get_target', return_value=(self.entry, self.space, b'\xcb')):
            return check_object(self.raw if raw is None else raw, self.target, self.space,
                                self.expected if expected is None else expected, '_target',
                                binding=self.proof if proof is None else proof, oracle=self.oracle)

    def freeze(self, raw):
        self.proof.update(declarations=declarations(OmfReader().read(raw)), fixups=fixes(OmfReader().read(raw)))

    def test_complete_far_equation_and_original_relocation_index_without_raw_edit(self):
        before = bytes(self.raw)
        r = self.check()
        self.assertTrue(r['exact'])
        self.assertEqual(r['covered_relocation_indices'], [1])
        self.assertEqual((r['relocations'], r['fixups'], r['ordinary_bytes_compared']), (1, 1, 2))
        self.assertEqual(r['equations'][0]['linked_offset'], 96)
        self.assertEqual(before, self.raw)
        self.assertNotEqual(r['emitted_sha256'], r['expected_sha256'])

    def test_nonzero_raw_addend_refused(self):
        with self.assertRaisesRegex(ValueError, 'zero encoded far addends'):
            self.check(raw=far_object(bytes.fromhex('9a01000000cb')))

    def test_wrong_frame_and_explicit_zero_displacement_refused(self):
        for body in ('cc01260101', 'cc0152010000'):
            raw = far_object(fix_body=bytes.fromhex(body)); self.freeze(raw)
            with self.assertRaisesRegex(ValueError, 'Unsupported far CALL fixup'):
                self.check(raw=raw)

    def test_non_pointer32_and_self_relative_refused(self):
        for body in ('c4015601', '8c015601'):
            raw = far_object(fix_body=bytes.fromhex(body)); self.freeze(raw)
            with self.assertRaisesRegex(ValueError, 'Unsupported far CALL fixup'):
                self.check(raw=raw)

    def test_callee_requires_frozen_reviewed_direct_entry(self):
        for key, value in (('entry_kind', 'rtlink-vector'), ('offset', 97), ('sha256', 'wrong')):
            entry = copy.deepcopy(self.entry); self.entry[key] = value
            with self.assertRaisesRegex(ValueError, 'reviewed direct-code entry'):
                self.check()
            self.entry = entry

    def test_unreviewed_callee_refused_by_fresh_target_lookup(self):
        with patch('match.get_target', side_effect=ValueError('Unknown/ambiguous reviewed target')):
            with self.assertRaisesRegex(ValueError, 'reviewed target'):
                check_object(self.raw, self.target, self.space, self.expected, '_target',
                             binding=self.proof, oracle=self.oracle)

    def test_no_independent_witness_refused(self):
        self.proof['symbols']['_callee']['witnesses'] = []
        with self.assertRaisesRegex(ValueError, 'no independent'):
            self.check()

    def test_owner_cannot_supply_its_own_callee_grounding(self):
        w = self.proof['symbols']['_callee']['witnesses'][0]
        w.update(image_offset=64, file_offset=1064, relocation_index=1)
        with self.assertRaisesRegex(ValueError, 'witness overlaps candidate'):
            self.check()

    def test_missing_extra_off_by_one_duplicate_and_boundary_straddling_relocation_refused(self):
        original = self.space
        variants = [(), ({'index': 1, 'image_offset': 66},),
                    ({'index': 1, 'image_offset': 67}, {'index': 2, 'image_offset': 69}),
                    ({'index': 1, 'image_offset': 67}, {'index': 2, 'image_offset': 63}),
                    ({'index': 1, 'image_offset': 67}, {'index': 2, 'image_offset': 67})]
        for relocs in variants:
            self.space = replace(original, relocations=(original.relocations[0],)+relocs)
            with self.assertRaisesRegex(ValueError, 'Original MZ relocation site set'):
                self.check()
        self.space = original

    def test_segment_and_offset_word_mismatch_are_equations_not_masking(self):
        for code in ('9a60000100cb', '9a61000000cb'):
            r = self.check(expected=bytes.fromhex(code))
            self.assertFalse(r['exact']); self.assertEqual(r['field_mismatch_count'], 1)
            self.assertEqual(r['covered_relocation_indices'], [])
        r = self.check(expected=bytes.fromhex('9a60000000c3'))
        self.assertFalse(r['exact']); self.assertEqual(r['ordinary_mismatch_count'], 1)
        self.assertEqual(r['covered_relocation_indices'], [])

    def test_overlay_owner_and_overlay_symbol_refused(self):
        self.target['space'] = 'overlay-2'
        with self.assertRaisesRegex(ValueError, 'root owner'):
            self.check()
        self.target['space'] = 'root'
        self.proof['symbols']['_callee']['space'] = 'overlay-2'
        with self.assertRaisesRegex(ValueError, 'overlay/vector'):
            self.check()

    def test_non_call_pointer_and_prefixed_call_refused(self):
        for code, body in [('ea00000000cb', 'cc015601'), ('669a00000000cb', 'cc025601')]:
            raw = far_object(bytes.fromhex(code), bytes.fromhex(body)); self.freeze(raw)
            self.target['size'] = len(bytes.fromhex(code))
            with self.assertRaises(ValueError):
                self.check(raw=raw, expected=bytes.fromhex(code))

    def test_ungenerated_literal_far_call_and_indirect_near_call_refused(self):
        for tail in ('9a60000000', 'ffd0', '0ee80000'):
            raw = far_object(bytes.fromhex('9a00000000'+tail+'cb')); self.freeze(raw)
            self.target['size'] = len(bytes.fromhex('9a00000000'+tail+'cb'))
            with self.assertRaisesRegex(ValueError, 'Unproved call'):
                self.check(raw=raw, expected=bytes.fromhex('9a60000000'+tail+'cb'))

    def test_witness_relocation_file_alias_and_bytes_are_fresh(self):
        w = self.proof['symbols']['_callee']['witnesses'][0]
        for key, value in [('relocation_index', 1), ('file_offset', 32),
                           ('bytes_hex', '9a61000000'), ('space', 'overlay-2')]:
            original = copy.deepcopy(w); w[key] = value
            with self.assertRaises(ValueError): self.check()
            w.clear(); w.update(original)
        self.space = replace(self.space, relocations=self.space.relocations[1:])
        self.oracle.spaces['root'] = self.space  # witnesses are read from the oracle by space
        with self.assertRaisesRegex(ValueError, 'provenance'):
            self.check()

    def test_relocated_overlay_call_is_an_independent_witness(self):
        overlay = Space('overlay-2', 0x40, 2000, bytes(16) + self.expected[:5] + bytes(11),
                        ({'index': 7, 'image_offset': 19},))
        self.oracle.spaces['overlay-2'] = overlay
        self.proof['symbols']['_callee']['witnesses'] = [
            {'space': 'overlay-2', 'image_offset': 16, 'bytes_hex': self.expected[:5].hex(),
             'relocation_index': 7, 'file_offset': 2016}]
        self.assertTrue(self.check()['exact'])

    def test_full_extent_no_trimming_and_ordered_declarations_remain_frozen(self):
        for key in ('fixups', 'declarations', 'relocation_indices'):
            proof = copy.deepcopy(self.proof); proof[key] = []
            with self.assertRaises(ValueError): self.check(proof=proof)
        with self.assertRaisesRegex(ValueError, 'extent differs'):
            self.check(expected=self.expected[:-1])

    def test_multiple_far_fields_preserve_fixupp_and_original_order(self):
        code = bytes.fromhex('9a000000009a00000000cb')
        raw = far_object(code, bytes.fromhex('cc065601cc015601')); self.freeze(raw)
        self.target['size'] = len(code)
        self.proof['relocation_indices'] = [1, 2]
        self.space = replace(self.space, relocations=(self.space.relocations[0],
                            {'index': 1, 'image_offset': 72}, {'index': 2, 'image_offset': 67}))
        expected = bytes.fromhex('9a600000009a60000000cb')
        r = self.check(raw=raw, expected=expected)
        self.assertTrue(r['exact']); self.assertEqual([e['offset'] for e in r['equations']], [6, 1])
        self.space = replace(self.space, relocations=(self.space.relocations[0],
                            {'index': 1, 'image_offset': 67}, {'index': 2, 'image_offset': 72}))
        with self.assertRaisesRegex(ValueError, 'within-symbol FIXUPP order'): self.check(raw=raw, expected=expected)

    def test_overlapping_far_fixups_refused(self):
        raw = far_object(fix_body=bytes.fromhex('cc015601cc015601')); self.freeze(raw)
        with self.assertRaisesRegex(ValueError, 'Overlapping'): self.check(raw=raw)

    def test_composition_uses_unchanged_zero_addend_dgroup_equation(self):
        baseline = test_binding.BindingTests(); baseline.setUp()
        code = bytes.fromhex('9a00000000a10000cb')
        raw = far_object(code, bytes.fromhex('cc015601c4065602'), mixed=True); self.freeze(raw)
        self.target['size'] = len(code)
        image = bytearray(self.space.data)
        image[10:15] = baseline.space.data[10:15]; image[40:43] = baseline.space.data[40:43]
        self.space = replace(self.space, data=bytes(image),
                             relocations=({'index': 2, 'image_offset': 11},)+self.space.relocations)
        self.oracle.spaces['root'] = self.space
        self.oracle.mz = baseline.oracle.mz
        self.proof['symbols']['_data'] = baseline.proof['symbols']['_data']
        self.proof['data'] = {k: baseline.proof[k] for k in ('mode', 'frame')}
        r = self.check(raw=raw, expected=bytes.fromhex('9a60000000a11000cb'))
        self.assertTrue(r['exact']); self.assertEqual((r['relocations'], r['fixups']), (1, 2))
        self.assertEqual([e['kind'] for e in r['equations']], ['far-call', 'dgroup-offset16'])
        raw = far_object(bytes.fromhex('9a00000000a10100cb'), bytes.fromhex('cc015601c4065602'), mixed=True)
        with self.assertRaisesRegex(ValueError, 'zero encoded data addends'):
            self.check(raw=raw, expected=bytes.fromhex('9a60000000a11100cb'))

    def test_nonintegral_or_outside_code_alias_refused(self):
        for value in (96.0, True, -1, 65536):
            self.proof['symbols']['_callee']['offset'] = value
            with self.assertRaisesRegex(ValueError, 'exact 16:16'):
                self.check()

    def test_literal_ds_address_cannot_bypass_data_composition(self):
        raw = far_object(bytes.fromhex('9a00000000a11000cb')); self.freeze(raw)
        self.target['size'] = 9
        with self.assertRaisesRegex(ValueError, 'lacks a symbolic'):
            self.check(raw=raw, expected=bytes.fromhex('9a60000000a11000cb'))

    def test_foreign_oracle_or_target_refused(self):
        for key in ('target_id', 'target_sha256'):
            proof = copy.deepcopy(self.proof); proof[key] = 'foreign'
            with self.assertRaisesRegex(ValueError, 'foreign far-call'):
                self.check(proof=proof)

    def test_metrics_report_actual_indices_and_reject_duplicate_or_count_only_claims(self):
        from metrics import metrics
        owner = dict(self.target, target_id='owner', kind='MATCHING_C',
                     relocation_count=1, relocation_indices=[1])
        manifest = {'owners': [owner], 'structural_link_status': 'UNRECOVERED',
                    'behavior_oracle_status': 'UNVALIDATED'}
        with patch('metrics.read_json', return_value={'functions': []}):
            r = metrics(self.oracle, manifest)
            self.assertEqual(r['spaces'][0]['covered_relocation_indices'], [1])
            self.assertEqual(r['proven_relocation_records'], 1)
            manifest['owners'].append(owner)
            with self.assertRaisesRegex(ValueError, 'Invalid/duplicate'):
                metrics(self.oracle, manifest)
            manifest['owners'] = [{**owner, 'relocation_indices': []}]
            with self.assertRaisesRegex(ValueError, 'Invalid/duplicate'):
                metrics(self.oracle, manifest)

    def test_word_pair_data_composes_and_keeps_member_witnesses(self):
        baseline = test_binding.WordPairBindingTests(); baseline.setUp()
        raw = far_object(bytes.fromhex('9a00000000a102000b060000cb'),
                         bytes.fromhex('cc015601c4065602c40a5602'), mixed=True)
        self.freeze(raw); self.target['size'] = 13
        image = bytearray(self.space.data)
        image[10:15] = baseline.space.data[10:15]
        image[40:47] = baseline.space.data[40:47]
        self.space = replace(self.space, data=bytes(image),
                             relocations=({'index': 2, 'image_offset': 11},)+self.space.relocations)
        self.oracle.spaces['root'] = self.space; self.oracle.mz = baseline.oracle.mz
        self.proof['symbols']['_data'] = copy.deepcopy(baseline.proof['symbols']['_data'])
        self.proof['data'] = {k: baseline.proof[k] for k in ('mode', 'frame')}
        expected = bytes.fromhex('9a60000000a112000b061000cb')
        r = self.check(raw=raw, expected=expected)
        self.assertTrue(r['exact'])
        self.assertEqual([e['encoded_addend'] for e in r['equations']], [0, 2, 0])
        self.proof['symbols']['_data']['members'][1]['witnesses'][0]['image_offset'] = 64
        with self.assertRaisesRegex(ValueError, 'overlaps candidate'):
            self.check(raw=raw, expected=expected)

    def test_manifest_and_recipe_cannot_claim_other_relocation_indices(self):
        from validate import check_manifest
        owner = dict(self.target, target_id='owner', kind='MATCHING_C', state='CODE_EXACT',
                     recipe='recipes/test.json', relocation_count=1, relocation_indices=[1])
        recipe = {'target_id': 'owner', 'relocation_indices': [0]}
        with patch('validate.get_target', return_value=(self.target, self.space, self.expected)), \
             patch('validate.read_json', return_value=recipe):
            with self.assertRaisesRegex(ValueError, 'original relocation indices'):
                check_manifest(self.oracle, {'owners': [owner]})
            recipe['relocation_indices'] = [1]; owner['relocation_count'] = 0
            with self.assertRaisesRegex(ValueError, 'original relocation indices'):
                check_manifest(self.oracle, {'owners': [owner]})


if __name__ == '__main__':
    unittest.main()


class FarCalleeKindTests(unittest.TestCase):
    """RTLink vectors and overlay entries ground like resident entries; nothing else does."""

    def ground(self, **symbol):
        from far_binding import ground_symbol
        from oracle import Oracle
        target = {"id": "probe", "space": "root", "segment": 0, "offset": 0, "size": 2}
        base = {"witnesses": [{"space": "root", "image_offset": 10987, "bytes_hex": "9adb0c0322",
                               "relocation_index": 248, "file_offset": 8192 + 10987}]}
        return ground_symbol({**base, **symbol}, target, Oracle.load())

    def test_original_rtlink_vector_is_grounded_with_its_destination(self):
        result = self.ground(kind="rtlink-vector", space="root", segment=0x2203, offset=0x0cdb)
        self.assertEqual(result["vector_destination"]["target_space"], "overlay-2")

    def test_address_that_is_not_a_vector_is_refused(self):
        with self.assertRaisesRegex(ValueError, "not an original RTLink vector"):
            self.ground(kind="rtlink-vector", space="root", segment=0x2203, offset=0x0cdc)


class AcceptedEntryTests(unittest.TestCase):
    """A far callee with no outside caller is grounded only by an accepted exact entry."""

    def ground(self, entry_target_id):
        from far_binding import ground_symbol
        from match import get_target
        from oracle import Oracle
        oracle = Oracle.load()
        callee, _, _ = get_target(entry_target_id, oracle)
        symbol = {"kind": "far-code-entry", "space": "root", "segment": callee["segment"],
                  "offset": callee["offset"], "entry_target_id": entry_target_id,
                  "entry_sha256": callee["sha256"], "witnesses": []}
        target = {"id": "probe", "space": "root", "segment": 0, "offset": 0, "size": 2}
        return ground_symbol(symbol, target, oracle)

    def test_accepted_owner_grounds_its_entry(self):
        self.assertEqual(self.ground("root:02cc:0002")["grounded_by"], "accepted exact entry")

    def test_reviewed_but_unaccepted_entry_needs_a_witness(self):
        # Any reviewed root direct-code entry that no canonical owner starts at, chosen at run time
        # because accepted owners grow with every wave.
        from common import ROOT, read_json
        from far_binding import _accepted_entry
        from oracle import Oracle
        oracle = Oracle.load()
        from far_binding import _structural_entry

        def structural(t):
            try:
                at = oracle.spaces["root"].position(t["segment"], t["offset"])
                _structural_entry(oracle, at, t["segment"], t["offset"], t["size"], "root", 0, 0)
                return True
            except ValueError:
                return False
        unaccepted = next(t["id"] for t in read_json(ROOT / "evidence/targets.json")["functions"]
                          if t["space"] == "root" and t.get("entry_kind") == "direct-code" and
                          not _accepted_entry(t["id"], oracle) and not structural(t))
        with self.assertRaisesRegex(ValueError, "accepted exact entry"):
            self.ground(unaccepted)


class StructuralEntryTests(unittest.TestCase):
    """A resident callee whose only caller is the candidate may be grounded by original-byte
    structure alone; every refusal below is a shape measured in the real image."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        from far_binding import _extents, _structural_entry
        cls.oracle = Oracle.load()
        cls.extents = _extents(cls.oracle)
        cls.check = staticmethod(_structural_entry)
        cls.entry = None
        for space, lo, hi, ident, accepted in cls.extents:
            if space != "root" or accepted:
                continue
            _, seg, off = ident.split(":")
            try:
                cls.check(cls.oracle, lo, int(seg, 16), int(off, 16), hi - lo, "root", 0, 0)
            except ValueError:
                continue
            cls.entry = (lo, int(seg, 16), int(off, 16), hi - lo)
            break

    def ground(self, entry=None, start=0, end=0, extents=None):
        from far_binding import _structural_entry
        lo, seg, off, size = entry or self.entry
        with patch("far_binding._extents", return_value=extents if extents is not None else self.extents):
            return _structural_entry(self.oracle, lo, seg, off, size, "root", start, end)

    def test_a_qualifying_entry_records_its_anchor_and_frame_witness(self):
        receipt = self.ground()
        self.assertIn(receipt["anchor"]["pad"], (0, 1))
        self.assertGreaterEqual(receipt["frame_witnesses"], 1)

    def test_mid_routine_retf_nop_label_is_refused(self):
        # MSC duplicates frameless epilogues: 078a:2830 has `retf 2; nop; mov ax,1` at 0xa104.
        with self.assertRaisesRegex(ValueError, "prologue"):
            self.ground((0xa104, 0x078a, 0xa104 - 0x78a0, 4))

    def test_entry_inside_another_extent_is_refused(self):
        lo = self.entry[0]
        with self.assertRaisesRegex(ValueError, "inside a reviewed or accepted extent"):
            self.ground(extents=self.extents + [("root", lo - 2, lo + 2, "probe", False)])

    def test_entry_overlapping_accepted_code_is_refused(self):
        lo = self.entry[0]
        with self.assertRaisesRegex(ValueError, "overlaps accepted code"):
            self.ground(extents=self.extents + [("root", lo + 1, lo + 3, "probe", True)])

    def test_without_a_reviewed_return_anchor_it_is_refused(self):
        lo = self.entry[0]
        kept = [e for e in self.extents if e[0] != "root" or e[2] not in (lo, lo - 1)]
        with self.assertRaisesRegex(ValueError, "decoded return"):
            self.ground(extents=kept)

    def test_frame_witness_must_lie_outside_the_candidate(self):
        root = self.oracle.spaces["root"]
        with self.assertRaisesRegex(ValueError, "segment word"):
            self.ground(start=0, end=root.size)

    def test_rtlink_region_is_refused(self):
        root = self.oracle.spaces["root"]
        with self.assertRaisesRegex(ValueError, "RTLink"):
            self.ground((root.position(0x2203, 0x0cbd), 0x2203, 0x0cbd, 10))


class VectorRecordTests(unittest.TestCase):
    """An RTLink vector with no outside caller is grounded by its own decoded record when the
    destination starts a reviewed target; otherwise it is refused."""

    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        from far_binding import _extents
        from tu_census import overlay_vectors
        cls.oracle = Oracle.load()
        overlays = {n: s for n, s in cls.oracle.spaces.items() if s.descriptor and s.descriptor["flags"] == 4}
        cls.extents = _extents(cls.oracle)
        starts = {(name, lo) for name, lo, _, _, _ in cls.extents}
        vectors = overlay_vectors(cls.oracle, overlays)
        cls.unreviewed, record = next((k, v) for k, v in vectors.items()
                                      if (v["target_space"], v["target_position"]) not in starts)
        # A reviewed row at that destination (injected: rows are added as overlays are reviewed).
        cls.with_row = cls.extents + [(record["target_space"], record["target_position"],
                                       record["target_position"] + 2, "probe-destination", False)]

    def ground(self, key, extents=None):
        from far_binding import ground_symbol
        symbol = {"kind": "rtlink-vector", "space": "root", "segment": key[0], "offset": key[1], "witnesses": []}
        with patch("far_binding._extents", return_value=extents if extents is not None else self.extents):
            return ground_symbol(symbol, {"id": "probe", "space": "root", "segment": 0, "offset": 0, "size": 2},
                                 self.oracle)

    def test_vector_to_a_reviewed_destination_is_grounded_by_its_record(self):
        self.assertEqual(self.ground(self.unreviewed, self.with_row)["grounded_by"], "vector-record")

    def test_vector_to_an_unreviewed_destination_is_refused(self):
        with self.assertRaisesRegex(ValueError, "does not start a reviewed target"):
            self.ground(self.unreviewed)


class SegmentBaseTests(unittest.TestCase):
    """A far data segment N is grounded by other original relocated words storing N."""

    def setUp(self):
        from oracle import Oracle
        self.oracle = Oracle.load()
        root = self.oracle.spaces["root"]
        site = next(r for r in root.relocations if r["index"] == 330)
        self.value = struct.unpack_from("<H", root.data, site["image_offset"])[0]
        self.other = next(r for r in root.relocations if r["index"] != 330 and
                          struct.unpack_from("<H", root.data, r["image_offset"])[0] == self.value)
        self.target = {"id": "probe", "space": "root", "segment": 0, "offset": site["image_offset"] - 1, "size": 4}

    def ground(self, witness):
        from far_binding import ground_symbol
        return ground_symbol({"kind": "segment-base", "segment": self.value, "witnesses": [witness]},
                             self.target, self.oracle)

    def test_far_data_member_needs_its_own_offset_witness(self):
        from far_binding import _ground_far_data
        spec = {"space": "root", "image_offset": self.other["image_offset"], "relocation_index": self.other["index"]}
        symbol = {"segment": self.value, "segment_witnesses": [spec], "offset": 0,
                  "members": [{"offset": 1, "offset_witnesses": []}]}
        with self.assertRaisesRegex(ValueError, "member needs"):
            _ground_far_data(symbol, self.target, self.oracle)

    def test_group_draft_region_segment_witness_counts_but_a_claimed_one_does_not(self):
        from far_binding import _ground_segment
        at = self.other["image_offset"]
        root = self.oracle.spaces["root"]
        hull = dict(root.address(at - 8), id="probe", size=16)
        spec = {"space": "root", "image_offset": at, "relocation_index": self.other["index"]}
        symbol = {"segment": self.value, "witnesses": [spec]}
        lo = at - 8
        self.assertEqual(_ground_segment(symbol, hull, self.oracle, claimed=[])["witnesses"][0]["image_offset"], at)
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            _ground_segment(symbol, hull, self.oracle, claimed=[(lo, lo + 16)])

    def test_other_relocated_word_with_the_same_segment_grounds_it(self):
        w = {"space": "root", "image_offset": self.other["image_offset"], "relocation_index": self.other["index"]}
        self.assertEqual(self.ground(w)["segment"], self.value)

    def test_the_candidates_own_relocation_cannot_ground_it(self):
        site = next(r for r in self.oracle.spaces["root"].relocations if r["index"] == 330)
        with self.assertRaisesRegex(ValueError, "overlaps candidate"):
            self.ground({"space": "root", "image_offset": site["image_offset"], "relocation_index": 330})


class AnchorInsidePadTests(unittest.TestCase):
    """A reviewed anchor whose extent already ends with its single alignment NOP after the
    return counts as the one pad byte (ROOFTOPS' callee 1101:83b4)."""

    def test_anchor_including_its_nop_pad_grounds_the_next_entry(self):
        from far_binding import _structural_entry
        from oracle import Oracle
        oracle = Oracle.load()
        at = oracle.spaces["root"].position(0x1101, 0x83b4)
        anchor = _structural_entry(oracle, at, 0x1101, 0x83b4, 4, "root", 0, 0)["anchor"]
        self.assertEqual((anchor["pad"], anchor.get("pad_inside_anchor")), (1, True))

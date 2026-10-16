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
        with self.assertRaisesRegex(ValueError, 'provenance'):
            self.check()

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

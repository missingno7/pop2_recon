"""Adversarial immediate-address/ES=DS proof tests, plus fresh MSC compiles."""
import copy
from dataclasses import replace
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from binding import IMMEDIATE_MODE, declarations, fixes
from compiler import compile_c, load_lock
from match import check_object, compile_and_check
from omf import OmfReader
import test_binding as legacy

ROOT = Path(__file__).resolve().parents[1]


class ImmediateBindingTests(unittest.TestCase):
    def setUp(self):
        baseline = legacy.BindingTests()
        baseline.setUp()
        self.oracle, self.space = baseline.oracle, baseline.space
        self.target, self.proof = copy.deepcopy(baseline.target), copy.deepcopy(baseline.proof)
        self.proof['mode'] = IMMEDIATE_MODE
        symbol = self.proof['symbols']['_data']
        symbol['kind'] = 'near-data-address-alias'
        del symbol['width']
        symbol['witnesses'][0]['operand_kind'] = 'direct'
        self.configure(bytes.fromhex('bf00001e07b90500f3aacb'))

    def configure(self, code, fields=(1,)):
        fix_body = b''.join(bytes((0xc4 | (at >> 8), at & 255, 0x56, 1)) for at in fields)
        self.raw = legacy.bound_object(code=code, fix_body=fix_body)
        expected = bytearray(code)
        for at in fields:
            expected[at:at+2] = b'\x10\x00'
        self.expected = bytes(expected)
        module = OmfReader().read(self.raw)
        self.proof.update(declarations=declarations(module), fixups=fixes(module))
        self.target['size'] = len(code)
        data = bytearray(self.space.data)
        data[64:64+len(code)] = self.expected
        self.space = replace(self.space, data=bytes(data))
        self.oracle.spaces['root'] = self.space

    def check(self, proof=None, expected=None):
        return check_object(self.raw, self.target, self.space,
                            self.expected if expected is None else expected, '_target',
                            binding=self.proof if proof is None else proof, oracle=self.oracle)

    def immediate_witness(self, use=bytes.fromhex('bf10001e07b90500f3aa'), at=96):
        data = bytearray(self.space.data)
        data[at:at+len(use)] = use
        self.space = replace(self.space, data=bytes(data))
        self.oracle.spaces['root'] = self.space
        self.proof['symbols']['_data']['witnesses'] = [
            dict(space='root', image_offset=at, bytes_hex=use[:3].hex(), operand_offset=1,
                 operand_kind='immediate', transfer_bytes=5,
                 base_use=dict(space='root', image_offset=at, bytes_hex=use.hex()))]

    def test_direct_witness_grounds_an_address_without_inventing_width(self):
        before = bytes(self.raw)
        result = self.check()
        self.assertTrue(result['exact'])
        self.assertEqual(result['ordinary_bytes_compared'], len(self.expected)-2)
        self.assertEqual(result['equations'][0]['encoded_addend'], 0)
        self.assertEqual(before, self.raw)

    def test_original_immediate_base_use_grounds_address_and_transfer_footprint(self):
        self.immediate_witness()
        self.assertTrue(self.check()['exact'])
        self.proof['symbols']['_data']['witnesses'][0]['transfer_bytes'] = 6
        with self.assertRaisesRegex(ValueError, 'footprint changed'):
            self.check()

    def test_immediate_instruction_alone_cannot_establish_a_ds_base(self):
        self.immediate_witness(use=bytes.fromhex('bf1000'))
        with self.assertRaisesRegex(ValueError, 'single ES=DS'):
            self.check()

    def test_candidate_instruction_or_its_base_use_cannot_be_its_own_witness(self):
        self.immediate_witness(at=64)
        with self.assertRaisesRegex(ValueError, 'overlaps candidate'):
            self.check()
        self.immediate_witness()
        self.proof['symbols']['_data']['witnesses'][0]['base_use']['image_offset'] = 64
        with self.assertRaisesRegex(ValueError, 'overlaps candidate'):
            self.check()

    def test_immediate_witness_must_be_on_the_context_instruction_boundary(self):
        self.immediate_witness()
        witness = self.proof['symbols']['_data']['witnesses'][0]
        data = bytearray(self.space.data)
        data[120:123] = bytes.fromhex('bf1000')
        self.space = replace(self.space, data=bytes(data))
        self.oracle.spaces['root'] = self.space
        witness['image_offset'] = 120
        with self.assertRaisesRegex(ValueError, 'not an instruction'):
            self.check()

    def test_nonzero_encoded_addend_and_explicit_displacements_fail(self):
        self.configure(bytes.fromhex('bf01001e07b90500f3aacb'))
        with self.assertRaisesRegex(ValueError, 'zero encoded data addends'):
            self.check()
        for fix_body in (bytes.fromhex('c40152010000'), bytes.fromhex('c40152010200')):
            self.configure(bytes.fromhex('bf00001e07b90500f3aacb'))
            self.raw = legacy.bound_object(code=bytes.fromhex('bf00001e07b90500f3aacb'), fix_body=fix_body)
            self.proof['fixups'] = fixes(OmfReader().read(self.raw))
            with self.assertRaisesRegex(ValueError, 'Unsupported DGROUP fixup'):
                self.check()

    def test_group_frames_and_segment_targets_remain_refused(self):
        symbol = copy.deepcopy(self.proof['symbols']['_data'])
        for fix_body in (bytes.fromhex('c401160101'), bytes.fromhex('c4015402')):
            self.raw = legacy.bound_object(code=bytes.fromhex('bf00001e07b90500f3aacb'), fix_body=fix_body)
            self.proof['fixups'] = fixes(OmfReader().read(self.raw))
            # Keep the target name grounded so the equation gate, rather than
            # the earlier missing-symbol check, must refuse F1 and T4.
            name = self.proof['fixups'][0]['target']['name']
            self.proof['symbols'] = {name: copy.deepcopy(symbol)}
            with self.assertRaisesRegex(ValueError, 'Unsupported DGROUP fixup'):
                self.check()

    def test_other_segment_register_writes_stay_refused(self):
        for tail in ('8ec0', 'c47e06', '8ed8', '1f', 'c57e06', '17', '8ed0',
                     '8ee0', '8ee8', '0fa1', '0fa9', '0fb47e06', '0fb57e06', '0fb27e06',
                     '0fb44606', '0fb54606', '0fb24606'):
            with self.subTest(tail=tail):
                self.configure(bytes.fromhex('bf00001e07' + tail + 'b90500f3aacb'))
                with self.assertRaisesRegex(ValueError, 'segment context|unknown code'):
                    self.check()

    def test_implicit_privileged_context_transfers_stay_refused(self):
        for opcode in ('0f34', '0f35', '0f05', '0f07', '0faa'):
            with self.subTest(opcode=opcode):
                self.configure(bytes.fromhex('bf00001e07' + opcode + 'b90500f3aacb'))
                with self.assertRaises(ValueError):
                    self.check()

    def test_pair_must_be_exact_adjacent_reached_and_unique(self):
        for pair in ('07', '1e5007', '1e9007', '1e071e07', '3e1e07'):
            with self.subTest(pair=pair):
                self.configure(bytes.fromhex('bf0000' + pair + 'b90500f3aacb'))
                with self.assertRaises(ValueError):
                    self.check()
        for body in ('eb00', '7400', 'e80000', 'cd21', 'cb'):
            self.configure(bytes.fromhex('bf0000' + body + '1e07b90500f3aacb'))
            with self.assertRaises(ValueError):
                self.check()

    def test_observed_plain_copy_repeat_word_copy_and_repeat_byte_clear(self):
        for op in ('a4', 'a5', 'f3a5', 'f3aa'):
            with self.subTest(op=op):
                copy_op = op[-2:] in ('a4', 'a5')
                head = 'bf0000be0000' if copy_op else 'bf0000'
                self.configure(bytes.fromhex(head + '1e07b90500' + op + 'cb'),
                               fields=(1, 4) if copy_op else (1,))
                self.assertTrue(self.check()['exact'])

    def test_string_segment_operand_address_lock_and_f2_prefixes_fail(self):
        for op in ('26f3aa', '3ef3aa', '36f3aa', '64f3aa', '65f3aa', '66f3aa', '67f3aa',
                   'f2aa', 'f0aa', 'f3f3aa', 'f3a6', 'ac', 'f3ae', 'aa', 'ab', 'f3a4', 'f3ab'):
            with self.subTest(op=op):
                self.configure(bytes.fromhex('bf00001e07b90500' + op + 'cb'))
                with self.assertRaises(ValueError):
                    self.check()

    def test_string_pointer_immediates_cannot_be_literal_substitutes(self):
        self.configure(bytes.fromhex('bf0000be10001e07b90500f3a5cb'))
        with self.assertRaisesRegex(ValueError, 'lacks a symbolic candidate fixup'):
            self.check()
        self.configure(bytes.fromhex('bf00008bfe1e07b90500f3aacb'))
        with self.assertRaisesRegex(ValueError, 'pointer lacks a symbolic'):
            self.check()

    def test_immediate_fixup_cannot_target_other_registers_or_unconsumed_fields(self):
        for code in ('bb00001e07b90500f3aacb', 'bf00001e07cb', 'bf0000bf00001e07b90500f3aacb'):
            self.configure(bytes.fromhex(code), fields=(1, 4) if code.startswith('bf0000bf') else (1,))
            with self.assertRaises(ValueError):
                self.check()

    def test_repeat_count_must_be_positive_and_reaching(self):
        for count in ('', 'b90000', 'b9050041', 'b90500b101'):
            self.configure(bytes.fromhex('bf00001e07' + count + 'f3aacb'))
            with self.assertRaisesRegex(ValueError, 'positive literal CX'):
                self.check()

    def test_address_alias_has_no_claimed_width_or_members(self):
        for key, value in (('width', 20), ('members', [])):
            proof = copy.deepcopy(self.proof)
            proof['symbols']['_data'][key] = value
            with self.assertRaisesRegex(ValueError, 'no width is inferred'):
                self.check(proof=proof)

    def test_original_relocations_touching_owner_or_independent_use_fail(self):
        for at in (63, 65):
            original = self.space
            self.space = replace(original, relocations=original.relocations+({'image_offset': at},))
            self.oracle.spaces['root'] = self.space
            with self.assertRaisesRegex(ValueError, 'not generated by offset16'):
                self.check()
            self.space = original
        self.immediate_witness()
        self.space = replace(self.space, relocations=self.space.relocations+({'image_offset': 95},))
        self.oracle.spaces['root'] = self.space
        with self.assertRaisesRegex(ValueError, 'touches an MZ relocation'):
            self.check()

    def test_v3_is_resident_only_and_legacy_modes_keep_refusing_the_idiom(self):
        for mode in ('external-dgroup-offset16-v1', 'external-dgroup-word-pair-offset16-v2',
                     'external-overlay-dgroup-offset16-v1'):
            proof = copy.deepcopy(self.proof)
            proof['mode'] = mode
            proof['symbols']['_data'].update(kind='near-data-alias', width=2)
            if 'overlay' in mode:
                self.target['space'] = 'overlay-2'
                self.oracle.spaces['overlay-2'] = replace(self.space, name='overlay-2')
            with self.assertRaisesRegex(ValueError, 'segment context|unknown code'):
                self.check(proof=proof)
        self.target['space'] = 'overlay-2'
        with self.assertRaisesRegex(ValueError, 'Unsupported owner space'):
            self.check()

    def test_immediate_field_and_ordinary_byte_mismatches_are_not_masked(self):
        for at, key in ((1, 'field_mismatch_count'), (10, 'ordinary_mismatch_count')):
            wrong = bytearray(self.expected)
            wrong[at] ^= 1
            result = self.check(expected=bytes(wrong))
            self.assertFalse(result['exact'])
            self.assertEqual(result[key], 1)


@unittest.skipUnless(Path(load_lock()['runner']['path']).is_file(), 'Pinned historical runner unavailable')
class FreshMSCImmediateTests(unittest.TestCase):
    def test_five_byte_struct_assignment_emits_unprefixed_strings(self):
        source = ('struct Record { char bytes[5]; };\n'
                  'extern struct Record saved, current;\n'
                  'void save_five_byte_state(void) { saved = current; }\n')
        fixture = ImmediateBindingTests()
        fixture.setUp()
        original = bytearray(fixture.space.data)
        original[44:47] = bytes.fromhex('a11800')
        expected = bytes.fromhex('5756bf1000be18001e07a5a5a45e5fcb')
        original[64:80] = expected
        fixture.space = replace(fixture.space, data=bytes(original))
        fixture.oracle.spaces['root'] = fixture.space
        fixture.target['size'] = len(expected)
        for opt in ('/Oe', '/O'):
            result = compile_c(source, 'msc600', ['/c', '/AM', opt, '/Gs', '/Zl', '/Gc'],
                workdir=ROOT / 'build/workers/wave9_immediate_sol/tests' / ('copy5_' + opt[1:]))
            self.assertTrue(result.ok, result.log)
            proof = fixture.proof
            proof.update(declarations=declarations(result.parsed), fixups=fixes(result.parsed))
            proof['symbols'] = {}
            for name, offset, at in (('SAVED', 16, 40), ('CURRENT', 24, 44)):
                proof['symbols'][name] = dict(kind='near-data-address-alias', group='DGROUP', offset=offset,
                    witnesses=[dict(space='root', image_offset=at, bytes_hex=original[at:at+3].hex(),
                                    operand_kind='direct', operand_offset=1)])
            report = check_object(result.obj_bytes, fixture.target, fixture.space, expected,
                                 'SAVE_FIVE_BYTE_STATE', binding=proof, oracle=fixture.oracle)
            self.assertTrue(report['exact'])
            self.assertEqual(report['fixups'], 2)

    def test_canonical_struct_copy_and_intrinsic_clear_from_fresh_objects(self):
        for name in ('ctrl/save_f_char', 'ctrl/init_input'):
            with self.subTest(name=name):
                recipe = json.loads((ROOT / 'recipes' / (name + '.json')).read_text())
                proof = json.loads((ROOT / recipe['binding']).read_text())
                result, report = compile_and_check(ROOT / recipe['source'], recipe['profile'], recipe['public'],
                    recipe['target_id'], ROOT / 'build/workers/wave9_immediate_sol/tests' / name,
                    recipe['flags'], binding=proof)
                self.assertTrue(report['exact'])
                self.assertEqual(result.obj_sha256, recipe['object_sha256'])
                self.assertEqual(report['relocations'], 0)
                self.assertTrue(all(f['frame']['kind'] == 'target_frame' and f['target']['method'] == 6
                                    and f['loc_type'] == 1 and f['displacement'] is None
                                    for f in fixes(result.parsed)))


if __name__ == '__main__':
    unittest.main()

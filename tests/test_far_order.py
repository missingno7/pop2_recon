"""RTLink bucket projection, qualified recipes, and the real 24-byte 62fe case."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from binding import declarations, fixes
from common import ROOT, read_json
from far_binding import ORDER_DEBT, relocation_order
from match import check_object, get_target
from omf import OmfReader
from oracle import Oracle
from validate import check_relocation_order
from test_match import make_omf, omf_record


class RelocationOrderTests(unittest.TestCase):
    def setUp(self):
        # Descending raw FIXUPPs, original order by distinct target-symbol bucket.
        self.fields = [{'site': 109, 'offset': 7, 'symbol': 'EXCHANGE'},
                       {'site': 103, 'offset': 1, 'symbol': 'LOAD'}]
        self.relocs = [{'index': 1453, 'image_offset': 103},
                       {'index': 1654, 'image_offset': 109}]
        self.indices = [1453, 1654]

    def check(self, relocs=None, fields=None, indices=None):
        return relocation_order(self.relocs if relocs is None else relocs,
                                self.fields if fields is None else fields,
                                self.indices if indices is None else indices)

    def test_between_group_permutation_is_explicit_debt(self):
        result = self.check()
        self.assertEqual(result['relocation_order'], 'GROUPED')
        self.assertEqual(result['relocation_order_debt'], ORDER_DEBT)
        self.assertEqual([g['symbol'] for g in result['relocation_groups']], ['EXCHANGE', 'LOAD'])
        self.assertEqual([g['original_relocation_indices'] for g in result['relocation_groups']], [[1654], [1453]])

    def test_matching_traversal_retains_legacy_exact_qualification(self):
        result = self.check(fields=list(reversed(self.fields)))
        self.assertEqual(result['relocation_order'], 'EXACT')
        self.assertEqual(result['relocation_order_debt'], [])

    def test_missing_extra_duplicated_or_one_byte_off_site_is_not_an_order_difference(self):
        for sites in ([103], [103, 109, 115], [103, 103], [104, 109], [103, 110], [102, 109]):
            relocs = [{'index': i, 'image_offset': site} for i, site in enumerate(sites)]
            with self.assertRaisesRegex(ValueError, 'site set'):
                self.check(relocs=relocs)

    def test_duplicate_original_index_cannot_explain_distinct_fields(self):
        relocs = copy.deepcopy(self.relocs); relocs[1]['index'] = 1453
        with self.assertRaisesRegex(ValueError, 'Duplicate/invalid original relocation index'):
            self.check(relocs=relocs, indices=[1453, 1453])

    def test_missing_duplicate_or_reordered_frozen_index_is_refused(self):
        for indices in ([1453], [1453, 1453], [1654, 1453], [1453, 1654, 2000]):
            with self.assertRaisesRegex(ValueError, 'Frozen original relocation indices'):
                self.check(indices=indices)

    def test_within_symbol_order_is_never_sorted_to_fit(self):
        fields = copy.deepcopy(self.fields)
        fields[1]['symbol'] = fields[0]['symbol']
        with self.assertRaisesRegex(ValueError, 'within-symbol FIXUPP order'):
            self.check(fields=fields)

    def test_projection_keeps_fixupp_order_across_ledata_chunks(self):
        sites = [743, 3, 1498, 753]
        fields = [{'site': s, 'offset': s-2, 'symbol': 'SAME'} for s in sites]
        relocs = [{'index': i, 'image_offset': s} for i, s in enumerate(sites)]
        result = self.check(relocs=relocs, fields=fields, indices=list(range(4)))
        self.assertEqual(result['relocation_order'], 'EXACT')
        self.assertEqual(result['relocation_groups'][0]['segment_word_sites'], sites)
        relocs[2], relocs[3] = relocs[3], relocs[2]
        with self.assertRaisesRegex(ValueError, 'within-symbol FIXUPP order'):
            self.check(relocs=relocs, fields=fields, indices=[r['index'] for r in relocs])

    def test_target_groups_must_be_contiguous_in_original_projection(self):
        fields = [{'site': s, 'offset': s-2, 'symbol': name}
                  for s, name in [(3, 'X'), (9, 'Y'), (15, 'X')]]
        relocs = [{'index': i, 'image_offset': f['site']} for i, f in enumerate(fields)]
        with self.assertRaisesRegex(ValueError, 'groups are interleaved'):
            self.check(relocs=relocs, fields=fields, indices=[0, 1, 2])

    def test_grouped_recipe_must_keep_qualification_and_debt(self):
        report = self.check()
        for recipe in ({}, {'relocation_order': 'GROUPED'},
                       {'relocation_order': 'EXACT', 'relocation_order_debt': ORDER_DEBT}):
            with self.assertRaisesRegex(ValueError, 'qualification/debt'):
                check_relocation_order(recipe, report)
        check_relocation_order({'relocation_order': 'GROUPED', 'relocation_order_debt': ORDER_DEBT}, report)
        check_relocation_order({}, self.check(fields=list(reversed(self.fields))))

    def test_real_62fe_complete_two_callee_shape_with_independent_original_grounding(self):
        oracle = Oracle.load()
        target, space, expected = get_target('root:1000:62fe', oracle)
        # A complete synthetic OMF fixture of the independently compiled 24-byte
        # source shape, with both raw pointer operands still zero and unedited.
        code = bytes.fromhex('558becff76069a00000000509a000000008be55dca020090')
        base = make_omf(code, publics=('LOAD_AND_EXCHANGE_ATTRIBUTE',),
                        externals=('EXCHANGE_CONTEXT_ATTRIBUTE', 'LOAD_FONT_ATTRIBUTE'))
        raw = b''.join(omf_record(k, b) for k, b, _ in OmfReader.records(base) if k != 0x8a)
        raw += omf_record(0x9c, bytes.fromhex('cc0d5601cc075602')) + omf_record(0x8a)
        obj = OmfReader().read(raw)
        binding = read_json(ROOT/'evidence/bindings/resc/set_font.json')
        binding.update(declarations=declarations(obj), fixups=fixes(obj))
        result = check_object(raw, target, space, expected, 'LOAD_AND_EXCHANGE_ATTRIBUTE',
                              binding=binding, oracle=oracle)
        self.assertTrue(result['exact'])
        self.assertEqual(result['emitted_size'], 24)
        self.assertEqual(result['covered_relocation_indices'], [1453, 1654])
        self.assertEqual(result['relocation_order'], 'GROUPED')
        self.assertEqual([e['offset'] for e in result['equations']], [13, 7])
        self.assertEqual(result['ordinary_bytes_compared'], 16)
        self.assertEqual(result['generated_relocation_indices'], [1654, 1453])


if __name__ == '__main__':
    unittest.main()

"""Refused proofs may expose fresh raw diagnostics, never ownership or stale output."""
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from common import sha
from match import CompiledObjectRejected, compile_and_check
from omf import OmfReader
from search import rejection_report
from test_match import make_omf


class RejectedSearchTests(unittest.TestCase):
    def result(self, raw):
        return SimpleNamespace(ok=True, obj_bytes=raw, parsed=OmfReader().read(raw, 'fresh'),
                               log='', obj=Path('must-not-read-a-prior-object.OBJ'))

    def test_bound_extent_refusal_retains_fresh_full_CODE_without_a_score(self):
        result = self.result(make_omf(b'\x90\xcb', actual_fixup=True))
        target = {'id': 'leaf'}
        space = SimpleNamespace(position=lambda *args: 0, relocations=[])
        target.update(segment=0, offset=0)
        with patch('match.Oracle.load', return_value=object()), \
                patch('match.get_target', return_value=(target, space, b'\xcb')), \
                patch('compiler.compile_c', return_value=result), \
                patch('binding.compare', side_effect=ValueError('Bound component extent differs')):
            with self.assertRaisesRegex(CompiledObjectRejected, 'extent differs') as caught:
                compile_and_check('candidate.c', 'msc600', '_target', 'leaf', 'work', binding={'mode': 'test'})
        report = rejection_report(caught.exception, 'leaf')
        self.assertFalse(report['exact'])
        self.assertEqual(report['state'], 'REJECTED')
        self.assertEqual(report['emitted_size'], 2)
        self.assertEqual(report['emitted_sha256'], sha(b'\x90\xcb'))
        self.assertEqual(report['expected_size'], 1)
        self.assertEqual(report['object_sha256'], sha(result.obj_bytes))
        self.assertNotIn('mismatch_count', report)
        self.assertNotIn('first_difference', report)
        self.assertTrue(report['fixup_thread_declarations'])

    def test_failed_compilation_cannot_expose_a_prior_object(self):
        result = self.result(make_omf(b'\xcb'))
        result.ok = False
        with patch('match.Oracle.load'), \
                patch('match.get_target', return_value=({}, object(), b'\xcb')), \
                patch('compiler.compile_c', return_value=result):
            with self.assertRaisesRegex(ValueError, 'Historical compilation failed') as caught:
                compile_and_check('candidate.c', 'msc600', '_target', 'leaf', 'work')
        self.assertNotIsInstance(caught.exception, CompiledObjectRejected)
        self.assertNotIn('emitted_sha256', rejection_report(caught.exception, 'leaf'))

    def test_CODE_holes_and_multiple_CODE_segments_have_no_raw_identity(self):
        for raw in (make_omf(b'\xcb', code_length=2),
                    make_omf(b'\xcb', extra_segments=(('_SECOND', 'CODE', 1),))):
            error = CompiledObjectRejected('refused', self.result(raw), {'id': 'leaf'}, b'\xcb', None)
            self.assertNotIn('emitted_sha256', rejection_report(error, 'leaf'))


if __name__ == '__main__':
    unittest.main()

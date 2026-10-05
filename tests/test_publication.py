"""Publication pins must survive Git's LF checkout policy on Windows."""
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import common
import promote


class PublicationIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.source = self.root / "build/workers/test/candidate.c"
        self.source.parent.mkdir(parents=True)
        self.original = b"void far f(void) {\r\n    return;\r\n}\r\n"
        self.source.write_bytes(self.original)
        self.manifest = self.root / "layout/manifest.json"
        common.write_json(self.manifest, {"owners": []})
        self.binding = self.root / "evidence/bindings/test.json"
        common.write_json(self.binding, {"schema": 1, "note": "independent fixture"})
        self.target = {"space": "root", "segment": 0, "offset": 32, "size": 2}
        self.report = {"exact": True, "state": "CODE_EXACT", "emitted_size": 2,
                       "object_sha256": "mocked compiler identity",
                       "proof_scope": "fixture", "fixups": 1, "relocations": 0}
        self.frozen = []

    def tearDown(self):
        self.temp.cleanup()

    def compile(self, source, *args, **kwargs):
        self.frozen.append(Path(source).read_bytes())
        return SimpleNamespace(flags=["/c"]), self.report

    def publish(self, *, verify_only=False):
        argv = ["promote.py", "root:0000:0020", str(self.source), "--profile", "test",
                "--symbol", "F", "--name", "f", "--binding", str(self.binding)]
        if verify_only:
            argv.append("--verify-only")
        with patch.object(common, "ROOT", self.root), patch.object(promote, "ROOT", self.root), \
             patch.object(promote.Oracle, "load", return_value=object()), \
             patch.object(promote, "check_manifest"), \
             patch.object(promote, "compile_and_check", side_effect=self.compile), \
             patch.object(promote, "get_target", return_value=(self.target, None, None)), \
             patch.object(sys, "argv", argv):
            with redirect_stdout(StringIO()):
                promote.main()

    def test_crlf_submission_publishes_checkout_stable_source_and_binding_pins(self):
        self.publish()
        expected = self.original.replace(b"\r\n", b"\n")
        canonical = self.root / "src/f.c"
        recipe_path = self.root / "recipes/f.json"
        recipe = common.read_json(recipe_path)
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertEqual(self.frozen, [expected])
        self.assertEqual(canonical.read_bytes(), expected)
        self.assertEqual(recipe["source_sha256"], common.sha(expected))
        self.assertNotIn(b"\r", self.binding.read_bytes())
        self.assertNotIn(b"\r", recipe_path.read_bytes())
        self.assertEqual(recipe["binding_sha256"], common.sha(self.binding.read_bytes()))

    def test_verify_only_normalizes_frozen_source_without_publishing_or_editing_input(self):
        before = self.manifest.read_bytes()
        self.publish(verify_only=True)
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertEqual(self.frozen, [self.original.replace(b"\r\n", b"\n")])
        self.assertEqual(self.manifest.read_bytes(), before)
        self.assertFalse((self.root / "src/f.c").exists())
        self.assertFalse((self.root / "recipes/f.json").exists())

    def test_crlf_binding_is_refused_before_canonical_publication(self):
        self.binding.write_bytes(self.binding.read_bytes().replace(b"\n", b"\r\n"))
        before = self.manifest.read_bytes()
        with self.assertRaisesRegex(ValueError, "LF newlines"):
            self.publish()
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertEqual(self.manifest.read_bytes(), before)
        self.assertFalse((self.root / "src/f.c").exists())
        self.assertFalse((self.root / "recipes/f.json").exists())


if __name__ == "__main__":
    unittest.main()

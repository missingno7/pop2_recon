"""Failure and crash injection for reports and publication, independent of DOS tools."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import common
import publication
import validate
import pin_runtime


class ReliabilityFixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.manifest = self.root / "layout/manifest.json"
        common.write_json(self.manifest, {"owners": []})
        self.before = self.manifest.read_bytes()
        self.after = {"owners": [{"target_id": "fixture"}]}
        self.files = {self.root / "src/f.c": b"void f(void) {}\n",
                      self.root / "recipes/f.json": b"{}\n"}
        self.report = self.root / "build/validation/report.json"
        common.write_json(self.report, {"status": "PASS", "old": True})


class ValidationReportTests(ReliabilityFixture):
    def run_validation(self, effect=None, *, run_tests=True):
        with patch.object(validate, "ROOT", self.root), \
             patch.object(validate, "_validate", side_effect=effect,
                          return_value={"status": "PASS", "accepted": []}):
            return validate.validate(run_tests)

    def test_old_pass_is_replaced_before_oracle_failure(self):
        def fail(_):
            self.assertEqual(common.read_json(self.report)["status"], "RUNNING")
            raise ValueError("bad oracle")
        with self.assertRaisesRegex(ValueError, "bad oracle"):
            self.run_validation(fail)
        report = common.read_json(self.report)
        self.assertEqual(report["status"], "FAIL")
        self.assertNotIn("old", report)
        self.assertIn("bad oracle", report["error"])

    def test_interrupt_has_explicit_status(self):
        with self.assertRaises(KeyboardInterrupt):
            self.run_validation(KeyboardInterrupt())
        self.assertEqual(common.read_json(self.report)["status"], "INTERRUPTED")

    def test_success_records_run_manifest_and_skipped_tests(self):
        report = self.run_validation(run_tests=False)
        self.assertEqual(report, common.read_json(self.report))
        self.assertFalse(report["tests_requested"])
        self.assertEqual(report["manifest_sha256"], common.sha(self.before))
        self.assertIn("finished_at", report)

    def test_manifest_change_during_run_cannot_pass(self):
        def change(_):
            common.write_json(self.manifest, self.after)
            return {"status": "PASS"}
        with self.assertRaisesRegex(ValueError, "Manifest changed"):
            self.run_validation(change)
        self.assertEqual(common.read_json(self.report)["status"], "FAIL")

    def test_pending_publication_blocks_validation(self):
        common.write_json(publication.journal_path(self.root), {})
        with self.assertRaisesRegex(ValueError, "Unfinished publication"):
            self.run_validation()
        self.assertEqual(common.read_json(self.report)["status"], "FAIL")

    def test_initial_report_write_failure_removes_old_pass(self):
        with patch.object(validate, "write_json", side_effect=OSError("disk failure")):
            with self.assertRaises(OSError):
                self.run_validation()
        self.assertFalse(self.report.exists())

    def test_failed_final_write_leaves_no_pass(self):
        def write(path, value):
            if value["status"] in ("PASS", "FAIL"):
                raise OSError("disk failure")
            common.write_json(path, value)
        with patch.object(validate, "write_json", side_effect=write):
            with self.assertRaises(OSError):
                self.run_validation()
        self.assertEqual(common.read_json(self.report)["status"], "RUNNING")


class PublicationRecoveryTests(ReliabilityFixture):
    def publish(self, check=lambda: None):
        publication.publish(self.root, self.files, self.before, self.after, check)

    def test_commit_checks_files_before_manifest_and_invalidates_report(self):
        def check():
            self.assertEqual(self.manifest.read_bytes(), self.before)
            for path, data in self.files.items():
                self.assertEqual(path.read_bytes(), data)
            self.assertTrue(publication.journal_path(self.root).exists())
        self.publish(check)
        self.assertEqual(common.read_json(self.manifest), self.after)
        self.assertEqual(common.read_json(self.report)["status"], "INVALIDATED")
        self.assertFalse(publication.journal_path(self.root).exists())

    def test_each_write_failure_rolls_back_only_transaction_files(self):
        unrelated = self.root / "src/user.c"
        unrelated.parent.mkdir()
        unrelated.write_bytes(b"user work")
        for failure_path in (self.report, *self.files, self.manifest):
            with self.subTest(path=failure_path):
                def write(path, data, **kwargs):
                    if path == failure_path:
                        raise OSError("injected write failure")
                    common.atomic_write(path, data, **kwargs)
                with patch.object(publication, "atomic_write", side_effect=write):
                    with self.assertRaises(OSError):
                        self.publish()
                self.assertEqual(self.manifest.read_bytes(), self.before)
                self.assertTrue(all(not path.exists() for path in self.files))
                self.assertFalse(publication.journal_path(self.root).exists())
                self.assertEqual(unrelated.read_bytes(), b"user work")

    def test_gate_refusal_and_interrupt_rollback(self):
        for error in (ValueError("gate refused"), KeyboardInterrupt()):
            def check():
                raise error
            with self.assertRaises(type(error)):
                self.publish(check)
            self.assertEqual(self.manifest.read_bytes(), self.before)
            self.assertTrue(all(not path.exists() for path in self.files))

    def crash_after(self, relative):
        script = '''
import os, sys
from pathlib import Path
sys.path.insert(0, sys.argv[1])
import common, publication
root = Path(sys.argv[2])
real_write = publication.atomic_write
def write(path, data, **kwargs):
    real_write(path, data, **kwargs)
    if path == root / sys.argv[3]:
        os._exit(23)
publication.atomic_write = write
publication.publish(root, {root/'src/f.c': b'void f(void) {}\\n',
                           root/'recipes/f.json': b'{}\\n'},
                    (root/'layout/manifest.json').read_bytes(),
                    {'owners': [{'target_id': 'fixture'}]}, lambda: None)
'''
        result = subprocess.run([sys.executable, "-c", script, str(Path(publication.__file__).parent),
                                 str(self.root), relative], capture_output=True, text=True)
        self.assertEqual(result.returncode, 23, result.stderr)

    def test_real_process_death_before_commit_is_recoverable(self):
        for stop in ("build/publication-pending.json", "src/f.c", "recipes/f.json"):
            with self.subTest(stop=stop):
                self.crash_after(stop)
                with self.assertRaisesRegex(ValueError, "Unfinished"):
                    self.publish()
                self.assertEqual(publication.recover(self.root), "Rolled back unfinished publication")
                self.assertEqual(self.manifest.read_bytes(), self.before)
                self.assertTrue(all(not path.exists() for path in self.files))
                self.assertEqual(publication.recover(self.root), "No unfinished publication")

    def test_real_process_death_after_commit_preserves_published_files(self):
        self.crash_after("layout/manifest.json")
        self.assertEqual(publication.recover(self.root), "Completed committed publication")
        self.assertEqual(common.read_json(self.manifest), self.after)
        for path, data in self.files.items():
            self.assertEqual(path.read_bytes(), data)

    def test_recovery_preflight_preserves_external_edits(self):
        self.crash_after("recipes/f.json")
        recipe = self.root / "recipes/f.json"
        recipe.write_bytes(b"user edit")
        with self.assertRaisesRegex(ValueError, "changed outside"):
            publication.recover(self.root)
        self.assertTrue((self.root / "src/f.c").exists())
        self.assertEqual(recipe.read_bytes(), b"user edit")
        self.assertTrue(publication.journal_path(self.root).exists())

    def test_changed_manifest_and_existing_destination_are_refused(self):
        common.write_json(self.manifest, self.after)
        with self.assertRaisesRegex(ValueError, "Manifest changed"):
            self.publish()
        self.manifest.write_bytes(self.before)
        source = self.root / "src/f.c"
        source.parent.mkdir()
        source.write_bytes(b"existing")
        with self.assertRaisesRegex(ValueError, "overwrite"):
            self.publish()
        self.assertEqual(source.read_bytes(), b"existing")

    def test_atomic_replace_failure_keeps_previous_complete_file(self):
        with patch.object(common.os, "replace", side_effect=OSError("replace failed")):
            with self.assertRaises(OSError):
                common.write_json(self.manifest, self.after)
        self.assertEqual(self.manifest.read_bytes(), self.before)
        self.assertFalse(list(self.manifest.parent.glob(".publish-*")))

    def test_runtime_publication_uses_the_same_recovery(self):
        source = self.root / "build/workers/runtime/recipe.json"
        common.write_json(source, {"name": "sample", "space": "root", "segment": 0,
                                  "offset": 32, "size": 1})
        report = {"proof_scope": "fixture", "fixups": 0, "relocations": 0}
        with patch.object(common, "ROOT", self.root), patch.object(pin_runtime, "ROOT", self.root), \
             patch.object(pin_runtime.Oracle, "load", return_value=object()), \
             patch.object(pin_runtime, "verify_member", return_value=report), \
             patch.object(pin_runtime, "check_manifest", side_effect=[None, ValueError("gate refused")]):
            with self.assertRaisesRegex(ValueError, "gate refused"):
                pin_runtime.pin(source)
        self.assertEqual(self.manifest.read_bytes(), self.before)
        self.assertFalse((self.root / "recipes/runtime/sample.json").exists())


if __name__ == "__main__":
    unittest.main()

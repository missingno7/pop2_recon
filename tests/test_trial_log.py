"""Worker journaling cannot publish, mix targets, lose best input or grind forever."""
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import trial_log
from common import read_json, sha, write_json


class TrialLogTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.work = self.root / "build/workers/leaf"
        self.work.mkdir(parents=True)
        self.patch = patch.object(trial_log, "ROOT", self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)
        self.card = {"target_id": "leaf", "canonical_head": "baseline", "max_trials": 6, "stagnation_limit": 2}
        write_json(self.work / "task.json", self.card)
        self.source = self.work / "candidate.c"
        self.source.write_bytes(b"void candidate(void) {}\n")

    def record(self, label, **updates):
        report = {"exact": False, "emitted_size": 2, "expected_size": 2,
                  "mismatch_count": 1, "mismatch_basis": "whole-extent byte differences",
                  "first_difference": 0, "emitted_sha256": sha(b"ab"), "expected_sha256": sha(b"ac"),
                  "object_sha256": "object", "fixup_thread_declarations": []}
        report.update(updates)
        return trial_log.record_trial(self.work, "leaf", label, self.source, "msc600", ["/c"], "c", report)

    def test_code_dedup_is_not_source_dedup_and_best_input_is_frozen(self):
        original = self.source.read_bytes()
        first = self.record("first")
        self.source.write_bytes(b"different readable hypothesis\n")
        second = self.record("same_output", object_sha256="different-object")
        self.assertEqual(second["unique_code_count"], 1)
        rows = read_json(self.work / "trials.json")["trials"]
        self.assertTrue(rows[-1]["duplicate_code"])
        self.assertTrue(rows[-1]["duplicate_result"])
        self.assertEqual((self.work / "best.c").read_bytes(), original)
        self.assertEqual(second["best"]["source_sha256"], first["best"]["source_sha256"])
        self.assertFalse((self.root / "src").exists())

    def test_same_raw_code_with_different_binding_is_not_same_result(self):
        self.record("one", binding_content_sha256="one")
        self.record("two", binding_content_sha256="two")
        row = read_json(self.work / "trials.json")["trials"][-1]
        self.assertTrue(row["duplicate_code"])
        self.assertFalse(row["duplicate_result"])

    def test_stagnation_stops_before_another_trial_or_mutation(self):
        self.record("one")
        self.record("two")
        last = self.record("three")
        self.assertTrue(last["stop_recommended"])
        self.assertEqual(last["status"], "blocked")
        prior = (self.work / "trials.json").read_bytes()
        with self.assertRaisesRegex(ValueError, "stagnation stop"):
            self.record("four")
        self.assertEqual((self.work / "trials.json").read_bytes(), prior)

    def test_budget_stops_even_when_every_code_output_is_new(self):
        self.card["max_trials"] = 2
        write_json(self.work / "task.json", self.card)
        self.record("one")
        last = self.record("two", emitted_sha256=sha(b"zz"))
        self.assertTrue(last["stop_recommended"])
        with self.assertRaisesRegex(ValueError, "budget reached"):
            self.record("three", emitted_sha256=sha(b"xy"))

    def test_exact_result_requires_supervisor_instead_of_more_grinding(self):
        last = self.record("exact", exact=True, mismatch_count=0, first_difference=None)
        self.assertEqual(last["status"], "exact")
        self.assertTrue(last["stop_recommended"])
        with self.assertRaisesRegex(ValueError, "supervisor"):
            self.record("more")

    def test_history_cannot_be_reused_for_another_head(self):
        self.record("one")
        self.card["canonical_head"] = "new-baseline"
        write_json(self.work / "task.json", self.card)
        with self.assertRaisesRegex(ValueError, "another target/HEAD"):
            self.record("two")

    def test_candidate_and_output_cannot_escape_worker_ownership(self):
        escaped = self.root / "src.c"
        escaped.write_bytes(b"canonical candidate")
        with self.assertRaisesRegex(ValueError, "owned by its worker"):
            trial_log.record_trial(self.work, "leaf", "bad", escaped, "msc600", [], "c", {})
        with self.assertRaisesRegex(ValueError, "one worker directory"):
            trial_log.check_task(self.root / "evidence", "leaf", "bad")
        self.assertFalse((self.work / "trials.json").exists())

    def test_trial_labels_and_target_must_match_the_card(self):
        with self.assertRaisesRegex(ValueError, "Invalid trial label"):
            trial_log.check_task(self.work, "leaf", "../escape")
        with self.assertRaisesRegex(ValueError, "target/HEAD card"):
            trial_log.check_task(self.work, "other", "one")
        self.record("one")
        with self.assertRaisesRegex(ValueError, "already recorded"):
            self.record("one")

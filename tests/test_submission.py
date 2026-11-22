"""Worker submissions may add target rows but never silently rewrite reviewed ones."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import submission


class ReplacementGuardTests(unittest.TestCase):
    def test_changed_existing_row_requires_listing(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "evidence").mkdir()
            row = {"id": "root:0000:0010", "sha256": "a" * 64, "size": 4}
            (tmp / "evidence/targets.json").write_text(json.dumps({"functions": [row]}))
            (tmp / "rows.json").write_text(json.dumps([dict(row, size=6, sha256="b" * 64)]))
            old_root = submission.ROOT
            submission.ROOT = tmp
            try:
                with self.assertRaises(ValueError):
                    submission.publish(tmp, {"schema": 1, "groups": [], "targets": "rows.json"})
                self.assertEqual(json.loads((tmp / "evidence/targets.json").read_text())["functions"], [row])
            finally:
                submission.ROOT = old_root

    def test_same_extent_keeps_the_canonical_row(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "evidence").mkdir()
            row = {"id": "root:0000:0010", "sha256": "a" * 64, "size": 4, "abi": "reviewed"}
            (tmp / "evidence/targets.json").write_text(json.dumps({"functions": [row]}))
            (tmp / "rows.json").write_text(json.dumps([dict(row, abi="reworded")]))
            old_root = submission.ROOT
            submission.ROOT = tmp
            try:
                submission.publish(tmp, {"schema": 1, "groups": [], "targets": "rows.json"})
                self.assertEqual(json.loads((tmp / "evidence/targets.json").read_text())["functions"], [row])
            finally:
                submission.ROOT = old_root

    def test_listed_replacement_with_the_same_extent_applies(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "evidence").mkdir()
            row = {"id": "root:0000:0010", "sha256": "a" * 64, "size": 4, "call_sites": []}
            new = dict(row, call_sites=[{"kind": "far_pointer"}])
            (tmp / "evidence/targets.json").write_text(json.dumps({"functions": [row]}))
            (tmp / "rows.json").write_text(json.dumps([new]))
            old_root = submission.ROOT
            submission.ROOT = tmp
            try:
                submission.publish(tmp, {"schema": 1, "groups": [], "targets": "rows.json",
                                         "target_replacements": [row["id"]]})
                self.assertEqual(json.loads((tmp / "evidence/targets.json").read_text())["functions"], [new])
            finally:
                submission.ROOT = old_root

    def test_blocked_kind_is_checked(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "submission.json"
            path.write_text(json.dumps({"schema": 1, "groups": [],
                                        "blocked": [{"target_id": "root:0000:0010", "kind": "hard"}]}))
            with self.assertRaises(ValueError):
                submission.load(path)


class DataOnlyGroupTests(unittest.TestCase):
    def test_data_group_runs_promote_data_without_claims_or_research_flags(self):
        from unittest.mock import patch
        seen = {}

        def fake_run(cmd, **kwargs):
            seen["cmd"] = cmd
            return type("R", (), {"returncode": 0, "stdout": "", "stderr": ""})()
        group = {"name": "tables/tables", "kind": "data", "source": "tables.c", "profile": "msc600a_bound",
                 "flags": ["/c", "/AM"]}
        with patch.object(submission.subprocess, "run", fake_run):
            submission._promote(Path("/w"), group, ["--extra-targets", "x.json"], True, Path("/w/stage"))
        cmd = seen["cmd"]
        self.assertTrue(cmd[1].endswith("promote_data.py"))
        self.assertNotIn("--claim", cmd)
        self.assertNotIn("--extra-targets", cmd)
        self.assertIn("--verify-only", cmd)
        with self.assertRaises(ValueError):
            submission._promote(Path("/w"), dict(group, claims=["root:0000:0010=F"]), [], True)

if __name__ == "__main__":
    unittest.main()

"""Assembler failures cannot reuse stale objects or unpinned source inputs."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import assembler
from compiler import CompileError


class AssemblerIsolationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "candidate.asm"
        self.source.write_text("PUBLIC f\nf PROC FAR\n retf\nf ENDP\nEND\n", encoding="ascii")
        self.work = self.root / "worker"
        self.spec = {"directory": "tools", "executable": "MASM.EXE", "flags": [], "allowed_flags": []}
        self.patches = [patch.object(assembler, "WORKERS", self.root),
                        patch.object(assembler, "verify_assembler", return_value=self.spec),
                        patch.object(assembler, "load_lock", return_value={"runner": {
                            "path": "MSDOS.EXE", "options": ["-e", "-v5.00"]}})]
        for item in self.patches:
            item.start()
            self.addCleanup(item.stop)

    def test_failed_assembly_cannot_reuse_stale_object(self):
        self.work.mkdir()
        (self.work / "UNIT.OBJ").write_bytes(b"stale accepted output")
        with patch.object(assembler.subprocess, "run", return_value=subprocess.CompletedProcess([], 1, b"error")):
            result = assembler.assemble_asm(self.source, "fixture", workdir=self.work)
        self.assertFalse(result.ok)
        self.assertIsNone(result.obj)
        self.assertFalse((self.work / "UNIT.OBJ").exists())

    def test_zero_exit_without_object_is_not_success(self):
        with patch.object(assembler.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, b"banner")):
            result = assembler.assemble_asm(self.source, "fixture", workdir=self.work)
        self.assertFalse(result.ok)

    def test_timeout_is_not_success(self):
        with patch.object(assembler.subprocess, "run", side_effect=subprocess.TimeoutExpired([], 1)):
            result = assembler.assemble_asm(self.source, "fixture", workdir=self.work)
        self.assertFalse(result.ok)
        self.assertTrue(result.timed_out)

    def test_unpinned_include_and_output_switch_are_refused_before_execution(self):
        with patch.object(assembler.subprocess, "run") as run:
            self.source.write_text(" include private.inc\n", encoding="ascii")
            with self.assertRaisesRegex(CompileError, "includes"):
                assembler.assemble_asm(self.source, "fixture", workdir=self.work)
            self.source.write_text("END\n", encoding="ascii")
            with self.assertRaisesRegex(CompileError, "allowlist"):
                assembler.assemble_asm(self.source, "fixture", ["/ooutside.obj"], workdir=self.work)
        run.assert_not_called()

    def test_workdir_must_stay_under_worker_root(self):
        with self.assertRaisesRegex(CompileError, "isolated"):
            assembler.assemble_asm(self.source, "fixture", workdir=self.root.parent / "elsewhere")


if __name__ == "__main__":
    unittest.main()

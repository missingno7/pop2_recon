"""Focused fail-closed tests for parsed OMF extents and compiler scratch handling."""
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from compiler import CompileError, compile_c
from omf import OmfError, OmfReader

def record(kind, body=b""):
    size = len(body) + 1
    prefix = bytes((kind,)) + struct.pack("<H", size) + body
    return prefix + bytes(((-sum(prefix)) & 0xff,))

def fixture(*, length=2, chunks=((0, b"\x90\xcb"),), fixups=(), extra=b"", end_body=b""):
    names = b"\x05_TEXT\x04CODE"
    rows = [
        record(0x80, b"\x04test"),
        record(0x96, names),
        record(0x98, b"\x28" + struct.pack("<H", length) + b"\x01\x02\x00"),
        record(0x90, b"\x00\x01\x07_target\x00\x00\x00"),
    ]
    for offset, payload in chunks:
        rows.append(record(0xA0, b"\x01" + struct.pack("<H", offset) + payload))
    rows.extend(record(0x9c, raw) for raw in fixups)
    if extra:
        rows.append(extra)
    rows.append(record(0x8a, end_body))
    return b"".join(rows)

class OmfReaderTests(unittest.TestCase):
    def test_preserves_full_segment_declaration_and_coverage(self):
        module = OmfReader().read(fixture(), "fixture.obj")
        self.assertEqual(module.segment_defs[0]["length"], 2)
        self.assertEqual(module.publics[0]["name"], "_target")
        self.assertEqual(module.initialized_ranges["_TEXT"], [(0, 2)])
        self.assertEqual(module.segments["_TEXT"], b"\x90\xcb")

    def test_uninitialized_bytes_remain_visible_as_a_gap(self):
        module = OmfReader().read(fixture(length=3, chunks=((0, b"\x90"),)), "gap.obj")
        self.assertEqual(module.segment_lengths["_TEXT"], 3)
        self.assertEqual(module.initialized_ranges["_TEXT"], [(0, 1)])
        self.assertEqual(module.segments["_TEXT"], b"\x90\x00\x00")

    def test_adjacent_ledata_records_cover_one_complete_extent(self):
        module = OmfReader().read(fixture(length=2, chunks=((0, b"\x90"), (1, b"\xcb"))))
        self.assertEqual(module.initialized_ranges["_TEXT"], [(0, 2)])

    def test_overlapping_ledat_is_rejected(self):
        with self.assertRaisesRegex(OmfError, "overlapping"):
            OmfReader().read(fixture(length=2, chunks=((0, b"\x90\xcb"), (1, b"\xcb"))))

    def test_thread_definitions_are_preserved_without_becoming_fixup_fields(self):
        module = OmfReader().read(fixture(fixups=(b"\x00\x01",)))
        self.assertEqual(module.fixups[0]["decoded"][0]["kind"], "thread")
        self.assertTrue(module.fixups[0]["anchor_known"])

    def test_real_fixup_subrecord_is_decoded_and_retained(self):
        module = OmfReader().read(fixture(fixups=(bytes.fromhex("84065601"),)))
        row = module.fixups[0]["decoded"][0]
        self.assertEqual(row["kind"], "fixup")
        self.assertEqual(row["location"], 6)
        self.assertEqual(row["target_index"], 1)
        self.assertEqual(module.fixups[0]["record_hex"], "84065601")

    def test_bad_checksum_is_refused(self):
        data = bytearray(fixture())
        data[5] ^= 1
        with self.assertRaisesRegex(OmfError, "checksum"):
            OmfReader().read(bytes(data))

    def test_unknown_storage_record_is_refused(self):
        with self.assertRaisesRegex(OmfError, "unsupported OMF record"):
            OmfReader().read(fixture(extra=record(0xF0, b"\0")))

    def test_records_after_modend_are_refused(self):
        with self.assertRaisesRegex(OmfError, "after MODEND"):
            OmfReader().read(fixture() + record(0x88, b"\0\0"))

    def test_module_entrypoint_is_refused(self):
        with self.assertRaisesRegex(OmfError, "entry-point"):
            OmfReader().read(fixture(end_body=b"\x02"))

    def test_compiler_modend_module_type_byte_is_preserved(self):
        module = OmfReader().read(fixture(end_body=b"\0"))
        self.assertEqual(module.records[-1]["body_hex"], "00")

class CompilerScratchTests(unittest.TestCase):
    def test_compile_only_switch_is_required_before_runner_execution(self):
        worker_root = ROOT / "build/workers"
        worker_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=worker_root) as directory:
            with patch("compiler.verify_profile", return_value={"flags": []}), \
                 patch("compiler.subprocess.run") as runner:
                with self.assertRaisesRegex(CompileError, "requires the /c"):
                    compile_c("int f(void) { return 1; }", "fixture", flags=["/AM"], workdir=directory)
                runner.assert_not_called()

    def test_unpinned_header_include_is_refused(self):
        with self.assertRaisesRegex(CompileError, "includes are refused"):
            compile_c('#include "unknown.h"\nint f(void) { return 1; }\n',
                      "msc510", flags=["/c"])

    def test_stale_object_is_removed_before_a_timed_out_run(self):
        worker_root = ROOT / "build" / "workers"
        worker_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=worker_root) as directory:
            work = Path(directory)
            (work / "UNIT.OBJ").write_bytes(b"stale object")
            (work / "UNIT.ASM").write_bytes(b"stale listing")
            with patch("compiler.verify_profile", return_value={
                "directory": r"C:\tools\msc-5.10", "executable": r"C:\tools\msc-5.10\CL.EXE"
            }), patch("compiler.subprocess.run",
                      side_effect=subprocess.TimeoutExpired("runner", 1, output=b"blocked")):
                result = compile_c("int f(void) { return 1; }\n", "msc510",
                                   workdir=work, flags=["/c"], timeout=1)
            self.assertFalse(result.ok)
            self.assertTrue(result.timed_out)
            self.assertIsNone(result.obj)
            self.assertFalse((work / "UNIT.OBJ").exists())
            self.assertFalse((work / "UNIT.ASM").exists())

if __name__ == "__main__":
    unittest.main()

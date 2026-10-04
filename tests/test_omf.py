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
        record(0x8c, b"\x07_target\x00"),
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
        module = OmfReader().read(fixture(length=8, chunks=((0, bytes(8)),),
                                          fixups=(bytes.fromhex("84065601"),)))
        row = module.fixups[0]["decoded"][0]
        self.assertEqual(row["kind"], "fixup")
        self.assertEqual(row["location"], 6)
        self.assertEqual(row["target_index"], 1)
        self.assertEqual(module.fixups[0]["record_hex"], "84065601")
        resolved = module.fixups[0]["resolved"][0]
        self.assertEqual(resolved["segment_offset"], 6)
        self.assertEqual(resolved["field_width"], 2)
        self.assertEqual(resolved["target"]["name"], "_target")

    def test_threaded_external_target_and_location_frame_resolve(self):
        # Frame thread 0 = location segment; target thread 0 = EXTDEF #1.
        # The final fixup is an 8-bit PC-relative field at segment offset 1.
        raw = bytes.fromhex("50080184018c")
        module = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),), fixups=(raw,)))
        rows = module.fixups[0]["resolved"]
        fixup = rows[-1]
        self.assertEqual(fixup["frame"], {
            "kind": "segment", "index": 1, "name": "_TEXT", "basis": "location"})
        self.assertEqual(fixup["target"]["kind"], "external")
        self.assertEqual(fixup["target"]["name"], "_target")
        self.assertEqual(fixup["target"]["method"], 6)  # target-thread T2 + P => T6
        self.assertEqual(fixup["segment_offset"], 1)
        self.assertEqual(fixup["field_width"], 2)

    def test_target_displacement_bit_selects_no_displacement_method(self):
        # Inline target T2 with P=1 is T6 (external with zero displacement).
        raw = bytes.fromhex("84004601")
        module = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),), fixups=(raw,)))
        target = module.fixups[0]["resolved"][0]["target"]
        self.assertEqual(target["kind"], "external")
        self.assertEqual(target["method"], 6)
        self.assertTrue(target["zero_displacement"])

    def test_target_thread_high_method_bit_comes_from_fixup_p_bit(self):
        # The high bit in a TARGET THREAD definition is reserved/ignored;
        # low method T0 is selected, then P=1 turns the use into T4.
        raw = bytes.fromhex("100184004c")
        module = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),), fixups=(raw,)))
        target = module.fixups[0]["resolved"][-1]["target"]
        self.assertEqual(target["kind"], "segment")
        self.assertEqual(target["name"], "_TEXT")
        self.assertEqual(target["method"], 4)
        self.assertTrue(target["zero_displacement"])

    def test_fixup_byte_offset_is_relative_to_anchoring_ledata(self):
        # Fixup offset 1 in LEDATA based at 2 maps to CODE offset 3.
        raw = bytes.fromhex("84014601")
        module = OmfReader().read(fixture(length=6, chunks=((0, b"\x90\x90"),
                                                           (2, b"\x90\x90\x90\x90")),
                                          fixups=(raw,)))
        self.assertEqual(module.fixups[0]["resolved"][0]["segment_offset"], 3)

    def test_fixup_field_must_fit_the_ledata_payload(self):
        # The field would fit in the SEGDEF, but crosses the end of the last
        # one-byte LEDATA payload and must not be silently located in a gap.
        raw = bytes.fromhex("84004601")
        with self.assertRaisesRegex(OmfError, "past its LEDATA record"):
            OmfReader().read(fixture(length=8, chunks=((0, b"\x90"),), fixups=(raw,)))

    def test_inline_and_threaded_f5_preserve_the_target_public_frame(self):
        inline = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),),
                                          fixups=(bytes.fromhex("84005601"),)))
        threaded_raw = bytes.fromhex("5484008601")
        threaded = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),),
                                            fixups=(threaded_raw,)))
        inline_frame = inline.fixups[0]["resolved"][0]["frame"]
        threaded_frame = threaded.fixups[0]["resolved"][-1]["frame"]
        self.assertEqual(inline_frame, threaded_frame)
        self.assertEqual(inline_frame["kind"], "target_frame")
        self.assertEqual(inline_frame["target"]["kind"], "external")
        self.assertEqual(inline_frame["target"]["name"], "_target")

    def test_absolute_target_frame_datums_are_fixed_words(self):
        # Explicit T3 carries a 16-bit frame number, not a variable OMF index.
        # The following displacement verifies that both high-byte datum bytes
        # were consumed. 0x1234 would look like index 0x34 followed by garbage.
        inline_raw = bytes.fromhex("84004334127856")
        inline = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),),
                                          fixups=(inline_raw,)))
        row = inline.fixups[0]["decoded"][0]
        resolved = inline.fixups[0]["resolved"][0]
        self.assertEqual(row["target_index"], 0x1234)
        self.assertEqual(row["displacement"], 0x5678)
        self.assertEqual(resolved["target"], {"kind": "absolute", "frame": 0x1234, "method": 3})

        threaded_raw = bytes.fromhex("0d34128400497856")
        threaded = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),),
                                            fixups=(threaded_raw,)))
        self.assertEqual(threaded.fixups[0]["decoded"][0]["datum"], 0x1234)
        self.assertEqual(threaded.fixups[0]["resolved"][-1]["target"], resolved["target"])

        # Only the TARGET THREAD method's low two bits are significant. Raw
        # method 7 aliases T3 and therefore has the same two-byte datum.
        raw_method7 = bytes.fromhex("1e341284004a7856")
        method7 = OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),),
                                           fixups=(raw_method7,)))
        self.assertEqual(method7.fixups[0]["decoded"][0]["method"], 7)
        self.assertEqual(method7.fixups[0]["decoded"][0]["datum"], 0x1234)
        self.assertEqual(method7.fixups[0]["decoded"][1]["displacement"], 0x5678)
        self.assertEqual(method7.fixups[0]["resolved"][-1]["target"], resolved["target"])

    def test_invalid_absolute_frame_methods_and_32bit_locat_are_refused(self):
        # F3 is invalid in the linker-facing OMF method table. Still ensure the
        # frame datum is consumed as a word before the resolver rejects it.
        frame_f3 = bytes.fromhex("840034341201")
        with self.assertRaisesRegex(OmfError, "unsupported FIXUPP frame method 3"):
            OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),), fixups=(frame_f3,)))
        with self.assertRaisesRegex(OmfError, "unsupported FIXUPP frame thread method 3"):
            OmfReader().read(fixture(fixups=(bytes.fromhex("4c"),)))
        with self.assertRaisesRegex(OmfError, "unsupported FIXUPP location type 9"):
            OmfReader().read(fixture(length=8, chunks=((0, bytes(8)),),
                                     fixups=(bytes.fromhex("a4004601"),)))

    def test_undefined_fixup_thread_is_rejected(self):
        # FIXDAT marks target-thread 1, but only target thread 0 is defined.
        raw = bytes.fromhex("50080184018d")
        with self.assertRaisesRegex(OmfError, "undefined FIXUPP target thread 1"):
            OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),), fixups=(raw,)))

    def test_undefined_fixup_index_is_rejected(self):
        # Target is EXTDEF #2, while the fixture contains only EXTDEF #1.
        raw = bytes.fromhex("84014602")
        with self.assertRaisesRegex(OmfError, "target external index 2"):
            OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),), fixups=(raw,)))

    def test_unsupported_fixup_location_and_lidata_anchor_are_rejected(self):
        # LOCAT 6 is reserved; use an otherwise well-formed target reference.
        raw = bytes.fromhex("98005601")
        with self.assertRaisesRegex(OmfError, "unsupported FIXUPP location type 6"):
            OmfReader().read(fixture(length=8, chunks=((0, bytes(8)),), fixups=(raw,)))

        # LIDATA is present in the existing reader but expansion-position
        # mapping is intentionally outside this symbolic resolver.
        names = b"\x05_TEXT\x04CODE"
        lidata = record(0xA2, b"\x01\x00\x00\x01\x00\x00\x00\x01\x90")
        obj = (record(0x80, b"\x04test") + record(0x96, names) +
               record(0x98, b"\x28\x02\x00\x01\x02\x00") +
               record(0x8c, b"\x07_target\x00") + lidata +
               record(0x9c, bytes.fromhex("84005601")) + record(0x8a))
        with self.assertRaisesRegex(OmfError, "unsupported LIDATA"):
            OmfReader().read(obj)

    def test_invalid_frame_method_and_target_method_seven_are_rejected(self):
        # Frame method 6 is invalid in 8086 OMF; target method 7 is reserved.
        with self.assertRaisesRegex(OmfError, "unsupported FIXUPP frame method 6"):
            OmfReader().read(fixture(fixups=(bytes.fromhex("58"),)))
        with self.assertRaisesRegex(OmfError, "unsupported FIXUPP target method 7"):
            OmfReader().read(fixture(length=4, chunks=((0, bytes(4)),),
                                     fixups=(bytes.fromhex("8400470100"),)))

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

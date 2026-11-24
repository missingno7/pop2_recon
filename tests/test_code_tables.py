"""Code tables (docs/acceptance.md, 2026-11-24): original data between accepted code of one MASM object."""
import json
import sys
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

from common import ROOT, sha
import module_gate


class CodeTableTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()

    def judge(self, group, tables=None, edit=lambda s: s, rows=(), language="asm", label="base"):
        recipe = json.loads((ROOT / f"recipes/{group}.json").read_text())
        binding = json.loads((ROOT / recipe["binding"]).read_text())
        source = edit((ROOT / recipe["source"]).read_text())
        real = module_gate.get_table
        proposed = {r["id"]: r for r in rows}

        def table(identifier, oracle):
            if identifier not in proposed:
                return real(identifier, oracle)
            space = oracle.spaces["root"]
            r = proposed[identifier]
            return r, space, space.extent(r["segment"], r["offset"], r["size"])
        wanted = [{k: t[k] for k in ("target_id", "public")} for t in recipe.get("tables", [])] if tables is None else tables
        picked = [{k: c[k] for k in ("target_id", "public")} for c in recipe["claims"]]
        with mock.patch.object(module_gate, "get_table", table):
            from binding import declarations, fixes
            from omf import OmfReader
            from assembler import assemble_asm
            if language == "asm":
                work = ROOT / "build/workers/tests/code_tables" / label
                work.mkdir(parents=True, exist_ok=True)
                (work / "GROUP.ASM").write_bytes(source.encode("ascii"))
                result = assemble_asm(work / "GROUP.ASM", recipe["profile"], flags=recipe["flags"], workdir=work / "asm")
                self.assertTrue(result.ok, result.log)
                module = OmfReader().read(result.obj_bytes)
                binding.update(declarations=declarations(module), fixups=fixes(module))
                return module_gate.check_group(result.obj_bytes, picked, self.oracle, binding=binding,
                                               group_id=recipe["group_id"], code_tables=wanted)
        return module_gate.compile_group(source, recipe["profile"], recipe["flags"], picked, ROOT / "build/workers/tests/x",
                                         binding=binding, group_id=recipe["group_id"], language=language, tables=wanted)

    @staticmethod
    def row(position, size):
        return {"id": f"root:1129:{position - 0x11290:04x}", "space": "root", "segment": 0x1129,
                "offset": position - 0x11290, "size": size}

    def test_every_region_of_the_vga_object_is_read_and_exact(self):
        report = self.judge("graf/vga_mode")
        tables = {t["public"]: t for t in report["tables"]}
        self.assertEqual(sum(t["size"] for t in tables.values()), 78 + 14 + 5 * 16)
        self.assertEqual(tables["VGA_MODE_DESCRIPTOR"]["fixups"], 8)
        self.assertTrue(all(t["state"] == "ASM_DATA_EXACT" and t["read_at"] for t in tables.values()))
        self.assertEqual(report["drafts"], [])                       # no table public is left as a draft
        self.assertEqual(len(report["claims"]), 10)                  # and none became a function

    def test_a_wrong_word_or_byte_is_refused(self):
        swap = lambda s: s.replace("DW OFFSET probe_vga, OFFSET initialize_vga", "DW OFFSET initialize_vga, OFFSET probe_vga")
        with self.assertRaisesRegex(ValueError, "word differs"):
            self.judge("graf/vga_mode", edit=self.changed(swap), label="swap")
        inside = lambda s: s.replace("OFFSET probe_vga,", "OFFSET probe_vga+1,")
        with self.assertRaisesRegex(ValueError, "names no instruction"):
            self.judge("graf/vga_mode", edit=self.changed(inside), label="inside")
        with self.assertRaisesRegex(ValueError, "differs from the original"):
            self.judge("graf/vga_mode", edit=self.changed(lambda s: s.replace("0A000h", "0A001h", 1)), label="byte")

    def changed(self, edit):
        def run(source):
            found = edit(source)
            self.assertNotEqual(found, source)
            return found
        return run

    def test_region_must_be_whole(self):
        for label, public, position, size, message in (
                ("middle", "VGA_PALETTE", 0x178F8, 48, "does not start where"),
                ("short", "VGA_MODE_DESCRIPTOR", 0x178DA, 30, "does not end where"),
                ("code", "VGA_COPY_DISPATCH", 0x17B19, 18, "does not end where"),
                ("moved", "VGA_COPY_DISPATCH", 0x17B1B, 14, "object placement")):
            row = self.row(position, size)
            with self.subTest(label), self.assertRaisesRegex(ValueError, message):
                self.judge("graf/vga_mode", tables=[{"target_id": row["id"], "public": public}], rows=[row])
        # The code in front of a table must itself be exact.
        with self.assertRaisesRegex(ValueError, "does not start where"):
            self.judge("graf/vga_mode", tables=[{"target_id": "root:1129:6889", "public": "VGA_COPY_DISPATCH"}],
                       edit=self.changed(lambda s: s.replace("add sp, -1Ch", "add sp, -1Eh", 1)), label="inexact")

    def test_alignment_byte_is_never_part_of_a_table(self):
        row = self.row(0x175FD, 273)     # BANKED16_BOUNDARY_ALIGNMENT, the linker's word padding
        with self.assertRaisesRegex(ValueError, "alignment byte"):
            self.judge("graf/banked16_mode", tables=[{"target_id": row["id"], "public": "BANKED16_TRANSPARENCY_MASKS"}],
                       rows=[row])
        fill = self.row(0x15EAF, 1)
        with self.assertRaisesRegex(ValueError, "alignment byte"):
            self.judge("graf/packed_video_modes", tables=[{"target_id": fill["id"], "public": "MODE_BOUNDARY_FILL"}], rows=[fill])

    def test_a_table_nothing_reads_is_refused(self):
        row = self.row(0x13C32, 4)       # the stub driver's link word: only its place speaks for it
        with self.assertRaisesRegex(ValueError, "reads the code table"):
            self.judge("sound2/load_sound_driver", tables=[{"target_id": row["id"], "public": "INVALID_DRIVER_HEADER"}], rows=[row])

    def test_only_reviewed_tables_from_assembly(self):
        with self.assertRaisesRegex(ValueError, "Unknown/ambiguous reviewed code table"):
            self.judge("graf/vga_mode", tables=[{"target_id": "root:1129:6698", "public": "VGA_COPY_DISPATCH"}])
        with self.assertRaisesRegex(ValueError, "MASM sources only"):
            self.judge("graf/vga_mode", language="c")

    def test_metrics_count_no_table_as_a_function(self):
        from metrics import metrics
        report = metrics()
        rows = json.loads((ROOT / "evidence/targets.json").read_text(encoding="utf-8"))
        owners = json.loads((ROOT / "layout/manifest.json").read_text())["owners"]
        tables = [o for o in owners if o["kind"] == "MATCHING_ASM_DATA"]
        self.assertEqual({o["target_id"] for o in tables}, {t["id"] for t in rows["code_tables"]})
        self.assertEqual(report["matching_asm_data_bytes"], sum(o["size"] for o in tables))
        self.assertEqual(report["matching_asm_bytes"], sum(o["size"] for o in owners if o["kind"] == "MATCHING_ASM"))
        self.assertEqual(report["exact_function_count"], sum(o["kind"] in ("MATCHING_C", "MATCHING_ASM") for o in owners))
        space = self.oracle.spaces["root"]
        for t in rows["code_tables"]:
            self.assertEqual(sha(space.extent(t["segment"], t["offset"], t["size"])), t["sha256"])


if __name__ == "__main__":
    unittest.main()

"""Rebuilt RTLink run-time modules and the pinned overlay manager (docs/runtime-fixups.md, 2026-11-24)."""
import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

from common import ROOT, read_json

AVAILABLE = (ROOT / "recipes/rtlutils/expand.json").exists() and Path("C:/tools/masm-6.00/files/D2/ML.EXE").exists()


@unittest.skipUnless(AVAILABLE, "pinned MASM 6.00 not installed")
class ManagerModuleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.manifest = read_json(ROOT / "layout/manifest.json")

    def judge(self, name, label="base", edit=None, recipe=None, binding=None):
        from manager_asm import verify
        found = copy.deepcopy(read_json(ROOT / f"recipes/rtlutils/{name}.json"))
        found.update(recipe or {})
        source = (ROOT / found["source"]).read_text()
        if edit:
            changed = edit(source)
            self.assertNotEqual(changed, source)
            source = changed
        report, _ = verify(found, source.encode("ascii"), self.oracle, self.manifest,
                           ROOT / "build/workers/tests/manager_asm" / f"{name}_{label}",
                           binding or copy.deepcopy(read_json(ROOT / found["binding"])))
        return report

    def test_both_modules_are_exact_and_deterministic(self):
        for name, size, fixups, publics in (("rtcommon", 467, 33, 9), ("expand", 7317, 6, 1)):
            report = self.judge(name)
            self.assertTrue(report["exact"])
            self.assertEqual((report["size"], report["fixups"], len(report["publics"]), report["relocations"]),
                             (size, fixups, publics, 1))
            self.assertEqual(report["object_sha256"], read_json(ROOT / f"recipes/rtlutils/{name}.json")["object_sha256"])

    def test_any_changed_byte_or_field_is_not_exact(self):
        # 6.10's own two bytes: the two tables behind the order list are padded the other way round.
        later = lambda s: s.replace("    add ax, 4\n    mov [si+FREQUENCIES], ax\n    add ax, dx\n    add ax, 2\n",
                                    "    add ax, 2\n    mov [si+FREQUENCIES], ax\n    add ax, dx\n    add ax, 4\n")
        self.assertFalse(self.judge("expand", "later", later)["exact"])
        self.assertFalse(self.judge("rtcommon", "handler", lambda s: s.replace(
            "$$RTL_FATAL_ERROR DD default_error", "$$RTL_FATAL_ERROR DD internal_error"))["exact"])

    def test_the_assembler_decides(self):
        # MASM 5.10 encodes `add ax, 4` with the accumulator form and needs other source for forward references.
        with self.assertRaisesRegex(ValueError, "Historical assembly failed"):
            self.judge("rtcommon", "masm510", recipe={"profile": "masm510"})

    def test_module_must_be_whole(self):
        for label, name, edit, message in (
                ("longer", "rtcommon", lambda s: s.replace("chain DW 24 DUP (0)", "chain DW 36 DUP (0)"), "reviewed extent"),
                ("hole", "expand", lambda s: s.replace("input_buffer DB 100h DUP (0)", "input_buffer DB 100h DUP (?)"),
                 "completely initialized"),
                ("second", "rtcommon", lambda s: s.replace("END\n", "OTHER SEGMENT BYTE PUBLIC 'DATA'\n DB 1\nOTHER ENDS\nEND\n")
                 if s.endswith("$$OVLMGR_UTILS ENDS\nEND\n") else s, "exactly one manager segment"),
                ("class", "rtcommon", lambda s: s.replace("PUBLIC '$$OVLMGR_UTILS'", "PUBLIC 'CODE'"), "public \\$\\$ manager segment"),
                ("private", "rtcommon", lambda s: s.replace("BYTE PUBLIC '$$OVLMGR", "BYTE '$$OVLMGR"), "public \\$\\$ manager segment")):
            with self.subTest(label), self.assertRaisesRegex(ValueError, message):
                self.judge(name, label, edit)

    def test_only_a_reviewed_module_row_and_bound_providers(self):
        with self.assertRaisesRegex(ValueError, "reviewed extent"):
            self.judge("rtcommon", "row", recipe={"target_id": "root:22f2:0220"})
        with self.assertRaisesRegex(ValueError, "Unknown/ambiguous"):
            self.judge("rtcommon", "unknown", recipe={"target_id": "root:22f2:0041"})
        binding = read_json(ROOT / "evidence/bindings/rtlutils/rtcommon.json")
        for change in ({"code_providers": {}}, {"code_providers": {"$$RTLCOMMON_MESSAGES": "rtl_pathsr_m"}},
                       {"segment_frames": {"$$OVLMGR_UTILS": {"segment": 0x22F3, "witnesses":
                                                              binding["segment_frames"]["$$OVLMGR_UTILS"]["witnesses"]}}}):
            with self.assertRaises(ValueError):
                self.judge("rtcommon", "binding", binding=dict(copy.deepcopy(binding), **change))

    def test_overlay_manager_takes_the_program_start_from_the_main_module(self):
        from rtlink_fixups import prove
        from runtime_fixups import read_member
        recipe = read_json(ROOT / "recipes/runtime/rtl_ovlmgr.json")
        base = read_json(ROOT / recipe["binding"])
        module, overwritten = read_member(recipe)
        code = module.segments["$$OVLMGR"]
        space = self.oracle.spaces["root"]
        start = space.position(recipe["segment"], recipe["offset"])

        def run(manifest=self.manifest, **change):
            return prove(module, overwritten, "$$OVLMGR", code, space.data[start:start + len(code)],
                         dict(copy.deepcopy(base), **change), recipe, self.oracle, manifest)
        self.assertTrue(run()["exact"])
        for change, message in (({"program_start": {"symbol": "$$MAIN", "recipe": "crt_crt0dat_medium"}}, "not the main module"),
                                ({"program_start": {"symbol": "$$MAIN", "recipe": "crt_nothing"}}, "not a pinned member"),
                                ({"program_start": {"symbol": "$$RTL_PRELOAD", "recipe": "crt_crt0_medium"}}, "Unsupported program start")):
            with self.assertRaisesRegex(ValueError, message):
                run(**change)
        without = {k: v for k, v in base.items() if k != "program_start"}
        with self.assertRaisesRegex(ValueError, "Missing pinned providers"):
            prove(module, overwritten, "$$OVLMGR", code, space.data[start:start + len(code)], without, recipe, self.oracle,
                  self.manifest)
        no_expand = dict(self.manifest, owners=[o for o in self.manifest["owners"] if o["target_id"] != "root:22f2:0220"])
        with self.assertRaisesRegex(ValueError, "RTLEXPANDSECTION is missing"):
            run(manifest=no_expand)


if __name__ == "__main__":
    unittest.main()

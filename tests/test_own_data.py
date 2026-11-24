"""Own far data, placement brackets and near-code address fields (2026-11-24): the accepted case and its refutations."""
import json
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

ROOT = Path(__file__).resolve().parents[1]


class FarPlacementTests(unittest.TestCase):
    CONTENT = bytes(range(1, 19))

    def check(self, content=CONTENT, paragraph=0x1002, image=None, *, relocs=(), kind="FAR_DATA", alignment=3,
              groups=(), ranges=None, fixups=(), publics=(), other=b""):
        from module_gate import _far_placements
        from oracle import Space
        module = SimpleNamespace(
            segment_defs=[{"name": "UNIT_TEXT", "class": "CODE", "length": 1, "alignment": 2},
                          {"name": "UNIT5_DATA", "class": kind, "length": len(content), "alignment": alignment}],
            segments={"UNIT5_DATA": content}, groups=[{"name": "DGROUP", "segments": list(groups)}],
            initialized_ranges={"UNIT5_DATA": ranges if ranges is not None else [(0, len(content))]},
            publics=list(publics))
        data = image if image is not None else bytes(0x20) + self.CONTENT + bytes(0x20)
        spaces = {"home": Space("home", 0x1000, 0, data, tuple({"index": i, "image_offset": r} for i, r in enumerate(relocs))),
                  "other": Space("other", 0x4000, 0, other or bytes(4), ())}
        return _far_placements(module, {"UNIT5_DATA": paragraph}, list(fixups), SimpleNamespace(spaces=spaces))

    def test_content_found_once_at_its_paragraph(self):
        self.assertEqual(self.check(), {"UNIT5_DATA": {"paragraph": 0x1002, "length": 18, "space": "home"}})
        self.assertTrue(self.check(ranges=[(0, 12), (12, 18)]))

    def test_everything_else_is_refused(self):
        refused = [dict(paragraph=0x1001),                                             # other bytes there
                   dict(content=bytes(16), image=bytes(0x60)),                         # repeated content
                   dict(other=b"x" + self.CONTENT),                                    # found in another space too
                   dict(relocs=(0x21,)), dict(relocs=(0x1F,)),                         # a relocated word inside
                   dict(kind="DATA"), dict(alignment=2), dict(groups=(2,)),
                   dict(ranges=[(0, 12)]), dict(ranges=[(0, 12), (13, 18)]),           # not completely initialized
                   dict(content=self.CONTENT[:15]),                                    # too short
                   dict(fixups=[{"segment": "UNIT5_DATA"}]), dict(publics=[{"segment": "UNIT5_DATA"}]),
                   dict(paragraph="1002")]
        for change in refused:
            with self.subTest(change), self.assertRaises(ValueError):
                self.check(**change)

    def test_iterated_data_never_fills_a_code_segment(self):
        from match import code_segment

        def record(kind, body):
            return {"type": kind, "body_hex": bytes(body).hex()}
        segment = {"name": "UNIT_TEXT", "class": "CODE", "length": 4, "alignment": 2, "big": False, "use32": False}
        far = {"name": "UNIT5_DATA", "class": "FAR_DATA", "length": 4, "alignment": 3, "big": False, "use32": False}
        zeros = [1, 0, 0, 4, 0, 0, 0, 1, 0x90]
        module = SimpleNamespace(segment_defs=[segment, far], segments={"UNIT_TEXT": b"\x90" * 4},
                                 records=[record(0xA0, [1, 0, 0, 0x90, 0x90, 0x90, 0x90]), record(0xA2, [2, *zeros[1:]])])
        self.assertEqual(code_segment(module, placed=("UNIT5_DATA",))[2], b"\x90" * 4)
        with self.assertRaisesRegex(ValueError, "secondary"):
            code_segment(module)
        module.records[0] = record(0xA2, zeros)
        with self.assertRaisesRegex(ValueError, "Iterated data"):
            code_segment(module, placed=("UNIT5_DATA",))


class BracketTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.home = Oracle.load().spaces["overlay-17"]

    def check(self, p=0x2580, length=144, below="group:xlattxt4/xlattxt4", above="group:itemdlg/itemdlg", **seg):
        from module_gate import _bracketed
        row = dict({"name": "_DATA", "length": length, "alignment": 2, "combine": 2}, **seg)
        _bracketed(row, p, {"below": below, "above": above}, self.home, 0x2940, "group:winlife/events")

    def test_segment_that_exactly_fills_its_gap(self):
        self.check()

    def test_any_other_place_or_neighbour_is_refused(self):
        for change in (dict(length=142), dict(p=0x257E), dict(p=0x2582, length=142), dict(length=146),
                       dict(alignment=3), dict(combine=6),
                       dict(below="group:itemdlg/itemdlg"), dict(below="group:winlife/events"),
                       dict(above="group:winlife/lifecycle"), dict(below="group:ems/ems_driver"),
                       dict(below="group:graf/graphics_mode"), dict(above="group:not_accepted")):
            with self.subTest(change), self.assertRaises(ValueError):
                self.check(**change)

    def test_two_proven_offsets_must_locate_it(self):
        from module_gate import _bracket_located

        def state(*fields):
            return {"placed": [{"key": ("_DATA", a), "equal": equal, "insn": i} for a, i, equal in fields]}
        exact = [{"public": "F", "exact": True}, {"public": "G", "exact": False}]
        located = state((0, 1, True), (0, 2, True), (8, 3, True), (8, 4, True), (4, 5, True))
        _bracket_located(["_DATA"], {"F": located}, exact)
        for states in ({"F": state((0, 1, True), (0, 2, True), (4, 5, True))},              # one offset only
                       {"F": state((0, 1, True), (0, 1, True), (8, 3, True), (8, 4, True))},  # one instruction
                       {"F": state((0, 1, True), (0, 2, True), (8, 3, True), (8, 4, False))},  # an unequal field
                       {"G": located}):                                                      # not exact
            with self.assertRaisesRegex(ValueError, "not located"):
                _bracket_located(["_DATA"], states, exact)

    def test_canonical_binding_needs_its_bracket(self):
        from module_gate import compile_group
        recipe = json.loads((ROOT / "recipes/winlife/events.json").read_text())
        binding = json.loads((ROOT / recipe["binding"]).read_text())
        for name, b, want in (("bracket", binding, True),
                              ("plain", {k: v for k, v in binding.items() if k != "brackets"}, False)):
            _, report = compile_group((ROOT / recipe["source"]).read_text(), recipe["profile"], recipe["flags"],
                                      [{"target_id": c["target_id"], "public": c["public"]} for c in recipe["claims"]],
                                      ROOT / "build/workers/tests/bracket" / name, binding=b,
                                      group_id=recipe["group_id"], language="asm")
            claim = next(c for c in report["claims"] if c["public"] == "DOS_CRITICAL_ERROR")
            self.assertIs(claim["exact"], want)


class NearAddressTests(unittest.TestCase):
    """`mov ax, OFFSET Near` / `mov ax, SEG Near` in CONFIGURE_SOUND_DEVICES."""

    def run_group(self, label, edit=lambda source: source, change=lambda binding: None):
        from assembler import assemble_asm
        from binding import declarations, fixes
        from module_gate import check_group
        from omf import OmfReader
        from oracle import Oracle
        recipe = json.loads((ROOT / "recipes/sound2/dos_sound_runtime.json").read_text())
        binding = json.loads((ROOT / recipe["binding"]).read_text())
        work = ROOT / "build/workers/tests/near_address" / label
        work.mkdir(parents=True, exist_ok=True)
        source = edit((ROOT / recipe["source"]).read_text())
        (work / "GROUP.ASM").write_bytes(source.encode("ascii"))
        result = assemble_asm(work / "GROUP.ASM", recipe["profile"], flags=recipe["flags"], workdir=work / "asm")
        self.assertTrue(result.ok, result.log)
        module = OmfReader().read(result.obj_bytes)
        binding.update(declarations=declarations(module), fixups=fixes(module))
        change(binding)
        report = check_group(result.obj_bytes, [{"target_id": c["target_id"], "public": c["public"]}
                                                for c in recipe["claims"]], Oracle.load(), binding=binding,
                             group_id=recipe["group_id"])
        return next(c for c in report["claims"] if c["public"] == "CONFIGURE_SOUND_DEVICES")

    def test_offset_and_frame_paragraph_are_equations(self):
        claim = self.run_group("exact")
        self.assertTrue(claim["exact"])
        rows = {e["offset"]: e for e in claim["equations"]}
        self.assertEqual((rows[0x1B]["linked_value"], rows[0x20]["linked_segment"]), (0x7D76, 0x1129))
        self.assertEqual((rows[0x26]["linked_value"], rows[0x2C]["linked_segment"]), (0x7E37, 0x1129))

    def test_swapped_routines_do_not_match(self):
        swap = lambda s: s.replace("OFFSET INSTALL_TIMER_HOOK", "OFFSET REMOVE_TIMER_HOOK", 1)
        self.assertFalse(self.run_group("swapped", swap)["exact"])

    def test_an_addend_or_another_frame_is_refused(self):
        addend = lambda s: s.replace("OFFSET INSTALL_TIMER_HOOK", "OFFSET INSTALL_TIMER_HOOK+1", 1)
        self.assertEqual(self.run_group("addend", addend)["state"], "REFUSED")
        other = lambda s: s.replace("SEG INSTALL_TIMER_HOOK", "SEG PROBE_FRAME", 1).replace(
            "EXTRN INSTALL_TIMER_HOOK:NEAR", "EXTRN INSTALL_TIMER_HOOK:NEAR\nEXTRN PROBE_FRAME:FAR", 1)
        self.assertFalse(self.run_group("frame", other)["exact"])


if __name__ == "__main__":
    unittest.main()

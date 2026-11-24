"""Linker-made runtime values (docs/runtime-fixups.md, 2026-11-24): the accepted members and the refutations of each rule."""
import copy
import sys
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from common import read_json
import common
import runtime_fixups
import runtime_layout

RECIPE = ROOT / "recipes/runtime/crt_crt0_medium.json"
AVAILABLE = RECIPE.exists() and Path(read_json(RECIPE)["library_path"]).exists()
RUN = "evidence/bindings/runtime_tail_run.json"


@unittest.skipUnless(AVAILABLE, "pinned MSC 6.00 medium-model library not installed")
class LinkerValueTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.manifest = read_json(ROOT / "layout/manifest.json")

    def binding(self, name):
        return copy.deepcopy(read_json(ROOT / f"evidence/bindings/crt_{name}_medium.json"))

    def prove(self, name, binding=None, run=None):
        recipe = read_json(ROOT / f"recipes/runtime/crt_{name}_medium.json")
        binding = binding or self.binding(name)
        module, overwritten = runtime_fixups.read_member(recipe)
        code = module.segments["_TEXT"]
        start = self.oracle.spaces["root"].position(recipe["segment"], recipe["offset"])
        real = common.read_json_shared
        patch = mock.patch("common.read_json_shared", lambda p: copy.deepcopy(run) if str(p).replace("\\", "/").endswith(RUN)
                           else real(p)) if run is not None else mock.patch("builtins.id", id)
        with patch:
            return runtime_fixups.prove(module, overwritten, "_TEXT", code,
                                        self.oracle.spaces["root"].data[start:start + len(code)], binding, recipe,
                                        self.oracle, self.manifest)

    def refuse(self, name, change, message=None):
        binding = self.binding(name)
        change(binding)
        try:
            report = self.prove(name, binding)
        except ValueError as error:
            if message:
                self.assertRegex(str(error), message)
            return
        self.assertFalse(report["exact"])

    def test_pinned_members_are_exact(self):
        for name in ("stackava", "fclose", "crt0dat", "crt0"):
            self.assertTrue(self.prove(name)["exact"], name)

    def test_communal_has_one_place_between_its_neighbours(self):
        def row(b):
            return b["communals"][0]
        self.refuse("stream", lambda b: row(b).update(offset=0x696A), "another place")
        self.refuse("stream", lambda b: row(b)["fit"].update(below=row(b)["fit"]["above"]))
        self.refuse("stream", lambda b: row(b)["fit"]["above"].update(bytes_hex="a06d6b"), "not reached")
        self.refuse("stream", lambda b: row(b)["fit"]["below"].update(image_offset=row(b)["fit"]["below"]["image_offset"] - 1),
                    "not reached")
        # A lower neighbour two bytes further down leaves a second even place.
        root = row(self.binding("stream"))["fit"]["below"]
        space = self.oracle.spaces[root["space"]]
        at = root["image_offset"] - 3
        wider = dict(root, image_offset=at, bytes_hex=bytes(space.data[at:at + 3]).hex())
        self.refuse("stream", lambda b: row(b)["fit"].update(below=wider), "another place")
        self.refuse("stream", lambda b: row(b).update(name="__bufout"), "Unsupported runtime communal")

    def test_end_needs_another_members_equal_fields(self):
        self.refuse("chkstk", lambda b: b["linker_symbols"]["_end"].update(offset=0x6B76), "not the value")
        self.refuse("crt0", lambda b: b["linker_symbols"].update(_end=self.binding("chkstk")["linker_symbols"]["_end"]),
                    "judged member")
        self.refuse("crt0", lambda b: b["linker_symbols"]["_end"]["witness"].update(recipe="crt_fclose_medium"), "not the value")
        self.refuse("crt0", lambda b: b["linker_symbols"]["_edata"].update(offset=0x27C2), "BSS class")
        self.refuse("chkstk", lambda b: b["linker_symbols"]["_end"].update(witness={"kind": "bss-start"}), "BSS class")

    def test_tail_run_is_fixed_by_order_lengths_and_both_neighbours(self):
        def changed(**fields):
            run = copy.deepcopy(read_json(ROOT / RUN))
            run.update(fields)
            return run
        base = read_json(ROOT / RUN)
        self.assertTrue(self.prove("d_rdwr", run=base)["exact"])
        swapped = [base["segments"][1], base["segments"][0], *base["segments"][2:]]
        bad = [changed(start=0x26A2), changed(start=0x26A6), changed(end=0x26C4), changed(segments=swapped),
               changed(segments=base["segments"][:-1]), changed(members=base["members"][:2]),
               changed(members=[base["members"][1], base["members"][0], base["members"][2]]),
               changed(below={"group": "group:itemdlg/itemdlg", "segment": "_DATA"}),
               changed(above={"group": "group:draw/draw", "segment": "CONST"}),
               changed(code_anchors=base["code_anchors"][:1]),
               changed(code_anchors=[dict(base["code_anchors"][0], offset=0x76), base["code_anchors"][1]]),
               changed(segment_bases={"C_ETEXT": {"segment": 0x3109, "witnesses": base["segment_bases"]["C_ETEXT"]["witnesses"]}}),
               changed(segment_bases={"C_ETEXT": {"segment": 0x313B, "witnesses": [
                   {"space": "overlay-17", "image_offset": 0x1457, "relocation_index": 18}]}}),
               changed(segment_bases={"C_ETEXT": {"segment": 0x306E, "witnesses": [
                   {"space": "overlay-17", "image_offset": 0x4FF2, "relocation_index": 147}]}})]
        for i, run in enumerate(bad):
            with self.subTest(i), self.assertRaises(ValueError):
                self.prove("d_rdwr", run=run)
        self.refuse("d_rdwr", lambda b: b["tail_runs"][0].update(sha256="0" * 64), "Tail run evidence changed")
        self.refuse("d_rdwr", lambda b: b["data_providers"][0].update(placement=0x26A8))
        self.refuse("d_rdwr", lambda b: b.pop("tail_runs"), "not a placeable")

    def test_empty_bss_markers_need_one_address_and_both_fields(self):
        self.refuse("crt0dat", lambda b: b.update(empty_consensus={"XOB": 0x2B88, "XOE": 0x2B8A}), "adjacent empty")
        self.refuse("crt0dat", lambda b: b.update(empty_consensus={"XOB": 0x2B88}), "adjacent empty")
        self.refuse("crt0dat", lambda b: b.update(empty_consensus={"XOB": 0x2B86, "XOE": 0x2B86}))
        self.refuse("crt0dat", lambda b: b.update(empty_consensus={"XOB": 0x2B88, "XIB": 0x2B88}), "adjacent empty")
        self.refuse("crt0dat", lambda b: b.update(empty_consensus={"XOB": 0x2B88, "XO": 0x2B88}))

    def test_second_code_segment_follows_the_last_owned_code(self):
        self.refuse("crt0dat", lambda b: b["second_code"]["EMULATOR_TEXT"].update(image_offset=0x220E0), "last owned code")
        self.refuse("crt0dat", lambda b: b.pop("second_code"), "Unplaced secondary storage EMULATOR_TEXT")
        self.refuse("crt0dat", lambda b: b.update(segment_bases={"EMULATOR_DATA": {"segment": 0x306E, "witnesses": [
            {"space": "root", "image_offset": 0x30C8, "relocation_index": 358}]}}))
        self.refuse("crt0dat", lambda b: b["segment_bases"]["EMULATOR_DATA"].update(witnesses=[]))

    def test_program_start_has_an_original_far_pointer(self):
        self.refuse("crt0", lambda b: b.pop("start"), "start address")
        self.refuse("crt0", lambda b: b["start"].update(image_offset=0x2279B), "no original far pointer")
        self.refuse("crt0", lambda b: b["start"].update(relocation_index=1990), "no original far pointer")
        self.refuse("chkstk", lambda b: b.update(start=self.binding("crt0")["start"]), "start address")

    def test_main_uses_the_frame_its_accepted_group_proves(self):
        self.refuse("crt0", lambda b: b["external_providers"][0].update(target_id="root:0000:00b2"), "does not name")
        self.refuse("crt0", lambda b: b.pop("external_providers"))

    def test_data_provider_rules_stay_closed(self):
        # `__bufin` and `_end` are the only external names a provider's data may hold.
        self.refuse("stream", lambda b: b.pop("communals"))
        self.refuse("stackava", lambda b: b.pop("linker_symbols"))


if __name__ == "__main__":
    unittest.main()

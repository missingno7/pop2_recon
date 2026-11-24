"""rtlink-manager-v1: whole RTLink 6.10 manager members with grounded output-segment frames.

Uses the immutable original and the pinned RTLink/Plus 6.10 RTLUTILS.LIB; skipped when
the independent library is not installed.
"""
import copy
import sys
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from common import read_json
import rtlink_fixups
from runtime import verify_member

RECIPES = ROOT / "recipes/runtime"
AVAILABLE = (RECIPES / "rtl_pathsrch.json").exists() and \
    Path(read_json(RECIPES / "rtl_pathsrch.json")["library_path"]).exists()


@unittest.skipUnless(AVAILABLE, "pinned RTLink 6.10 RTLUTILS.LIB not installed")
class RtlinkManagerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.manifest = read_json(ROOT / "layout/manifest.json")

    def recipe(self, name):
        return read_json(RECIPES / f"{name}.json")

    def prove(self, name, *, recipe=None, binding=None, expected=None, manifest=None):
        recipe = recipe or self.recipe(name)
        binding = binding or read_json(ROOT / recipe["binding"])
        module, overwritten = rtlink_fixups.read_member(recipe)
        code = module.segments[recipe["object_segment"]]
        space = self.oracle.spaces["root"]
        start = space.position(recipe["segment"], recipe["offset"])
        expected = expected or space.data[start:start+len(code)]
        return rtlink_fixups.prove(module, overwritten, recipe["object_segment"], code, expected,
                                   binding, recipe, self.oracle, manifest or self.manifest)

    def test_accepts_members_with_far_call_and_site_record_frames(self):
        report = verify_member(self.recipe("rtl_pathsrch"), self.oracle, self.manifest)
        self.assertTrue(report["exact"])
        self.assertEqual(report["fixups"], 7)
        self.assertEqual({e["target"] for e in report["equations"]},
                         {"segment:$$OVLMGR_UTILS", "external:$$RTLPATHSEARCH_MESSAGES"})
        self.assertEqual(report["segment_frames"]["$$OVLMGR_UTILS"]["segment"], 0x22f2)
        # ovlmgr_m: frame 220e from an MZ record outside it; `4 dup (?)` gaps link as zero.
        report = verify_member(self.recipe("rtl_ovlmgr_m"), self.oracle, self.manifest)
        self.assertTrue(report["exact"])
        self.assertEqual(report["fixups"], 11)
        self.assertEqual(report["segment_frames"]["$$OVLMGR"]["witnesses"][0]["kind"], "site_record")

    def test_wrong_segment_frame_refused(self):
        binding = read_json(ROOT / self.recipe("rtl_pathsrch")["binding"])
        wrong = copy.deepcopy(binding)
        wrong["segment_frames"]["$$OVLMGR_UTILS"]["segment"] = 0x22f1
        with self.assertRaisesRegex(ValueError, "does not reach a unique pinned public"):
            self.prove("rtl_pathsrch", binding=wrong)
        # A root CALL FAR through the $$OVLMGR group frame (vector 220e:0cbd) is no
        # witness for a segment frame: it reaches no pinned public of that segment.
        group = copy.deepcopy(binding)
        group["segment_frames"]["$$OVLMGR_UTILS"] = {"segment": 0x220e, "witnesses": [
            {"kind": "far_call", "image_offset": 0x7, "relocation_index": 28,
             "provider": "rtl_cnv_val", "symbol": "$$RTLOVL_CONVERT_VAL"}]}
        with self.assertRaisesRegex(ValueError, "does not reach a unique pinned public"):
            self.prove("rtl_pathsrch", binding=group)
        # An MZ record of another segment (frame 220d, before the manager) refuses.
        site = read_json(ROOT / self.recipe("rtl_ovlmgr_m")["binding"])
        site["segment_frames"]["$$OVLMGR"] = {"segment": 0x220d, "witnesses": [
            {"kind": "site_record", "relocation_index": 1947}]}
        with self.assertRaisesRegex(ValueError, "does not overlap"):
            self.prove("rtl_ovlmgr_m", binding=site)
        # The equations themselves depend on the frame: a frame off by one paragraph fails.
        original = rtlink_fixups._ground_frames

        def shifted(*args):
            frames, receipts = original(*args)
            return {k: v-1 for k, v in frames.items()}, receipts
        with mock.patch.object(rtlink_fixups, "_ground_frames", shifted):
            self.assertFalse(self.prove("rtl_pathsrch", binding=binding)["exact"])

    def test_changed_byte_refused(self):
        recipe = self.recipe("rtl_ovlmgr_m")
        space = self.oracle.spaces["root"]
        start = space.position(recipe["segment"], recipe["offset"])
        for at in (0x30, 174):  # message text, and a zero-linked `?` gap
            data = bytearray(space.data[start:start+recipe["size"]])
            data[at] ^= 0x01
            report = self.prove("rtl_ovlmgr_m", expected=bytes(data))
            self.assertFalse(report["exact"])
            self.assertEqual(report["first_difference"], at)
        data = bytearray(space.data[start:start+recipe["size"]])
        data[0] ^= 0x01  # a fixup field (dw offset quote_newline)
        self.assertFalse(self.prove("rtl_ovlmgr_m", expected=bytes(data))["exact"])

    def test_unprovable_manager_providers_refused(self):
        # The library's ovlmgr.asm member needs linker-defined providers, including $$MAIN.
        # A v1.1 site record outside the manager body establishes the $$OVLMGR frame.
        from common import sha
        recipe = self.recipe("rtl_ovlmgr_m")
        raw = Path(recipe["library_path"]).read_bytes()[0x1200:0x1200+3674]
        recipe.update(archive_offset=0x1200, member_extent=3674, size=2491, offset=0x152,
                      member_sha256=sha(raw))
        binding = {"schema": 1, "mode": rtlink_fixups.MODE, "target_sha256": sha(self.oracle.data),
                   "segment_frames": {"$$OVLMGR": {"segment": 0x220e, "witnesses": [
                       {"kind": "site_record", "relocation_index": 1948}]}}}
        with self.assertRaisesRegex(ValueError, r"Missing pinned providers for \['\$\$MAIN'"):
            self.prove("rtl_ovlmgr", recipe=recipe, binding=binding)

    def test_manager_class_needs_rtlink_library_and_alignment(self):
        recipe = self.recipe("rtl_ovlmgr_m")
        module, _ = rtlink_fixups.read_member(recipe)
        segment = module.segment_defs[0]
        ranges = module.initialized_ranges[segment["name"]]
        rtlink_fixups.check_segment(recipe, segment, ranges)
        with self.assertRaisesRegex(ValueError, "pinned RTLink library"):
            rtlink_fixups.check_segment(dict(recipe, library_sha256="0"*64), segment, ranges)
        with self.assertRaisesRegex(ValueError, "incomplete or trimmed"):
            rtlink_fixups.check_segment(recipe, segment, ranges[:-1])
        with self.assertRaisesRegex(ValueError, "alignment"):
            rtlink_fixups.check_alignment(segment, 0x220e8)


if __name__ == "__main__":
    unittest.main()

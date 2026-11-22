"""runtime-symbolic-v1 extensions: signature-anchored code providers, code addresses in
placed data, and the MSG class run (common PAD/EPAD).

Uses the immutable original and the pinned MSC 6.00 medium-model library; skipped when
the independent library is not installed.
"""
import copy
import sys
import unittest
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import common
from common import read_json, sha
import runtime_fixups
from omf import OmfReader

LIBRARY = "C:/tools/msc-6.00/LIB/mlibcr.lib"
AVAILABLE = Path(LIBRARY).exists() and (ROOT / "evidence/bindings/crt_fopen_medium.json").exists()
FRAME = 0x1fe6
RUN = "evidence/bindings/runtime_msg_run.json"
_real_read_json = common.read_json


def member(archive_offset):
    from runtime_scan import archive_members
    data = Path(LIBRARY).read_bytes()
    raw = dict(archive_members(data))[archive_offset]
    return {"library_path": LIBRARY, "library_sha256": sha(data), "archive_offset": archive_offset,
            "member_extent": len(raw), "member_sha256": sha(raw)}


# name -> (archive offset, frame offset)
MEMBERS = {"crt_fopen_medium": (0x2b90, 0x70c), "crt_nheapchk_medium": (0xcdb0, 0x181c),
           "crt_output_medium": (0x4d40, 0xb7a), "crt_crt0fp_medium": (0xe70, 0x316),
           "crt_crt0msg_medium": (0xc90, 0x2f4), "crt_chksum_medium": (0x1180, 0x340)}


def recipe(name):
    offset, frame_offset = MEMBERS[name]
    return dict(member(offset), name=name, space="root", segment=FRAME, offset=frame_offset,
                object_segment="_TEXT", binding=f"evidence/bindings/{name}.json")


def owner(name, size, path):
    offset = MEMBERS[name][1]
    return {"space": "root", "segment": FRAME, "offset": offset, "size": size,
            "target_id": f"root:{FRAME:04x}:{offset:04x}", "kind": "PINNED_RUNTIME",
            "state": "PINNED_RUNTIME", "recipe": path}


@unittest.skipUnless(AVAILABLE, "pinned MSC 6.00 medium-model library not installed")
class RuntimeAnchorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.manifest = read_json(ROOT / "layout/manifest.json")

    def prove(self, name, *, binding=None, manifest=None, oracle=None, fake=None):
        """Prove a member; `fake` maps extra recipe paths to recipe dicts (simulated publication)."""
        r = recipe(name)
        binding = binding or read_json(ROOT / r["binding"])
        oracle = oracle or self.oracle
        module, overwritten = runtime_fixups.read_member(r)
        code = module.segments["_TEXT"]
        start = oracle.spaces["root"].position(FRAME, r["offset"])

        def fake_read(path):
            key = Path(path).resolve().relative_to(ROOT).as_posix() if Path(path).is_absolute() else str(path)
            return copy.deepcopy(fake[key]) if fake and key in fake else _real_read_json(path)
        with mock.patch("common.read_json", fake_read):
            return runtime_fixups.prove(module, overwritten, "_TEXT", code,
                                        oracle.spaces["root"].data[start:start+len(code)], binding, r,
                                        oracle, manifest or self.manifest)

    def binding(self, name):
        return read_json(ROOT / f"evidence/bindings/{name}.json")

    def with_crt0fp(self):
        """The manifest after publishing crt0fp (its recipe served from memory). Once crt0fp is
        canonically pinned, the canonical owner is replaced so the provider stays unique."""
        manifest = copy.deepcopy(self.manifest)
        manifest["owners"] = [o for o in manifest["owners"]
                              if not str(o.get("recipe", "")).endswith("crt_crt0fp_medium.json")]
        path = "build/workers/test_anchor/crt_crt0fp_medium.json"
        manifest["owners"].append(owner("crt_crt0fp_medium", 6, path))
        return manifest, {path: recipe("crt_crt0fp_medium")}

    # E1 signature-anchored providers
    def test_anchored_providers_prove_fopen(self):
        report = self.prove("crt_fopen_medium")
        self.assertTrue(report["exact"])
        self.assertEqual(report["relocation_indices"], [1920, 1933, 1944])
        anchors = {a["name"]: a for a in report["code_anchors"]}
        self.assertEqual(set(anchors), {"anchor_open", "anchor_stream"})
        self.assertEqual(anchors["anchor_stream"]["position"], FRAME*16+0x1062)
        self.assertGreaterEqual(anchors["anchor_stream"]["ordinary_bytes"], 16)

    def test_shifted_or_short_or_ambiguous_anchor_refused(self):
        binding = self.binding("crt_fopen_medium")
        binding["code_anchors"][1]["offset"] -= 1        # unowned alignment byte before stream
        with self.assertRaisesRegex(ValueError, "ordinary bytes differ"):
            self.prove("crt_fopen_medium", binding=binding)
        trampoline = dict(member(0x6440), name="anchor_malloc", object_segment="_TEXT", offset=0x1440)
        binding = self.binding("crt_fopen_medium")
        binding["code_anchors"].append(trampoline)
        binding["code_providers"]["_malloc"] = "anchor_malloc"
        with self.assertRaisesRegex(ValueError, "fewer than 16 ordinary bytes"):
            self.prove("crt_fopen_medium", binding=binding)
        with mock.patch.object(runtime_fixups, "ANCHOR_MIN_ORDINARY", 1):
            with self.assertRaisesRegex(ValueError, "not unique in the frame window"):
                self.prove("crt_fopen_medium", binding=binding)

    def test_anchor_overlapping_judged_member_or_owner_refused(self):
        binding = self.binding("crt_fopen_medium")
        binding["code_anchors"].append(dict(member(0x2b90), name="anchor_self", object_segment="_TEXT", offset=0x70c))
        binding["code_providers"]["__fsopen"] = "anchor_self"
        with self.assertRaisesRegex(ValueError, "overlaps the judged member"):
            self.prove("crt_fopen_medium", binding=binding)
        binding = self.binding("crt_fopen_medium")
        binding["code_anchors"][1]["offset"] = 0x5fc        # crt_dosret_medium's owned extent
        with self.assertRaisesRegex(ValueError, "overlaps owner root:1fe6:05fc"):
            self.prove("crt_fopen_medium", binding=binding)

    def test_unused_or_misnamed_anchor_refused(self):
        binding = self.binding("crt_fopen_medium")
        del binding["code_providers"]["__getstream"]
        with self.assertRaisesRegex(ValueError, "Unused anchored code provider"):
            self.prove("crt_fopen_medium", binding=binding)
        binding = self.binding("crt_fopen_medium")
        binding["code_providers"]["__getstream"] = "anchor_open"
        with self.assertRaisesRegex(ValueError, "does not declare __getstream"):
            self.prove("crt_fopen_medium", binding=binding)
        binding = self.binding("crt_fopen_medium")
        binding["code_anchors"][1]["name"] = "crt_dosret_medium"
        with self.assertRaisesRegex(ValueError, "recipe name"):
            self.prove("crt_fopen_medium", binding=binding)

    def test_anchor_may_coincide_with_same_member_pinned_later(self):
        manifest = copy.deepcopy(self.manifest)
        path = "build/workers/test_anchor/crt_stream_medium.json"
        manifest["owners"].append(dict(owner("crt_fopen_medium", 52, path), offset=0x1062,
                                       target_id="root:1fe6:1062"))
        pinned = dict(member(0x5500), name="crt_stream_medium", space="root", segment=FRAME, offset=0x1062,
                      object_segment="_TEXT")
        self.assertTrue(self.prove("crt_fopen_medium", manifest=manifest, fake={path: pinned})["exact"])
        other = dict(pinned, **member(0x3bb0), name="crt_getbuf_medium")   # a different member there
        with self.assertRaisesRegex(ValueError, "overlaps owner root:1fe6:1062"):
            self.prove("crt_fopen_medium", manifest=manifest, fake={path: other})

    # E3 code addresses inside placed data
    def test_code_offset_in_provider_data(self):
        report = self.prove("crt_nheapchk_medium")
        self.assertTrue(report["exact"])
        crt0 = next(p for p in report["data_placements"] if p["placement"] == 0x2256)
        self.assertEqual([(e["kind"], e["linked"]) for e in crt0["equations"]], [("CODE frame offset", 0x238)])
        binding = self.binding("crt_nheapchk_medium")
        del binding["code_providers"]["__exit"]
        binding["code_anchors"] = []
        with self.assertRaisesRegex(ValueError, "Missing pinned code provider for __exit"):
            self.prove("crt_nheapchk_medium", binding=binding)

    def test_far_code_pointers_in_provider_data(self):
        manifest, fake = self.with_crt0fp()
        report = self.prove("crt_output_medium", manifest=manifest, fake=fake)
        self.assertTrue(report["exact"])
        self.assertEqual(report["fixups"], 64)
        cmisc = next(p for p in report["data_placements"] if p["placement"] == 0x249c)
        self.assertEqual([e["kind"] for e in cmisc["equations"]], ["far code pointer"]*6)
        self.assertEqual({e["linked"] for e in cmisc["equations"]}, {(FRAME << 16) | 0x316})
        self.assertEqual(cmisc["relocation_indices"], [116, 117, 118, 119, 120, 121])
        without = dict(manifest, owners=[o for o in manifest["owners"]
                                         if "crt_crt0fp_medium" not in str(o.get("recipe", ""))])
        with self.assertRaisesRegex(ValueError, "Code provider __fptrap is missing"):
            self.prove("crt_output_medium", manifest=without)
        binding = self.binding("crt_output_medium")
        binding["data_providers"][0]["placement"] += 2
        with self.assertRaisesRegex(ValueError, "differs from the original DGROUP bytes"):
            self.prove("crt_output_medium", binding=binding, manifest=manifest, fake=fake)

    def test_far_code_pointer_needs_its_original_relocation(self):
        manifest, fake = self.with_crt0fp()
        space = self.oracle.spaces["overlay-17"]
        relocations = tuple(r for r in space.relocations if r["index"] != 118)
        spaces = dict(self.oracle.spaces, **{"overlay-17": replace(space, relocations=relocations)})
        oracle = SimpleNamespace(data=self.oracle.data, spaces=spaces, mz=self.oracle.mz)
        with self.assertRaisesRegex(ValueError, "original relocations differ from the generated"):
            self.prove("crt_output_medium", manifest=manifest, fake=fake, oracle=oracle)

    # E4 MSG class run and common segments
    def test_class_run_places_common_pad(self):
        for name in ("crt_crt0fp_medium", "crt_crt0msg_medium", "crt_chksum_medium"):
            report = self.prove(name)
            self.assertTrue(report["exact"], name)
            pad = next(p for p in report["data_placements"] if p["segment"] == "PAD")
            self.assertEqual((pad["placement"], pad["common"]), (0x292c, True))
            self.assertEqual(report["class_runs"][0]["end"], 0x292f)
        runtime_fixups.check_layout([self.prove(n) for n in ("crt_crt0fp_medium", "crt_chksum_medium")])

    def test_common_segment_without_or_against_run_refused(self):
        binding = self.binding("crt_crt0fp_medium")
        binding["placements"]["PAD"] = 0x292d
        with self.assertRaisesRegex(ValueError, "differs from its class run"):
            self.prove("crt_crt0fp_medium", binding=binding)
        del binding["class_runs"]
        with self.assertRaisesRegex(ValueError, "not a placeable public DGROUP data segment"):
            self.prove("crt_crt0fp_medium", binding=binding)
        binding = self.binding("crt_fopen_medium")
        binding["class_runs"] = self.binding("crt_crt0fp_medium")["class_runs"]
        with self.assertRaisesRegex(ValueError, "Unused class run evidence"):
            self.prove("crt_fopen_medium", binding=binding)

    def prove_run(self, mutate):
        run = read_json(ROOT / RUN)
        mutate(run)

        def fake_read(path):
            return copy.deepcopy(run) if Path(path).as_posix().endswith(RUN) else _real_read_json(path)
        r = recipe("crt_crt0fp_medium")
        module, overwritten = runtime_fixups.read_member(r)
        code = module.segments["_TEXT"]
        start = FRAME*16+0x316
        with mock.patch("common.read_json", fake_read):
            return runtime_fixups.prove(module, overwritten, "_TEXT", code, self.oracle.spaces["root"].data[start:start+6],
                                        self.binding("crt_crt0fp_medium"), r, self.oracle, self.manifest)

    def test_class_run_must_be_gap_free_to_payload_end(self):
        self.assertTrue(self.prove_run(lambda run: None)["exact"])
        with self.assertRaisesRegex(ValueError, "differs from the original DGROUP bytes"):
            self.prove_run(lambda run: run["contributions"].pop(2))        # drop crt0fp MSG: gap
        with self.assertRaisesRegex(ValueError, "does not end exactly at the original DGROUP payload end"):
            self.prove_run(lambda run: run["contributions"].pop())          # drop EPAD
        with self.assertRaisesRegex(ValueError, "must have one contributor"):
            self.prove_run(lambda run: run["contributions"][1]["members"].append(member(0xe70)))
        with self.assertRaisesRegex(ValueError, "not a complete fixup-free MSG DGROUP contribution"):
            self.prove_run(lambda run: run["contributions"].__setitem__(0, {"segment": "_DATA", "members": [member(0x10)]}))
        binding = self.binding("crt_crt0fp_medium")
        binding["class_runs"][0]["sha256"] = "0"*64
        with self.assertRaisesRegex(ValueError, "Class run evidence changed"):
            self.prove("crt_crt0fp_medium", binding=binding)

    def test_layout_allows_only_shared_common_starts(self):
        pad = {"member_sha256": "a", "segment": "PAD", "placement": 16, "length": 2, "common": True}
        runtime_fixups.check_layout([{"data_placements": [pad]},
                                     {"data_placements": [dict(pad, member_sha256="b")]}])
        with self.assertRaisesRegex(ValueError, "overlap"):
            runtime_fixups.check_layout([{"data_placements": [pad]},
                                         {"data_placements": [dict(pad, member_sha256="b", placement=17)]}])
        with self.assertRaisesRegex(ValueError, "overlap"):
            runtime_fixups.check_layout([{"data_placements": [pad]},
                                         {"data_placements": [dict(pad, member_sha256="b", common=False)]}])


if __name__ == "__main__":
    unittest.main()

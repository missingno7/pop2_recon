"""runtime-symbolic-v1 provider extensions: accepted external code providers (E7) and
same-library selection among duplicate pinned publics (E6).

Uses the immutable original, the accepted nmalloc owners and the pinned MSC 6.00
medium-model library; skipped when the independent library is not installed.
"""
import copy
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import common
from common import read_json, sha
import runtime_fixups

LIBRARY = "C:/tools/msc-6.00/LIB/mlibcr.lib"
AVAILABLE = Path(LIBRARY).exists() and (ROOT / "evidence/bindings/crt_malloc_medium.json").exists()
FRAME = 0x1fe6
_real_read_json = common.read_json

# name -> (archive offset, frame offset, size)
MEMBERS = {"crt_malloc_medium": (25664, 0x1440, 5), "crt_free_medium": (25824, 0x1446, 5),
           "crt_ncalloc_medium": (52240, 0x17ee, 46), "crt_strdup_medium": (79120, 0x213a, 40)}
FAKE = "build/workers/test_providers/crt_malloc_medium.json"


def member(archive_offset):
    from runtime_scan import archive_members
    data = Path(LIBRARY).read_bytes()
    raw = dict(archive_members(data))[archive_offset]
    return {"library_path": LIBRARY, "library_sha256": sha(data), "archive_offset": archive_offset,
            "member_extent": len(raw), "member_sha256": sha(raw)}


def recipe(name):
    offset, frame_offset, _ = MEMBERS[name]
    return dict(member(offset), name=name, space="root", segment=FRAME, offset=frame_offset,
                object_segment="_TEXT", binding=f"evidence/bindings/{name}.json")


@unittest.skipUnless(AVAILABLE, "pinned MSC 6.00 medium-model library not installed")
class RuntimeProviderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from oracle import Oracle
        cls.oracle = Oracle.load()
        cls.manifest = read_json(ROOT / "layout/manifest.json")

    def prove(self, name, *, binding=None, manifest=None, fake=None):
        r = recipe(name)
        binding = binding or read_json(ROOT / r["binding"])
        module, overwritten = runtime_fixups.read_member(r)
        code = module.segments["_TEXT"]
        start = FRAME*16+r["offset"]

        def fake_read(path):
            key = Path(path).resolve().relative_to(ROOT).as_posix() if Path(path).is_absolute() else str(path)
            return copy.deepcopy(fake[key]) if fake and key in fake else _real_read_json(path)
        with mock.patch("common.read_json", fake_read):
            return runtime_fixups.prove(module, overwritten, "_TEXT", code,
                                        self.oracle.spaces["root"].data[start:start+len(code)], binding, r,
                                        self.oracle, manifest or self.manifest)

    def binding(self, name):
        return read_json(ROOT / f"evidence/bindings/{name}.json")

    def with_malloc(self):
        """The manifest after publishing the _malloc trampoline (recipe served from memory)."""
        if any(o["target_id"] == "root:1fe6:1440" for o in self.manifest["owners"]):
            return self.manifest, {}
        manifest = copy.deepcopy(self.manifest)
        manifest["owners"].append({"space": "root", "segment": FRAME, "offset": 0x1440, "size": 5,
                                   "target_id": "root:1fe6:1440", "kind": "PINNED_RUNTIME",
                                   "state": "PINNED_RUNTIME", "recipe": FAKE})
        return manifest, {FAKE: recipe("crt_malloc_medium")}

    # E7 accepted external code providers
    def test_accepted_owner_provides_far_target(self):
        for name, symbol, relocation in (("crt_malloc_medium", "__nmalloc", 1931), ("crt_free_medium", "__nfree", 1915),
                                         ("crt_ncalloc_medium", "__nmalloc", 1932)):
            report = self.prove(name)
            self.assertTrue(report["exact"], name)
            self.assertEqual(report["relocation_indices"], [relocation])
            receipt = report["external_providers"][0]
            self.assertEqual((receipt["symbol"], receipt["frame"], receipt["case_folded"]), (symbol, FRAME, True))
            self.assertEqual(receipt["public"], symbol.upper())

    def test_frame_witness_must_be_reviewed_and_outside(self):
        binding = self.binding("crt_malloc_medium")
        binding["external_providers"][0]["frame_witness"] = 0x212a1   # the trampoline's own far pointer
        with self.assertRaisesRegex(ValueError, "witness lies inside the judged member"):
            self.prove("crt_malloc_medium", binding=binding)
        binding["external_providers"][0]["frame_witness"] = 4998
        with self.assertRaisesRegex(ValueError, "not a reviewed relocated far call/pointer"):
            self.prove("crt_malloc_medium", binding=binding)

    def test_provider_must_be_an_accepted_owner_with_that_public(self):
        binding = self.binding("crt_malloc_medium")
        binding["external_providers"][0]["target_id"] = "root:0000:0000"
        with self.assertRaisesRegex(ValueError, "not one accepted exact resident owner"):
            self.prove("crt_malloc_medium", binding=binding)
        binding = self.binding("crt_malloc_medium")
        binding["external_providers"][0]["target_id"] = "root:1fe6:001a"
        binding["external_providers"][0]["frame_witness"] = 5114
        with self.assertRaisesRegex(ValueError, "does not name __nmalloc"):
            self.prove("crt_malloc_medium", binding=binding)
        recipe_path = "recipes/nmalloc/nmalloc.json"
        as_c = dict(_real_read_json(ROOT / recipe_path), language="c")
        with self.assertRaisesRegex(ValueError, "does not name __nmalloc"):
            self.prove("crt_malloc_medium", fake={recipe_path: as_c})

    def test_external_provider_cannot_shadow_pinned_or_be_unused(self):
        binding = self.binding("crt_malloc_medium")
        binding["external_providers"][0]["symbol"] = "__dosreturn"
        with self.assertRaisesRegex(ValueError, "duplicates a pinned or bound provider"):
            self.prove("crt_malloc_medium", binding=binding)
        binding = self.binding("crt_malloc_medium")
        binding["external_providers"].append({"symbol": "__nfree", "target_id": "root:1fe6:001a",
                                              "frame_witness": 5114})
        with self.assertRaisesRegex(ValueError, "Unused runtime provider bindings"):
            self.prove("crt_malloc_medium", binding=binding)

    def test_other_frame_provider_is_far_only(self):
        ctx = object.__new__(runtime_fixups.Context)
        ctx.frame, ctx.space, ctx.used = FRAME, SimpleNamespace(name="root"), set()
        ctx.code, ctx.code_frames = {"_main": 0}, {"_main": 0}
        self.assertEqual(ctx.far_target("_main", {}, 0), (0, 0))
        with self.assertRaisesRegex(ValueError, "another frame"):
            ctx.code_target("_main", {}, 0)

    # E6 same-library selection
    def test_same_library_selection_proves_strdup(self):
        manifest, fake = self.with_malloc()
        report = self.prove("crt_strdup_medium", manifest=manifest, fake=fake)
        self.assertTrue(report["exact"])
        self.assertEqual(report["relocation_indices"], [1903, 1919, 1929])
        self.assertEqual({s["symbol"]: s["recipe"] for s in report["same_library_selection"]},
                         {"_strcpy": "crt_strcpy_medium", "_strlen": "crt_strlen_medium"})

    def test_same_library_selection_refusals(self):
        manifest, fake = self.with_malloc()
        binding = self.binding("crt_strdup_medium")
        del binding["same_library_selection"]
        with self.assertRaisesRegex(ValueError, "Code provider _str(len|cpy) is missing, ambiguous"):
            self.prove("crt_strdup_medium", binding=binding, manifest=manifest, fake=fake)
        binding = self.binding("crt_strdup_medium")
        binding["code_providers"]["_strcpy"] = "crt_strcpy"           # the llibcr declaration
        with self.assertRaisesRegex(ValueError, "Same-library selection of _strcpy is not unambiguous"):
            self.prove("crt_strdup_medium", binding=binding, manifest=manifest, fake=fake)
        binding = self.binding("crt_strdup_medium")
        binding["same_library_selection"].append("_malloc")          # single declaration: nothing to select
        with self.assertRaisesRegex(ValueError, "Same-library selection of _malloc is not unambiguous"):
            self.prove("crt_strdup_medium", binding=binding, manifest=manifest, fake=fake)
        binding = self.binding("crt_strdup_medium")
        binding["same_library_selection"].append("_strcat")
        with self.assertRaisesRegex(ValueError, "Invalid same-library selection"):
            self.prove("crt_strdup_medium", binding=binding, manifest=manifest, fake=fake)


if __name__ == "__main__":
    unittest.main()

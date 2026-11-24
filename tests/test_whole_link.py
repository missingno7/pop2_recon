"""The whole-link plan stays complete and consistent (the link itself is `python tools/whole_link.py --check`)."""
import sys
import unittest
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from common import ROOT, read_json
import whole_link


class LinkPlanTests(unittest.TestCase):
    def setUp(self):
        self.plan = read_json(whole_link.PLAN)

    def placed(self):
        out = list(self.plan["root"])

        def walk(area):
            for section in area:
                out.extend(section["files"])
                for nested in section.get("areas", []):
                    walk(nested)
        for area in self.plan["areas"]:
            walk(area)
        for library in self.plan["libraries"]:
            out.extend(library.get("members", []))
        return out

    def test_every_accepted_component_is_linked_exactly_once(self):
        manifest = read_json(ROOT / "layout/manifest.json")
        paths = {o["recipe"] for o in manifest["owners"]} | set(manifest["data_groups"])
        accepted = {p[len("recipes/"):-len(".json")] for p in paths
                    if read_json(ROOT / p).get("kind") != "PINNED_RUNTIME"}
        modules = {"link:" + m["name"] for m in read_json(ROOT / "link/modules.json")}
        # Rebuilt run-time modules take their members' places in RTLUTILS.LIB instead of a FILE or library member.
        rebuilt = {p for p in accepted if read_json(ROOT / f"recipes/{p}.json").get("kind") == "manager_asm"}
        self.assertEqual(sorted(row["module"] for row in self.plan["rtlutils"]), sorted(rebuilt))
        accepted -= rebuilt
        counts = Counter(self.placed())
        self.assertEqual({n for n, c in counts.items() if c > 1}, set())
        self.assertEqual(set(counts), accepted | modules)
        self.assertEqual(set(self.plan["modules"]), accepted | modules)

    def test_file_names_are_unique_dos_names(self):
        files = [m["file"] for m in self.plan["modules"].values()]
        self.assertEqual(len(files), len(set(files)))
        self.assertTrue(all(f.replace("_", "").isalnum() and f == f.upper() and len(f) <= 8 for f in files))

    def test_fills_name_linked_modules(self):
        placed = set(self.placed())
        for fill in self.plan["fills"]:
            self.assertIn(fill.get("after") or fill["before"], placed)

    def test_rebuilt_modules_replace_whole_members_of_the_shipped_library(self):
        out = ROOT / "build/workers/tests/whole_link"
        shipped = Path(self.plan["tools"]["rtlutils"]["path"]).read_bytes()
        library = whole_link.rtlutils(self.plan, out)
        self.assertEqual(len(library), len(shipped))
        changed = [i for i in range(len(shipped)) if shipped[i] != library[i]]
        self.assertTrue(all(any(r["at"] <= i < r["at"] + r["length"] for r in self.plan["rtlutils"]) for i in changed))
        for row in self.plan["rtlutils"]:
            self.assertEqual({shipped[row["at"]], library[row["at"]]}, {0x80})            # a THEADR, before and after
            self.assertIn(shipped[row["at"] + row["length"]], (0, 0x80))                  # padding or the next member
        for change in ({"old_sha256": "0" * 64}, {"length": 16}):
            with self.assertRaises(ValueError):
                whole_link.rtlutils(dict(self.plan, rtlutils=[dict(self.plan["rtlutils"][0], **change)]), out)

    def test_comparison_of_the_original_with_itself(self):
        from oracle import Oracle
        oracle = Oracle.load()
        report = whole_link.compare(oracle.data, oracle)
        self.assertTrue(report["identical"] and report["mz_header_equal"])
        self.assertEqual(report["bytes_equal"], report["bytes_total"])


if __name__ == "__main__":
    unittest.main()

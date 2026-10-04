"""Fresh authoritative bootstrap validation; never alters locks or oracle assets."""
import argparse
import struct
import subprocess
import sys

from common import ROOT, read_json, require, sha, project_path, write_json
from match import compile_and_check, get_target
from metrics import metrics
from oracle import Oracle


def check_manifest(oracle, manifest):
    owned = {}
    ids = set()
    for owner in manifest["owners"]:
        require(owner["target_id"] not in ids, "Duplicate target owner")
        ids.add(owner["target_id"])
        require((owner["kind"], owner["state"]) in (("MATCHING_C", "CODE_EXACT"),
                                                      ("MATCHING_ASM", "ASM_EXACT"),
                                                      ("PINNED_RUNTIME", "PINNED_RUNTIME")),
                "Unsupported bootstrap ownership kind/state")
        recipe = read_json(project_path(owner["recipe"]))
        if owner["kind"] in ("MATCHING_C", "MATCHING_ASM"):
            target, space, _ = get_target(owner["target_id"], oracle)
        else:
            target = recipe
            space = oracle.spaces[target["space"]]
            require(owner["target_id"] == f"{space.name}:{target['segment']:04x}:{target['offset']:04x}",
                    "Runtime address identity mismatch")
        require(all(owner[key] == target[key] for key in ("space", "segment", "offset", "size")),
                "Owner disagrees with reviewed oracle extent")
        start = space.position(owner["segment"], owner["offset"])
        span = set(range(start, start+owner["size"]))
        require(not owned.setdefault(space.name, set()).intersection(span), "Overlapping canonical owners")
        owned[space.name].update(span)
        if owner["kind"] == "PINNED_RUNTIME":
            from runtime import verify_member
            verify_member(recipe, oracle)
            continue
        require(recipe["target_id"] == owner["target_id"], "Recipe target mismatch")
        source = project_path(recipe["source"])
        language = recipe.get("language", "c")
        require(language == ("asm" if owner["kind"] == "MATCHING_ASM" else "c"),
                "Recipe source language disagrees with ownership")
        source_dir = ROOT / ("asm" if language == "asm" else "src")
        require(source.is_relative_to(source_dir), "Canonical source outside its language directory")
        require(sha(source.read_bytes()) == recipe["source_sha256"], "Canonical source changed without acceptance")


def validate(run_tests=True):
    oracle = Oracle.load()
    structure = read_json(ROOT / "layout/structure.lock.json")
    spaces = [{"name": s.name, "link_segment": s.link_segment, "file_offset": s.file_offset,
               "size": s.size, "sha256": sha(s.data), "ordered_relocation_count": len(s.relocations),
               "ordered_relocation_sha256": sha(b"".join(struct.pack("<HH", r["offset"], r["segment"])
                                                        for r in s.relocations))}
              for s in oracle.spaces.values()]
    require(structure["target_sha256"] == sha(oracle.data) and
            structure["spaces"] == spaces and structure["gaps"] == oracle.gaps,
            "Parsed structure disagrees with original-derived frozen structure")
    manifest = read_json(ROOT / "layout/manifest.json")
    check_manifest(oracle, manifest)
    from compiler import load_lock, verify_profile
    profiles = read_json(ROOT / "recipes/compiler-profiles.json")
    # Pin checks cover installed tested profiles even before one owns canonical code.
    require(set(profiles["profiles"]).issubset(load_lock()["profiles"]), "Unknown recipe compiler profile")
    for profile in load_lock()["profiles"]:
        verify_profile(profile)
    from assembler import verify_assembler
    for profile in load_lock().get("assemblers", {}):
        verify_assembler(profile)
    accepted = []
    for owner in manifest["owners"]:
        recipe = read_json(project_path(owner["recipe"]))
        if owner["kind"] == "PINNED_RUNTIME":
            from runtime import verify_member
            accepted.append(verify_member(recipe, oracle))
            continue
        _, report = compile_and_check(project_path(recipe["source"]), recipe["profile"],
                                      recipe["public"], owner["target_id"],
                                      ROOT / "build/workers/validation" / recipe["name"], recipe["flags"],
                                      language=recipe.get("language", "c"))
        require(report["exact"], "Previously accepted source no longer compiles exactly")
        require(report["object_sha256"] == recipe["object_sha256"], "Accepted OMF identity changed")
        accepted.append(report)
    if run_tests:
        result = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v"], cwd=ROOT)
        require(result.returncode == 0, "Focused invariant tests failed")
    report = {"status": "PASS", "target_sha256": sha(oracle.data), "accepted": accepted,
              "metrics": metrics(oracle, manifest),
              "limits": ["Component code proof only; RTLink structural closure unrecovered",
                         "Fixup-bearing candidate acceptance not implemented"]}
    write_json(ROOT / "build/validation/report.json", report)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-tests", action="store_true", help="Local acceptance check; normal validation runs tests")
    args = parser.parse_args()
    report = validate(not args.skip_tests)
    print(f"PASS: {report['metrics']['exact_function_count']} freshly recompiled exact functions; "
          f"{report['metrics']['matching_c_bytes']} C bytes; "
          f"{report['metrics']['matching_asm_bytes']} ASM bytes; "
          f"{report['metrics']['pinned_runtime_bytes']} pinned runtime bytes; structural link UNRECOVERED")


if __name__ == "__main__":
    main()

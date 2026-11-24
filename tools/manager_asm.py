"""A whole hand-written RTLink run-time module rebuilt from MASM source (docs/runtime-fixups.md "Rebuilt manager modules").

The shipped RTLUTILS.LIB is a later revision than the original's for some members, so those cannot be pinned. A member
whose shipped source is assembly may be rebuilt instead: the freshly assembled object must hold exactly one segment, a
`$$` manager segment, and the complete segment must equal the original with every fixup proven as for a pinned manager
member (`rtlink_fixups.prove`). No byte is trimmed, masked or selected, and the module's publics provide other members.
"""
import argparse
import json
import re
from pathlib import Path

from common import CANONICAL_NAME, ROOT, json_bytes, project_path, read_json, require, sha
from omf import OmfReader
from rtlink_fixups import CLASSES, MODE, check_alignment, check_module_segment, prove

KIND = "manager_asm"


def verify(recipe, source, oracle, manifest, workdir, binding):
    """Assemble `source` (bytes) and judge the whole module; (report, object bytes)."""
    from assembler import assemble_asm
    from match import get_target
    require(recipe.get("kind") == KIND and recipe.get("language") == "asm" and binding.get("mode") == MODE,
            "Unsupported manager module recipe")
    work = Path(workdir)
    work.mkdir(parents=True, exist_ok=True)
    (work / "MODULE.ASM").write_bytes(source)
    result = assemble_asm(work / "MODULE.ASM", recipe["profile"], flags=recipe["flags"], workdir=work / "asm")
    require(result.ok, "Historical assembly failed: " + result.log)
    module = OmfReader().read(result.obj_bytes, "manager module")
    name = recipe["object_segment"]
    segments = [s for s in module.segment_defs if s["length"]]
    require(len(segments) == 1 and segments[0]["name"] == name == recipe["segment_class"] and name in CLASSES,
            "A manager module contributes exactly one manager segment")
    check_module_segment(segments[0], module.initialized_ranges.get(name))
    require(module.initialized_ranges.get(name) and _tiled(module.initialized_ranges[name], segments[0]["length"]),
            "A rebuilt manager module must be completely initialized")
    actual = module.segments[name]
    target, space, expected = get_target(recipe["target_id"], oracle)
    require(target["space"] == "root" and target.get("entry_kind") == "module" and
            all(recipe[k] == target[k] for k in ("space", "segment", "offset", "size")) and
            len(actual) == target["size"], "Manager module differs from its reviewed extent")
    start = space.position(target["segment"], target["offset"])
    check_alignment(segments[0], start)
    publics = sorted(({"name": p["name"], "offset": p["offset"]} for p in module.publics), key=lambda p: (p["offset"], p["name"]))
    require(all(p["segment"] == name and p["group_index"] == 0 and not p.get("local") for p in module.publics) and
            len({p["name"] for p in publics}) == len(publics), "Manager module public outside its segment")
    report = prove(module, [], name, actual, expected, binding, recipe, oracle, manifest)
    report.update(state="ASM_EXACT" if report["exact"] else "CANDIDATE_ASM", publics=publics, size=len(actual),
                  object_sha256=sha(result.obj_bytes), name=recipe["name"], target_id=recipe["target_id"],
                  covered_relocation_indices=report["relocation_indices"],
                  proof_scope="Complete rebuilt manager module: every byte and fixup from its assembly source; original "
                              "source text, natural RTLink placement and behavior unproved")
    return report, result.obj_bytes


def _tiled(ranges, length):
    at = 0
    for lo, hi in sorted(ranges):
        if lo != at:
            return False
        at = hi
    return at == length


def verify_recipe(recipe, oracle, manifest):
    """Fresh check of a canonical recipe."""
    source, raw = project_path(recipe["source"]).read_bytes(), project_path(recipe["binding"]).read_bytes()
    require(sha(source) == recipe["source_sha256"] and sha(raw) == recipe["binding_sha256"],
            "Manager module source or binding changed without acceptance")
    report, _ = verify(recipe, source, oracle, manifest, ROOT / "build/workers/validation" / recipe["name"], json.loads(raw))
    require(report["exact"] and report["object_sha256"] == recipe["object_sha256"] and
            report["publics"] == recipe["publics"] and report["fixups"] == recipe["fixup_count"] and
            report["relocation_indices"] == recipe["relocation_indices"],
            "Accepted manager module no longer assembles to its recorded proof")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", help="MASM source under build/workers")
    parser.add_argument("--name", required=True)
    parser.add_argument("--target", required=True, help="reviewed target row of kind `module`")
    parser.add_argument("--segment", required=True, choices=CLASSES)
    parser.add_argument("--binding", required=True)
    parser.add_argument("--profile", default="masm600")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    from match import get_target
    from oracle import Oracle
    from origin import require_genuine_asm
    from publication import publish, require_idle
    from validate import check_manifest
    require(re.fullmatch(CANONICAL_NAME, args.name), "Invalid canonical name")
    require_idle(ROOT)
    oracle = Oracle.load()
    manifest_before = (ROOT / "layout/manifest.json").read_bytes()
    manifest = json.loads(manifest_before)
    check_manifest(oracle, manifest)
    target, _, _ = get_target(args.target, oracle)
    require(all(o["target_id"] != args.target for o in manifest["owners"]), "Manager module is already owned")
    require_genuine_asm([args.target])
    raw = project_path(args.binding).read_bytes()
    require(b"\r" not in raw, "Canonical binding proof must use LF newlines")
    source = project_path(args.source).read_bytes().replace(b"\r\n", b"\n")
    recipe = {"schema": 1, "kind": KIND, "name": args.name, "language": "asm", "profile": args.profile, "flags": [],
              "target_id": args.target, "object_segment": args.segment, "segment_class": args.segment,
              "source": f"asm/{args.name}.asm", "source_sha256": sha(source),
              "binding": f"evidence/bindings/{args.name}.json", "binding_sha256": sha(raw),
              **{k: target[k] for k in ("space", "segment", "offset", "size")}}
    report, _ = verify(recipe, source, oracle, manifest, ROOT / "build/workers/promotion" / args.name, json.loads(raw))
    require(report["exact"], f"Manager module differs at +{report['first_difference']}")
    print(f"Verified {args.name}: {report['size']} ASM_EXACT bytes, {report['fixups']} fixups, "
          f"{report['relocations']} relocations, {len(report['publics'])} publics")
    if args.verify_only:
        return
    recipe.update(publics=report["publics"], fixup_count=report["fixups"], object_sha256=report["object_sha256"],
                  relocation_indices=report["relocation_indices"], proof_scope=report["proof_scope"])
    recipe_path = ROOT / "recipes" / (args.name + ".json")
    files = {project_path(recipe["source"]): source, project_path(recipe["binding"]): raw, recipe_path: json_bytes(recipe)}
    require(not any(p.exists() for p in files), "Canonical publication would overwrite a file")
    owner = {k: target[k] for k in ("space", "segment", "offset", "size")}
    owner.update(target_id=args.target, kind="REBUILT_RUNTIME", state="ASM_EXACT", recipe=recipe_path.relative_to(ROOT).as_posix(),
                 fixup_count=report["fixups"], relocation_count=report["relocations"],
                 relocation_indices=report["relocation_indices"])
    manifest["owners"].append(owner)
    publish(ROOT, files, manifest_before, manifest, lambda: check_manifest(oracle, manifest))
    print(f"Published {args.name}")


if __name__ == "__main__":
    main()

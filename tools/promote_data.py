"""Serialized publication of a data-only C group after a fresh historical compilation.

A data-only group is an original source file that defines initialized DGROUP data and no code
(a string or table module such as FM Towns ENGLISH.C or TABLES.C). It owns no code bytes; its
placed segments are judged exactly as a group's (module_gate.check_data_group) and its data
publics ground other groups' externs. The manifest lists it under `data_groups`.
"""
import argparse
import json
import re

from common import CANONICAL_NAME, ROOT, json_bytes, project_path, read_json, require, sha
from module_gate import DATA_GROUP_KIND, compile_data_group, require_no_case_alias, require_no_segname
from oracle import Oracle
from publication import publish, require_idle
from validate import check_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source")
    parser.add_argument("--name", required=True, help="Canonical name (src/NAME.c, recipes/NAME.json)")
    parser.add_argument("--profile", required=True)
    parser.add_argument("--flags", nargs="+")
    parser.add_argument("--binding", required=True, help="Binding with placements (and dgroup_segment)")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--stage", help="verify-only: staging directory (a worker's own build/workers/NAME/...)")
    args = parser.parse_args()
    require(re.fullmatch(CANONICAL_NAME, args.name), "Invalid canonical name")
    require(not args.stage or args.verify_only, "--stage is for --verify-only runs only")
    require_idle(ROOT)
    oracle = Oracle.load()
    manifest_before = (ROOT / "layout/manifest.json").read_bytes()
    manifest = json.loads(manifest_before)
    check_manifest(oracle, manifest)
    group_id = "group:" + args.name
    binding_bytes = project_path(args.binding).read_bytes()
    require(b"\r" not in binding_bytes, "Canonical binding proof must use LF newlines")
    source_bytes = project_path(args.source).read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    require_no_segname(source_bytes, group_id)
    require_no_case_alias(source_bytes, group_id)
    stage = project_path(args.stage) if args.stage else ROOT / "build/workers/promotion" / args.name
    stage.mkdir(parents=True, exist_ok=True)
    result, report = compile_data_group(source_bytes.decode("ascii"), args.profile, args.flags, stage / "compile",
                                        binding=json.loads(binding_bytes), group_id=group_id)
    for seg, p in report["placements"].items():
        print(f"  {seg} at DS:{p['placement']:04x} ({p['length']} bytes): {p['basis']}")
    print(f"  data publics {[d['name'] for d in report['data_publics']]}")
    if args.verify_only:
        preview = {"kind": DATA_GROUP_KIND, "name": args.name, "group_id": group_id, "space": "root",
                   "binding": project_path(args.binding).relative_to(ROOT).as_posix(),
                   "binding_sha256": sha(binding_bytes), "data_publics": report["data_publics"]}
        (stage / "provider.preview.json").write_bytes(json_bytes(preview))
        print(f"Strict data-only group: {len(report['placements'])} placed segments; preview {stage / 'provider.preview.json'}")
        return
    source_path = ROOT / "src" / (args.name + ".c")
    recipe_path = ROOT / "recipes" / (args.name + ".json")
    canonical_binding = ROOT / "evidence/bindings" / (args.name + ".json")
    recipe = {"schema": 1, "kind": DATA_GROUP_KIND, "name": args.name, "group_id": group_id, "language": "c",
              "source": source_path.relative_to(ROOT).as_posix(), "source_sha256": sha(source_bytes),
              "profile": args.profile, "flags": list(result.flags), **({"includes": report["includes"]} if report["includes"] else {}),
              "object_sha256": report["object_sha256"],
              "space": "root", "placements": report["placements"], "data_publics": report["data_publics"],
              "binding": canonical_binding.relative_to(ROOT).as_posix(), "binding_sha256": sha(binding_bytes)}
    rel = recipe_path.relative_to(ROOT).as_posix()
    manifest["data_groups"] = sorted(set(manifest.get("data_groups", [])) | {rel})
    files = {source_path: source_bytes, recipe_path: json_bytes(recipe), canonical_binding: binding_bytes}
    replace = [p for p in files if p.exists()]
    require(not replace or rel in json.loads(manifest_before).get("data_groups", []),
            "Canonical publication would overwrite a file")
    publish(ROOT, files, manifest_before, manifest, lambda: check_manifest(oracle, manifest), replace=replace)
    print(f"Published data-only group {args.name}: {sum(p['length'] for p in report['placements'].values())} bytes")


if __name__ == "__main__":
    main()

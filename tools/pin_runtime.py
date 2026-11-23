"""Publish a whole independent runtime member only after a fresh strict proof."""
import argparse
import json
import re

from common import ROOT, project_path, read_json, require, json_bytes
from publication import publish, require_idle
from oracle import Oracle
from runtime import verify_member
from validate import check_manifest


def pin(recipe_path, *, verify_only=False, replaces=None):
    require_idle(ROOT)
    source = project_path(recipe_path)
    require(source.is_relative_to(ROOT/"build/workers"), "Runtime submissions must live in worker scratch")
    recipe = read_json(source)
    if "binding" in recipe:
        require(b"\r" not in project_path(recipe["binding"]).read_bytes(),
                "Canonical runtime binding must use LF newlines for stable Git identity")
    name = recipe["name"]
    require(re.fullmatch(r"[a-z][a-z0-9_]*", name), "Invalid runtime recipe name")
    oracle = Oracle.load()
    manifest_before = (ROOT/"layout/manifest.json").read_bytes()
    manifest = json.loads(manifest_before)
    check_manifest(oracle, manifest)
    identifier = f"{recipe['space']}:{recipe['segment']:04x}:{recipe['offset']:04x}"
    retire = []
    if replaces:
        # Re-pin (2026-11-23): an owned runtime target attributed to the wrong archive member is replaced by
        # another member with the same extent, freshly proven; the old recipe retires in the same transaction.
        old_path = project_path(replaces)
        old = [o for o in manifest["owners"] if o.get("recipe") == old_path.relative_to(ROOT).as_posix()]
        require(len(old) == 1 and old[0]["kind"] == "PINNED_RUNTIME" and old[0]["target_id"] == identifier and
                old[0]["size"] == recipe["size"], "--replaces must name the pinned runtime owner of this exact extent")
        manifest["owners"].remove(old[0])
        retire = [old_path]
    require(all(o["target_id"] != identifier for o in manifest["owners"]), "Runtime target already owned")
    report = verify_member(recipe, oracle, manifest)
    if verify_only:
        return report
    path = ROOT/"recipes/runtime"/(name+".json")
    require(not path.exists(), "Runtime publication would overwrite a canonical recipe")
    recipe["proof_scope"] = report["proof_scope"]
    recipe["fixup_count"] = report["fixups"]
    owner = {key: recipe[key] for key in ("space", "segment", "offset", "size")}
    owner.update(target_id=identifier, kind="PINNED_RUNTIME", state="PINNED_RUNTIME",
                 recipe=path.relative_to(ROOT).as_posix(), fixup_count=report["fixups"],
                 relocation_count=report["relocations"])
    if report.get("relocation_indices"):
        owner["relocation_indices"] = report["relocation_indices"]
    manifest["owners"].append(owner)
    publish(ROOT, {path: json_bytes(recipe)}, manifest_before, manifest,
            lambda: check_manifest(oracle, manifest), retire=retire)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("recipe", help="Independent member submission JSON under build/workers")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--replaces", help="recipes/runtime/OLD.json owning the same extent (re-pin)")
    args = parser.parse_args()
    report = pin(args.recipe, verify_only=args.verify_only, replaces=args.replaces)
    print(f"{'Verified' if args.verify_only else 'Published'} {report['name']}: "
          f"{report['size']} PINNED_RUNTIME bytes, {report['fixups']} proven fixups"
          + (f", {report['relocations']} relocations" if report.get("relocations") else "")
          + (f", {len(report['data_placements'])} placed DGROUP segments" if report.get("data_placements") else ""))


if __name__ == "__main__":
    main()

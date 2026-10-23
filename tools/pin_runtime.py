"""Publish a whole independent runtime member only after a fresh strict proof."""
import argparse
import json
import re

from common import ROOT, project_path, read_json, require, json_bytes
from publication import publish, require_idle
from oracle import Oracle
from runtime import verify_member
from validate import check_manifest


def pin(recipe_path, *, verify_only=False):
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
    require(all(o["target_id"] != identifier for o in manifest["owners"]), "Runtime target already owned")
    report = verify_member(recipe, oracle)
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
    manifest["owners"].append(owner)
    publish(ROOT, {path: json_bytes(recipe)}, manifest_before, manifest,
            lambda: check_manifest(oracle, manifest))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("recipe", help="Independent member submission JSON under build/workers")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    report = pin(args.recipe, verify_only=args.verify_only)
    print(f"{'Verified' if args.verify_only else 'Published'} {report['name']}: "
          f"{report['size']} PINNED_RUNTIME bytes, {report['fixups']} proven fixups")


if __name__ == "__main__":
    main()

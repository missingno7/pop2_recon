#!/usr/bin/env python3
"""Withdraw whole canonical owners (groups or single components) from acceptance.

Used when an accepted claim turns out to rest on a wrong premise, e.g. a MASM transcription
of a routine whose original source was C (only genuine original assembly may be matched as
ASM). Every owner whose recipe is named is removed from the manifest, and its recipe, source
and binding files are retired through the journaled publication path; the remaining manifest
must still pass check_manifest. Nothing else is rewritten.
"""
import argparse

from common import ROOT, read_json, project_path, require
from oracle import Oracle
from publication import publish, require_idle
from validate import check_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("names", nargs="+", help="Canonical recipe names (recipes/NAME.json)")
    parser.add_argument("--reason", required=True)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    require_idle(ROOT)
    oracle = Oracle.load()
    manifest_path = ROOT / "layout/manifest.json"
    manifest = read_json(manifest_path)
    before = manifest_path.read_bytes()
    recipes = {project_path("recipes/" + name + ".json") for name in args.names}
    for path in recipes:
        require(path.is_file(), f"No canonical recipe {path}")
    gone = [o for o in manifest["owners"] if o.get("recipe") and project_path(o["recipe"]) in recipes]
    kept = [o for o in manifest["owners"] if o not in gone]
    require(gone, "No owners use these recipes")
    still_used = set()
    for owner in kept:
        if owner.get("recipe"):
            recipe = read_json(project_path(owner["recipe"]))
            still_used.add(project_path(owner["recipe"]))
            if "source" in recipe:
                still_used.add(project_path(recipe["source"]))
            if "binding" in recipe:
                still_used.add(project_path(recipe["binding"]))
    retire = []
    for path in sorted(recipes):
        recipe = read_json(path)
        for p in [path, project_path(recipe["source"])] + ([project_path(recipe["binding"])] if "binding" in recipe else []):
            if p not in still_used and p not in retire:
                retire.append(p)
    print(f"Retiring {len(gone)} owners ({sum(o['size'] for o in gone)} bytes): {args.reason}")
    for o in gone:
        print(f"  {o['target_id']} {o['state']} {o['recipe']}")
    if args.dry_run:
        return
    after = dict(manifest, owners=kept)
    publish(ROOT, {}, before, after, lambda: check_manifest(oracle, after), retire=retire)
    print(f"Retired {len(retire)} files")


if __name__ == "__main__":
    main()

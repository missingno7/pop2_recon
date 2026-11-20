"""Serialized publication of a same-source group after a fresh historical compilation.

Every claim must be CODE_EXACT. Existing single-function owners inside the
group's placement must be among the claims: they are migrated (owner replaced,
old source/recipe/binding retired) in the same transaction, never double counted.
Owners of any other kind inside the placement refuse the publication.
"""
import argparse
import json
import re

from common import CANONICAL_NAME, ROOT, json_bytes, project_path, read_json, require, sha
from module_gate import compile_group
from oracle import Oracle
from publication import publish, require_idle
from validate import GROUP_KIND, check_manifest

def parse_claim(text):
    target, sep, public = text.partition("=")
    require(sep and target and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", public), "Claim must be TARGET_ID=PUBLIC")
    return {"target_id": target, "public": public}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source")
    parser.add_argument("--name", required=True, help="Canonical group name (src/NAME.c, recipes/NAME.json)")
    parser.add_argument("--profile", required=True)
    parser.add_argument("--flags", nargs="+")
    parser.add_argument("--claim", action="append", required=True, type=parse_claim,
                        help="TARGET_ID=PUBLIC, repeated in any order")
    parser.add_argument("--binding", help="Reviewed group binding under evidence/bindings")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    require(re.fullmatch(CANONICAL_NAME, args.name), "Invalid canonical group name")
    require_idle(ROOT)
    oracle = Oracle.load()
    manifest_before = (ROOT / "layout/manifest.json").read_bytes()
    manifest = json.loads(manifest_before)
    check_manifest(oracle, manifest)
    group_id = "group:" + args.name
    binding_path = project_path(args.binding) if args.binding else None
    binding_bytes = None
    if binding_path:
        require(binding_path.is_relative_to(ROOT / "evidence/bindings"),
                "Canonical binding proof must live under evidence/bindings")
        # One read: the proven, pinned and committed binding are the same bytes.
        binding_bytes = binding_path.read_bytes()
        require(b"\r" not in binding_bytes, "Canonical binding proof must use LF newlines")
    source_bytes = project_path(args.source).read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    stage = ROOT / "build/workers/promotion" / args.name
    stage.mkdir(parents=True, exist_ok=True)
    (stage / "CAND.C").write_bytes(source_bytes)
    result, report = compile_group(source_bytes.decode("ascii"), args.profile, args.flags, args.claim,
                                   stage / "compile",
                                   binding=json.loads(binding_bytes) if binding_bytes else None,
                                   group_id=group_id)
    for claim in report["claims"]:
        print(f"  {claim['target_id']} {claim['public']}: {claim['state']}"
              + (f" ({claim['reason']})" if claim.get("reason") else "")
              + (f" first difference {claim['first_difference']}" if claim.get("first_difference") is not None else ""))
    for draft in report["drafts"]:
        print(f"  draft {draft['public']} at +{draft['object_offset']} (owns nothing)")
    require(report["exact"], "Refusing promotion: not every claim is CODE_EXACT")
    space = oracle.spaces[report["space"]]
    hull = set(range(report["origin"], report["origin"] + report["emitted_size"]))
    claimed = {c["target_id"] for c in args.claim}
    migrated, kept = [], []
    for owner in manifest["owners"]:
        position = space.position(owner["segment"], owner["offset"]) if owner["space"] == space.name else None
        inside = position is not None and hull.intersection(range(position, position + owner["size"]))
        if not inside and owner["target_id"] not in claimed:
            kept.append(owner)
            continue
        recipe = read_json(project_path(owner["recipe"]))
        require(owner["kind"] == "MATCHING_C" and recipe.get("kind") != GROUP_KIND and
                owner["target_id"] in claimed,
                f"Owner {owner['target_id']} inside the group placement cannot be migrated")
        migrated.append((owner, recipe))
    if args.verify_only:
        print(f"Strict group CODE_EXACT: {len(report['claims'])} claims at {space.name}+{report['origin']:#x}, "
              f"{report['emitted_size']} bytes; would migrate {[o['target_id'] for o, _ in migrated]}")
        return
    source_path = ROOT / "src" / (args.name + ".c")
    recipe_path = ROOT / "recipes" / (args.name + ".json")
    require(not source_path.exists() and not recipe_path.exists(), "Canonical publication would overwrite a file")
    claims = [{"target_id": c["target_id"], "public": c["public"], "fixup_count": c["fixups"]}
              for c in report["claims"]]
    recipe = {"schema": 1, "kind": GROUP_KIND, "name": args.name, "group_id": group_id, "language": "c",
              "source": source_path.relative_to(ROOT).as_posix(), "source_sha256": sha(source_bytes),
              "profile": args.profile, "flags": list(result.flags), "object_sha256": report["object_sha256"],
              "space": report["space"], "origin": report["origin"], "emitted_size": report["emitted_size"],
              "frame": report["frame"],
              "claims": claims, "drafts": report["drafts"], "proof_scope": report["proof_scope"]}
    if binding_path:
        recipe.update(binding=binding_path.relative_to(ROOT).as_posix(), binding_sha256=sha(binding_bytes))
    owners = []
    for claim in report["claims"]:
        target = next(t for t in read_json(ROOT / "evidence/targets.json")["functions"]
                      if t["id"] == claim["target_id"])
        owner = {key: target[key] for key in ("space", "segment", "offset", "size")}
        owner.update(target_id=claim["target_id"], kind="MATCHING_C", state="CODE_EXACT",
                     recipe=recipe_path.relative_to(ROOT).as_posix(), group=args.name,
                     relocation_count=0, fixup_count=claim["fixups"])
        owners.append(owner)
    manifest["owners"] = kept + owners
    # Retire only files that no recipe in the final manifest still references.
    still_used = {binding_path} if binding_path else set()
    for owner in kept:
        if owner["kind"] == "PINNED_RUNTIME":
            continue
        kept_recipe = read_json(project_path(owner["recipe"]))
        still_used |= {project_path(owner["recipe"]), project_path(kept_recipe["source"])}
        if "binding" in kept_recipe:
            still_used.add(project_path(kept_recipe["binding"]))
    retire = []
    for old_owner, old in migrated:
        paths = [project_path(old["source"]), project_path(old_owner["recipe"])]
        if "binding" in old:
            paths.append(project_path(old["binding"]))
        retire += [path for path in paths if path not in still_used and path not in retire]

    def gate():
        check_manifest(oracle, manifest)
        require(binding_path is None or binding_path.read_bytes() == binding_bytes,
                "Binding changed after its proof")

    publish(ROOT, {source_path: source_bytes, recipe_path: json_bytes(recipe)}, manifest_before, manifest,
            gate, retire=retire)
    print(f"Published group {args.name}: {sum(o['size'] for o in owners)} CODE_EXACT bytes in {len(owners)} claims; "
          f"migrated {[o['target_id'] for o, _ in migrated]}")


if __name__ == "__main__":
    main()
